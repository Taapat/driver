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

#ifndef _ICS_EVENT_SYS_H
#define _ICS_EVENT_SYS_H

typedef enum ics_event_state
{
  _ICS_EVENT_FREE = 0,
  _ICS_EVENT_ALLOC,
  _ICS_EVENT_ABORTED,
} ics_event_state_t;

typedef struct ics_event
{
  ics_event_state_t	 state;		/* Event state */

  struct list_head	 list;		/* Doubly linked list entry */

  _ICS_OS_EVENT		 event;		/* OS Event for blocking on */

  void 			*data;		/* Event specific data */

} ics_event_t;

ICS_EXPORT ICS_ERROR ics_event_alloc (ics_event_t **event);
ICS_EXPORT void      ics_event_free (ics_event_t *event);
ICS_EXPORT ICS_ERROR ics_event_wait (ics_event_t *event, ICS_UINT timeout);
ICS_EXPORT ICS_ERROR ics_event_test (ics_event_t *event, ICS_BOOL *readyp);

ICS_EXPORT ICS_ERROR ics_event_init (void);
ICS_EXPORT void      ics_event_term (void);

/* 
 * Signal an event
 */
_ICS_OS_INLINE_FUNC_PREFIX 
void ics_event_post (ics_event_t *event)
{
  ICS_ASSERT(event);
  
  _ICS_OS_EVENT_POST(&event->event);
}


#endif /* _ICS_EVENT_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
