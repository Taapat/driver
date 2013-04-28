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
 * Allocate an new msg consisting of a local desc
 * and a chunk of memory from the MME (shared) heap
 */
static mme_msg_t *allocMsg (MME_UINT size)
{
  mme_msg_t *msg;
  void      *memBase;

  MME_ASSERT(mme_state->heap);

  /* Allocate the local msg desc */
  _ICS_OS_ZALLOC(msg, sizeof(mme_msg_t));
  if (msg == NULL)
  {
    return NULL;
  }

  /* Allocate a chunk of memory which is shared with the Companions */
  memBase = ics_heap_alloc(mme_state->heap, size, _MME_MSG_CACHE_FLAGS);
  if (memBase == NULL)
  {
    goto error_free;
  }
  
  MME_PRINTF(MME_DBG_COMMAND, "Allocated msg %p memBase %p\n",
	     msg, memBase);
  
  /* Initialise msg link head */
  INIT_LIST_HEAD(&msg->list);

  /* Remember the allocated size */
  msg->size = size;

  /* Assign the SHM buffer to the message */
  msg->buf = (void *) memBase;

  MME_PRINTF(MME_DBG_COMMAND,
	     "Allocated msg %p size %d buf %p\n",
	     msg, size, msg->buf);

  return msg;

error_free:
  _ICS_OS_FREE(msg);

  return NULL;
}

/*
 * Allocate an MME meta data message
 *
 * These consist of a local desc and a pointer to
 * a buffer which is mapped into the Companion address space
 *
 * A cache of previously allocated msgs is kept, but only
 * msgs of <= _MME_MSG_SMALL_SIZE are kept on this list
 *
 */
mme_msg_t *mme_msg_alloc (MME_UINT size)
{
  mme_msg_t *msg;

  MME_ASSERT(mme_state);

  /* Must allocate whole cachelines as these messages
   * get flushed in and out of the caches
   */
  size = (MME_UINT) MME_CACHE_LINE_ALIGN(size);
  
  /* Small msg request, check cache */
  if (size <= _MME_MSG_SMALL_SIZE)
  {
    _ICS_OS_MUTEX_TAKE(&mme_state->lock);
    
    if (!list_empty(&mme_state->freeMsgs))
    {
      msg = list_first_entry(&mme_state->freeMsgs, mme_msg_t, list);
      list_del_init(&msg->list);
      
      _ICS_OS_MUTEX_RELEASE(&mme_state->lock);
      
      MME_ASSERT(msg->size >= size);

      /* DEBUG: Remember who allocated this */
      msg->owner = RETURN_ADDRESS(0);

      return msg;
    }
    else
    {
      /* Allocate a small sized message */
      size = _MME_MSG_SMALL_SIZE;
 
      _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

      /* FALLTHRU to allocate new msg */
    }
 }    

  /*
   * New msg request
   */
   
  /* Attempt to dynamically allocate a new msg */
  msg = allocMsg(size);

  if (msg)
  {
    MME_PRINTF(MME_DBG_COMMAND,
	       "Allocated msg %p size %d buf %p\n",
	       msg, size, msg->buf);

    /* DEBUG: Remember who allocated this */
    msg->owner = RETURN_ADDRESS(0);
  }
  else
  {
    MME_EPRINTF(MME_DBG_COMMAND,
		"Failed to allocated msg of size %d\n",
		size);
  }

  return msg;
}


/*
 * Free an MME meta data message
 *
 * We cache small msgs by keeping them on a freelist
 */
void mme_msg_free (mme_msg_t *msg)
{
  MME_ASSERT(mme_state);
  MME_ASSERT(msg);

  /* msg must be free */
  MME_ASSERT(list_empty(&msg->list));

  if (msg->size <= _MME_MSG_SMALL_SIZE)
  {
    /* 
     * Small msg, cache msg + desc
     */
    _ICS_OS_MUTEX_TAKE(&mme_state->lock);
    
    MME_assert(list_empty(&msg->list));

    msg->owner = NULL;

    /* Free msg back to head of freelist */
    list_add(&msg->list, &mme_state->freeMsgs);
    
    _ICS_OS_MUTEX_RELEASE(&mme_state->lock);
  }
  else
  {
    /*
     * Large msg free off msg + desc
     */
    MME_ASSERT(mme_state->heap);
    MME_ASSERT(msg->buf);

    MME_PRINTF(MME_DBG_COMMAND,
	       "Free msg %p size %d buf %p\n",
	       msg, msg->size, msg->buf);

    ics_heap_free(mme_state->heap, msg->buf);

    _ICS_OS_FREE(msg);
  }

  return;
}

/*
 * The MME Meta data messages are dynamically
 * allocated and maintained as a cache via a linked
 * list
 */
MME_ERROR mme_msg_init (void)
{
  MME_assert(mme_state);

  /* Initialise the cache linked list head */
  INIT_LIST_HEAD(&mme_state->freeMsgs);

  return MME_SUCCESS;
}

/*
 * Release the MME meta data memory and unmap any translations
 *
 * Called during MME_Term() holding the mme_state lock
 */
void mme_msg_term (void)
{
  mme_msg_t *msg, *tmp;
  
  MME_assert(mme_state);

  /* Cope with partial initialisation call from MME_Term() */
  if (!mme_state->heap)
    return;
  
  /* Free off all the remaining cached msgs */
  list_for_each_entry_safe(msg, tmp, &mme_state->freeMsgs, list)
  {
    MME_assert(msg->buf);

    list_del(&msg->list);
    
    ics_heap_free(mme_state->heap, msg->buf);

    _ICS_OS_FREE(msg);
  }

  MME_ASSERT(list_empty(&mme_state->freeMsgs));

  return;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
