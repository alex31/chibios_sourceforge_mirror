/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    STM32/spi_lld.c
 * @brief   STM32 SPI subsystem low level driver source.
 *
 * @addtogroup SPI
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief SPI1 driver identifier.*/
#if STM32_SPI_USE_SPI1 || defined(__DOXYGEN__)
SPIDriver SPID1;
#endif

/** @brief SPI2 driver identifier.*/
#if STM32_SPI_USE_SPI2 || defined(__DOXYGEN__)
SPIDriver SPID2;
#endif

/** @brief SPI3 driver identifier.*/
#if STM32_SPI_USE_SPI3 || defined(__DOXYGEN__)
SPIDriver SPID3;
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

static uint16_t dummytx;
static uint16_t dummyrx;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Stops the SPI DMA channels.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
#define dma_stop(spip) {                                                    \
  dmaChannelDisable(spip->dmatx);                                           \
  dmaChannelDisable(spip->dmarx);                                           \
}

/**
 * @brief   Starts the SPI DMA channels.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
#define dma_start(spip) {                                                   \
  dmaChannelEnable((spip)->dmarx);                                          \
  dmaChannelEnable((spip)->dmatx);                                          \
}

/**
 * @brief   Shared end-of-rx service routine.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void spi_lld_serve_rx_interrupt(SPIDriver *spip, uint32_t flags) {

  /* DMA errors handling.*/
#if defined(STM32_SPI_DMA_ERROR_HOOK)
  if ((flags & DMA_ISR_TEIF1) != 0) {
    STM32_SPI_DMA_ERROR_HOOK(spip);
  }
#else
  (void)flags;
#endif

  /* Stop everything.*/
  dma_stop(spip);

  /* Portable SPI ISR code defined in the high level driver, note, it is
     a macro.*/
  _spi_isr_code(spip);
}

/**
 * @brief   Shared end-of-tx service routine.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void spi_lld_serve_tx_interrupt(SPIDriver *spip, uint32_t flags) {

  /* DMA errors handling.*/
#if defined(STM32_SPI_DMA_ERROR_HOOK)
  if ((flags & DMA_ISR_TEIF1) != 0) {
    STM32_SPI_DMA_ERROR_HOOK(spip);
  }
#else
  (void)spip;
  (void)flags;
#endif
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {

  dummytx = 0xFFFF;

#if STM32_SPI_USE_SPI1
  spiObjectInit(&SPID1);
  SPID1.thread  = NULL;
  SPID1.spi     = SPI1;
  SPID1.dmarx   = STM32_DMA1_CH2;
  SPID1.dmatx   = STM32_DMA1_CH3;
#endif

#if STM32_SPI_USE_SPI2
  spiObjectInit(&SPID2);
  SPID2.thread  = NULL;
  SPID2.spi     = SPI2;
  SPID2.dmarx   = STM32_DMA1_CH4;
  SPID2.dmatx   = STM32_DMA1_CH5;
#endif

#if STM32_SPI_USE_SPI3
  spiObjectInit(&SPID3);
  SPID3.thread  = NULL;
  SPID3.spi     = SPI3;
  SPID3.dmarx   = STM32_DMA2_CH1;
  SPID3.dmatx   = STM32_DMA2_CH2;
#endif
}

