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
 * File: ics_msg_send.c
 *
 * Description
 *    Routines to implement the ICS message passing API
 *
 **************************************************************/

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

/*
 * Internal msg send code
 * This is made available so internal routines can send messages whilst
 * holding the state lock.
 * If the target cpu is not mapped it will do an ics_cpu_connect() which will connect
 * to the target but not do the region table update as that can deadlock the admin tasks
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
ICS_ERROR _ics_msg_send (ICS_PORT port, ICS_VOID *buf, ICS_MEM_FLAGS mflags, ICS_UINT size, ICS_UINT flags, 
			 ICS_UINT cpuNum)
{
  ICS_ERROR     err = ICS_SUCCESS;

  size_t        msgSize = 0;
  ics_msg_t     msg;

  ics_cpu_t    *cpu;

#ifdef CONFIG_SMP
  unsigned long  iflags;
#endif /*CONFIG_SMP*/

  cpu = &ics_state->cpu[cpuNum];

  /* Check that the target cpu is at least mapped */
  if (cpu->state != _ICS_CPU_CONNECTED && cpu->state != _ICS_CPU_MAPPED)
  {
    if (flags & ICS_MSG_CONNECT)
    {
      /* Connect to target cpu, but don't do the update regions */
      err = _ics_cpu_connect(cpuNum, 0, _ICS_CONNECT_TIMEOUT);
      if (err != ICS_SUCCESS)
	return ICS_NOT_CONNECTED;
    }
    else
      return ICS_NOT_CONNECTED;
  }

#ifdef CONFIG_SMP
    _ICS_OS_SPINLOCK_ENTER(&ics_state->spinLock, iflags);
#endif /*CONFIG_SMP*/

  msg.data = ICS_BAD_OFFSET;
  
  /* If the payload is inline, copy it to msg buffer */
  if (mflags & ICS_INLINE)
  {
    if (size > ICS_MSG_INLINE_DATA)
    {

#ifdef CONFIG_SMP   
      _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, iflags);
#endif /*CONFIG_SMP*/

      /* Inline messages must fit within a single FIFO slot */
      return ICS_INVALID_ARGUMENT;
    }

    /* Marshall data onto the stack */
    _ICS_OS_MEMCPY(msg.payload, buf, size);

    msgSize = offsetof(ics_msg_t, payload[size]);	/* msgSize includes hdr + payload */
  }
  else
  {
    if (size)
    {
      ICS_MEM_FLAGS buf_mflags;

      /* Convert a region buf ptr to a physical address 
       * The paddr will then be converted back to a local pointer
       * in the target cpu. Only works if the buffer has been allocated
       * from the local heaps or registered regions
       */
      /* Holding state lock call internal API */
      err = _ics_region_virt2phys(buf, &msg.data, &buf_mflags);
      if (err != ICS_SUCCESS)
      {
	ICS_EPRINTF(ICS_DBG_MSG, "Failed to convert %s addr %p to paddr : %d\n",
		    ((buf_mflags & ICS_CACHED) ? "CACHED" : "UNCACHED"), buf, err);
#ifdef CONFIG_SMP   
      _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, iflags);
#endif /*CONFIG_SMP*/
	return err;
      }

      ICS_PRINTF(ICS_DBG_MSG, "Converted %s addr %p to paddr 0x%x\n",
		 ((buf_mflags & ICS_CACHED) ? "CACHED" : "UNCACHED"), buf, msg.data);
      
      if (buf_mflags & ICS_CACHED)
	/* Need to flush the local buffer out of our cache */
	/* XXXX Use PURGE here to be L2 safe on SH4 ?*/
	_ICS_OS_CACHE_PURGE(buf, msg.data, size);
    }

    msgSize = offsetof(ics_msg_t, payload);		/* msgSize is just the msg hdr */
  }

  ICS_ASSERT(cpu->state == _ICS_CPU_CONNECTED || cpu->state == _ICS_CPU_MAPPED);
  ICS_ASSERT(cpu->sendChannel != ICS_INVALID_HANDLE_VALUE);
  
  /* Fill out the FIFO message structure */
  msg.srcCpu   = ics_state->cpuNum;
  msg.port     = port;
  msg.mflags   = mflags;	/* Remote buffer memory flags */
  msg.size     = size;
  msg.seqNo    = cpu->txSeqNo;	/* Debugging */

