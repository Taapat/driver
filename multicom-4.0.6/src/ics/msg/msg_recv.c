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

/*
 * Update the user's rdesc structure with the supplied message
 *
 * NB: Can be called from Interrupt context
 *
 * Called holding the state SPINLOCK
 */
_ICS_OS_INLINE_FUNC_PREFIX
void rdescUpdate (ICS_MSG_DESC *rdesc, ics_msg_t *msg)
{
  ICS_SIZE      size   = msg->size;
  ICS_MEM_FLAGS mflags = msg->mflags;
  ICS_UINT      srcCpu = msg->srcCpu;

  /* Copy msg info + data into rdesc desc */
  rdesc->mflags = mflags;
  rdesc->size   = size;
  rdesc->srcCpu = srcCpu;

  ICS_PRINTF(ICS_DBG_MSG, "rdesc %p mflags 0x%x size %d srcCpu %d\n",
	     rdesc, rdesc->mflags, rdesc->size, rdesc->srcCpu);
  
  /* Copy any inline data payload */
  if (mflags & ICS_INLINE)
  {
    ICS_ASSERT(size <= ICS_MSG_INLINE_DATA);
    
    ICS_PRINTF(ICS_DBG_MSG, "Copy msg payload %p to %p size %d\n",
	       msg->payload, rdesc->payload, size);
    
    _ICS_OS_MEMCPY(rdesc->payload, msg->payload, size);
    
    /* Point data at inline data for simplicity */
    rdesc->data = (ICS_OFFSET) &rdesc->payload;
  }
  else if (mflags & ICS_PHYSICAL)
  {
    /* Present physical address to the receiver */
    rdesc->data = msg->data;
  }
  else if (rdesc->size)
  {
    ICS_ERROR  err;
    ICS_OFFSET paddr = msg->data;

    ICS_ASSERT(paddr != ICS_BAD_OFFSET);
    
    /* Zero-copy payload - convert the paddr to a local virtual (cached/uncached) mapping */
    err = ICS_region_phys2virt(paddr, rdesc->size, rdesc->mflags, (void *)&rdesc->data);

    if (err != ICS_SUCCESS)
    {
      ICS_EPRINTF(ICS_DBG_MSG, "Failed to convert paddr 0x%x cpu %d to %s addr %p: %d\n",
		  paddr, rdesc->srcCpu, 
		  ((rdesc->mflags & ICS_CACHED) ? "CACHED" : "UNCACHED"), 
		  rdesc->data, err);

#ifdef ICS_DEBUG
      /* Dump out the region table on error */
      ics_region_dump();
#endif
    }
    else
      ICS_PRINTF(ICS_DBG_MSG, "Converted paddr 0x%x cpu %d to %s addr %p\n",
		 paddr, rdesc->srcCpu, 
		 ((rdesc->mflags & ICS_CACHED) ? "CACHED" : "UNCACHED"), 
		 rdesc->data);

    /* XXXX Cannot recover */
    ICS_assert(err == ICS_SUCCESS);
      
    if (rdesc->mflags & ICS_CACHED)
    {
      /* Receiver is about to read, so need to purge any old cache entries */
      /* XXXX Use PURGE instead of INVALIDATE as it is safer, especially if it's a loopback send */
      _ICS_OS_CACHE_PURGE(rdesc->data, paddr, rdesc->size);
    }
  }

  return;
}

/*
 * Per channel message callback
 * Called in ISR context for each new message on the CPU FIFO
 *
 * Needs to decode the msg target port and copy the message
 * to the associated per port message queue
 *
 * MULTITHREAD SAFE: Needs to be atomic against new rdescs being posted
 * and adding to the MQ. This is achieved by using the ics state spinLock
 */
