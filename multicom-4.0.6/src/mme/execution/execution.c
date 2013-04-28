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

#include <mme.h>	/* External defines and prototypes */

#include "_mme_sys.h"	/* Internal defines and prototypes */

static
void mme_execution_loop (void *param)
{
  mme_execution_task_t *exec = (mme_execution_task_t *) param;

  MME_PRINTF(MME_DBG_EXEC, "Execution Loop exec %p start\n", exec);

  while (1)
  {
    mme_receiver_t *receiver, *lowestDueReceiver;
    MME_Time_t      dueTime, lowestDueTime;

    MME_PRINTF(MME_DBG_EXEC, "exec %p blocking on event %p\n",
	       exec, &exec->event);
  
    /* Block waiting for some work to do */
    _ICS_OS_EVENT_WAIT(&exec->event, ICS_TIMEOUT_INFINITE, ICS_FALSE);

    if (exec->taskStop)
      /* We have been requested to terminate */
      return;

    /* Wakeup - examine all the associated receiver tasks
     * looking for the message with the lowest Due Time
     */
    _ICS_OS_MUTEX_TAKE(&exec->lock);

    lowestDueReceiver = NULL;
    lowestDueTime     = 0;

    if (!list_empty(&exec->receivers))
      MME_PRINTF(MME_DBG_EXEC, "Scanning receiver list %p\n",
		 &exec->receivers);

    list_for_each_entry(receiver, &exec->receivers, list)
    {
      MME_ERROR res;
      
      /* Query the lowest Due Time of this receiver */
      res = mme_receiver_due_time(receiver, &dueTime);
      if (res != MME_SUCCESS)
	/* Ignore errors and continue looking */
	continue;

      /* Is this the first message found, or before the current lowest DueTime ? */
      if (lowestDueReceiver == NULL || _MME_TIME_BEFORE(dueTime, lowestDueTime))
      {
	lowestDueTime = dueTime;
	lowestDueReceiver = receiver;
      }
    }

    _ICS_OS_MUTEX_RELEASE(&exec->lock);

    /* Did we find any work to do ? */
    if (lowestDueReceiver)
    {
      MME_PRINTF(MME_DBG_EXEC, "Found receiver %p DueTime %d\n",
		 lowestDueReceiver, lowestDueTime);

      /* Process the next command on this receiver */
      mme_receiver_process(lowestDueReceiver);
    }

  } /* while (1) */
}


/* Convert task id into the corresponding MME tuneable and then lookup its value  */
static
MME_UINT lookupTaskPriority (MME_UINT id)
{
  MME_UINT lookup[_MME_EXECUTION_TASKS] = 
    {
      MME_TUNEABLE_EXECUTION_LOOP_LOWEST_PRIORITY,
      MME_TUNEABLE_EXECUTION_LOOP_BELOW_NORMAL_PRIORITY,
      MME_TUNEABLE_EXECUTION_LOOP_NORMAL_PRIORITY,
      MME_TUNEABLE_EXECUTION_LOOP_ABOVE_NORMAL_PRIORITY,
      MME_TUNEABLE_EXECUTION_LOOP_HIGHEST_PRIORITY,
    };
  
  return MME_GetTuneable(lookup[id]);
}

#ifdef _ICS_OS_TASK_LOAD
/* STATISTICS: Execution loop callback */
static
void mme_execution_stats (ICS_STATS_HANDLE handle, void *param)
{
  mme_execution_task_t *exec = (mme_execution_task_t *) param;

  /* Update the CPU load of this task */
  ICS_stats_update(handle, 
		   _ICS_OS_TIME_TICKS2MSEC(_ICS_OS_TASK_LOAD(&exec->task)),
		   _ICS_OS_TIME_TICKS2MSEC(_ICS_OS_TIME_NOW()));

  return;
}
#endif /* _ICS_OS_TASK_LOAD */


/* Create an execution task structure for task id */
static
MME_ERROR createExectionTask (MME_UINT id)
{
  ICS_ERROR  err;
  MME_ERROR  res;
  char       taskName[_ICS_OS_MAX_TASK_NAME];

  mme_execution_task_t *exec;

  MME_assert(mme_state->executionTask[id] == NULL);

  MME_PRINTF(MME_DBG_EXEC, "Creating execution task id %d\n", id);

  /* Create a new Execution task struct */
  _ICS_OS_ZALLOC(exec, sizeof(*exec));
  if (exec == NULL)
  {
    res = MME_NOMEM;
    goto error;
  }

  /* exec lock */
  if (!_ICS_OS_MUTEX_INIT(&exec->lock))
  {
    res = MME_ICS_ERROR;
    goto error_free;
  }

  /* exec blocking event */
  if (!_ICS_OS_EVENT_INIT(&exec->event))
  {
    res = MME_ICS_ERROR;
    goto error_free;
  }

  snprintf(taskName, _ICS_OS_MAX_TASK_NAME, "mme_exec%02d", id);
  err = _ICS_OS_TASK_CREATE(mme_execution_loop, exec, lookupTaskPriority(id), taskName, &exec->task);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_EXEC, "Failed to create exec[%d] task priority %d : %s (%d)\n",
		id, lookupTaskPriority(id),
		ics_err_str(err), err);

    res = MME_ICS_ERROR;
    goto error_free;
  }

  /* Initialise rest of the Execution task struct */
  INIT_LIST_HEAD(&exec->receivers);

  /* Add the new execution task to the MME state */
  mme_state->executionTask[id] = exec;