#ifdef CONFIG_SMP  
  _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, iflags);
#endif /*CONFIG_SMP*/

  /* 
   * Send the message via the ICS send channel
   *
   * Hold the mutex across this call to ensure the txSeqNo arrive
   * in order at the target Port
   */
  err = ICS_channel_send(cpu->sendChannel, &msg, msgSize, 0 /* flags */);

  /* Only increment seqNo on a successful send */
  if (err == ICS_SUCCESS)
    cpu->txSeqNo++;

  if (err != ICS_SUCCESS)
    /* Failed to send to channel */
    return err;

  return ICS_SUCCESS;
}


/*
 * Send a message to the target port.
 *
 * Messages can either be sent inline (mflags & ICS_INLINE) via a copy
 * into the inter cpu FIFOs or via zero copy by simply translating the buf address.
 *
 * For inline message they must fit within a FIFO slot which has the
 * limit ICS_MSG_INLINE_DATA
 *
 * For non inline messages the mflags determine what translation is presented
 * to the receiver, i.e. a CACHED or UNCACHED virtual address
 *
 * For the zero copy path to work, the buffer must have been allocated 
 * from an ICS registered heap/region
 */
ICS_ERROR ICS_msg_send (ICS_PORT port, ICS_VOID *buf, ICS_MEM_FLAGS mflags, ICS_UINT size, ICS_UINT flags)
{
  ICS_ERROR     err = ICS_SUCCESS;

  ICS_MEM_FLAGS validMFlags = (ICS_INLINE|ICS_CACHED|ICS_UNCACHED|ICS_WRITE_BUFFER|ICS_PHYSICAL);

  ICS_UINT      validFlags  = (ICS_MSG_CONNECT);

  int           type, cpuNum, ver, idx;

  ics_cpu_t    *cpu;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  ICS_PRINTF(ICS_DBG_MSG, "port 0x%x buf %p size %d mflags 0x%x flags 0x%x\n",
	     port, buf, size, mflags, flags);

  if (mflags == 0 || (mflags & ~validMFlags) || (flags & ~validFlags) || (size && (buf == NULL)))
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  /* Decode the target port handle */
  _ICS_DECODE_HDL(port, type, cpuNum, ver, idx);

  /* Check the target port */
  if (type != _ICS_TYPE_PORT || cpuNum >= _ICS_MAX_CPUS || idx >= _ICS_MAX_PORTS)
  {
    ICS_EPRINTF(ICS_DBG_MSG, "Invalid port 0x%x -> type 0x%x cpuNum %d ver %d idx %d\n",
		port, type, cpuNum, ver, idx);

    err = ICS_HANDLE_INVALID;
    goto error;
  }

  ICS_PRINTF(ICS_DBG_MSG, "port 0x%x -> type 0x%x cpuNum %d ver %d idx %d\n",
	     port, type, cpuNum, ver, idx);

  cpu = &ics_state->cpu[cpuNum];
 
  /* We must be connected before we send a message or else
   * the region tables may not have been exchanged
   */
  /* Check that the target cpu is connected (NB: not holding state lock) */
  cpu = &ics_state->cpu[cpuNum];
  if (cpu->state != _ICS_CPU_CONNECTED)
  {
    err = ICS_NOT_CONNECTED;
    
    if (flags & ICS_MSG_CONNECT)
    {
      /* Connect to CPU if it is currently not connected 
       * This call will also do the region table exchange
       */
      err = ICS_cpu_connect(cpuNum, 0 /* flags */, _ICS_CONNECT_TIMEOUT);
    }
    
    if (err != ICS_SUCCESS)
      goto error;
  }

  /* Protect the cpu table */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);
    
  /* Call internal msg send fn (whilst holding the cpu lock) */
  err = _ics_msg_send(port, buf, mflags, size, flags, cpuNum);
  if (err != ICS_SUCCESS)
    goto error_release;
  
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return ICS_SUCCESS;

error_release:
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

error:
  ICS_EPRINTF(ICS_DBG_MSG, "Port 0x%x buf %p size %d mflags 0x%x flags 0x%x : Failed %s (%d)\n",
	      port, buf, size, mflags, flags,
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