ICS_ERROR ics_message_handler (ICS_CHANNEL channel, void *p, void *buf)
{
  ics_cpu_t  *cpu = (ics_cpu_t *) p;
  ics_msg_t  *msg = (ics_msg_t *) buf;
  ics_port_t *lp;

  size_t      msgSize;
  int         type, cpuNum, ver, idx;

  unsigned long flags;

  ICS_ERROR   err = ICS_SYSTEM_ERROR;

  ICS_ASSERT(ics_state);
  ICS_ASSERT(cpu);
  ICS_ASSERT(buf);

  ICS_PRINTF(ICS_DBG_MSG, "cpu %p msg %p flags 0x%x seqNo %d\n",
	     cpu, msg, msg->mflags, msg->seqNo);

  /* Decode the target port handle */
  _ICS_DECODE_HDL(msg->port, type, cpuNum, ver, idx);

  _ICS_OS_SPINLOCK_ENTER(&ics_state->spinLock, flags);

  ICS_PRINTF(ICS_DBG_MSG, "port 0x%x -> type 0x%x cpuNum %d ver %d idx %d\n",
	     msg->port, type, cpuNum, ver, idx);

  /* Check the target port */
  if (type != _ICS_TYPE_PORT || cpuNum >= _ICS_MAX_CPUS || idx >= ics_state->portEntries)
  {
    ICS_EPRINTF(ICS_DBG_MSG, "Invalid port 0x%x -> type 0x%x cpuNum %d ver %d idx %d\n",
		msg->port, type, cpuNum, ver, idx);
		
    err = ICS_HANDLE_INVALID;
    goto consumeMsg;		/* Error but still consume msg slot */
  }

  /* Check msg seqNo */
  if (msg->seqNo != cpu->rxSeqNo)
  {
    ICS_EPRINTF(ICS_DBG_MSG, "cpu %p[%d] state %d msg %p seqNo 0x%x != rxSeqNo 0x%x\n", 
		cpu, 
		((ICS_OFFSET)cpu - (ICS_OFFSET)&ics_state->cpu[0])/sizeof(ics_cpu_t),
		cpu->state,
		msg, msg->seqNo, cpu->rxSeqNo);

    /* XXXX Most likely caused by a CPU reboot - watchdog should catch this */
    ICS_ASSERT(msg->seqNo == cpu->rxSeqNo);

    goto consumeMsg;
  }

  /* Lookup local port desc */
  lp = ics_state->port[idx];

  /* Check that this Port exists and is open and is the correct incarnation */
  if (lp == NULL || lp->state != _ICS_PORT_OPEN || lp->version != ver)
  {
#ifdef ICS_DEBUG
    if (lp != NULL)
      ICS_EPRINTF(ICS_DBG_MSG, "msg %p port 0x%x ver 0x%x dropped : lp %p state %d ver 0x%x\n",
		  msg, msg->port, ver, lp, lp->state, lp->version);
#endif

    err = ICS_PORT_CLOSED;
    goto consumeMsg;		/* Error but still consume msg slot */
  }

  /* First check whether this Port has an associated callback
   * If it does then we call the callback function with an rdesc
   * but allow the handler to return an error if it can't handle
   * the supplied message. In which case, attempt to handle it
   * as normal by matching it against a posted receive or enqueueing
   * it on the Port MQ
   */

  /*
   * Maintaining msg order with a hybrid of callbacks and MQs is tricky.
   * Once a callback returns ICS_FULL the msg will be placed on the MQ, but to
   * preserve order, the MQ must be emptied before callbacks can be allowed again.
   * Even then there is a race where the MQ servicing thread wakes up to process
   * the last data on the queue, only to be immediately interrupted by the callback
   * with a new msg
   *
   * The solution here is to not call the callback again until the callbackBlock
   * flag is cleared having been set below on ICS_FULL being returned.
   * It is not cleared again until a new receive is posted to the postedRecvs list,
   * which will only happen once the MQ has been emptied and the last msg processed
   */
  if (lp->callback && !lp->callbackBlock)
  {
    ICS_MSG_DESC rdesc;

    /* Copy the msg info+data into an rdesc */
    rdescUpdate(&rdesc, msg);

    /* Call user defined Port callback */
    err = (lp->callback) (lp->handle, lp->callbackParam, &rdesc);
    if (err == ICS_SUCCESS)
      goto consumeMsg;

    /* Prevent further callbacks to preserve msg order */
    lp->callbackBlock = ICS_TRUE;
    
    /* XXXX ASSERT on errors other than ICS_FULL ? */
  }

  /* Now check to see if there is a posted recv */
  if (!list_empty(&lp->postedRecvs))
  {
    ics_event_t  *event;
    ICS_MSG_DESC *rdesc;
    
    /* Remove posted receive event */
    event = list_first_entry(&lp->postedRecvs, ics_event_t, list);
    list_del_init(&event->list);

    rdesc = (ICS_MSG_DESC *) event->data;

    ICS_PRINTF(ICS_DBG_MSG, "lp %p update rdesc %p with msg %p size %d\n",
	       lp, rdesc, msg, msg->size);

    /* Copy the incoming message to the user's receive desc */
    rdescUpdate(rdesc, msg);

    ICS_PRINTF(ICS_DBG_MSG, "poke event %p[%p]\n",
	       event, &event->event);

    /* Now signal completion to the posted receive event */
    _ICS_OS_EVENT_POST(&event->event);

#ifdef MULTICOM_STATS
    /* STATISTICS */
    lp->matched++;
#endif /* MULTICOM_STATS */

    err = ICS_SUCCESS;
  }
  else if (lp->mq)
  {
    /* No posted recvs, copy incoming msg onto the Port MQ */

#ifdef MULTICOM_STATS
    /* STATISTICS */
    lp->unmatched++;
#endif /* MULTICOM_STATS */

    msgSize = offsetof(ics_msg_t, payload);
    
    /* If this is an inline message then we need to copy the payload too */
    if (msg->mflags & ICS_INLINE)
      msgSize += msg->size;
    
    ICS_PRINTF(ICS_DBG_MSG, "lp %p insert msg %p size %d into mq %p\n",
	       lp, msg, msgSize, lp->mq);
    
    /* Copy and insert message into the per Port message queue */
    err = ics_mq_insert(lp->mq, msg, msgSize);
    
    if (err == ICS_FULL)
    {
#ifdef ICS_DEBUG_VERBOSE
      ICS_EPRINTF(ICS_DBG_MSG, "lp %p insert msg %p size %d into mq %p - FULL\n",
		  lp, msg, msgSize, lp->mq);
#endif

      /* Don't consume the channel FIFO slot if we cannot copy the message */
      goto error_unlock;
    }
  }
  else
  {
#ifdef MULTICOM_STATS
    /* STATISTICS */
    lp->unmatched++;
#endif /* MULTICOM_STATS */

    /* There is no message queue */
    err = ICS_FULL;
    
    /* Don't consume the channel FIFO slot if we cannot copy the message */
    goto error_unlock;
  }

consumeMsg:

  /* Expect next msg SeqNo (NB: Not incremented if MQ FIFO is FULL above) */
  cpu->rxSeqNo++;

  _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, flags);

  /* Now we need to release the channel buffer */
  (void) ICS_channel_release(channel, buf);

  return err;

