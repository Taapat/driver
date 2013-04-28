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

#if defined(__KERNEL__)

#include <linux/sched.h>

static int setPriority (struct task_struct *kt, int policy, struct sched_param *sp)
{
  int res = -1;
  
  res = sched_setscheduler(kt, policy, sp); 	/* GPL only function */

  if (res)
    ICS_EPRINTF(ICS_DBG_INIT, "sched_setscheduler(%p, %d) failed : %d\n",
		kt, sp->sched_priority, res);
  
  return res;
}

/* Set the priority/class of a task (NULL means current) */
int ics_os_set_priority (struct task_struct *kt, ICS_INT prio)
{
  int res = 0;
  
  if (kt == NULL)
    kt = current;

  /* priorities beyond -20 provoke a different scheduling policy */
  if (prio < -20)
  {
    struct sched_param sp;
    
    /* Set the RT task priority (1 .. 99) and RT scheduler class */
    sp.sched_priority = -(prio + 20);
    
    res = setPriority(kt, SCHED_RR, &sp);
  }
  else
  {
    /* Priorities >= -20 we assume its a nice level */
    set_user_nice(kt, prio);
  }
  
  return res;
}

static int ics_task_helper (void *param)
{
  _ICS_OS_TASK_INFO *t = (_ICS_OS_TASK_INFO *) param;

  /* Set the desired task priority/class */
  ics_os_set_priority(t->task, t->priority);
  
  /* Signal that we have started */
  _ICS_OS_EVENT_POST(&t->event);

  /* Run the actual task code */
  t->entry(t->param);

  /* Signal that we are stopping */
  _ICS_OS_EVENT_POST(&t->event);
  
  /* wait for the user to call kthread_stop() */
  set_current_state(TASK_INTERRUPTIBLE);
  while (!kthread_should_stop())
  {
    schedule();
    set_current_state(TASK_INTERRUPTIBLE);
  }

  __set_current_state(TASK_RUNNING);

  return 0;
}

ICS_ERROR ics_os_task_create (void (*entry)(void *), void *param, ICS_INT priority, const ICS_CHAR *name,
			      _ICS_OS_TASK_INFO *t)
{
    /* Fill out supplied task info struct */
    t->entry = entry;
    t->param = param;
    t->priority = priority;

    if (!_ICS_OS_EVENT_INIT(&t->event))
      return ICS_SYSTEM_ERROR;

    t->task = kthread_create(ics_task_helper, t, "%s", name);

    if (IS_ERR(t->task))
      return ICS_SYSTEM_ERROR;

    /* Now start the task */
    wake_up_process(t->task);

    /* Wait for task to start */
    _ICS_OS_EVENT_WAIT(&t->event, ICS_TIMEOUT_INFINITE, ICS_FALSE);

    return ICS_SUCCESS;
}

ICS_ERROR ics_os_task_destroy (_ICS_OS_TASK_INFO *t)
{
  int res = ICS_SUCCESS;
  
  if (t->task == NULL)
    return ICS_HANDLE_INVALID;

  /* Wait for task to exit */
  _ICS_OS_EVENT_WAIT(&t->event, ICS_TIMEOUT_INFINITE, ICS_FALSE);

  res = kthread_stop(t->task);
  if (res != 0)
  {
#if 0
    ICS_EPRINTF(ICS_DBG_INIT, "kthread_stop t %p kthread %p(%s) returned %d\n",
		t, t->task, t->task->comm, res);
#endif
    
    res = ICS_SYSTEM_INTERRUPT;
    
    /* FALLTHRU */
  }

  /* Mark as invalid */
  t->task = NULL;

  _ICS_OS_EVENT_DESTROY(&t->event);

  return res;
}

#endif /* __KERNEL__ */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
