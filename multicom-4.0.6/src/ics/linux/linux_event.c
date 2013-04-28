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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
/* 
 * In the absence of down_timeout() we emulate
 * the generic semaphore code as an ICS event
 */
struct event_wait {
  struct list_head list;
  struct task_struct *task;
  int ready;
};

ICS_ERROR ics_os_event_init (_ICS_OS_EVENT *event)
{
  event->count = 0;
  spin_lock_init(&event->lock);
  INIT_LIST_HEAD(&event->wait_list);

  return ICS_SUCCESS;
}
EXPORT_SYMBOL(ics_os_event_init);

static inline
ICS_ERROR event_wait (_ICS_OS_EVENT *event, ICS_ULONG timeout, ICS_BOOL interruptible)
{
  struct task_struct *task = current;
  struct event_wait wait;

  list_add_tail(&wait.list, &event->wait_list);
  wait.task = task;
  wait.ready = 0;

  for (;;) {
    if (interruptible && signal_pending(task))
      goto interrupted;

    if (timeout <= 0)
      goto timed_out;
	
    __set_current_state(interruptible ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE);
    spin_unlock_irq(&event->lock);
    timeout = schedule_timeout(timeout);
    spin_lock_irq(&event->lock);
	
    if (wait.ready)
      /* Normal wakeup */
      return ICS_SUCCESS;
  }

timed_out:
  list_del(&wait.list);
  return ICS_SYSTEM_TIMEOUT;

interrupted:
  list_del(&wait.list);
  return ICS_SYSTEM_INTERRUPT;
}

/*
 * Block forever waiting for EVENT to be signalled,
 * or timeout after timeout (ms) have expired
 * set timeout to ICS_TIMEOUT_INFINITE to wait forever
 *
 * Returns ICS_SUCCESS if signalled, ICS_SYSTEM_TIMEOUT otherwise
 */
ICS_ERROR ics_os_event_wait (_ICS_OS_EVENT *event, ICS_ULONG timeout, ICS_BOOL interruptible)
{
  ICS_ERROR err = ICS_SUCCESS;
  unsigned long flags;

  /* Convert timeout value */
  if (timeout == ICS_TIMEOUT_INFINITE)
    timeout = MAX_SCHEDULE_TIMEOUT;
  else
    timeout = _ICS_OS_TIME_MSEC2TICKS(timeout);
    
  spin_lock_irqsave(&event->lock, flags);
  if (event->count > 0)
    event->count--;
  else
    err = event_wait(event, timeout, interruptible);
  spin_unlock_irqrestore(&event->lock, flags);
  
  return err;
}
EXPORT_SYMBOL(ics_os_event_wait);

void ics_os_event_post (_ICS_OS_EVENT *event)
{
  unsigned long flags;
    
  spin_lock_irqsave(&event->lock, flags);
  if (list_empty(&event->wait_list))
    event->count++;
  else
  {
    struct event_wait *wait = list_first_entry(&event->wait_list, struct event_wait, list);
	
    list_del(&wait->list);
    wait->ready = 1;
    wake_up_process(wait->task);
  }
  spin_unlock_irqrestore(&event->lock, flags);
}
EXPORT_SYMBOL(ics_os_event_post);

#else

/*
 * Block forever waiting for EVENT to be signalled,
 * or timeout after timeout (ms) have expired
 * set timeout to ICS_TIMEOUT_INFINITE to wait forever
 *
 * Returns ICS_SUCCESS if signalled, ICS_SYSTEM_TIMEOUT otherwise
 */
ICS_ERROR ics_os_event_wait (_ICS_OS_EVENT *event, ICS_ULONG timeout, ICS_BOOL interruptible)
{
  int res;
  
  ICS_ASSERT(event);

  /* Convert timeout value */
  if (timeout == ICS_TIMEOUT_INFINITE)
    timeout = MAX_SCHEDULE_TIMEOUT;
  else
    timeout = _ICS_OS_TIME_MSEC2TICKS(timeout);
  
  if (timeout == MAX_SCHEDULE_TIMEOUT)
    /* Make infinite timeouts either interruptible or killable */
    res = interruptible ? down_interruptible(event) : down_killable(event);
  else
    /* Deschedule for up to the timeout period */
    res = down_timeout(event, timeout);
  
  if (res == 0)
    /* Normal wakeup */
    return ICS_SUCCESS;
  
  /* Failure: distinguish between a timeout and an interrupt */
  return ((res == -ETIME) ? ICS_SYSTEM_TIMEOUT : ICS_SYSTEM_INTERRUPT);
}
EXPORT_SYMBOL(ics_os_event_wait);

#endif /* LINUX_VERSION_CODE < 2.6.26 */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
