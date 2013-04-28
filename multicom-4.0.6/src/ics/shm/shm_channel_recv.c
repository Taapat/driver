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


#include "_ics_shm.h"	/* SHM transport specific headers */

/*
 * Receiver/Local side of the SHM channel implementation
 */

/*
 * Main interrupt entry point for the hw mailbox
 *
 * param is the Mailbox #
 * status is the interrupt status bits of that Mailbox
 *
 * We use one bit for each channel, hence 32 channels
 * per mbox are supported
 */
void ics_shm_channel_handler (void *param, ICS_UINT status)
{
  ICS_UINT idx, mbox;

  ICS_ASSERT(ics_shm_state);
  ICS_ASSERT(ics_shm_state->shm);

  ICS_PRINTF(ICS_DBG_CHN, "param %p status 0x%x\n", param, status);

  /* The parameter is simply the mbox number */
  mbox = (unsigned int) param;

  ICS_ASSERT(mbox < _ICS_MAX_MAILBOXES);

  /* Clear the interrupt for all channels */
  ics_mailbox_status_mask(ics_shm_state->mbox[mbox], 0 /*set*/, status /*clear*/);

  /* Service the channels round-robin for fairness */
  idx = ics_shm_state->lastIdx[mbox];

  /* The status bitmask indicates which channels have new
   * entries to process. Loop processing every set bit
   */
  for ( ; status; idx++)
  {
    ICS_ERROR err;

    /* Loop back to bit 0 if no upper bits set */
    if (idx >= 32 || (1U << idx) > status)
    {
      idx = 0;
    }
    
    /* Does this channel need servicing ? */
    if (!(status & (1U << idx)))
      continue;
    
    /* Clear status bit */
    status &= ~(1U << idx);

    /* Process all new entries on this channel */
    err = ics_shm_service_channel(_ICS_MAILBOX_MBOX2CHN(mbox, idx));
    
    /* We allow the service routine to indicate it couldn't
     * fully process this channel and hence has left entries
     * on it. Disable the interrupt until it is explicately
     * unblocked 
     */
    if (err == ICS_FULL)
    {
      ICS_PRINTF(ICS_DBG_CHN, "disabling IRQ mbox %p idx %d\n",
		 ics_shm_state->mbox[mbox], idx);
      
      ics_mailbox_interrupt_disable(ics_shm_state->mbox[mbox], idx);
    }
  }

  /* Start at this idx on next IRQ */
  ics_shm_state->lastIdx[mbox] = idx;

  /* Update the SHM watchdog timestamp */
  ics_shm_watchdog(_ICS_OS_TIME_TICKS2MSEC(_ICS_OS_TIME_NOW()));
}


/*
 * Main ISR channel service routine
 * Called with the channel index of the FIFO with new data
 * which is extracted from the mailbox status register
 * Hence initially only 32 channels per mbox are supported
 *
 * Iterates over all the new entries in the FIFO calling
 * either the user callback function or setting an OS event
 */
ICS_ERROR ics_shm_service_channel (ICS_UINT idx)
{
  ics_shm_channel_t *channel;
  ics_chn_t         *chn;

  ICS_UINT           off;
  unsigned long  iflags;

  ICS_ASSERT(idx < _ICS_MAX_CHANNELS);
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
  channel = &ics_shm_state->channel[idx];
  chn     = &ics_shm_state->shm->chn[idx];

  ICS_PRINTF(ICS_DBG_CHN, "idx %d channel %p chn %p\n", idx, channel, chn);

#ifdef MULTICOM_STATS
  /* STATISTICS */
  chn->fifo.interrupts++;
#endif /* MULTICOM_STATS */

  /* NB: We empty the FIFO on each invocation so there may be
   * subsequent IRQS where the FIFO is found to be empty
   */

  /* Process all new entries in the SHM channel
   * 
   * We supply a local mirror of the bptr so we can iterate over all
   * the new entries without actually consuming them. The handler
   * or the user will need to do that by calling ICS_channel_release()
   * which will move on the real SHM chn bptr
   */
  while ((off = ics_chn_receive(chn, &channel->bptr, _ICS_SHM_CTRL_CACHED)) != _ICS_CHN_FIFO_EMPTY)
  {
    ICS_ERROR err;
    
    /* Convert the FIFO offset to a local address */
    void *buf = (void *) ((ICS_OFFSET) channel->fifo + off);

    /* About to read this, must flush from the cache */
    if (_ICS_SHM_FIFO_CACHED)
      _ICS_OS_CACHE_PURGE(buf, chn->base + off, chn->ss);

    if (channel->callback)
    {
      ICS_PRINTF(ICS_DBG_CHN, "call handler %p handle 0x%x param %p buf %p\n",
		 channel->callback, channel->handle, channel->param, buf); 

      /* Call the user supplied callback in ISR context */
      err = (*channel->callback)(channel->handle, channel->param, buf);
      if (err == ICS_FULL)
      {
	ICS_PRINTF(ICS_DBG_CHN, "channel %p callback returned blocked\n",
		   channel);
		   
	/* The handler failed to move the entry off the FIFO
	 * so we have to jump out here and leave it in place.
	 *
	 * To recover from this the handler code will need
	 * to unblock the channel again once it has space.
	 */
	
	/* Didn't consume the message; so must reset the soft bptr */
	channel->bptr = chn->fifo.bptr;

#ifdef MULTICOM_STATS
	/* STATISTICS */
	channel->full++;
#endif /* MULTICOM_STATS */

  /* release spinlok */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);

	/* Also inform mbox handler code of this blockage */
	return err;
      }
    }
    else
    {
      ICS_PRINTF(ICS_DBG_CHN, "Post event %p\n",
		 &channel->event);

      /* Post an event for each new FIFO entry */
      _ICS_OS_EVENT_POST(&channel->event);
    }
  }

  /* release spinlok */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
  return ICS_SUCCESS;
}

