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

/*
 * Routines to implement an Message queue where
 * messages are managed on priority sorted FIFOs
 */


/* Initialise a message queue of the given number of entries */
MME_ERROR mme_messageq_init (mme_messageq_t *msgQ, MME_UINT entries)
{
  int i;
  
  mme_messageq_msg_t *msgs;

  MME_assert(msgQ);
  MME_assert(entries);
  
  MME_assert(msgQ->msgs == NULL);

  if (!(_ICS_OS_MUTEX_INIT(&msgQ->lock)))
    return MME_ICS_ERROR;
  
  /* Allocate the indicated number of message queue entries */
  _ICS_OS_ZALLOC(msgs, sizeof(mme_messageq_msg_t) * entries);
  if (msgs == NULL)
    return MME_NOMEM;

  /* Stash base addr of message array */
  msgQ->msgs = msgs;

  INIT_LIST_HEAD(&msgQ->unsorted);
  INIT_LIST_HEAD(&msgQ->sorted);
  INIT_LIST_HEAD(&msgQ->freelist);

  /* Add all messages to the free list */
  for (i = 0; i < entries; i++)
    list_add_tail(&msgs[i].list, &msgQ->freelist);

  MME_assert(!list_empty(&msgQ->freelist));
  MME_assert(list_empty(&msgQ->sorted));
  MME_assert(list_empty(&msgQ->unsorted));

  return MME_SUCCESS;
}

/* Destroy a message queue, releasing all the memory */
void mme_messageq_term (mme_messageq_t *msgQ)
{
  MME_assert(msgQ);
  MME_assert(msgQ->msgs);

  /* All messages should now be back on the freelist */
  MME_assert(list_empty(&msgQ->sorted));
  MME_assert(list_empty(&msgQ->unsorted));

  /* Free off all message memory */
  _ICS_OS_FREE(msgQ->msgs);
  
  _ICS_OS_MUTEX_DESTROY(&msgQ->lock);

  /* Zero control structure */
  _ICS_OS_MEMSET(msgQ, 0, sizeof(*msgQ));

  return;
}

/* take the unsorted list and merge it with the sorted list.
 * The time comparisions allow time to wrap. they do not directly 
 * compare values but subtract with overflow wrap and compare
 * against 0. note ANSI C only guarantees overflow wrap for 
 * unsigned values (which is why the priority member is unsigned).
 *
 */
static void sortMessageQueue (mme_messageq_t *msgQ)
{
  mme_messageq_msg_t *unsorted, *tmp;
  
  /* Check for nothing to do */
  if (list_empty(&msgQ->unsorted))
    return;
  
  /* For each msg on the unsorted queue */
  list_for_each_entry_safe(unsorted, tmp, &msgQ->unsorted, list)
  {
    /* search the sorted queue until we find a entry with a (strictly) more
     * distant due date
     */
    mme_messageq_msg_t *sorted;
    
    /* Remove from unsorted list */
    list_del_init(&unsorted->list);

    /* search the queue until we find our insertion point.
     */
    list_for_each_entry(sorted, &msgQ->sorted, list)
    {
      /* Break out as soon as we find insertion point */
      if (!_MME_TIME_BEFORE(sorted->priority, unsorted->priority))
	break;
    }
    
    /* End of list reached or sorted is after unsorted, add unsorted before sorted */
    list_add_tail(&unsorted->list, &sorted->list);

  } /* for each unsorted */

  MME_ASSERT(list_empty(&msgQ->unsorted));
  MME_ASSERT(!list_empty(&msgQ->sorted));
}

