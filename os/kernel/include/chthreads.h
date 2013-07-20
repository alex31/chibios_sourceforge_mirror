/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

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
 * @file    chthreads.h
 * @brief   Threads module macros and structures.
 *
 * @addtogroup threads
 * @{
 */

#ifndef _CHTHREADS_H_
#define _CHTHREADS_H_

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Thread states
 * @{
 */
#define THD_STATE_READY         0   /**< @brief Waiting on the ready list.  */
#define THD_STATE_CURRENT       1   /**< @brief Currently running.          */
#define THD_STATE_SUSPENDED     2   /**< @brief Created in suspended state. */
#define THD_STATE_WTSEM         3   /**< @brief Waiting on a semaphore.     */
#define THD_STATE_WTMTX         4   /**< @brief Waiting on a mutex.         */
#define THD_STATE_WTCOND        5   /**< @brief Waiting on a condition
                                         variable.                          */
#define THD_STATE_SLEEPING      6   /**< @brief Waiting in @p chThdSleep()
                                         or @p chThdSleepUntil().           */
#define THD_STATE_WTEXIT        7   /**< @brief Waiting in @p chThdWait().  */
#define THD_STATE_WTOREVT       8   /**< @brief Waiting for an event.       */
#define THD_STATE_WTANDEVT      9   /**< @brief Waiting for several events. */
#define THD_STATE_SNDMSGQ       10  /**< @brief Sending a message, in queue.*/
#define THD_STATE_SNDMSG        11  /**< @brief Sent a message, waiting
                                         answer.                            */
#define THD_STATE_WTMSG         12  /**< @brief Waiting for a message.      */
#define THD_STATE_WTQUEUE       13  /**< @brief Waiting on an I/O queue.    */
#define THD_STATE_FINAL         14  /**< @brief Thread terminated.          */

/**
 * @brief   Thread states as array of strings.
 * @details Each element in an array initialized with this macro can be
 *          indexed using the numeric thread state values.
 */
#define THD_STATE_NAMES                                                     \
  "READY", "CURRENT", "SUSPENDED", "WTSEM", "WTMTX", "WTCOND", "SLEEPING",  \
  "WTEXIT", "WTOREVT", "WTANDEVT", "SNDMSGQ", "SNDMSG", "WTMSG", "WTQUEUE", \
  "FINAL"
/** @} */

/**
 * @name    Thread flags and attributes
 * @{
 */
#define THD_MEM_MODE_MASK       3   /**< @brief Thread memory mode mask.    */
#define THD_MEM_MODE_STATIC     0   /**< @brief Static thread.              */
#define THD_MEM_MODE_HEAP       1   /**< @brief Thread allocated from a
                                         Memory Heap.                       */
#define THD_MEM_MODE_MEMPOOL    2   /**< @brief Thread allocated from a
                                         Memory Pool.                       */
#define THD_TERMINATE           4   /**< @brief Termination requested flag. */
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/* Forward declaration required by the mutexes stack structure present
   in every thread.*/
#if CH_CFG_USE_MUTEXES
typedef struct mutex mutex_t;
#endif

/**
 * @brief   Generic threads single link list, it works like a stack.
 */
typedef struct {

  thread_t              *p_next;    /**< @brief Next in the list/queue.     */
} threads_list_t;

/**
 * @extends threads_list_t
 *
 * @brief   Generic threads bidirectional linked list header and element.
 */
typedef struct {
  thread_t              *p_next;    /**< @brief Next in the list/queue.     */
  thread_t              *p_prev;    /**< @brief Previous in the queue.      */
} threads_queue_t;

/**
 * @extends threads_queue_t
 *
 * @brief   Structure representing a thread.
 * @note    Not all the listed fields are always needed, by switching off some
 *          not needed ChibiOS/RT subsystems it is possible to save RAM space
 *          by shrinking the @p thread_t structure.
 */
typedef struct thread {
  thread_t              *p_next;    /**< @brief Next in the list/queue.     */
  /* End of the fields shared with the threads_list_t structure.*/
  thread_t              *p_prev;    /**< @brief Previous in the queue.      */
  /* End of the fields shared with the threads_queue_t structure.*/
  tprio_t               p_prio;     /**< @brief Thread priority.            */
  struct context        p_ctx;      /**< @brief Processor context.          */
#if CH_CFG_USE_REGISTRY || defined(__DOXYGEN__)
  thread_t              *p_newer;   /**< @brief Newer registry element.     */
  thread_t              *p_older;   /**< @brief Older registry element.     */
#endif
  /* End of the fields shared with the ReadyList structure. */
#if CH_CFG_USE_REGISTRY || defined(__DOXYGEN__)
  /**
   * @brief Thread name or @p NULL.
   */
  const char            *p_name;
#endif
#if CH_DBG_ENABLE_STACK_CHECK || defined(__DOXYGEN__)
  /**
   * @brief Thread stack boundary.
   */
  stkalign_t            *p_stklimit;
#endif
  /**
   * @brief Current thread state.
   */
  tstate_t              p_state;
  /**
   * @brief Various thread flags.
   */
  tmode_t               p_flags;
#if CH_CFG_USE_DYNAMIC || defined(__DOXYGEN__)
  /**
   * @brief References to this thread.
   */
  trefs_t               p_refs;
#endif
  /**
   * @brief Number of ticks remaining to this thread.
   */
#if (CH_CFG_TIME_QUANTUM > 0) || defined(__DOXYGEN__)
  tslices_t             p_preempt;
#endif
#if CH_DBG_THREADS_PROFILING || defined(__DOXYGEN__)
  /**
   * @brief Thread consumed time in ticks.
   * @note  This field can overflow.
   */
  volatile systime_t    p_time;
#endif
  /**
   * @brief State-specific fields.
   * @note  All the fields declared in this union are only valid in the
   *        specified state or condition and are thus volatile.
   */
  union {
    /**
     * @brief Thread wakeup code.
     * @note  This field contains the low level message sent to the thread
     *        by the waking thread or interrupt handler. The value is valid
     *        after exiting the @p chSchWakeupS() function.
     */
    msg_t               rdymsg;
    /**
     * @brief Thread exit code.
     * @note  The thread termination code is stored in this field in order
     *        to be retrieved by the thread performing a @p chThdWait() on
     *        this thread.
     */
    msg_t               exitcode;
    /**
     * @brief Pointer to a generic "wait" object.
     * @note  This field is used to get a generic pointer to a synchronization
     *        object and is valid when the thread is in one of the wait
     *        states.
     */
    void                *wtobjp;
#if CH_CFG_USE_EVENTS || defined(__DOXYGEN__)
    /**
     * @brief Enabled events mask.
     * @note  This field is only valid while the thread is in the
     *        @p THD_STATE_WTOREVT or @p THD_STATE_WTANDEVT states.
     */
    eventmask_t         ewmask;
#endif
  }                     p_u;
#if CH_CFG_USE_WAITEXIT || defined(__DOXYGEN__)
  /**
   * @brief Termination waiting list.
   */
  threads_list_t        p_waiting;
#endif
#if CH_CFG_USE_MESSAGES || defined(__DOXYGEN__)
  /**
   * @brief Messages queue.
   */
  threads_queue_t       p_msgqueue;
  /**
   * @brief Thread message.
   */
  msg_t                 p_msg;
#endif
#if CH_CFG_USE_EVENTS || defined(__DOXYGEN__)
  /**
   * @brief Pending events mask.
   */
  eventmask_t           p_epending;
#endif
#if CH_CFG_USE_MUTEXES || defined(__DOXYGEN__)
  /**
   * @brief List of the mutexes owned by this thread.
   * @note  The list is terminated by a @p NULL in this field.
   */
  mutex_t               *p_mtxlist;
  /**
   * @brief Thread's own, non-inherited, priority.
   */
  tprio_t               p_realprio;
#endif
#if (CH_CFG_USE_DYNAMIC && CH_CFG_USE_MEMPOOLS) || defined(__DOXYGEN__)
  /**
   * @brief Memory Pool where the thread workspace is returned.
   */
  void                  *p_mpool;
#endif
#if defined(CH_CFG_THREAD_EXTRA_FIELDS)
  /* Extra fields defined in chconf.h.*/
  CH_CFG_THREAD_EXTRA_FIELDS
#endif
} thread_t;

/**
 * @brief   Thread function.
 */
typedef msg_t (*tfunc_t)(void *);

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Returns a pointer to the current @p thread_t.
 * @note    Can be invoked in any context.
 *
 * @special
 */
#define chThdSelf() currp

/**
 * @brief   Returns the current thread priority.
 * @note    Can be invoked in any context.
 *
 * @special
 */
#define chThdGetPriority() (currp->p_prio)

/**
 * @brief   Returns the number of ticks consumed by the specified thread.
 * @note    This function is only available when the
 *          @p CH_DBG_THREADS_PROFILING configuration option is enabled.
 * @note    Can be invoked in any context.
 *
 * @param[in] tp        pointer to the thread
 *
 * @special
 */
#define chThdGetTicks(tp) ((tp)->p_time)

/**
 * @brief   Verifies if the specified thread is in the @p THD_STATE_FINAL state.
 * @note    Can be invoked in any context.
 *
 * @param[in] tp        pointer to the thread
 * @retval true         thread terminated.
 * @retval false        thread not terminated.
 *
 * @special
 */
#define chThdTerminated(tp) ((tp)->p_state == THD_STATE_FINAL)

/**
 * @brief   Verifies if the current thread has a termination request pending.
 * @note    Can be invoked in any context.
 *
 * @retval true         termination request pending.
 * @retval false        termination request not pending.
 *
 * @special
 */
#define chThdShouldTerminate() (currp->p_flags & THD_TERMINATE)

/**
 * @brief   Resumes a thread created with @p chThdCreateI().
 *
 * @param[in] tp        pointer to the thread
 *
 * @iclass
 */
#define chThdResumeI(tp) chSchReadyI(tp)

/**
 * @brief   Suspends the invoking thread for the specified time.
 *
 * @param[in] time      the delay in system ticks, the special values are
 *                      handled as follow:
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *                      .
 *
 * @sclass
 */
#define chThdSleepS(time) chSchGoSleepTimeoutS(THD_STATE_SLEEPING, time)

/**
 * @brief   Delays the invoking thread for the specified number of seconds.
 * @note    The specified time is rounded up to a value allowed by the real
 *          system tick clock.
 * @note    The maximum specifiable value is implementation dependent.
 *
 * @param[in] sec       time in seconds, must be different from zero
 *
 * @api
 */
#define chThdSleepSeconds(sec) chThdSleep(S2ST(sec))

/**
 * @brief   Delays the invoking thread for the specified number of
 *          milliseconds.
 * @note    The specified time is rounded up to a value allowed by the real
 *          system tick clock.
 * @note    The maximum specifiable value is implementation dependent.
 *
 * @param[in] msec      time in milliseconds, must be different from zero
 *
 * @api
 */
#define chThdSleepMilliseconds(msec) chThdSleep(MS2ST(msec))

/**
 * @brief   Delays the invoking thread for the specified number of
 *          microseconds.
 * @note    The specified time is rounded up to a value allowed by the real
 *          system tick clock.
 * @note    The maximum specifiable value is implementation dependent.
 *
 * @param[in] usec      time in microseconds, must be different from zero
 *
 * @api
 */
#define chThdSleepMicroseconds(usec) chThdSleep(US2ST(usec))
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
   thread_t *_thread_init(thread_t *tp, tprio_t prio);
#if CH_DBG_FILL_THREADS
  void _thread_memfill(uint8_t *startp, uint8_t *endp, uint8_t v);
#endif
  thread_t *chThdCreateI(void *wsp, size_t size,
                         tprio_t prio, tfunc_t pf, void *arg);
  thread_t *chThdCreateStatic(void *wsp, size_t size,
                              tprio_t prio, tfunc_t pf, void *arg);
  tprio_t chThdSetPriority(tprio_t newprio);
  thread_t *chThdResume(thread_t *tp);
  void chThdTerminate(thread_t *tp);
  void chThdSleep(systime_t time);
  void chThdSleepUntil(systime_t time);
  void chThdYield(void);
  void chThdExit(msg_t msg);
  void chThdExitS(msg_t msg);
#if CH_CFG_USE_WAITEXIT
  msg_t chThdWait(thread_t *tp);
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* _CHTHREADS_H_ */

/** @} */
