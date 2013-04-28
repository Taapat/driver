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

/* Generic channel implementation 
 *
 * Currently just call the SHM channel implementation directly
 * perhaps one day replace this with a per cpu lookup routing table
 * where different channel transports per cpu can be used
 */

#include "_ics_shm.h"	/* SHM transport specific headers */

/*
 * Allocate a uni-directional commununication channel on this CPU
 */
ICS_ERROR ICS_channel_alloc (ICS_CHANNEL_CALLBACK callback,
			     ICS_VOID *param,
			     ICS_VOID *base, ICS_UINT nslots, ICS_UINT ssize, ICS_UINT flags,
			     ICS_CHANNEL *handlep)
{
  ICS_ERROR err;
  ICS_UINT  validFlags = 0;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  /* Validate parameters */
  if (handlep == NULL || (flags & ~validFlags))
  {
    return ICS_INVALID_ARGUMENT;
  }

  /* Validate number of slots */
  if (nslots < 2 || !powerof2(nslots))
  {
    return ICS_INVALID_ARGUMENT;
  }

  /* Validate slot size */
  /* XXXX Do we have to enforce ICS_CACHELINE_ALIGNED ssize ??? */
  if (ssize == 0 || !ALIGNED(ssize, sizeof(int)) /* || ssize < ICS_CACHELINE_SIZE || !ICS_CACHELINE_ALIGNED(ssize) */)
  {
    return ICS_INVALID_ARGUMENT;
  }

  /* Check optional memory base is aligned */
  if (base && !ICS_PAGE_ALIGNED(base))
  {
    return ICS_INVALID_ARGUMENT;
  }
    
  /* Allocate a shm channel */
  err = ics_shm_channel_alloc(callback, param, base, nslots, ssize, handlep);
 
  if (err != ICS_SUCCESS)
  {
    return err;
  }
  

  return ICS_SUCCESS;
}

/*
 * Free off a local communication channel
 *
 * Any unprocessed messages will be lost
 */
ICS_ERROR ICS_channel_free (ICS_CHANNEL handle, ICS_UINT flags)
{
  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  return ics_shm_channel_free(handle);
}

/* Called by _ics_cpu_connect() to avoid a deadlock as ICS_channel_open()
 * now calls ICS_cpu_connect()
 */
ICS_ERROR _ics_channel_open (ICS_CHANNEL handle, ICS_UINT flags, ICS_CHANNEL_SEND *schannelp)
{
  return ics_shm_channel_open(handle, (ics_shm_channel_send_t **) schannelp);
}

/*
 * Open a send channel for communications
 *
 * Takes the channel handle of a local or remote channel and prepares
 * a send channel for communicating via that channel on this CPU
 */
ICS_ERROR ICS_channel_open (ICS_CHANNEL handle, ICS_UINT flags, ICS_CHANNEL_SEND *schannelp)
{
  ICS_UINT  validFlags = 0;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  if (schannelp == NULL || (flags & ~validFlags))
    return ICS_INVALID_ARGUMENT;

  /*
   * Handle loosely coupled connection case 
   */
  {
    ICS_ERROR err;
    int type, cpuNum, ver, idx;

    /* Decode the channel handle */
    _ICS_DECODE_HDL(handle, type, cpuNum, ver, idx);
    
    /* Check the target channel handle */
    if (type != _ICS_TYPE_CHANNEL || cpuNum >= _ICS_MAX_CPUS || idx >= _ICS_MAX_CHANNELS)
      return ICS_HANDLE_INVALID;

    /* Have we already made a connection to this CPU ? */
    if (ics_state->cpu[cpuNum].state != _ICS_CPU_CONNECTED)
    {
      /* Attempt to connect to remote CPU, updating the region tables too */
      err = ICS_cpu_connect(cpuNum, 0, _ICS_CONNECT_TIMEOUT);
      if (err != ICS_SUCCESS)
	return err;
    }
  }

  return _ics_channel_open(handle, flags, schannelp);
}

/*
 * Close a send channel 
 */
ICS_ERROR ICS_channel_close (ICS_CHANNEL_SEND schannel)
{
  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  if (schannel == ICS_INVALID_HANDLE_VALUE)
    return ICS_HANDLE_INVALID;

  return ics_shm_channel_close(schannel);
}

/*
 * Send a buffer via an open Send channel
 */
ICS_ERROR ICS_channel_send (ICS_CHANNEL_SEND schannel, ICS_VOID *buf, ICS_SIZE size, ICS_UINT flags)
{
  ICS_UINT  validFlags = 0;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  if ((size && buf == NULL) || (flags & ~validFlags))
    return ICS_INVALID_ARGUMENT;

  if (schannel == ICS_INVALID_HANDLE_VALUE)
    return ICS_HANDLE_INVALID;

  return ics_shm_channel_send(schannel, buf, size);
}

/*
 * Block on an channel waiting for new messages
 * Called for the case where a channel callback is not being used
 *
 * Returns a message pointer to the caller, this must then be 
 * later released by calling ICS_channel_release
 *
 */
ICS_ERROR ICS_channel_recv (ICS_CHANNEL handle, ICS_VOID **bufp, ICS_LONG timeout)
{
  ICS_ERROR err;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;
  
  if (bufp == NULL)
    return ICS_INVALID_ARGUMENT;

  /* Blocking call */
  err = ics_shm_channel_recv(handle, timeout, bufp); 
  
  return err;
}

/*
 * Release a channel buffer having consumed it
 *
 * Can be called from ISR context
 */
ICS_ERROR ICS_channel_release (ICS_CHANNEL handle, ICS_VOID *buf)
{
  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  if (buf == NULL)
    return ICS_INVALID_ARGUMENT;

  return ics_shm_channel_release(handle, buf);
}

/*
 * Called to unblock a previously blocked channel (ICS_FULL)
 *
 * Can be called from ISR context
 */
ICS_ERROR ICS_channel_unblock (ICS_CHANNEL channel)
{
  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  return ics_shm_channel_unblock(channel);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
