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


/*
 * ICS CPU connection implementation
 *
 */

/* This is an internal API called during ICS init and CPU connection
 * It doesn't do the region table update.
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
ICS_ERROR _ics_cpu_connect (ICS_UINT cpuNum, ICS_UINT flags, ICS_LONG timeout)
{
  ICS_ERROR  err;

  ics_cpu_t *cpu;
  
  ICS_ASSERT(ics_state);
  
  cpu = &ics_state->cpu[cpuNum];
  if (cpu->state == _ICS_CPU_CONNECTED || cpu->state == _ICS_CPU_MAPPED)
  {
    /* Already connected */
    return ICS_SUCCESS;
  }

  /* Get out of here if we are currently trying to disconnect */
  if (cpu->state == _ICS_CPU_DISCONNECTING)
    return ICS_NOT_CONNECTED;

  /* Call transport specific connect function */
  err = ics_transport_connect(cpuNum, timeout);
  if (err != ICS_SUCCESS)
  {
    goto error;
  }
  
  /* Generate a channel handle for communicating with target cpu
   * We use our own CPU number as the channel index assuming that the
   * target CPU has allocated a unique channel for each CPU
   *
   * XXXX Should we make this dynamic and actually request a channel handle
   * via a control channel or look it up in the SHM segment ?
   */
  cpu->remoteChannel = _ICS_HANDLE(_ICS_TYPE_CHANNEL, cpuNum, 1 /* version */, ics_state->cpuNum);

  /* Safety check to catch multiple connects */
  ICS_assert(cpu->sendChannel == ICS_INVALID_HANDLE_VALUE);

  /* Call the internal connection function as ICS_channel_open() may attempt to make
   * a connection to the target CPU
   */
  if ((err = _ics_channel_open(cpu->remoteChannel, 0 /* flags */, &cpu->sendChannel)) != ICS_SUCCESS)
  {
    goto error_disconnect;
  } 
  
  ICS_assert(cpu->sendChannel != ICS_INVALID_HANDLE_VALUE);

  /* Mark the cpu as being mapped */
  cpu->state = _ICS_CPU_MAPPED;

  return ICS_SUCCESS;

error_disconnect:
  ics_transport_disconnect(cpuNum, 0);

error:
  ICS_EPRINTF(ICS_DBG_INIT, "Failed : %s (%d)\n", 
	      ics_err_str(err), err);

  return err;
}

ICS_ERROR ICS_cpu_connect (ICS_UINT cpuNum, ICS_UINT flags, ICS_LONG timeout)
{
  ICS_ERROR  err;
  ICS_UINT   validFlags = 0;
  
  ics_cpu_t *cpu;

  ICS_ASSERT(ics_state);
  
  ICS_PRINTF(ICS_DBG_INIT, "cpuNum %d flags 0x%x timeout %d\n",
	     cpuNum, flags, timeout);

  if (cpuNum >= _ICS_MAX_CPUS || flags & ~validFlags)
  {
    return ICS_INVALID_ARGUMENT;
  }
  
  /* Protect the cpu table */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  cpu = &ics_state->cpu[cpuNum];

  /* This will map in the remote CPU and get its state to _ICS_CPU_MAPPED */
  err = _ics_cpu_connect(cpuNum, flags, timeout);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  if (err != ICS_SUCCESS)
    return err;
  
  /* SUCCESS: Update this new CPU with all our memory region info */

  /* XXXX Locking here is a pain. We cannot hold the state lock
   * across the ics_region_cpu_up() call
   */
  if (cpu->state == _ICS_CPU_MAPPED && cpuNum != ics_state->cpuNum)
  {
    err = ics_region_cpu_up(cpuNum);
    if (err != ICS_SUCCESS) 
    {
      /* (void) ics_cpu_disconnect(cpuNum, 0); */

      ICS_EPRINTF(ICS_DBG_INIT, "cpu %d failed to update region tables : %s (%d)\n",
		  cpuNum,
		  ics_err_str(err), err);
      return err;
    }
  }

  /* Protect the cpu table */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Check it hasn't been disconnected behind our backs */
  if (cpu->state == _ICS_CPU_MAPPED || cpu->state == _ICS_CPU_CONNECTED)
  {
    /* Mark the cpu as now being fully connected */
    cpu->state = _ICS_CPU_CONNECTED;

    ICS_PRINTF(ICS_DBG_INIT, "Successfully connected to cpu %d\n",
	       cpuNum);
  }
  else
    ICS_EPRINTF(ICS_DBG_INIT, "cpu %d state changed %d during connect\n",
		cpuNum, cpu->state);
  
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return (cpu->state == _ICS_CPU_CONNECTED) ? ICS_SUCCESS : ICS_NOT_CONNECTED;
}