error_unlock:
  /*
   * If we are going to return ICS_FULL to the channel handler, then
   * we remember the channel handle so that we can subsequently
   * unblock it when space is available.
   */
  if (err == ICS_FULL)
  {
    ICS_ASSERT(lp->blockedChans < _ICS_MAX_CPUS);
    lp->blockedChan[lp->blockedChans++] = channel;
  }

  _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, flags);
  
#ifdef ICS_DEBUG
  /* Don't report FIFO full as errors */
  if (err != ICS_FULL)
#endif
    ICS_EPRINTF(ICS_DBG_MSG, "Failed : %s (%d)\n",
		ics_err_str(err), err);

  return err;
}

/*
 * Attempt to remove a new message off the incoming message queue
 * If a message is found then the supplied receive desc is
 * updated with the message info
 *
 * Returns ICS_QUEUE_EMPTY if no messages are available 
 *
 * MULTITHREAD SAFE: Called holding the local port SPINLOCK and ics state lock
 */
static
ICS_ERROR receiveMsg (ics_port_t *lp, ICS_MSG_DESC *rdesc)
{
  ics_msg_t *msg;
 
  ICS_PRINTF(ICS_DBG_MSG, "lp %p rdesc %p\n",
	     lp, rdesc);

  /* No MQ means it must be empty! */
  if (lp->mq == NULL)
    return ICS_EMPTY;
  
  /* Grab a new message */
  msg = (ics_msg_t *) ics_mq_recv(lp->mq);
  if (msg == NULL)
  {
    /* msg queue was empty */
    return ICS_EMPTY;
  }
  
  ICS_PRINTF(ICS_DBG_MSG, "mq %p msg %p srcCpu %d\n", 
	     lp->mq, msg, msg->srcCpu);
  
  /* Copy the incoming message to the user's receive desc */
  rdescUpdate(rdesc, msg);

  /* Release the MQ FIFO slot */
  ics_mq_release(lp->mq);
  
  /* XXXX Should we unblock channels now, or say at 50% full ? */

  return ICS_SUCCESS;
}

