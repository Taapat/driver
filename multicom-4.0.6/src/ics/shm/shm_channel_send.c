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
 * Sender side of the SHM channel implementation
 */


/*
 * Send a message via the specified sender channel
 *
 * A new slot in the channel FIFO is allocated and then the buffer
 * is copied into it. Then the remote cpu mbox is signalled
 *
 * MULTITHREAD SAFE: Uses the ics_shm_state SPINLOCK
 */
ICS_ERROR ics_shm_channel_send (ics_shm_channel_send_t *schannel, void *buf, size_t size)
{
  ICS_ERROR      err;

  ics_shm_cpu_t *cpu;
  ics_chn_t     *chn;

  ICS_UINT       cpuNum, chan;
  
  ICS_OFFSET     off;
  void          *msg;

  unsigned long  iflags;

  ICS_ASSERT(ics_shm_state);
  ICS_ASSERT(ics_shm_state->shm);

  ICS_ASSERT(schannel);
  
  ICS_PRINTF(ICS_DBG_CHN, "schannel %p buf %p size %d\n", schannel, buf, size);
  
  /* Sequentialise access to the cpu table and also the chn descriptor
   *
   * Use a spinlock here to protect when we are being called
   * from an channel callback handler in ISR context
   */
  if (!_ICS_OS_TASK_INTERRUPT())
    _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);

  /* Check this send channel is valid */
  if (schannel->fifo == NULL || !ICS_PAGE_ALIGNED(schannel->fifoSize))
  {
    err = ICS_INVALID_ARGUMENT;
    goto error_release;
  }

  /* Extract the target cpu and channel index */
  cpuNum = schannel->cpuNum;
  chan   = schannel->idx;

  ICS_PRINTF(ICS_DBG_CHN, "cpuNum %d chan %d\n", cpuNum, chan);

  ICS_ASSERT(chan < _ICS_MAX_CHANNELS);
  ICS_ASSERT(cpuNum < _ICS_MAX_CPUS);
  ICS_ASSERT(schannel->fifo);

  /* Get a handle to the target cpu desc */
  cpu = &ics_shm_state->cpu[cpuNum];
  if (cpu->shm == NULL)
  {
    ICS_EPRINTF(ICS_DBG_CHN, "cpuNum %d not mapped in\n", cpuNum);

    /* Target cpu is not connected */
    err = ICS_NOT_CONNECTED;
    goto error_release;
  }
    
  /* Get a handle onto the target SHM cpu channel */
  chn = &cpu->shm->chn[chan];

  /* Check that the send size is valid */
  if (size > chn->ss)
  {
    ICS_EPRINTF(ICS_DBG_CHN, "cpu %p chn %p size %d > ss %d\n",
		cpu, chn, size, chn->ss);
    
    err = ICS_INVALID_ARGUMENT;
    goto error_release;
  }

  /* Claim a chn FIFO slot */
  off = ics_chn_claim(chn, _ICS_SHM_CTRL_CACHED);
  
  if (off == _ICS_CHN_FIFO_FULL)
  {
    ICS_PRINTF(ICS_DBG_CHN, "cpuNum %d chan %d schannel chn %p FULL\n", cpuNum, chan, chn);

    ICS_EPRINTF(ICS_DBG_CHN, "chn %p[%d] fptr %d bptr %d\n", chn, chan, chn->fifo.fptr, chn->fifo.bptr);

    /* FIFO is full */
    err = ICS_FULL;
    goto error_release;
  }

  /* Convert chn offset to a local FIFO pointer */
  msg = (void *) ((ICS_OFFSET) schannel->fifo + off);

  ICS_PRINTF(ICS_DBG_CHN, "cpu %d chan %d buf %p size %d off %x msg %p\n",
	     cpuNum, chan, buf, size, off, msg);

  if (size)
  {
    /* Copy message into the FIFO */
    if ((size == sizeof(unsigned int)) && ALIGNED(msg, sizeof(unsigned int)))
      /* FASTPATH: 4-byte write */
      *((unsigned int *) msg) = *((unsigned int *) buf);
    else if ((size == sizeof(unsigned long long)) && ALIGNED(msg, sizeof(unsigned long long)))
      /* FASTPATH: 8-byte write */
      *((unsigned long long *) msg) = *((unsigned long long *) buf);
    else
      _ICS_OS_MEMCPY(msg, buf, size);

    /* Purge FIFO memory to make it visible to remote */
    if (_ICS_SHM_FIFO_CACHED)
      _ICS_OS_CACHE_PURGE(msg, chn->base + off, size);
  }

#ifdef MULTICOM_STATS
  chn->fifo.sends++;
#endif /* MULTICOM_STATS */

  ICS_ASSERT(cpu->rmbox[_ICS_MAILBOX_CHN2MBOX(chan)]);

  /* Move on chn FIFO fptr and signal remote CPU */
  ics_chn_send(chn, cpu->rmbox[_ICS_MAILBOX_CHN2MBOX(chan)], _ICS_MAILBOX_CHN2IDX(chan), _ICS_SHM_CTRL_CACHED);

  if (!_ICS_OS_TASK_INTERRUPT())
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);

  return ICS_SUCCESS;

error_release:
  if (!_ICS_OS_TASK_INTERRUPT())
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);

  ICS_EPRINTF(ICS_DBG_CHN, " Failed: %s(%d)\n",
	      ics_err_str(err), err);

  return err;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