/*
 * Called to unblock a previously blocked channel (ICS_FULL)
 *
 * Can be called from ISR context
 */
ICS_ERROR ics_shm_channel_unblock (ICS_HANDLE handle)
{
  ICS_ERROR          err;

  ics_shm_channel_t *channel;

  ICS_UINT           mbox, idx;

  int type, cpuNum, ver, chan;

  ICS_ASSERT(ics_shm_state);
  ICS_ASSERT(ics_shm_state->shm);

  /* Decode the channel handle */
  _ICS_DECODE_HDL(handle, type, cpuNum, ver, chan);
  
  /* Check the channel handle */
  if (type != _ICS_TYPE_CHANNEL || cpuNum >= _ICS_MAX_CPUS || chan >= _ICS_MAX_CHANNELS)
  {
    err = ICS_HANDLE_INVALID;
    goto error;
  }
  
  /* Must be a local channel */
  if (cpuNum != ics_state->cpuNum)
  {
    err = ICS_HANDLE_INVALID;
    goto error;
  }

  /* Grab handle to the local channel desc */
  channel = &ics_shm_state->channel[chan];

  ICS_assert(channel->fifo != NULL);

  /* Convert chan into mbox & idx */
  mbox = _ICS_MAILBOX_CHN2MBOX(chan);
  idx  = _ICS_MAILBOX_CHN2IDX(chan);
  
  ICS_PRINTF(ICS_DBG_CHN, "unblocking channel %p[%d] mbox %d idx %d\n", channel, chan, mbox, idx);

  /* Channel was blocked, leaving unprocessed messages on its FIFO 
   * Restart it by re-enabling and then raising the relevant mbox interrupt
   */
  ics_mailbox_interrupt_enable(ics_shm_state->mbox[mbox], idx);
  ics_mailbox_interrupt_raise(ics_shm_state->mbox[mbox], idx);

  return ICS_SUCCESS;

error:
  return err;

}

/*
 * Block on an channel handle waiting for new messages
 * Called for the case where a channel handler is not being used
 *
 * Returns a message pointer directly into the FIFO to the caller, the
 * FIFO slot must then be later released by calling ics_shm_channel_release
 *
 * MULTITHREAD locking: Channels should really be single threaded only
 * as having multiple threads service the FIFO would could lead to out of
 * order calls to channel_release. So we use a per channel mutex to inforce 
 * this (chanLock), which is held across the recv/release calls
 *
 * Cannot be called from ISR context
 */
