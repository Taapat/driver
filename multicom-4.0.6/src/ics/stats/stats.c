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
 * Map these directly onto the transport layer
 */

ICS_ERROR ICS_stats_sample (ICS_UINT cpuNum, ICS_STATS_ITEM *stats, ICS_UINT *nstatsp)
{
  /* Sample all the current stats from the transport */
  return ics_transport_stats_sample(cpuNum, stats, nstatsp);
}

ICS_ERROR ICS_stats_update (ICS_STATS_HANDLE handle, ICS_STATS_VALUE value, ICS_STATS_TIME timestamp)
{
  ics_stats_t *stats;
  
  stats = (ics_stats_t *) handle;

  if (stats == NULL || stats->handle >= ICS_STATS_MAX_ITEMS)
    return ICS_HANDLE_INVALID;

  /* Update the value in the transport */
  return ics_transport_stats_update(stats->handle, value, timestamp);
}

ICS_ERROR ICS_stats_add (const ICS_CHAR *name, ICS_STATS_TYPE type, ICS_STATS_CALLBACK callback, ICS_VOID *param,
			 ICS_STATS_HANDLE *handlep)
{
  ICS_ERROR err;

  ics_stats_t *stats;

  ICS_STATS_HANDLE handle;

  if (name == NULL || handlep == NULL || strlen(name) == 0 || strlen(name) > ICS_STATS_MAX_NAME)
    return ICS_INVALID_ARGUMENT;

  ICS_PRINTF(ICS_DBG_STATS, "name '%s' callback %p param %p handlep %p\n",
	     name, callback, param, handlep);

  /* Attempt to allocate a transport stats desc */
  err = ics_transport_stats_add(name, type, &handle);
  if (err != ICS_SUCCESS)
    return err;

  /* Allocate a new descriptor */
  _ICS_OS_ZALLOC(stats, sizeof(*stats));
  if (stats == NULL)
  {
    ics_transport_stats_remove(handle);
    return ICS_ENOMEM;
  }

  /* 
   * Fill out/zero stat entry
   */
  stats->handle    = handle;
  stats->callback  = callback;
  stats->param     = param;

  INIT_LIST_HEAD(&stats->list);
  INIT_LIST_HEAD(&stats->tlist);

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Add to stats list */
  list_add(&stats->list, &ics_state->statsCallback);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  ICS_PRINTF(ICS_DBG_STATS, "stats %p handle 0x%x\n",
	     stats, stats->handle);

  /* Return opaque handle to caller */
  *handlep = (ICS_STATS_HANDLE) stats;

  return ICS_SUCCESS;
}

ICS_ERROR ICS_stats_remove (ICS_STATS_HANDLE handle)
{
  ics_stats_t *stats;

  stats = (ics_stats_t *) handle;

  if (stats == NULL || stats->handle >= ICS_STATS_MAX_ITEMS)
    return ICS_HANDLE_INVALID;

  ICS_PRINTF(ICS_DBG_STATS, "stats %p handle 0x%x\n", stats, stats->handle);

  /* Should not already be free */
  if (list_empty(&stats->list))
    return ICS_HANDLE_INVALID;

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Must not be on a trigger list */
  ICS_assert(list_empty(&stats->tlist));

  /* Remove from list */
  list_del_init(&stats->list);

  /* Remove from transport layer */
  ics_transport_stats_remove(stats->handle);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  /* Free off stats desc */
  _ICS_OS_FREE(stats);

  return ICS_SUCCESS;
}

/*
 * Called periodically (by the watchdog task) to update all registered stats
 *
 * As we need to call a user function for each triggered callback
 * the callback descriptors are copied onto a temporary list whilst holding
 * the state lock. Then having dropped the lock, all the callbacks are called/
 
 * This allows the user callback to use other ICS functions safely
 */
void ics_stats_trigger (void)
{
  ics_stats_t *stats, *tmp;

  LIST_HEAD(triggerList);

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);
  
  list_for_each_entry(stats, &ics_state->statsCallback, list)
  {
    if (stats->callback)
    {
      /* Add to the trigger list */
      list_add(&stats->tlist, &triggerList);
    }
  }

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  /* Now whilst not holding the state lock call all
   * the callbacks we found above
   */
  list_for_each_entry_safe(stats, tmp, &triggerList, tlist)
  {
    /* Remove from the trigger list */
    list_del_init(&stats->tlist);
    
    /* Call the user callback, having dropped the state lock */
    (stats->callback) ((ICS_STATS_HANDLE) stats, stats->param);
  }

  return;
}

/*
 * The Stats helper task
 *
 * Blocks on an OS event, running the Stats run function
 * every _ICS_STATS_INTERVAL time period
 *
 * Will exit once the OS Event is signalled (i.e. no timeout)
 */