/**
 * @brief   Configures and activates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_start(SPIDriver *spip) {

  /* If in stopped state then enables the SPI and DMA clocks.*/
  if (spip->state == SPI_STOP) {
#if STM32_SPI_USE_SPI1
    if (&SPID1 == spip) {
      /* Note, the DMA must be enabled before the IRQs.*/
      dmaAllocate(STM32_DMA1_ID, STM32_DMA_CHANNEL_2,
                  (stm32_dmaisr_t)spi_lld_serve_rx_interrupt, (void *)spip);
      dmaAllocate(STM32_DMA1_ID, STM32_DMA_CHANNEL_3,
                  (stm32_dmaisr_t)spi_lld_serve_tx_interrupt, (void *)spip);
      NVICEnableVector(DMA1_Channel2_IRQn,
                       CORTEX_PRIORITY_MASK(STM32_SPI_SPI1_IRQ_PRIORITY));
      NVICEnableVector(DMA1_Channel3_IRQn,
                       CORTEX_PRIORITY_MASK(STM32_SPI_SPI1_IRQ_PRIORITY));
      RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    }
#endif
#if STM32_SPI_USE_SPI2
    if (&SPID2 == spip) {
      /* Note, the DMA must be enabled before the IRQs.*/
      dmaAllocate(STM32_DMA1_ID, STM32_DMA_CHANNEL_4,
                  (stm32_dmaisr_t)spi_lld_serve_rx_interrupt, (void *)spip);
      dmaAllocate(STM32_DMA1_ID, STM32_DMA_CHANNEL_5,
                  (stm32_dmaisr_t)spi_lld_serve_tx_interrupt, (void *)spip);
      NVICEnableVector(DMA1_Channel4_IRQn,
                       CORTEX_PRIORITY_MASK(STM32_SPI_SPI2_IRQ_PRIORITY));
      NVICEnableVector(DMA1_Channel5_IRQn,
                       CORTEX_PRIORITY_MASK(STM32_SPI_SPI2_IRQ_PRIORITY));
      RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    }
#endif
#if STM32_SPI_USE_SPI3
    if (&SPID3 == spip) {
      /* Note, the DMA must be enabled before the IRQs.*/
      dmaAllocate(STM32_DMA2_ID, STM32_DMA_CHANNEL_1,
                  (stm32_dmaisr_t)spi_lld_serve_rx_interrupt, (void *)spip);
      dmaAllocate(STM32_DMA2_ID, STM32_DMA_CHANNEL_2,
                  (stm32_dmaisr_t)spi_lld_serve_tx_interrupt, (void *)spip);
      NVICEnableVector(DMA2_Channel1_IRQn,
                       CORTEX_PRIORITY_MASK(STM32_SPI_SPI3_IRQ_PRIORITY));
      NVICEnableVector(DMA2_Channel2_IRQn,
                       CORTEX_PRIORITY_MASK(STM32_SPI_SPI3_IRQ_PRIORITY));
      RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    }
#endif

    /* DMA setup.*/
    dmaChannelSetPeripheral(spip->dmarx, &spip->spi->DR);
    dmaChannelSetPeripheral(spip->dmatx, &spip->spi->DR);
  }

  /* More DMA setup.*/
  if ((spip->config->cr1 & SPI_CR1_DFF) == 0)
    spip->dmaccr = (STM32_SPI_SPI2_DMA_PRIORITY << 12) |
                       DMA_CCR1_TEIE;               /* 8 bits transfers.    */
  else
    spip->dmaccr = (STM32_SPI_SPI2_DMA_PRIORITY << 12) |
                       DMA_CCR1_TEIE | DMA_CCR1_MSIZE_0 |
                       DMA_CCR1_PSIZE_0;            /* 16 bits transfers.   */

  /* SPI setup and enable.*/
  spip->spi->CR1 = 0;
  spip->spi->CR2 = SPI_CR2_SSOE | SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
  spip->spi->CR1 = spip->config->cr1 | SPI_CR1_MSTR | SPI_CR1_SPE;
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_stop(SPIDriver *spip) {

  /* If in ready state then disables the SPI clock.*/
  if (spip->state == SPI_READY) {

    /* SPI disable.*/
    spip->spi->CR1 = 0;

#if STM32_SPI_USE_SPI1
    if (&SPID1 == spip) {
      NVICDisableVector(DMA1_Channel2_IRQn);
      NVICDisableVector(DMA1_Channel3_IRQn);
      dmaRelease(STM32_DMA1_ID, STM32_DMA_CHANNEL_2);
      dmaRelease(STM32_DMA1_ID, STM32_DMA_CHANNEL_3);
      RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
    }
#endif
#if STM32_SPI_USE_SPI2
    if (&SPID2 == spip) {
      NVICDisableVector(DMA1_Channel4_IRQn);
      NVICDisableVector(DMA1_Channel5_IRQn);
      dmaRelease(STM32_DMA1_ID, STM32_DMA_CHANNEL_4);
      dmaRelease(STM32_DMA1_ID, STM32_DMA_CHANNEL_5);
      RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;
    }
#endif
#if STM32_SPI_USE_SPI3
    if (&SPID3 == spip) {
      NVICDisableVector(DMA2_Channel1_IRQn);
      NVICDisableVector(DMA2_Channel2_IRQn);
      dmaRelease(STM32_DMA2_ID, STM32_DMA_CHANNEL_1);
      dmaRelease(STM32_DMA2_ID, STM32_DMA_CHANNEL_2);
      RCC->APB1ENR &= ~RCC_APB1ENR_SPI3EN;
    }
#endif
  }
}

