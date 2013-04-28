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

/**************************************************************
 *
 * File: msg_recv.c
 *
 * Description
 *    Routines to implement the ICS message passing API
 *
 **************************************************************/

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

static
ICS_ERROR msgCancel (ics_event_t *event)
{
  ICS_ERROR err = ICS_SUCCESS;

  unsigned long flags;

  ICS_ASSERT(event);

  ICS_PRINTF(ICS_DBG_MSG, "Cancelling event %p[%p] state %d\n",
	     event, &event->event, event->state);
  
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);
  
  /* This needs to be protected from the interrupt handler by
   * using the interrupt blocking SPINLOCK
   */

  /* Protect against the ISR */
  _ICS_OS_SPINLOCK_ENTER(&ics_state->spinLock, flags);
  
  ICS_PRINTF(ICS_DBG_MSG, "event %p [count %d]\n",
	     event, _ICS_OS_EVENT_COUNT(&event->event));
  
  /* We must acquiesce the event before freeing it;
   * the EVENT_WAIT call will do this - it will not block
   */
  _ICS_OS_EVENT_WAIT(&event->event, ICS_TIMEOUT_IMMEDIATE, ICS_FALSE);
  
  /* Remove from posted Event list */
  list_del_init(&event->list);
  
  _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, flags);
  
  /* Now we can free off the event desc */
  ics_event_free(event);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return err;
}

ICS_ERROR ICS_msg_cancel (ICS_MSG_EVENT handle)
{
  ICS_ERROR    err;
  ics_event_t *event;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;
  
  event = (ics_event_t *) handle;

  /* Try and detect bogus handles */
  if (handle == ICS_INVALID_HANDLE_VALUE || event == NULL || event->state > _ICS_EVENT_ABORTED)
    return ICS_HANDLE_INVALID;

  if (event->state == _ICS_EVENT_FREE)
  {
    ICS_EPRINTF(ICS_DBG_MSG, "event %p[%p] state %d\n",
		event, &event->event, event->state);
    return ICS_HANDLE_INVALID;
  }
  
  /* Cancel the message event */
  err = msgCancel(event);
  
  if (err != ICS_SUCCESS)
    goto error;

  return ICS_SUCCESS;

error:
  ICS_EPRINTF(ICS_DBG_MSG, "event %p failed : %s (%d)\n",
	      event, ics_err_str(err), err);

  return err;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
