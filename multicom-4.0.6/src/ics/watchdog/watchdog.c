/**************************************************************
 * Copyright (C) 2010   STMicroelectronics. All Rights Reserved.
 * This file is part of the latest release of the Multicom4 project. This release 
 * is fully functional and provides all of the original MME functionality.This 
 * release  is now considered stable and ready for integration with other software 
 * components.

 * Multicom4 is a free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License as published by the Free Software Foundation 
 * version 2.

 * Multicom4 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along with Multicom4; 
 * see the file COPYING.  If not, write to the Free Software Foundation, 59 Temple Place - 
 * Suite 330, Boston, MA 02111-1307, USA.

 * Written by Multicom team at STMicroelectronics in November 2010.  
 * Contact multicom.support@st.com. 
**************************************************************/

/* 
 * 
 */ 

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */


/*
 * Iterate over all the installed watchdog callbacks
 * building up a bitmask of all monitored CPUs
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static
void watchdogMaskUpdate (void)
{
  ics_watchdog_callback_t *wdc;
  ICS_ULONG                mask;
  
  mask = 0;

  list_for_each_entry(wdc, &ics_state->watchdogCallback, list)
  {
    mask |= wdc->cpuMask;
  }

  /* Update the watchdog mask */
  ics_state->watchdogMask = mask;

  ICS_PRINTF(ICS_DBG_WATCHDOG, "new mask 0x%lx\n", ics_state->watchdogMask);
}

/*
 * Called when a cpu failure is detected
 *
 * For each registered watchdog, call the callback function if
 * they have registered an interest in the failed cpu
 *
 * As we need to call a user function for each triggered callback
 * the callback descriptors are copied onto a temporary list whilst holding
 * the state lock. Then having dropped the lock, all the callbacks for the
 * failed cpu are called.
 * This allows the user callback to either remove or reprime the callback
 * safely
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static
void watchdogTrigger (ICS_UINT cpuNum)
{
  ics_watchdog_callback_t *wdc, *tmp;

  LIST_HEAD(triggerList);

  ICS_PRINTF(ICS_DBG_WATCHDOG, "cpuNum %d cpuMask 0x%lx\n", cpuNum, ics_state->watchdogMask);

  list_for_each_entry(wdc, &ics_state->watchdogCallback, list)
  {
    /* Check the callback mask */
    if (wdc->cpuMask & (1UL << cpuNum))
    {
      /* Call the user's callback function if they are interested in
       * state changes for the given cpu
       */
      ICS_PRINTF(ICS_DBG_WATCHDOG, "wdc %p calling callback %p param %p cpuNum %d\n",
		 wdc, wdc->callback, wdc->param, cpuNum);

      /* Stop monitoring this CPU until reprimed */
      wdc->cpuMask &= ~(1UL << cpuNum);

      /* Add wdc to the trigger list */
      list_add(&wdc->tlist, &triggerList);
    }
  }

  /* Update the watchdog mask */
  watchdogMaskUpdate();

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  /* Now whilst NOT holding the state lock call all
   * the callbacks we found above
   */
  list_for_each_entry_safe(wdc, tmp, &triggerList, tlist)
  {
    /* Remove from the trigger list */
    list_del_init(&wdc->tlist);
    
    /* Call the user callback, having dropped the state lock */
    wdc->callback((ICS_WATCHDOG) wdc, wdc->param, cpuNum);
  }

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  return;
}

/*
 * Query whether a given CPU is responding or not
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static
ICS_ERROR watchdogQuery (ICS_UINT cpuNum)
{
  ICS_ULONG currStamp, lastStamp;
  
  ics_cpu_t *cpu;

  cpu = &ics_state->cpu[cpuNum];

  /* CPU must be connected or mapped */
  if (cpu->state != _ICS_CPU_CONNECTED && cpu->state != _ICS_CPU_MAPPED)
    return ICS_NOT_CONNECTED;
  
  /* Get the latest timestamp for the target cpu */
  currStamp = ics_transport_watchdog_query(cpuNum);

  /* Get last timestamp query value */
  lastStamp = cpu->watchdogTs;

  /* Once the timestamp is non zero start counting the
   * number of times it hasn't changed between each
   * watchdog interval
   */
  if (currStamp && (currStamp == lastStamp))
  {
    /* Count watchdog failures */
    cpu->watchdogFails++;

    /* Check for watchdog failure limit being reached */
    if (cpu->watchdogFails >= _ICS_WATCHDOG_FAILURES)
    {
      if (cpu->watchdogFails == _ICS_WATCHDOG_FAILURES)
	ICS_EPRINTF(ICS_DBG_WATCHDOG,
		    "Fail: cpuNum %d currStamp 0x%lx lastStamp 0x%lx failures %d\n",
		    cpuNum, currStamp, lastStamp, cpu->watchdogFails);

      return ICS_SYSTEM_TIMEOUT;
    }
  }
  else
  {
    /* Update the cpu watchdog state */
    cpu->watchdogFails = 0;
    cpu->watchdogTs    = currStamp;
  }

  return ICS_SUCCESS;
}