#ifdef _ICS_OS_TASK_LOAD
  /* STATISTICS */
  {
    ICS_CHAR name[ICS_STATS_MAX_NAME+1];
    
    /* Generate a unique name for this statistic */
    snprintf(name, ICS_STATS_MAX_NAME+1, "MME Execution loop[%d] CPU load", id);
    
    ICS_stats_add(name, ICS_STATS_PERCENT, mme_execution_stats, exec, &exec->stats);
  }
#endif /* _ICS_OS_TASK_LOAD */

  MME_PRINTF(MME_DBG_EXEC, "Created execution task %p [%d] priority %d\n",
	     exec, id, lookupTaskPriority(id));

  return MME_SUCCESS;

error_free:
  _ICS_OS_FREE(exec);

error:
  return res;
}

/* Add a new receiver task to the corresponding execution task
 * Will create a new execution task if one at the requested priority does
 * not yet exist
 */
MME_ERROR mme_execution_add (mme_receiver_t *receiver, MME_Priority_t priority)
{
  MME_ERROR             res = MME_INTERNAL_ERROR;
  
  mme_execution_task_t *exec;
  
  MME_UINT              id;

  MME_PRINTF(MME_DBG_EXEC, "receiver %p priority %d\n",
	     receiver, priority);

  /* Convert the MME_Priority_t to an Execution task id */
  id = MME_PRIORITY_TO_ID(priority);

  MME_assert(id < _MME_EXECUTION_TASKS);

  _ICS_OS_MUTEX_TAKE(&mme_state->lock);

  if (mme_state->executionTask[id] == NULL)
  {
    res = createExectionTask(id);
    if (res != MME_SUCCESS)
      goto error_release;
  }

  exec = mme_state->executionTask[id];
  MME_assert(exec);

  /* Has MME_Term() been called */
  if (exec == (void *) -1)
  {
    res = MME_UNKNOWN_TRANSFORMER;
    goto error_release;
  }
    
  /* Associate this execution task with the receiver */
  receiver->exec = exec;

  _ICS_OS_MUTEX_TAKE(&exec->lock);

  MME_assert(list_empty(&receiver->list));

  /* Add new receiver to the tail of the receiver list */
  list_add_tail(&receiver->list, &exec->receivers);

  _ICS_OS_MUTEX_RELEASE(&exec->lock);

  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

  MME_PRINTF(MME_DBG_EXEC, "Successfully added receiver %p to exec %p\n",
	     receiver, exec);

  return MME_SUCCESS;

error_release:
  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

  return res;
}

/*
 * Remove a receiver task desc from the execution task list
 */
void mme_execution_remove (mme_receiver_t *receiver)
{
 mme_execution_task_t *exec;

 MME_assert(receiver);

 exec = receiver->exec;

 MME_assert(exec);

 _ICS_OS_MUTEX_TAKE(&exec->lock);

 MME_assert(!list_empty(&receiver->list));

 /* Remove receiver from exec receivers list */
 list_del_init(&receiver->list);
 
 _ICS_OS_MUTEX_RELEASE(&exec->lock);

 return;
}

/* Stop and destroy all execution tasks, freeing off memory descs */
static
void destroyAllTasks (void)
{
  int i;

  for (i = 0; i < _MME_EXECUTION_TASKS; i++)
  {
    mme_execution_task_t *exec = mme_state->executionTask[i];
    
    if (exec == NULL)
      continue;

    /* Request the task to stop and wake it */
    exec->taskStop = 1;
    _ICS_OS_EVENT_POST(&exec->event);

    /* Wait for task to exit */
    _ICS_OS_TASK_DESTROY(&exec->task);

    ICS_stats_remove(exec->stats);

    _ICS_OS_EVENT_DESTROY(&exec->event);

    _ICS_OS_MUTEX_DESTROY(&exec->lock);

    /* Signal that MME_Term() has been called */
    mme_state->executionTask[i] = (void *) -1;
    _ICS_OS_FREE(exec);
  }

  return;
}

/*
 * Terminate all the execution tasks
 *
 * Called during MME_Term() holding the mme_state lock
 */
MME_ERROR mme_execution_term (void)
{
  int i;

  for (i = 0; i < _MME_EXECUTION_TASKS; i++)
  {
    mme_execution_task_t *exec = mme_state->executionTask[i];
    
    if (exec == NULL)
      continue;

    MME_assert(exec != (void *) -1);

    /* Look for any instantiated transformers. We cannot terminate if
     * there are any. They need to be shutdown with MME_TermTransformer() first
     */
    if (!list_empty(&exec->receivers))
      return MME_COMMAND_STILL_EXECUTING;
  }

  /* Now stop and destroy all the Execution tasks */
  destroyAllTasks();

  return MME_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
