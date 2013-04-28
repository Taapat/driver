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
 * File: nsrv_client.c
 *
 * Description
 *    Client routines to call the global ICS nameserver
 *
 **************************************************************/

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

/* 
 * Send a nsrv request message and then block waiting for a reply
 * For the request , the caller passes in the req msg and object data size (may be 0) 
 *
 * For the reply, the caller supplies a data buffer and a size pointer
 */
static
ICS_ERROR sendMsg (ics_nsrv_msg_type_t mtype, ics_nsrv_msg_t *msg, ICS_SIZE size, ICS_LONG timeout,
		   ICS_VOID *data, ICS_SIZE *sizep, ICS_NSRV_HANDLE *handlep)
{
  ICS_ERROR     err;
  ICS_PORT      nsrvPort;
  ICS_MSG_DESC  rdesc;

  ICS_PORT      nsrvReply;

  ics_nsrv_reply_t *reply;

  int           type, cpuNum, ver, idx;

  nsrvPort  = ics_state->nsrvPort;

  ICS_assert(nsrvPort != ICS_INVALID_HANDLE_VALUE);

  /* Decode the target port handle */
  _ICS_DECODE_HDL(nsrvPort, type, cpuNum, ver, idx);

  /* Create a temporary local port for the nsrv reply messages */
  err = ICS_port_alloc(NULL, NULL, NULL, ICS_PORT_MIN_NDESC, 0, &nsrvReply);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_ADMIN, "Failed to create a local port : %d\n", err);
    goto error;
  }

  ICS_ASSERT(nsrvReply != ICS_INVALID_HANDLE_VALUE);

  ICS_PRINTF(ICS_DBG_NSRV, "Sending msg %p type 0x%x to port 0x%x replyPort 0x%x\n",
	     msg, mtype, nsrvPort, nsrvReply);

  /* Fill out rest of msg */
  msg->type = mtype;
  msg->replyPort = nsrvReply;

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Send request, including object data size accordingly */
  err = _ics_msg_send(nsrvPort, msg, ICS_INLINE, offsetof(ics_nsrv_msg_t, data[size]), ICS_MSG_CONNECT, cpuNum);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  if (err != ICS_SUCCESS)
    goto error_closePort;
  
  /* Post a blocking receive for the reply */
  err = ICS_msg_recv(nsrvReply, &rdesc, timeout);
  if (err != ICS_SUCCESS)
    goto error_closePort;

  /* We expect an inline reply */
  ICS_assert(rdesc.mflags & ICS_INLINE);

  /* Decode reply result */
  reply = (ics_nsrv_reply_t *)rdesc.data;
  err = reply->err;

  /* Free off the local port */
  (void) ICS_port_free(nsrvReply, 0);

  if (err != ICS_SUCCESS)
    goto error;

  /* Copy returned object data to caller */
  if (data)
  {
    /* Determine object size from msg desc */
    *sizep = rdesc.size - offsetof(ics_nsrv_reply_t, data);

    ICS_assert(*sizep <= ICS_NSRV_MAX_DATA);

    _ICS_OS_MEMCPY(data, &reply->data[0], *sizep);
  }

  /* Return handle to caller if requested */
  if (handlep)
  {
    *handlep = reply->handle;
  }

  return ICS_SUCCESS;

error_closePort:
  (void) ICS_port_free(nsrvReply, 0);

error:
  ICS_EPRINTF(ICS_DBG_ADMIN, "nsrv request 0x%x failed reply err : %s (%d)\n",
	      mtype, ics_err_str(err), err);

  return err;
}

/*
 * Client: Register a name in the nameserver 
 */
ICS_ERROR
ICS_nsrv_add (const ICS_CHAR *name, ICS_VOID *data, ICS_SIZE size, ICS_UINT flags, ICS_NSRV_HANDLE *handlep)
{
  ICS_ERROR         err;
  ICS_UINT          validFlags = 0;
  ics_nsrv_msg_t    msg;

  if (ics_state == NULL || ics_state->nsrvPort == ICS_INVALID_HANDLE_VALUE)
  {
    err = ICS_NOT_INITIALIZED;
    goto error;
  }

  if (name == NULL || strlen(name) == 0 || strlen(name) > ICS_NSRV_MAX_NAME)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  if (data == NULL || size == 0 || size > ICS_NSRV_MAX_DATA || handlep == NULL)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  ICS_PRINTF(ICS_DBG_NSRV, "name '%s' data %p size %d flags 0x%x\n", name, data, size, flags);
  
  if (flags & ~validFlags)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  /* Part fill out the nameserver req message */
  strcpy(msg.name, name);	/* Will also copy the '\0' */

  /* Copy in the object data */
  _ICS_OS_MEMCPY(&msg.data[0], data, size);

  /* Send off the message, and wait for reply */
  err = sendMsg(_ICS_NSRV_REG, &msg, size, _ICS_COMMS_TIMEOUT, NULL, 0, handlep);

  if (err != ICS_SUCCESS)
    goto error;

  return ICS_SUCCESS;

error:
  ICS_EPRINTF(ICS_DBG_NSRV, "Failed : %s (%d)\n", 
	      ics_err_str(err), err);

  return err;
}