/* This is an internal API called during ICS init and CPU disconnection
 * It doesn't do the region table update or the nameserver tidy up
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
ICS_ERROR _ics_cpu_disconnect (ICS_UINT cpuNum, ICS_UINT flags)
{
  ICS_ERROR  err;
  ics_cpu_t *cpu;

  unsigned long iflags;

  ICS_ASSERT(ics_state);

  cpu = &ics_state->cpu[cpuNum];

  /* Close the send channel */
  err = ICS_channel_close(cpu->sendChannel);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "Failed to close cpuNum %d channel 0x%x : %s (%d)\n",
		cpuNum,
		cpu->sendChannel,
		ics_err_str(err), err);
  }
  cpu->sendChannel = ICS_INVALID_HANDLE_VALUE;

  /* Disconnect the transport */
  err = ics_transport_disconnect(cpuNum, flags);

  /* Protect against the Port msg handler ISR */
  _ICS_OS_SPINLOCK_ENTER(&ics_state->spinLock, iflags);

  /* Clear down remote cpu state */
  cpu->remoteChannel = ICS_INVALID_HANDLE_VALUE;
  cpu->txSeqNo       = 0;
  cpu->rxSeqNo       = 0;
  cpu->watchdogTs    = 0;
  cpu->watchdogFails = 0;

  cpu->state         = _ICS_CPU_DISCONNECTED;

  _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, iflags);

  return err;
}

ICS_ERROR ICS_cpu_disconnect (ICS_UINT cpuNum, ICS_UINT flags)
{
  ICS_ERROR  err;
  ICS_UINT   validFlags = ICS_CPU_DEAD;
  ics_cpu_t *cpu;

  unsigned long iflags;

  ICS_ASSERT(ics_state);

  ICS_PRINTF(ICS_DBG_INIT, "disconnecting cpu %d flags 0x%x\n",
	     cpuNum, flags);

  if (cpuNum >= _ICS_MAX_CPUS || flags & ~validFlags)
    return ICS_INVALID_ARGUMENT;

  /* Protect the cpu table */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  cpu = &ics_state->cpu[cpuNum];
  if (cpu->state != _ICS_CPU_CONNECTED && cpu->state != _ICS_CPU_MAPPED)
  {
    _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

    /* Not connected */
    return ICS_NOT_CONNECTED;
  }
  
  /* Protect against the Port msg handler ISR */
  _ICS_OS_SPINLOCK_ENTER(&ics_state->spinLock, iflags);

  /* Flag state to indicate we are disconnecting
   * then drop the state lock so we can tidy up
   */
  cpu->state = _ICS_CPU_DISCONNECTING;

  _ICS_OS_SPINLOCK_EXIT(&ics_state->spinLock, iflags);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  /* Update this CPU to remove all the memory region info */
  ics_region_cpu_down(cpuNum, flags);

  /* Inform nameserver if its dead */
  if ((flags & ICS_CPU_DEAD))
  {
    (void) ics_nsrv_cpu_down(cpuNum, flags);
  }
  
  /* Protect the cpu table */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  err = _ics_cpu_disconnect(cpuNum, flags);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  ICS_PRINTF(ICS_DBG_INIT, "Completed : %s (%d)\n", 
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
