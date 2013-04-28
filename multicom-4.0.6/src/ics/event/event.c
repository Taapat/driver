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
 * File: ics_event.c
 *
 * Description
 *    Routines to allocate and initialise ICS event descs
 *
 **************************************************************/

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */


/*
 * Allocate a new event desc
 *
 * MULTITHREAD SAFE: Called holding the ics state lock
 */
ICS_ERROR ics_event_alloc (ics_event_t **eventp)
{
  ICS_ERROR    err = ICS_SYSTEM_ERROR;
  ics_event_t *event;
  
  ICS_assert(ics_state);

   /* Attempt to grab a desc from the freelist */
  if (list_empty(&ics_state->events))
  {
    /* XXXX Do we need to limit numbers or use a fixed pool size ? */

    _ICS_OS_ZALLOC(event, sizeof(ics_event_t));

    /* Failed, allocate new event desc from free memory */
    if (event == NULL)
    {
      /* Failed to allocate local memory */
      err = ICS_ENOMEM;
      goto error;
    }
    
    /* Must initialise the linked list member */
    INIT_LIST_HEAD(&event->list);

    /* Initialise the OS blocking event (NB this can be an expensive operation) */
    if (!(_ICS_OS_EVENT_INIT(&event->event)))
    {
      /* Failed for some reason */
      err = ICS_SYSTEM_ERROR;
      goto error;
    }

    /* Keep a running count of how many allocated */
    ics_state->eventCount++;

    ICS_PRINTF(ICS_DBG_MSG, "Allocated new event %p count %d\n",
	       event, ics_state->eventCount);
  }
  else
  {
    /* Remove EVENT from freelist */
    event = list_first_entry(&ics_state->events, ics_event_t, list);
    list_del_init(&event->list);
  }

  /* Event should still be free */
  ICS_ASSERT(event->state == _ICS_EVENT_FREE);

  /* Event must not be on any lists */
  ICS_ASSERT(list_empty(&event->list));

  /* Event must be idle */
  ICS_ASSERT(_ICS_OS_EVENT_COUNT(&event->event) == 0);

  /* Mark event as being allocated */
  event->state = _ICS_EVENT_ALLOC;

  /* Return the allocated event pointer */
  *eventp = event;

  return ICS_SUCCESS;

error:

  return err;
}

/*
 * Release an event desc back to the free list
 *
 * MULTITHREAD SAFE: Called holding the ics state lock
 */
void ics_event_free (ics_event_t *event)
{
  ICS_ASSERT(ics_state);
  ICS_ASSERT(event);

  ICS_PRINTF(ICS_DBG_MSG, "Free event %p state %d\n", event, event->state);

  /* Event should not be already free */
  ICS_ASSERT(event->state != _ICS_EVENT_FREE);

  /* Event must not be on any lists */
  ICS_ASSERT(list_empty(&event->list));

  /* Event must be idle */
  ICS_ASSERT(_ICS_OS_EVENT_COUNT(&event->event) == 0);

  /* Mark state as being free */
  event->state = _ICS_EVENT_FREE;

  /* Release to head of freelist */
  list_add(&event->list, &ics_state->events);

  return;
}

ICS_ERROR ics_event_init (void)
{
  /* Initialise events list */
  INIT_LIST_HEAD(&ics_state->events);
  
  return ICS_SUCCESS;
}

/*
 * Free off all events (called during ICS_term)
 */
void ics_event_term (void)
{
  ics_event_t *event, *tmp;

  list_for_each_entry_safe(event, tmp, &ics_state->events, list)
  {
    list_del(&event->list);
    
    ICS_ASSERT(_ICS_OS_EVENT_COUNT(&event->event) == 0);

    _ICS_OS_EVENT_DESTROY(&event->event);

    _ICS_OS_FREE(event);
  }
}


/*
 * Block on an event 
 */
ICS_ERROR ics_event_wait (ics_event_t *event, ICS_UINT timeout)
{
  ICS_ERROR err;

  ICS_ASSERT(event);

  ICS_ASSERT(event->state != _ICS_EVENT_FREE);

  /* Blocking wait on event desc - may timeout */
  err = _ICS_OS_EVENT_WAIT(&event->event, timeout, ICS_FALSE);

  return err;
}

/*
 * Test an event for being ready
 */
ICS_ERROR ics_event_test (ics_event_t *event, ICS_BOOL *readyp)
{
  ICS_ASSERT(event);
  ICS_ASSERT(readyp);

  /* Event should not be already free */
  ICS_ASSERT(event->state != _ICS_EVENT_FREE);

  /* Probe the current event state */
  *readyp = _ICS_OS_EVENT_READY(&event->event);

  return ICS_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