/*
 * Client: Deregister a name from the nameserver 
 */
ICS_ERROR
ICS_nsrv_remove (ICS_NSRV_HANDLE handle, ICS_UINT flags)
{
  ICS_ERROR         err;
  
  ICS_UINT          validFlags = 0;

  ics_nsrv_msg_t    msg;

  if (ics_state == NULL || ics_state->nsrvPort == ICS_INVALID_HANDLE_VALUE)
  {
    err = ICS_NOT_INITIALIZED;
    goto error;
  }

  if (handle == ICS_INVALID_HANDLE_VALUE)
  {
    return ICS_HANDLE_INVALID;
  }

  ICS_PRINTF(ICS_DBG_NSRV, "handle 0x%x flags 0x%x\n", handle, flags);
  
  if (flags & ~validFlags)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  /* Part fill out the nameserver req message */
  msg.u.reg.flags = flags;
  msg.u.reg.handle = handle;

  /* Send off the message, and wait for reply */
  err = sendMsg(_ICS_NSRV_UNREG, &msg, 0, _ICS_COMMS_TIMEOUT, NULL, 0, NULL);

  if (err != ICS_SUCCESS)
    goto error;
  
  return ICS_SUCCESS;

error:
  ICS_EPRINTF(ICS_DBG_NSRV, "Failed : %s (%d)\n", 
	      ics_err_str(err), err);

  return err;
}


/*
 * Client: Lookup a name in the nameserver 
 *
 * Block waiting for it to be created if ICS_BLOCK is set
 */
ICS_ERROR
ICS_nsrv_lookup (const ICS_CHAR *name, ICS_UINT flags, ICS_LONG timeout, ICS_VOID *data, ICS_SIZE *sizep)
{
  ICS_ERROR         err;

  ICS_UINT          validFlags = ICS_BLOCK;

  ics_nsrv_msg_t    msg;

  if (ics_state == NULL || ics_state->nsrvPort == ICS_INVALID_HANDLE_VALUE)
  {
    err = ICS_NOT_INITIALIZED;
    goto error;
  }

  ICS_PRINTF(ICS_DBG_NSRV, "name '%s' flags 0x%x timeout %d\n",
	     name, flags, timeout);
  
  if (data == NULL || sizep == NULL || name == NULL || strlen(name) == 0 || strlen(name) > ICS_NSRV_MAX_NAME)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }
  
  if (flags & ~validFlags)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  if ((flags & ICS_BLOCK) && (timeout == ICS_TIMEOUT_IMMEDIATE))
  {
    /* Doesn't make sense to request a blocking lookup with an immediate timeout */
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }
    
  /* Part fill out the nameserver lookup message */
  msg.u.lookup.timeout = timeout;
  msg.u.lookup.flags   = flags;
  strcpy(msg.name, name);	/* Will also copy the '\0' */

  /* Send off the message, and wait for reply 
   * On success, object data will have been copied to data buffer and
   * its size returned in sizep
   */
  err = sendMsg(_ICS_NSRV_LOOKUP, &msg, 0, timeout, data, sizep, NULL);

  if (err != ICS_SUCCESS)
    goto error;

  ICS_PRINTF(ICS_DBG_NSRV, "name '%s' -> object size %d\n", name, *sizep);

  return ICS_SUCCESS;

error:
  ICS_EPRINTF(ICS_DBG_NSRV, "Failed : %s (%d)\n", 
	      ics_err_str(err), err);

  return err;
}

/* 
 * Called to inform the nameserver that a CPU is going down
 * hence it should release all objects owned by the failed cpu
 *
 * Can also be used when a cpu has crashed by setting the
 * ICS_CPU_DEAD flag which will mean no messages are sent to
 * the failed cpu
 */
ICS_ERROR
ics_nsrv_cpu_down (ICS_UINT cpuNum, ICS_UINT flags)
{
  ICS_ERROR         err;

  ICS_UINT          validFlags = ICS_CPU_DEAD;

  ics_nsrv_msg_t    msg;

  if (ics_state == NULL || ics_state->nsrvPort == ICS_INVALID_HANDLE_VALUE)
  {
    err = ICS_NOT_INITIALIZED;
    goto error;
  }

  ICS_PRINTF(ICS_DBG_NSRV, "cpuNum %d flags 0x%x\n", cpuNum, flags);

  if (cpuNum >= _ICS_MAX_CPUS || flags & ~validFlags)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }
  
  /* Part fill out the nameserver req message */
  msg.u.cpu_down.cpuNum = cpuNum;
  msg.u.cpu_down.flags  = flags;

  /* Send off the message, and wait for reply */
  err = sendMsg(_ICS_NSRV_CPU_DOWN, &msg, 0, _ICS_COMMS_TIMEOUT, NULL, 0, NULL);

  if (err != ICS_SUCCESS)
    goto error;

  return ICS_SUCCESS;

error:
  ICS_EPRINTF(ICS_DBG_NSRV, "Failed : %s (%d)\n", 
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
