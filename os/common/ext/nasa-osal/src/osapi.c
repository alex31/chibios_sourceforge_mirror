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
 * @file    osapi.c
 * @brief   OS API module code.
 *
 * @addtogroup nasa_osapi
 * @{
 */

#include <string.h>

#include "ch.h"

#include "common_types.h"
#include "osapi.h"

#if CH_CFG_USE_MUTEXES == FALSE
#error "NASA OSAL requires CH_CFG_USE_MUTEXES"
#endif

#if CH_CFG_USE_SEMAPHORES == FALSE
#error "NASA OSAL requires CH_CFG_USE_SEMAPHORES"
#endif

#if CH_CFG_USE_REGISTRY == FALSE
#error "NASA OSAL requires CH_CFG_USE_REGISTRY"
#endif

#if CH_CFG_USE_MEMCORE == FALSE
#error "NASA OSAL requires CH_CFG_USE_MEMCORE"
#endif

#if CH_CFG_USE_MEMPOOLS == FALSE
#error "NASA OSAL requires CH_CFG_USE_MEMPOOLS"
#endif

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define MIN_PRIORITY        1
#define MAX_PRIORITY        255

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/**
 * @brief   Type of OSAL main structure.
 */
typedef struct {
  bool                  printf_enabled;
  memory_pool_t         binary_semaphores_pool;
  memory_pool_t         count_semaphores_pool;
  memory_pool_t         mutexes_pool;
  binary_semaphore_t    binary_semaphores[OS_MAX_BIN_SEMAPHORES];
  semaphore_t           count_semaphores[OS_MAX_COUNT_SEMAPHORES];
  mutex_t               mutexes[OS_MAX_MUTEXES];
} osal_t;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static osal_t osal;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/*-- Initialization API -----------------------------------------------------*/

/**
 * @brief   OS initialization.
 * @details This function returns initializes the internal data structures
 *          of the OS Abstraction Layer. It must be called in the application
 *          startup code before calling any other OS routines.
 *
 * @return                      kAn error code.
 *
 * @api
 */
int32 OS_API_Init(void) {

  chSysInit();

  /* OS_printf() initially disabled.*/
  osal.printf_enabled = false;

  /* Binary Semaphores pool initialization.*/
  chPoolObjectInit(&osal.binary_semaphores_pool,
                   sizeof (binary_semaphore_t),
                   NULL);
  chPoolLoadArray(&osal.binary_semaphores_pool,
                  &osal.binary_semaphores,
                  sizeof (binary_semaphore_t));

  /* Counter Semaphores pool initialization.*/
  chPoolObjectInit(&osal.count_semaphores_pool,
                   sizeof (semaphore_t),
                   NULL);
  chPoolLoadArray(&osal.count_semaphores_pool,
                  &osal.count_semaphores,
                  sizeof (semaphore_t));

  /* Mutexes pool initialization.*/
  chPoolObjectInit(&osal.mutexes_pool,
                   sizeof (mutex_t),
                   NULL);
  chPoolLoadArray(&osal.mutexes_pool,
                  &osal.mutexes,
                  sizeof (mutex_t));

  return OS_SUCCESS;
}

/*-- Various API -----------------------------------------------------------*/

/**
 * @brief   OS printf-like function.
 * @note    It is initially disabled.
 * @note    It is not currently implemented.
 *
 * @param[in] string            formatter string
 *
 * @api
 */
void OS_printf(const char *string, ...) {

  (void)string;

  if (osal.printf_enabled) {

  }
}

/**
 * @brief   Disables @p OS_printf().
 *
 * @api
 */
void OS_printf_disable(void) {

  osal.printf_enabled = false;
}

/**
 * @brief   Enables @p OS_printf().
 *
 * @api
 */
void OS_printf_enable(void) {

  osal.printf_enabled = true;
}

/**
 * @brief   System tick period in microseconds.
 *
 * @return                      The system tick period.
 */
int32 OS_Tick2Micros(void) {

  return 1000000 / CH_CFG_ST_FREQUENCY;
}

/**
 * @brief   Returns the local time.
 * @note    It is not currently implemented.
 *
 * @param[out] time_struct      the system time
 * @return                      An error code.
 *
 * @api
 */
int32 OS_GetLocalTime(OS_time_t *time_struct) {

  if (time_struct == NULL) {
     return OS_INVALID_POINTER;
  }

  time_struct->seconds = 0;
  time_struct->microsecs = 0;

  return OS_ERR_NOT_IMPLEMENTED;
}

/**
 * @brief   Changes the local time.
 * @note    It is not currently implemented.
 *
 * @param[in] time_struct       the system time
 * @return                      An error code.
 *
 * @api
 */
int32 OS_SetLocalTime(OS_time_t *time_struct) {

  if (time_struct == NULL) {
     return OS_INVALID_POINTER;
  }

  return OS_ERR_NOT_IMPLEMENTED;
}

/**
 * @brief   Conversion from milliseconds to ticks.
 *
 * @param[in] milli_seconds     the time in milliseconds
 * @return                      The system ticks.
 *
 * @api
 */
int32 OS_Milli2Ticks(uint32 milli_seconds) {

  return (int32)MS2ST(milli_seconds);
}

/*-- Queues API -------------------------------------------------------------*/

/*-- Semaphore and Mutex API ------------------------------------------------*/

/**
 * @brief   Binary semaphore creation.
 *
 * @param[out] sem_id           pointer to a binary semaphore id variable
 * @param[in] sem_name          the binary semaphore name
 * @param[in] sem_initial_value semaphore intial value
 * @param[in] options           semaphore options
 * @return                      An error code.
 *
 * @api
 */
int32 OS_BinSemCreate (uint32 *sem_id, const char *sem_name,
                       uint32 sem_initial_value, uint32 options) {
  binary_semaphore_t *bsp;

  (void)options;

  /* NULL pointer checks.*/
  if ((sem_id == NULL) || (sem_name == NULL)) {
    return OS_INVALID_POINTER;
  }

  /* Checking semaphore name length.*/
  if (strlen(sem_name) >= OS_MAX_API_NAME) {
    return OS_ERR_NAME_TOO_LONG;
  }

  /* Semaphore counter check, it is binary so only 0 and 1.*/
  if (sem_initial_value > 1) {
    return OS_INVALID_INT_NUM;
  }

  bsp = chPoolAlloc(&osal.binary_semaphores_pool);

  if (bsp == 0) {
    return OS_SEM_FAILURE;
  }

  /* Semaphore is initialized.*/
  chBSemObjectInit(bsp, sem_initial_value == 0 ? false : true);

  *sem_id = (uint32)bsp;

  return OS_SUCCESS;
}

/**
 * @brief   Binary semaphore deletion.
 *
 * @param[in] sem_id            binary semaphore id variable
 * @return                      An error code.
 *
 * @api
 */
int32 OS_BinSemDelete(uint32 sem_id) {
  binary_semaphore_t *bsp = (binary_semaphore_t *)sem_id;

  if ((bsp < &osal.binary_semaphores[0]) ||
      (bsp >= &osal.binary_semaphores[OS_MAX_BIN_SEMAPHORES])) {
    return OS_ERR_INVALID_ID;
  }

  chSysLock();

  /* Resetting the semaphore, no threads in queue.*/
  chBSemResetI(bsp, true);

  /* Flagging it as unused and returning it to the pool.*/
  bsp->sem.queue.prev = NULL;
  chPoolFreeI(&osal.binary_semaphores_pool, (void *)bsp);

  /* Required because some thready could have been made ready.*/
  chSchRescheduleS();

  chSysUnlock();

  return OS_SUCCESS;
}

/**
 * @brief   Binary semaphore flush.
 * @note    The state of the binary semaphore is not changed.
 *
 * @param[in] sem_id            binary semaphore id variable
 * @return                      An error code.
 *
 * @api
 */
int32 OS_BinSemFlush(uint32 sem_id) {
  binary_semaphore_t *bsp = (binary_semaphore_t *)sem_id;

  if ((bsp < &osal.binary_semaphores[0]) ||
      (bsp >= &osal.binary_semaphores[OS_MAX_BIN_SEMAPHORES])) {
    return OS_ERR_INVALID_ID;
  }

  chSysLock();

  /* If the semaphore is not in use then error.*/
  if (bsp->sem.queue.prev == NULL) {
    chSysUnlock();
    return OS_SEM_FAILURE;
  }

  if (bsp->sem.cnt < 0) {
    chBSemResetI(bsp, true);
    chSchRescheduleS();
  }

  chSysUnlock();

  return OS_SUCCESS;
}

/**
 * @brief   Binary semaphore give.
 *
 * @param[in] sem_id            binary semaphore id variable
 * @return                      An error code.
 *
 * @api
 */
int32 OS_BinSemGive(uint32 sem_id) {
  binary_semaphore_t *bsp = (binary_semaphore_t *)sem_id;

  if ((bsp < &osal.binary_semaphores[0]) ||
      (bsp >= &osal.binary_semaphores[OS_MAX_BIN_SEMAPHORES])) {
    return OS_ERR_INVALID_ID;
  }

  chSysLock();

  /* If the semaphore is not in use then error.*/
  if (bsp->sem.queue.prev == NULL) {
    chSysUnlock();
    return OS_SEM_FAILURE;
  }

  chBSemSignalI(bsp);
  chSchRescheduleS();

  chSysUnlock();

  return OS_SUCCESS;
}

/**
 * @brief   Binary semaphore take.
 *
 * @param[in] sem_id            binary semaphore id variable
 * @return                      An error code.
 *
 * @api
 */
int32 OS_BinSemTake(uint32 sem_id) {
  binary_semaphore_t *bsp = (binary_semaphore_t *)sem_id;

  if ((bsp < &osal.binary_semaphores[0]) ||
      (bsp >= &osal.binary_semaphores[OS_MAX_BIN_SEMAPHORES])) {
    return OS_ERR_INVALID_ID;
  }

  chSysLock();

  /* If the semaphore is not in use then error.*/
  if (bsp->sem.queue.prev == NULL) {
    chSysUnlock();
    return OS_SEM_FAILURE;
  }

  (void) chBSemWaitS(bsp);

  chSysUnlock();

  return OS_SUCCESS;
}

/**
 * @brief   Binary semaphore take with timeout.
 *
 * @param[in] sem_id            binary semaphore id variable
 * @param[in] msecs             timeout in milliseconds
 * @return                      An error code.
 *
 * @api
 */
int32 OS_BinSemTimedWait(uint32 sem_id, uint32 msecs) {
  binary_semaphore_t *bsp = (binary_semaphore_t *)sem_id;
  msg_t msg;

  if ((bsp < &osal.binary_semaphores[0]) ||
      (bsp >= &osal.binary_semaphores[OS_MAX_BIN_SEMAPHORES])) {
    return OS_ERR_INVALID_ID;
  }

  /* Timeouts of zero not allowed.*/
  if (msecs == 0) {
    return OS_INVALID_INT_NUM;
  }

  chSysLock();

  /* If the semaphore is not in use then error.*/
  if (bsp->sem.queue.prev == NULL) {
    chSysUnlock();
    return OS_SEM_FAILURE;
  }

  msg = chBSemWaitTimeoutS(bsp, MS2ST(msecs));

  chSysUnlock();

  return msg == MSG_TIMEOUT ? OS_SEM_TIMEOUT : OS_SUCCESS;
}

/**
 * @brief   Retrieves a binary semaphore id by name.
 * @note    It is not currently implemented.
 *
 * @param[out] sem_id           pointer to a binary semaphore id variable
 * @param[in] sem_name          the binary semaphore name
 * @return                      An error code.
 *
 * @api
 */
int32 OS_BinSemGetIdByName(uint32 *sem_id, const char *sem_name) {

  /* NULL pointer checks.*/
  if ((sem_id == NULL) || (sem_name == NULL)) {
    return OS_INVALID_POINTER;
  }

  return OS_ERR_NOT_IMPLEMENTED;
}

/**
 * @brief   Returns binary semaphore information.
 *
 * @param[in] sem_id            binary semaphore id variable
 * @param[in] bin_prop          binary semaphore properties
 * @return                      An error code.
 *
 * @api
 */
int32 OS_BinSemGetInfo(uint32 sem_id, OS_bin_sem_prop_t *bin_prop) {
  binary_semaphore_t *bsp = (binary_semaphore_t *)sem_id;

  /* NULL pointer checks.*/
  if (bin_prop == NULL) {
    return OS_INVALID_POINTER;
  }

  if ((bsp < &osal.binary_semaphores[0]) ||
      (bsp >= &osal.binary_semaphores[OS_MAX_BIN_SEMAPHORES])) {
    return OS_ERR_INVALID_ID;
  }

  chSysLock();

  /* If the semaphore is not in use then error.*/
  if (bsp->sem.queue.prev == NULL) {
    chSysUnlock();
    return OS_SEM_FAILURE;
  }

  chSysUnlock();

  return OS_ERR_NOT_IMPLEMENTED;
}

/*-- Task Control API -------------------------------------------------------*/

/**
 * @brief   Task creation.
 *
 * @param[out] task_id          pointer to a task id variable
 * @param[in] task_name         the task name
 * @param[in] function_pointer  the task function
 * @param[in] stack_pointer     base of stack area
 * @param[in] stack_size        size of stack area
 * @param[in] priority          the task priority
 * @param[in] flags             task attributes
 * @return                      An error code.
 *
 * @api
 */
int32 OS_TaskCreate(uint32 *task_id,
                    const char *task_name,
                    osal_task_entry function_pointer,
                    const uint32 *stack_pointer,
                    uint32 stack_size,
                    uint32 priority,
                    uint32 flags) {
  tprio_t rt_prio;
  thread_t *tp;

  (void)flags;

  /* NULL pointer checks.*/
  if ((task_id == NULL) || (task_name == NULL) ||
      (function_pointer == NULL) || (stack_pointer == NULL)) {
    return OS_INVALID_POINTER;
  }

  /* Checking alignment of stack base and size, it is application
     responsibility to pass correct values.*/
  if (!MEM_IS_ALIGNED(stack_pointer, PORT_WORKING_AREA_ALIGN) ||
      !MEM_IS_ALIGNED(stack_size, sizeof (stkalign_t))) {
    return OS_ERROR_ADDRESS_MISALIGNED;
  }

  /* Checking task name length.*/
  if (strlen(task_name) >= OS_MAX_API_NAME) {
    return OS_ERR_NAME_TOO_LONG;
  }

  /* Checking priority range.*/
  if ((priority < MIN_PRIORITY) || (priority > MAX_PRIORITY)) {
    return OS_ERR_INVALID_PRIORITY;
  }

  /* Checking if the specified stack size is below the bare minimum.*/
  if (stack_size < (uint32)THD_WORKING_AREA_SIZE(0)) {
    return OS_INVALID_INT_NUM;
  }

  /* Converting priority to RT type.*/
  rt_prio = (tprio_t)256 - (tprio_t)priority;

  tp = chThdCreateFromHeap(NULL, (size_t)stack_size, task_name,
                           rt_prio, (tfunc_t)function_pointer, NULL);

  /* Out-of-memory condition.*/
  if (tp == NULL) {
    return OS_ERROR;
  }

  /* Storing the task id.*/
  *task_id = (uint32)tp;

  return OS_SUCCESS;
}

/**
 * @brief   Installs a deletion handler.
 * @note    It is not currently implemented.
 *
 * @param[in] function_pointer  the handler function
 * @return                      An error code.
 *
 * @api
 */
int32 OS_TaskInstallDeleteHandler(void *function_pointer) {

  (void)function_pointer;

  return OS_ERR_NOT_IMPLEMENTED;
}

/**
 * @brief   Task delete.
 * @note    It is not currently implemented.
 *
 * @param[in] task_id           the task id
 * @return                      An error code.
 *
 * @api
 */
int32 OS_TaskDelete(uint32 task_id) {

  (void)task_id;

  return OS_ERR_NOT_IMPLEMENTED;
}

/**
 * @brief   Task exit.
 *
 * @api
 */
void OS_TaskExit(void) {

  chThdExit(MSG_OK);
}

/**
 * @brief   Task delay.
 *
 * @param[in] milli_second      the period in miliseconds
 * @return                      An error code.
 *
 * @api
 */
int32 OS_TaskDelay(uint32 milli_second) {

  chThdSleepMilliseconds(milli_second);
  return OS_SUCCESS;
}

/**
 * @brief   Change task priority.
 *
 * @param[in] task_id           the task id
 * @param[in] new_priority      the task new priority
 * @return                      An error code.
 *
 * @api
 */
int32 OS_TaskSetPriority (uint32 task_id, uint32 new_priority) {
  tprio_t rt_newprio;
  thread_t *tp = (thread_t *)task_id;

  /* Checking priority range.*/
  if ((new_priority < MIN_PRIORITY) || (new_priority > MAX_PRIORITY)) {
    return OS_ERR_INVALID_PRIORITY;
  }

  /* Converting priority to RT type.*/
  rt_newprio = (tprio_t)256 - (tprio_t)new_priority;

  if (chThdGetPriorityX() == rt_newprio) {
    return OS_SUCCESS;
  }

  chSysLock();

  /* Changing priority.*/
  if ((tp->prio == tp->realprio) || (rt_newprio > tp->prio)) {
    tp->prio = rt_newprio;
  }
  tp->realprio = rt_newprio;

  /* The following states need priority queues reordering.*/
  switch (tp->state) {
  case CH_STATE_WTMTX:
#if CH_CFG_USE_CONDVARS
  case CH_STATE_WTCOND:
#endif
#if CH_CFG_USE_SEMAPHORES_PRIORITY
  case CH_STATE_WTSEM:
#endif
#if CH_CFG_USE_MESSAGES && CH_CFG_USE_MESSAGES_PRIORITY
  case CH_STATE_SNDMSGQ:
#endif
    /* Re-enqueues tp with its new priority on the queue.*/
    queue_prio_insert(queue_dequeue(tp),
                      (threads_queue_t *)tp->u.wtobjp);
    break;
  case CH_STATE_READY:
#if CH_DBG_ENABLE_ASSERTS
    /* Prevents an assertion in chSchReadyI().*/
    tp->state = CH_STATE_CURRENT;
#endif
    /* Re-enqueues tp with its new priority on the ready list.*/
    chSchReadyI(queue_dequeue(tp));
    break;
  }

  /* Rescheduling.*/
  chSchRescheduleS();
  chSysUnlock();

  return OS_SUCCESS;
}

/**
 * @brief   Task registration.
 * @note    In ChibiOS/RT it does nothing.
 *
 * @return                      An error code.
 *
 * @api
 */
int32 OS_TaskRegister(void) {

  return OS_SUCCESS;
}

/**
 * @brief   Current task id.
 *
 * @return                      The current task id.
 *
 * @api
 */
uint32 OS_TaskGetId(void) {

  return (uint32)chThdGetSelfX();
}

/**
 * @brief   Retrieves a task id by name.
 *
 * @param[out] task_id          pointer to a task id variable
 * @param[in] task_name         the task name
 * @return                      An error code.
 *
 * @api
 */
int32 OS_TaskGetIdByName (uint32 *task_id, const char *task_name) {
  thread_t *tp;

  /* NULL pointer checks.*/
  if ((task_id == NULL) || (task_name == NULL)) {
    return OS_INVALID_POINTER;
  }

  /* Checking task name length.*/
  if (strlen(task_name) >= OS_MAX_API_NAME) {
    return OS_ERR_NAME_TOO_LONG;
  }

  /* Scanning registry.*/
  tp = chRegFirstThread();
  do {
    if (strcmp(chRegGetThreadNameX(tp), task_name) == 0) {
      *task_id = (uint32)tp;
      return OS_SUCCESS;
    }
    tp = chRegNextThread(tp);
  } while (tp != NULL);

  return OS_ERR_NAME_NOT_FOUND;
}

/**
 * @brief   Returns task information.
 *
 * @param[in] sem_id            binary semaphore id variable
 * @param[in] task_prop         task properties
 * @return                      An error code.
 *
 * @api
 */
int32 OS_TaskGetInfo(uint32 task_id, OS_task_prop_t *task_prop) {
  thread_t *tp = (thread_t *)task_id;
  size_t wasize = (size_t)tp - (size_t)tp->stklimit + sizeof (thread_t);

  /* NULL pointer checks.*/
  if (task_prop == NULL) {
    return OS_INVALID_POINTER;
  }

  strncpy(task_prop->name, tp->name, OS_MAX_API_NAME - 1);
  task_prop->creator    = (uint32)chSysGetIdleThreadX();
  task_prop->stack_size = (uint32)MEM_ALIGN_NEXT(wasize, PORT_STACK_ALIGN);
  task_prop->priority   = (uint32)256U - (uint32)tp->realprio;
  task_prop->OStask_id  = task_id;

  return OS_ERR_NOT_IMPLEMENTED;
}

/** @} */