MME_ERROR mme_messageq_enqueue (mme_messageq_t *msgQ, void *message, MME_UINT priority)
{
  mme_messageq_msg_t *msg;

  mme_messageq_msg_t *last, *first;

  MME_ASSERT(msgQ);
  MME_ASSERT(msgQ->msgs);
  MME_ASSERT(message);

  _ICS_OS_MUTEX_TAKE(&msgQ->lock);

  if (list_empty(&msgQ->freelist))
  {
    _ICS_OS_MUTEX_RELEASE(&msgQ->lock);

    return MME_NOMEM;
  }

  /* Pop the head of the freelist */
  msg = list_first_entry(&msgQ->freelist, mme_messageq_msg_t, list);
  list_del_init(&msg->list);

  /* Fill out the message desc */
  msg->priority = priority;
  msg->message  = message;

  /*
   * Attempt to insert the new msg onto the sorted list
   * First check tail and then head for simple cases
   * otherwise defer sort until later
   */
  if (list_empty(&msgQ->sorted))
  {
    MME_ASSERT(list_empty(&msgQ->unsorted));

    /* sorted list empty, just add it */
    list_add(&msg->list, &msgQ->sorted);
  }
#if 1
  /* XXXX Is this safe ? */
  else if (list_empty(&msgQ->unsorted))
  {
    /* Examine the tail of the sorted list */
    last = list_last_entry(&msgQ->sorted, mme_messageq_msg_t, list);
    
    if (_MME_TIME_BEFORE(last->priority, msg->priority))
	/* Add to tail of sorted list */
	list_add_tail(&msg->list, &msgQ->sorted);
    else
    {
      /* Examine the head of the sorted list */
      first = list_first_entry(&msgQ->sorted, mme_messageq_msg_t, list);
      if (!_MME_TIME_BEFORE(first->priority, msg->priority))
	/* Add to head of sorted list */
	list_add(&msg->list, &msgQ->sorted);
      else
	/* Give up and sort later */
	list_add_tail(&msg->list, &msgQ->unsorted);
    }
  }
#endif
  else
    /* Sort later */
    list_add_tail(&msg->list, &msgQ->unsorted);
	   
  _ICS_OS_MUTEX_RELEASE(&msgQ->lock);

  return MME_SUCCESS;
}

/* Return the payload pointer of the sorted message queue
 *
 * Returns MME_INVALID_HANDLE if no message are available
 */
MME_ERROR mme_messageq_dequeue (mme_messageq_t *msgQ, void **messagep)
{
  mme_messageq_msg_t *msg;
  
  MME_ASSERT(msgQ);
  MME_ASSERT(messagep);

  _ICS_OS_MUTEX_TAKE(&msgQ->lock);

  /* Need to sort list */
  sortMessageQueue(msgQ);

  if (list_empty(&msgQ->sorted))
  {
    _ICS_OS_MUTEX_RELEASE(&msgQ->lock);

    return MME_INVALID_HANDLE;
  }

  /* Pop the head of the sorted list */
  msg = list_first_entry(&msgQ->sorted, mme_messageq_msg_t, list);
  list_del_init(&msg->list);

  /* Return payload pointer */
  *messagep = msg->message;

  /* Release the message desc back to the freelist (add to head to improve cache hits) */
  list_add(&msg->list, &msgQ->freelist);

  _ICS_OS_MUTEX_RELEASE(&msgQ->lock);

  return MME_SUCCESS;
}

/* Peek at the head of the sorted message queue and return its priority value
 *
 * Returns MME_INVALID_HANDLE if no messages are available
 */
MME_ERROR mme_messageq_peek (mme_messageq_t *msgQ, MME_UINT *priorityp)
{
  mme_messageq_msg_t *msg;
  
  MME_ASSERT(msgQ);
  MME_ASSERT(priorityp);

  _ICS_OS_MUTEX_TAKE(&msgQ->lock);

  /* Need to sort list */
  sortMessageQueue(msgQ);

  if (list_empty(&msgQ->sorted))
  {
    _ICS_OS_MUTEX_RELEASE(&msgQ->lock);

    return MME_INVALID_HANDLE;
  }

  /* Look at the head of the sorted list */
  msg = list_first_entry(&msgQ->sorted, mme_messageq_msg_t, list);

  /* Return the associated priority */
  *priorityp = msg->priority;

  _ICS_OS_MUTEX_RELEASE(&msgQ->lock);

  return MME_SUCCESS;
}


/* Scan the messageq looking for the supplied message address
 * and remove that entry if found.
 */
MME_ERROR mme_messageq_remove (mme_messageq_t *msgQ, void *message)
{
  mme_messageq_msg_t *msg;

  _ICS_OS_MUTEX_TAKE(&msgQ->lock);

  /* Need to sort list */
  sortMessageQueue(msgQ);

  if (list_empty(&msgQ->sorted))
  {
    _ICS_OS_MUTEX_RELEASE(&msgQ->lock);

    return MME_INVALID_HANDLE;
  }

  /* Scan whole list looking for a matching id */
  list_for_each_entry(msg, &msgQ->sorted, list)
  {
    if (msg->message == message)
    {
      /* Found a match, remove, free and exit */
      list_del(&msg->list);
      list_add(&msg->list, &msgQ->freelist);

      _ICS_OS_MUTEX_RELEASE(&msgQ->lock);
      
      return MME_SUCCESS;
    }
  }
  
  _ICS_OS_MUTEX_RELEASE(&msgQ->lock);

  /* No match found */

  return MME_INVALID_HANDLE;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