ICS_ERROR ics_shm_channel_recv (ICS_HANDLE handle, ICS_ULONG timeout, void **bufp)
{
  ICS_ERROR          err;
  ics_shm_channel_t *channel;
  ics_chn_t         *chn;

  ICS_UINT           off;
  void              *buf;

  int type, cpuNum, ver, chan;
#ifdef CONFIG_SMP
  unsigned long  iflags;
#endif

  ICS_ASSERT(ics_shm_state);
  ICS_ASSERT(ics_shm_state->shm);

  /* Decode the channel handle */
  _ICS_DECODE_HDL(handle, type, cpuNum, ver, chan);
  
  /* Check the channel handle */
  if (type != _ICS_TYPE_CHANNEL || cpuNum >= _ICS_MAX_CPUS || chan >= _ICS_MAX_CHANNELS)
  {
    err = ICS_HANDLE_INVALID;
    goto error;
  }
  
  /* Must be a local channel */
  if (cpuNum != ics_state->cpuNum)
  {
    err = ICS_HANDLE_INVALID;
    goto error;
  }

#ifdef CONFIG_SMP
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Grab handles to the local and SHM channel descs */
  channel = &ics_shm_state->channel[chan];
  chn     = &ics_shm_state->shm->chn[chan];

  /* Assert channel is initialised */
  ICS_ASSERT(channel->fifo);

  ICS_PRINTF(ICS_DBG_CHN, "channel %p[%d] event %p count %d\n",
	     channel, chan, &channel->event, _ICS_OS_EVENT_COUNT(&channel->event));
#ifdef CONFIG_SMP
  /* release spinlok */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Block waiting for new entries */
  err = _ICS_OS_EVENT_WAIT(&channel->event, timeout, ICS_FALSE);
  if (err != ICS_SUCCESS)
  {
    /* Probably a time-out */
    goto error;
  }
  /* MULTITHREAD SAFE: Hold this lock until the slot is released */
  _ICS_OS_MUTEX_TAKE(&channel->chanLock);

#ifdef CONFIG_SMP
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Get the next entry offset from the FIFO base */
  off = ics_chn_receive(chn, NULL, _ICS_SHM_CTRL_CACHED);

  /* FIFO cannot be empty */
  ICS_ASSERT(off != _ICS_CHN_FIFO_EMPTY);

  /* Convert the FIFO offset to a local address */
  buf = (void *) ((ICS_OFFSET) channel->fifo + off);

  /* About to read this, must flush from the cache */
  if (_ICS_SHM_FIFO_CACHED)
    _ICS_OS_CACHE_PURGE(buf, chn->base + off, chn->ss);

  ICS_PRINTF(ICS_DBG_CHN, "channel %p[%d] wakeup buf %p\n", channel, chan, buf);

  /* Return FIFO entry address to caller
   *
   * User will need to consume FIFO slot with ics_shm_channel_release()
   *
   * MULTITHREAD SAFE: Still holding channel MUTEX
   *
   */
  *bufp = buf;
#ifdef CONFIG_SMP
  /* release spinlok */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  return ICS_SUCCESS;

error:
  return err;
}

/*
 * Called to move on the channel bptr effectively consuming the message
 *
 * Can be called from ISR context
 */
ICS_ERROR ics_shm_channel_release (ICS_HANDLE handle, ICS_VOID *buf)
{
  ICS_ERROR          err;

  ics_shm_channel_t *channel;
  ics_chn_t         *chn;

  ICS_UINT           off;

  int type, cpuNum, ver, chan;
  unsigned long  iflags;

#ifdef CONFIG_SMP
  if (!_ICS_OS_TASK_INTERRUPT())
     /* aquire spinlock */
     _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  ICS_ASSERT(ics_shm_state);
  ICS_ASSERT(ics_shm_state->shm);

  /* Decode the channel handle */
  _ICS_DECODE_HDL(handle, type, cpuNum, ver, chan);
  
  /* Check the channel handle */
  if (type != _ICS_TYPE_CHANNEL || cpuNum >= _ICS_MAX_CPUS || chan >= _ICS_MAX_CHANNELS)
  {
    err = ICS_HANDLE_INVALID;
    goto error;
  }
  
  /* Must be a local channel */
  if (cpuNum != ics_state->cpuNum)
  {
    err = ICS_HANDLE_INVALID;
    goto error;
  }

  /* Grab handles to the local and SHM channel descs */
  channel = &ics_shm_state->channel[chan];
  chn     = &ics_shm_state->shm->chn[chan];

  /* Assert channel is initialised */
  ICS_ASSERT(channel->fifo);

  /* Move on the SHM chn bptr */
  off = ics_chn_release(chn, _ICS_SHM_CTRL_CACHED);

  /* Check for an obviously incorrect release */
  if (off == _ICS_CHN_FIFO_EMPTY)
  {
#ifdef CONFIG_SMP
    if (!_ICS_OS_TASK_INTERRUPT())
      /* aquire spinlock */
      _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    return ICS_INVALID_ARGUMENT;
  }

  /* MULTITHREAD SAFE: Now we can release the channel lock 
   * But only if we're not servicing the channel from an IRQ handler
   */
  if (!_ICS_OS_TASK_INTERRUPT())
  {
#ifdef CONFIG_SMP
    /* release spinlok */
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    ICS_ASSERT(_ICS_OS_MUTEX_HELD(&channel->chanLock));
    _ICS_OS_MUTEX_RELEASE(&channel->chanLock);
  }

  return ICS_SUCCESS;

error:
  if (!_ICS_OS_TASK_INTERRUPT())
     _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
  return err;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