/*
 * This routine is called every _ICS_WATCHDOG_INTERVAL on each cpu
 */
void ics_watchdog_run (void)
{
  ICS_ULONG cpuMask;
  ICS_ULONG timestamp;
  
  static int run_stats = 0;

  ICS_ASSERT(ics_state);
  
  /* Get the current time in msec */
  timestamp = _ICS_OS_TIME_TICKS2MSEC(_ICS_OS_TIME_NOW());

  /* Update the transport with the current watchdog timestamp */
  ics_transport_watchdog(timestamp);

  /* Periodically run any statistics handlers */
  if (run_stats++ > (_ICS_STATS_INTERVAL/_ICS_WATCHDOG_INTERVAL))
  {
    ics_stats_trigger();
    run_stats = 0;
  }

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);
  
  /* Call the query routine if we are monitoring any cpus */
  if ((cpuMask = ics_state->watchdogMask))
  {
    ICS_UINT cpu;

    for (cpu = 0; cpuMask; cpu++, cpuMask >>= 1)
    {
      if ((cpuMask & 1) && (watchdogQuery(cpu) == ICS_SYSTEM_TIMEOUT))
      {
	/* Inform all registered callbacks of a failure (may drop/acquire lock) */
	watchdogTrigger(cpu);
      }
    }
  }

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return;
}

/*
 * Called during shutdown to release all resources 
 */
static
void removeAllCallbacks (void)
{
  ics_watchdog_callback_t *wdc, *tmp;
  
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);
  
  list_for_each_entry_safe(wdc, tmp, &ics_state->watchdogCallback, list)
  {
    /* Remove the callback from list */
    list_del_init(&wdc->list);

    _ICS_OS_FREE(wdc);
  }
  
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
}

/*
 * The Watchdog task
 *
 * Blocks on an OS event, running the watchdog run function
 * every _ICS_WATCHDOG_INTERVAL time period
 *
 * Will exit once the OS Event is signalled (i.e. no timeout)
 */
void ics_watchdog_task (void *param)
{
  ICS_ERROR err = ICS_SUCCESS;
    
  ICS_ASSERT(ics_state);
  
  ICS_PRINTF(ICS_DBG_INIT, "Starting: state %p\n", ics_state);
  
  /* Run the watchdog immediately to generate the first timestamp */
  ics_watchdog_run();

  while (1)
  {
    /* Block on the watchdog event, waking after _ICS_WATCHDOG_INTERVAL
     *
     * This event only gets signalled when we are being asked to exit
     */
    err = _ICS_OS_EVENT_WAIT(&ics_state->watchdogEvent, _ICS_WATCHDOG_INTERVAL, ICS_FALSE);
    
    /* Stop requested: get out of here */
    if (err != ICS_SYSTEM_TIMEOUT)
      break;
    
    /* Timer expiry, run the watchdog */
    ics_watchdog_run();
  }

  ICS_PRINTF(ICS_DBG_INIT, "Exiting: cpuMask 0x%lx : %s (%d)\n", ics_state->watchdogMask,
	     ics_err_str(err), err);

  /* Remove any installed callbacks
   *
   * This is strictly an error as the user should have removed them first
   * but users are lazy and it's easier for us to tidy up for them.
   * A bit like having children really ...
   */
  removeAllCallbacks();

  ICS_assert(list_empty(&ics_state->watchdogCallback));

  /* Task exit */
  return;
}

/*
 * Create and start the Watchdog task 
 * Runs on all CPUs
 */
static
ICS_ERROR createWatchdogTask (void)
{
  ICS_ERROR err;

  char taskName[_ICS_OS_MAX_TASK_NAME];

  ICS_ASSERT(ics_state);

  snprintf(taskName, _ICS_OS_MAX_TASK_NAME, "ics_watchdog");

  /* Create a high priority thread for servicing these messages */
  err = _ICS_OS_TASK_CREATE(ics_watchdog_task, NULL, _ICS_OS_TASK_DEFAULT_PRIO, taskName,
			    &ics_state->watchdogTask);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "Failed to create Watchdog task %s : %d\n", taskName, err);
    
    return ICS_NOT_INITIALIZED;
  }
  
  ICS_PRINTF(ICS_DBG_INIT, "created Watchdog task %p\n", ics_state->watchdogTask.task);

  return ICS_SUCCESS;
}


void ics_watchdog_term (void)
{
  ICS_ASSERT(ics_state);

  ICS_PRINTF(ICS_DBG_WATCHDOG|ICS_DBG_INIT, "Shutting down cpu %d\n", ics_state->cpuNum);

  if (ics_state->watchdogTask.task != NULL)
  {
    /* Wake the watchdog thread */
    _ICS_OS_EVENT_POST(&ics_state->watchdogEvent);

    _ICS_OS_TASK_DESTROY(&ics_state->watchdogTask);
  }
}