/**
 * @brief   Asserts the slave select signal and prepares for transfers.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_select(SPIDriver *spip) {

  palClearPad(spip->config->ssport, spip->config->sspad);
}

/**
 * @brief   Deasserts the slave select signal.
 * @details The previously selected peripheral is unselected.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_unselect(SPIDriver *spip) {

  palSetPad(spip->config->ssport, spip->config->sspad);
}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This asynchronous function starts the transmission of a series of
 *          idle words on the SPI bus and ignores the received data.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be ignored
 *
 * @notapi
 */
void spi_lld_ignore(SPIDriver *spip, size_t n) {

  dmaChannelSetup(spip->dmarx, n, &dummyrx,
                  spip->dmaccr | DMA_CCR1_TCIE | DMA_CCR1_EN);
  dmaChannelSetup(spip->dmatx, n, &dummytx,
                  spip->dmaccr | DMA_CCR1_DIR | DMA_CCR1_EN);
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be exchanged
 * @param[in] txbuf     the pointer to the transmit buffer
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_exchange(SPIDriver *spip, size_t n,
                      const void *txbuf, void *rxbuf) {

  dmaChannelSetup(spip->dmarx, n, rxbuf,
                  spip->dmaccr | DMA_CCR1_TCIE | DMA_CCR1_MINC |
                  DMA_CCR1_EN);
  dmaChannelSetup(spip->dmatx, n, txbuf,
                  spip->dmaccr | DMA_CCR1_DIR | DMA_CCR1_MINC |
                  DMA_CCR1_EN);
}

/**
 * @brief   Sends data over the SPI bus.
 * @details This asynchronous function starts a transmit operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */
void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {

  dmaChannelSetup(spip->dmarx, n, &dummyrx,
                  spip->dmaccr | DMA_CCR1_TCIE | DMA_CCR1_EN);
  dmaChannelSetup(spip->dmatx, n, txbuf,
                  spip->dmaccr | DMA_CCR1_DIR | DMA_CCR1_MINC |
                  DMA_CCR1_EN);
}

/**
 * @brief   Receives data from the SPI bus.
 * @details This asynchronous function starts a receive operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {

  dmaChannelSetup(spip->dmarx, n, rxbuf,
                  spip->dmaccr | DMA_CCR1_TCIE | DMA_CCR1_MINC |
                  DMA_CCR1_EN);
  dmaChannelSetup(spip->dmatx, n, &dummytx,
                  spip->dmaccr | DMA_CCR1_DIR | DMA_CCR1_EN);
}

/**
 * @brief   Exchanges one frame using a polled wait.
 * @details This synchronous function exchanges one frame using a polled
 *          synchronization method. This function is useful when exchanging
 *          small amount of data on high speed channels, usually in this
 *          situation is much more efficient just wait for completion using
 *          polling than suspending the thread waiting for an interrupt.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] frame     the data frame to send over the SPI bus
 * @return              The received data frame from the SPI bus.
 */
uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame) {

  spip->spi->DR = frame;
  while ((spip->spi->SR & SPI_SR_RXNE) == 0)
    ;
  return spip->spi->DR;
}

#endif /* HAL_USE_SPI */

/** @} */
