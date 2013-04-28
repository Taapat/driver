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

#ifndef _MME_MESSAGEQ_SYS_H
#define _MME_MESSAGEQ_SYS_H

/*
 * The time comparisions allow time to wrap. they do not directly 
 * compare values but subtract with overflow wrap and compare
 * against 0. note ANSI C only guarantees overflow wrap for 
 * unsigned values (which is why the priority member is unsigned).
 *
 * Return TRUE if time A is before (or the same) as B
 *
 * Extended to allow magic time value of _MME_HIGH_PRIORITY to always
 * be before other time values.
 */
/* #define _MME_TIME_BEFORE(A, B)	((int)((A) - (B)) <= 0) */

#define _MME_TIME_BEFORE(A, B) (((A) == _MME_HIGH_PRIORITY || ((B) != _MME_HIGH_PRIORITY && ((int)((A) - (B)) <= 0))))

typedef struct mme_messageq_msg
{
  MME_UINT		 id;		/* Unique message identifier */
  MME_UINT		 priority;	/* Message priority (sort field) */
  void			*message;	/* Actual message payload */

  struct list_head	 list;		/* Doubly linked list element */
} mme_messageq_msg_t;

typedef struct mme_messageq
{
  _ICS_OS_MUTEX		 lock;		/* Protect these lists */

  mme_messageq_msg_t	*msgs;		/* Base of message array */

  struct list_head	 unsorted;	/* Unsorted messages */
  struct list_head	 sorted;	/* Sorted messages */
  struct list_head	 freelist;	/* Message freelist */
  
} mme_messageq_t;


/* Exported internal APIs */
MME_ERROR mme_messageq_init (mme_messageq_t *msgQ, MME_UINT entries);
void      mme_messageq_term (mme_messageq_t *msgQ);
MME_ERROR mme_messageq_enqueue (mme_messageq_t *msgQ, void *message, MME_UINT priority);
MME_ERROR mme_messageq_dequeue (mme_messageq_t *msgQ, void **messagep);

MME_ERROR mme_messageq_peek (mme_messageq_t *msgQ, MME_UINT *priorityp);
MME_ERROR mme_messageq_remove (mme_messageq_t *msgQ, void *message);

#endif /* _MME_MESSAGEQ_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