void ics_stats_task (void *param)
{
  ICS_ERROR err = ICS_SUCCESS;
    
  ICS_ASSERT(ics_state);
  
  ICS_PRINTF(ICS_DBG_INIT, "Starting: state %p\n", ics_state);
  
  /* Run the triggers immediately to generate the first timestamp */
  ics_stats_trigger();

  while (1)
  {
    /* Block on the watchdog event, waking after _ICS_STATS_INTERVAL
     *
     * This event only gets signalled when we are being asked to exit
     */
    err = _ICS_OS_EVENT_WAIT(&ics_state->statsEvent, _ICS_STATS_INTERVAL, ICS_FALSE);
    
    /* Stop requested: get out of here */
    if (err != ICS_SYSTEM_TIMEOUT)
      break;
    
    /* Timer expiry, run the Stats callbacks */
    ics_stats_trigger();
  }

  ICS_PRINTF(ICS_DBG_INIT, "Exiting:  %s (%d)\n",
	     ics_err_str(err), err);

  /* Task exit */
  return;
}

#ifdef STATS_TASK
/*
 * Create and start the Stats helper task 
 * Runs on all CPUs
 */
static
ICS_ERROR createStatsTask (void)
{
  ICS_ERROR err;

  char taskName[_ICS_OS_MAX_TASK_NAME];

  ICS_ASSERT(ics_state);

  snprintf(taskName, _ICS_OS_MAX_TASK_NAME, "ics_stats");

  /* Create a high priority thread */
  err = _ICS_OS_TASK_CREATE(ics_stats_task, NULL, _ICS_OS_TASK_DEFAULT_PRIO, taskName,
			    &ics_state->statsTask);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "Failed to create Stats task %s : %d\n", taskName, err);
    
    return ICS_NOT_INITIALIZED;
  }
  
  ICS_PRINTF(ICS_DBG_INIT, "created Stats task %p\n", ics_state->statsTask.task);

  return ICS_SUCCESS;
}
#endif /* STATS_TASK */

ICS_ERROR ics_stats_init (void)
{
#ifdef STATS_TASK
  ICS_ERROR err;
#endif

  /* Initialise registered stats handler list */
  INIT_LIST_HEAD(&ics_state->statsCallback);

#ifdef STATS_TASK
  /* Stats task waits on this */
  if (!(_ICS_OS_EVENT_INIT(&ics_state->statsEvent)))
  {
    return ICS_SYSTEM_ERROR;
  }

  /* Create the Stats task */
  err = createStatsTask();
  if (err != ICS_SUCCESS)
  {
    /* Failed to create the Stats task */
    ICS_EPRINTF(ICS_DBG_STATS|ICS_DBG_INIT, "Failed to create Stats task : %s (%d)\n", 
		ics_err_str(err), err);
    
    return err;
  }
#endif /* STATS_TASK */

  return ICS_SUCCESS;
}

void ics_stats_term (void)
{
  ics_stats_t *stats, *tmp;

#ifdef STATS_TASK
  if (ics_state->statsTask.task != NULL)
  {
    /* Wake the Stats task */
    _ICS_OS_EVENT_POST(&ics_state->statsEvent);

    _ICS_OS_TASK_DESTROY(&ics_state->statsTask);
  }
#endif /* STATS_TASK */

  /* Remove all registered stats handlers */
  list_for_each_entry_safe(stats, tmp, &ics_state->statsCallback, list)
  {
    ICS_ERROR err;
    
    err = ICS_stats_remove((ICS_STATS_HANDLE) stats);
    if (err != ICS_SUCCESS)
    {
      ICS_EPRINTF(ICS_DBG_STATS, "Failed to remove stats entry %p : %s (%d)\n",
		  stats,
		  ics_err_str(err), err);
    }
  }

  ICS_assert(list_empty(&ics_state->statsCallback));

  return;
}

/*
 * ICS Memory usage stats callback handler
 */
void
ics_stats_mem (ICS_STATS_HANDLE handle, void *param)
{
  ICS_STATS_VALUE total = _ICS_DEBUG_MEM_TOTAL();
  
  ICS_stats_update(handle, total, _ICS_OS_TIME_TICKS2MSEC(_ICS_OS_TIME_NOW()));
  
  return;
}

/*
 * System Memory usage stats callback handler
 */
void
ics_stats_mem_sys (ICS_STATS_HANDLE handle, void *param)
{
  ICS_STATS_VALUE total = _ICS_OS_MEM_USED();
  
  ICS_stats_update(handle, total, _ICS_OS_TIME_TICKS2MSEC(_ICS_OS_TIME_NOW()));
  
  return;
}

/*
 * CPU load stats callback handler 
 */
void
ics_stats_cpu (ICS_STATS_HANDLE handle, void *param)
{
  ICS_stats_update(handle, 
		   _ICS_OS_TIME_TICKS2MSEC(_ICS_OS_TIME_IDLE()), /* idle msec */
		   _ICS_OS_TIME_TICKS2MSEC(_ICS_OS_TIME_NOW())); /* now in msec */

  return;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