/*
 * Post an asynchronous Receive Desc to the local port
 *
 * MULTITHREAD SAFE: Called holding the ics state lock
 * Also protects itself against the ISR by taking the
 * ics state spinLock 
 */
static
ICS_ERROR receivePost (ics_port_t *lp, ICS_MSG_DESC *rdesc, ics_event_t **eventp)
{
  ICS_ERROR    err;
  ics_event_t *event;

  unsigned long flags;
  
  /* 
   * Post an rx desc to the local port
   */

  ICS_PRINTF(ICS_DBG_MSG, "lp %p rdesc %p\n",
	     lp, rdesc);

  /* Allocate an event desc for blocking on
   *
   * NB: We do this whilst *not* holding the spinlock
   * as it may need to allocate memory
   */
  err = ics_event_alloc(&event);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_MSG, "Failed to allocate event desc : %s (%d)\n",
		ics_err_str(err), err);

    return err;
  }

  /* Protect against the ISR */
  _ICS_OS_SPINLOCK_ENTER(&ics_state->spinLock, flags);

  /* Stash the user's rdesc in the event so we can fill it in */
  event->data = rdesc;
  
  
  /* Before we post, have another quick peek at the message queue
   * and if it fails we enqueue the posted rx whilst still holding
   * the lock so as to be atomic wrt the ISR
   */
  if ((err = receiveMsg(lp, rdesc)) == ICS_SUCCESS)
  {
    /* There was a message available */
    _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, flags);

    ICS_PRINTF(ICS_DBG_MSG, "completing event %p\n",
	       event);

    /* Simulate normal completion */
    _ICS_OS_EVENT_POST(&event->event);
  }
  else
  {
    /* Now add this event to the per port list of posted receives
     * This is done atomically against the ISR
     */
    list_add_tail(&event->list, &lp->postedRecvs);

    /* Unblock any blocked channels */
    if (lp->blockedChans)
    {
      int i;

      for (i = 0; i < lp->blockedChans; i++)
      {
	err = ICS_channel_unblock(lp->blockedChan[i]);
	ICS_ASSERT(err == ICS_SUCCESS);
      }
      
      lp->blockedChans = 0;
    }

    /* Allow callbacks again */
    lp->callbackBlock = ICS_FALSE;

    _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, flags);
  }

  ICS_PRINTF(ICS_DBG_MSG, "return event %p\n",
	     event);

  /* Return event pointer to caller */
  *eventp = event;

  return ICS_SUCCESS;
}

