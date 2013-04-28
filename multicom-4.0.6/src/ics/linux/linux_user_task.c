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

#include "_ics_os.h"

static
int ics_os_set_priority (ICS_INT prio)
{
#ifdef ROUND_ROBIN_THREADS
  pthread_attr_t      attr;
  pthread_attr_init( &attr );
  
  struct sched_param  schedParam;
  
  /* Round robin scheduling */
  pthread_attr_setschedpolicy( &attr, SCHED_RR );
  
  /* Can only priority adjust if running SCHED_RR or SCHED_FIFO */
  schedParam.sched_priority = priority;
  pthread_attr_setschedparam( &attr,  &schedParam );
#endif

  return 0;
}

static
void *ics_task_helper (void *param)
{
  _ICS_OS_TASK_INFO *t = (_ICS_OS_TASK_INFO *) param;
  
  /* Set the desired task priority/class */
  ics_os_set_priority(t->priority);
  
  /* Signal that we have started */
  _ICS_OS_EVENT_POST(&t->event);

  /* Run the actual task code */
  t->entry(t->param);

  /* Signal that we are stopping */
  _ICS_OS_EVENT_POST(&t->event);
  
  pthread_exit(NULL);

  return NULL;
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

    /* Create the new task */
    if (pthread_create(&t->task, NULL, ics_task_helper, t)) {
      return ICS_SYSTEM_ERROR;
    }

    /* Wait for task to start */
    _ICS_OS_EVENT_WAIT(&t->event, ICS_TIMEOUT_INFINITE, ICS_FALSE);
    
    return ICS_SUCCESS;
}

ICS_ERROR ics_os_task_destroy (_ICS_OS_TASK_INFO *t)
{
  if (t->task == 0)
    return ICS_HANDLE_INVALID;

  /* Wait for task to exit */
  _ICS_OS_EVENT_WAIT(&t->event, ICS_TIMEOUT_INFINITE, ICS_FALSE);

  if (pthread_join(t->task, NULL)) {
    return ICS_SYSTEM_ERROR;
  }
  
  return ICS_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */


