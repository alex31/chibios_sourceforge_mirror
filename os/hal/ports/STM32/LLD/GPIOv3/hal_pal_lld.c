/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    GPIOv3/hal_pal_lld.c
 * @brief   STM32 PAL low level driver code.
 *
 * @addtogroup PAL
 * @{
 */

#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if defined(STM32L4XX)
#define AHB2_EN_MASK    STM32_GPIO_EN_MASK
#define AHB2_LPEN_MASK  0

#else
#error "missing or unsupported platform for GPIOv3 PAL driver"
#endif

/* Handling a difference in ST headers.*/
#if defined(STM32L4XX)
#define EMR     EMR1
#define IMR     IMR1
#define PR      PR1
#define RTSR    RTSR1
#define FTSR    FTSR1
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   Event records for the 16 GPIO EXTI channels.
 */
palevent_t _pal_events[16];

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void initgpio(stm32_gpio_t *gpiop, const stm32_gpio_setup_t *config) {

  gpiop->OTYPER  = config->otyper;
  gpiop->ASCR    = config->ascr;
  gpiop->OSPEEDR = config->ospeedr;
  gpiop->PUPDR   = config->pupdr;
  gpiop->ODR     = config->odr;
  gpiop->AFRL    = config->afrl;
  gpiop->AFRH    = config->afrh;
  gpiop->MODER   = config->moder;
  gpiop->LOCKR   = config->lockr;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   STM32 I/O ports configuration.
 * @details Ports A-D(E, F, G, H) clocks enabled.
 *
 * @param[in] config    the STM32 ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {

  /*
   * Enables the GPIO related clocks.
   */
#if defined(STM32L4XX)
  RCC->AHB2ENR |= AHB2_EN_MASK;
#endif

  /*
   * Initial GPIO setup.
   */
#if STM32_HAS_GPIOA
  initgpio(GPIOA, &config->PAData);
#endif
#if STM32_HAS_GPIOB
  initgpio(GPIOB, &config->PBData);
#endif
#if STM32_HAS_GPIOC
  initgpio(GPIOC, &config->PCData);
#endif
#if STM32_HAS_GPIOD
  initgpio(GPIOD, &config->PDData);
#endif
#if STM32_HAS_GPIOE
  initgpio(GPIOE, &config->PEData);
#endif
#if STM32_HAS_GPIOF
  initgpio(GPIOF, &config->PFData);
#endif
#if STM32_HAS_GPIOG
  initgpio(GPIOG, &config->PGData);
#endif
#if STM32_HAS_GPIOH
  initgpio(GPIOH, &config->PHData);
#endif
#if STM32_HAS_GPIOI
  initgpio(GPIOI, &config->PIData);
#endif
#if STM32_HAS_GPIOJ
  initgpio(GPIOJ, &config->PJData);
#endif
#if STM32_HAS_GPIOK
  initgpio(GPIOK, &config->PKData);
#endif
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as push pull at minimum
 *          speed.
 *
 * @param[in] port      the port identifier
 * @param[in] mask      the group mask
 * @param[in] mode      the mode
 *
 * @notapi
 */
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {

  uint32_t moder   = (mode & PAL_STM32_MODE_MASK) >> 0;
  uint32_t otyper  = (mode & PAL_STM32_OTYPE_MASK) >> 2;
  uint32_t ospeedr = (mode & PAL_STM32_OSPEED_MASK) >> 3;
  uint32_t pupdr   = (mode & PAL_STM32_PUPDR_MASK) >> 5;
  uint32_t altr    = (mode & PAL_STM32_ALTERNATE_MASK) >> 7;
  uint32_t ascr    = (mode & PAL_STM32_ASCR_MASK) >> 11;
  uint32_t lockr   = (mode & PAL_STM32_LOCKR_MASK) >> 12;
  uint32_t bit     = 0;
  while (true) {
    if ((mask & 1) != 0) {
      uint32_t altrmask, m1, m2, m4;

      altrmask = altr << ((bit & 7) * 4);
      m1 = 1 << bit;
      m2 = 3 << (bit * 2);
      m4 = 15 << ((bit & 7) * 4);
      port->OTYPER  = (port->OTYPER & ~m1) | otyper;
      port->ASCR    = (port->ASCR & ~m1) | ascr;
      port->OSPEEDR = (port->OSPEEDR & ~m2) | ospeedr;
      port->PUPDR   = (port->PUPDR & ~m2) | pupdr;
       if ((mode & PAL_STM32_MODE_MASK) == PAL_STM32_MODE_ALTERNATE) {
        /* If going in alternate mode then the alternate number is set
           before switching mode in order to avoid glitches.*/
        if (bit < 8)
          port->AFRL = (port->AFRL & ~m4) | altrmask;
        else
          port->AFRH = (port->AFRH & ~m4) | altrmask;
        port->MODER   = (port->MODER & ~m2) | moder;
      }
      else {
        /* If going into a non-alternate mode then the mode is switched
           before setting the alternate mode in order to avoid glitches.*/
        port->MODER   = (port->MODER & ~m2) | moder;
        if (bit < 8)
          port->AFRL = (port->AFRL & ~m4) | altrmask;
        else
          port->AFRH = (port->AFRH & ~m4) | altrmask;
      }
      port->LOCKR   = (port->LOCKR & ~m1) | lockr;
    }
    mask >>= 1;
    if (!mask)
      return;
    otyper <<= 1;
    ospeedr <<= 2;
    pupdr <<= 2;
    moder <<= 2;
    bit++;
  }
}

/**
 * @brief   Pad event enable.
 * @details This function programs an event callback in the specified mode.
 * @note    Programming an unknown or unsupported mode is silently ignored.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 * @param[in] mode      pad event mode
 * @param[in] callback  event callback function
 * @param[in] arg       callback argument
 *
 * @notapi
 */
void _pal_lld_enablepadevent(ioportid_t port,
                             iopadid_t pad,
                             ioeventmode_t mode,
                             palcallback_t callback,
                             void *arg) {

  uint32_t padmask, cridx, crmask, portidx;

  /* Mask of the pad.*/
  padmask = 1U << (uint32_t)pad;

  /* Multiple channel setting of the same channel not allowed, first disable
     it. This is done because on STM32 the same channel cannot be mapped on
     multiple ports.*/
  osalDbgAssert(((EXTI->RTSR & padmask) == 0U) &&
                ((EXTI->FTSR & padmask) == 0U), "channel already in use");

  /* Index and mask of the SYSCFG CR register to be used.*/
  cridx  = (uint32_t)pad >> 2U;
  crmask = ~(0xFU << (((uint32_t)pad & 3U) * 4U));

  /* Port index is obtained assuming that GPIO ports are placed at regular
     0x400 intervalis in memory space. So far this is true for all devices.*/
  portidx = (uint32_t)port >> 10U;

  /* Port selection in SYSCFG.*/
  SYSCFG->EXTICR[cridx] = (SYSCFG->EXTICR[cridx] & crmask) | portidx;

  /* Programming edge registers.*/
  if (mode & PAL_EVENT_MODE_RISING_EDGE)
    EXTI->RTSR |= padmask;
  else
    EXTI->RTSR &= ~padmask;
  if (mode & PAL_EVENT_MODE_FALLING_EDGE)
    EXTI->FTSR |= padmask;
  else
    EXTI->FTSR &= ~padmask;

  /* Programming interrupt and event registers.*/
  EXTI->IMR |= padmask;
  EXTI->EMR &= ~padmask;

  /* Setting up callback and argument for this event.*/
  _pal_set_event(pad, callback, arg);
}

/**
 * @brief   Pad event disable.
 * @details This function disables previously programmed event callbacks.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 *
 * @notapi
 */
void _pal_lld_disablepadevent(ioportid_t port, iopadid_t pad) {
  uint32_t padmask, rtsr1, ftsr1;

  rtsr1 = EXTI->RTSR1;
  ftsr1 = EXTI->FTSR1;

  /* Mask of the pad.*/
  padmask = 1U << (uint32_t)pad;

  /* If either RTRS1 or FTSR1 is enabled then the channel is in use.*/
  if (((rtsr1 | ftsr1) & padmask) != 0U) {
    uint32_t cridx, croff, crport, portidx;

    /* Index and mask of the SYSCFG CR register to be used.*/
    cridx  = (uint32_t)pad >> 2U;
    croff = ((uint32_t)pad & 3U) * 4U;

    /* Port index is obtained assuming that GPIO ports are placed at regular
       0x400 intervalis in memory space. So far this is true for all devices.*/
    portidx = (uint32_t)port >> 10U;

    crport = (SYSCFG->EXTICR[cridx] >> croff) & 0xFU;

    osalDbgAssert(crport == portidx, "channel mapped on different port");

    /* Disabling channel.*/
    EXTI->IMR  &= ~padmask;
    EXTI->EMR  &= ~padmask;
    EXTI->RTSR  = rtsr1 & ~padmask;
    EXTI->FTSR  = ftsr1 & ~padmask;
    EXTI->PR    = padmask;

    /* Clearing callback and argument for this event.*/
    _pal_clear_event(pad);
  }
}

#endif /* HAL_USE_PAL */

/** @} */