static
ICS_ERROR msgWait (ics_event_t *event, ICS_LONG timeout)
{
  ICS_ERROR err = ICS_SUCCESS;

  unsigned long flags;

  ICS_ASSERT(event);

  ICS_PRINTF(ICS_DBG_MSG, "blocking on event %p[%p] state %d timeout 0x%x\n",
	     event, &event->event, event->state, timeout);
  
  /* Now block waiting for a message to come in */
  err = ics_event_wait(event, timeout);
  
  ICS_PRINTF(ICS_DBG_MSG, "wakeup on event %p rdesc %p : %d\n",
	     event, event->data, err);
  
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);
  
    /* Protect against the ISR */
    _ICS_OS_SPINLOCK_ENTER(&ics_state->spinLock, flags);
  /* If the event wait failed (e.g. TIMEOUT) we need to remove
   * the posted event from the port list and clear its event
   *
   * This needs to be protected from the interrupt handler by
   * using the interrupt blocking SPINLOCK
   */
  if (err != ICS_SUCCESS)
  {
    
    ICS_PRINTF(ICS_DBG_MSG, "event %p [count %d]\n",
	       event, _ICS_OS_EVENT_COUNT(&event->event));
    
    /* There is a race here, between timing out on the semaphore
     * and taking the SPINLOCK, the interrupt may have fired and
     * signalled the event. We must acquiesce the event before
     * freeing it; the EVENT_WAIT call will do this - it will
     * not block
     */
    _ICS_OS_EVENT_WAIT(&event->event, ICS_TIMEOUT_IMMEDIATE, ICS_FALSE);
    
    /* Remove from posted Event list */
    list_del_init(&event->list);
    
  }
  
  /* An ABORTED event signals Port closure */
  if (event->state == _ICS_EVENT_ABORTED)
  {
    err = ICS_PORT_CLOSED;
  }
  
  /* Now we can free off the event desc */
  ics_event_free(event);

    _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, flags);
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return err;
}

/*
 * Perform a blocking receive into rdesc
 *
 * Will wait timeout ms before returning ICS_SYSTEM_TIMEOUT
 */
ICS_ERROR ICS_msg_recv (ICS_PORT port, ICS_MSG_DESC *rdesc, ICS_LONG timeout)
{
  ICS_ERROR   err = ICS_SUCCESS;

  ics_port_t *lp;

  unsigned int type, cpuNum, ver, idx;
  
  unsigned long flags;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  ICS_PRINTF(ICS_DBG_MSG, "port 0x%x rdesc %p timeout 0x%lx\n",
	     port, rdesc, timeout);

  if (rdesc == NULL)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  /* Decode the Port handle */
  _ICS_DECODE_HDL(port, type, cpuNum, ver, idx);

  /* Check the receive port */
  if (type != _ICS_TYPE_PORT || cpuNum >= _ICS_MAX_CPUS || idx >= ics_state->portEntries)
  {
    err = ICS_HANDLE_INVALID;
    goto error;
  }

  /* Protects against other threads (e.g. ICS_port_close) but not the ISR */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Get a handle onto the local port desc */
  lp = ics_state->port[idx];

  /* Check that this Port exists and is open and is the correct incarnation */
  if (lp == NULL || lp->state != _ICS_PORT_OPEN || lp->version != ver)
  {
    err = ICS_PORT_CLOSED;
    goto error_release;
  }

  /* Protect against the ISR */
  _ICS_OS_SPINLOCK_ENTER(&ics_state->spinLock, flags);

  /* First try a non blocking receive */
  err = receiveMsg(lp, rdesc);

  /* Drop the ISR lock in case we need to block below */
  _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, flags);

  /* Didn't find a message so now we need to post an Rx 
   * event desc and then block on it
   */
  if (err == ICS_EMPTY)
  {
    ics_event_t *event = NULL;

    /* Allocate and post an asynchronous receive event */
    err = receivePost(lp, rdesc, &event);
    if (err != ICS_SUCCESS)
    {
      goto error_release;
    }

    /* Drop the ics_state lock whilst we wait/block */
    _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

    err = msgWait(event, timeout);
  }
  else
  {
    _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
  }
  
  return err;

error_release:
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
  
error:

  /* Don't report PORT_CLOSED errors */
  if (err != ICS_PORT_CLOSED)
    ICS_EPRINTF(ICS_DBG_MSG, "Failed Port 0x%x : %s (%d)\n",
		port,
		ics_err_str(err), err);
  
  return err;
}

/*
 * Post an asynchronous receive desc
 *
 * Allocated receive event handle is returned in handle
 */
ICS_ERROR ICS_msg_post (ICS_PORT port, ICS_MSG_DESC *rdesc, ICS_MSG_EVENT *eventp)
{
  ICS_ERROR    err;

  ics_port_t  *lp;
  ics_event_t *event;

  unsigned int type, cpuNum, ver, idx;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  ICS_PRINTF(ICS_DBG_MSG, "port 0x%x rdesc %p\n",
	     port, rdesc);

  if (rdesc == NULL || eventp == NULL)
    return ICS_INVALID_ARGUMENT;

  /* Decode the Port handle */
  _ICS_DECODE_HDL(port, type, cpuNum, ver, idx);

  /* Check the receive port */
  if (type != _ICS_TYPE_PORT || cpuNum >= _ICS_MAX_CPUS || idx >= ics_state->portEntries)
  {
    err = ICS_HANDLE_INVALID;
    goto error;
  }

  /* Protects against other threads (e.g. ICS_port_close) but not the ISR */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Get a handle onto the local port desc */
  lp = ics_state->port[idx];

  /* Check that this Port exists and is open and is the correct incarnation */
  if (lp == NULL || lp->state != _ICS_PORT_OPEN || lp->version != ver)
  {
    err = ICS_PORT_CLOSED;
    goto error_release;
  }

  /* Post a receive event */
  err = receivePost(lp, rdesc, &event);
  if (err != ICS_SUCCESS)
    goto error_release;

  /* Return event desc pointer to caller as an opaque handle */
  *eventp = (ICS_HANDLE) event;

  ICS_PRINTF(ICS_DBG_MSG, "port 0x%x posted rdesc %p event=0x%x\n",
	     port, rdesc, *eventp);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return ICS_SUCCESS;

error_release:
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

error:

  /* Don't report PORT_CLOSED errors */
  if (err != ICS_PORT_CLOSED)
    ICS_EPRINTF(ICS_DBG_MSG, "Failed Port 0x%x : %s (%d)\n",
		port, 
		ics_err_str(err), err);

  return err;
}

/*
 * Block waiting for an asynchronous receive to complete
 */
ICS_ERROR ICS_msg_wait (ICS_MSG_EVENT handle, ICS_LONG timeout)
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

  /* Block waiting for message or timeout */
  err = msgWait(event, timeout);

  return err;
}

/*
 * Test whether an asynchronous receive is ready
 */
ICS_ERROR ICS_msg_test (ICS_MSG_EVENT handle, ICS_BOOL *readyp)
{
  ICS_ERROR    err;
  ics_event_t *event;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;
  
  if (readyp == NULL)
    return ICS_INVALID_ARGUMENT;

  event = (ics_event_t *) handle;

  /* Try and detect bogus handles */
  if (handle == ICS_INVALID_HANDLE_VALUE || event == NULL || event->state > _ICS_EVENT_ABORTED)
    return ICS_HANDLE_INVALID;

  ICS_PRINTF(ICS_DBG_MSG, "testing event %p[%p] state %d\n",
	     event, &event->event, event->state);

  if (event->state == _ICS_EVENT_FREE)
  {
    ICS_EPRINTF(ICS_DBG_MSG, "event %p[%p] state %d\n",
		event, &event->event, event->state);
    
    return ICS_HANDLE_INVALID;
  }

  err = ics_event_test(event, readyp);
  
  return err;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