ICS_ERROR ics_watchdog_init (void)
{
  ICS_ERROR err;

  ICS_ASSERT(ics_state);

  /* Initialise the watchdog callback list */
  INIT_LIST_HEAD(&ics_state->watchdogCallback);

  ics_state->watchdogMask = 0;
  
  if (!(_ICS_OS_EVENT_INIT(&ics_state->watchdogEvent)))
  {
    return ICS_SYSTEM_ERROR;
  }

  /* Create the Watchdog task */
  err = createWatchdogTask();
  if (err != ICS_SUCCESS)
  {
    /* Failed to create the Watchdog task */
    ICS_EPRINTF(ICS_DBG_WATCHDOG|ICS_DBG_INIT, "Failed to create Watchdog task : %s (%d)\n", 
		ics_err_str(err), err);
    
    return err;
  }
  
  return ICS_SUCCESS;
}

/*
 * Install a new watchdog callback
 *
 * Handle is returned via handelp
 */ 
ICS_ERROR ICS_watchdog_add (ICS_WATCHDOG_CALLBACK callback, ICS_VOID *param,
			    ICS_ULONG cpuMask, ICS_UINT flags,
			    ICS_WATCHDOG *handlep)
{
  ICS_UINT                validFlags = 0;
  ics_watchdog_callback_t *wdc;
  
  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  if (callback == NULL || handlep == NULL || cpuMask == 0 || flags & ~(validFlags))
    return ICS_INVALID_ARGUMENT;

  ICS_PRINTF(ICS_DBG_WATCHDOG, "cpuMask 0x%lx callback %p param %p flags 0x%x\n",
	     cpuMask, callback, param, flags);

  /* Can't monitor any CPUs that are not present */
  if (cpuMask & ~ics_state->cpuMask)
    return ICS_INVALID_ARGUMENT;

  /* Allocate a new descriptor */
  _ICS_OS_ZALLOC(wdc, sizeof(*wdc));
  if (wdc == NULL)
    return ICS_ENOMEM;

  /* Initialise the desc */
  wdc->cpuMask   = cpuMask;
  wdc->callback  = callback;
  wdc->param     = param;

  INIT_LIST_HEAD(&wdc->list);
  INIT_LIST_HEAD(&wdc->tlist);
  
  /* Add to list of registered watchdog callbacks */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  list_add(&wdc->list, &ics_state->watchdogCallback);

  /* Update the watchdog mask */
  watchdogMaskUpdate();

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  /* Return opaque handle to caller */
  *handlep = (ICS_WATCHDOG) wdc;

  return ICS_SUCCESS;
}

ICS_ERROR ICS_watchdog_reprime (ICS_WATCHDOG handle, ICS_UINT cpuNum)
{
  ics_watchdog_callback_t *wdc;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  wdc = (ics_watchdog_callback_t *) handle;

  /* Try and catch bogus descriptors */
  if (handle == ICS_INVALID_HANDLE_VALUE || wdc->callback == NULL || (wdc->cpuMask & ~ics_state->cpuMask))
    return ICS_HANDLE_INVALID;

  ICS_PRINTF(ICS_DBG_WATCHDOG, "wdc %p cpuNum %d cpuMask 0x%x\n",
	     wdc, cpuNum, wdc->cpuMask);

  /* Check cpuNum is valid */
  if (!((1UL << cpuNum) & ics_state->cpuMask))
    return ICS_INVALID_ARGUMENT;

  /* The callback element must be on the watchdog list */
  if (list_empty(&wdc->list))
    return ICS_HANDLE_INVALID;

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Update the trigger cpuMask */
  wdc->cpuMask |= (1UL << cpuNum);

  /* Update the watchdog mask */
  watchdogMaskUpdate();
  
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return ICS_SUCCESS;
}

/*
 * Remove the watchdog callback associated with the
 * supplied handle
 */
ICS_ERROR ICS_watchdog_remove (ICS_WATCHDOG handle)
{
  ics_watchdog_callback_t *wdc;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  wdc = (ics_watchdog_callback_t *) handle;

  /* Try and catch bogus descriptors */
  if (handle == ICS_INVALID_HANDLE_VALUE || wdc->callback == NULL || (wdc->cpuMask & ~ics_state->cpuMask))
    return ICS_HANDLE_INVALID;

  ICS_PRINTF(ICS_DBG_WATCHDOG, "wdc %p cpuMask 0x%lx\n",
	     wdc, wdc->cpuMask);
    
  /* The callback element must be on the watchdog list */
  if (list_empty(&wdc->list))
    return ICS_HANDLE_INVALID;

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Check we're not on the trigger list */
  ICS_assert(list_empty(&wdc->tlist));

  /* Remove the callback from list */
  list_del_init(&wdc->list);

  /* Update the watchdog mask */
  watchdogMaskUpdate();

  _ICS_OS_MEMSET(wdc, 0, sizeof(*wdc));
  _ICS_OS_FREE(wdc);
  
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return ICS_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
