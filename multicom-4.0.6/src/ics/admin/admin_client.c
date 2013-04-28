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
 * Description
 *    Client routines to call the per CPU ICS admin task
 *
 **************************************************************/

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

/*
 * Generic function to send an admin message to the remote cpu admin task and then
 * block waiting for the reply
 */
static
ICS_ERROR
ics_admin_send_msg (ics_admin_msg_type_t msgType, ics_admin_msg_t *msg, ics_admin_reply_t *reply,
		    ICS_UINT cpuNum, ICS_UINT flags)
{
  ICS_ERROR     err = ICS_SUCCESS;

  ICS_PORT      adminPort;
  ICS_PORT      adminReply;

  ICS_MSG_DESC  rdesc;

  ICS_ASSERT(ics_state);
  ICS_ASSERT(cpuNum < _ICS_MAX_CPUS);
  ICS_ASSERT(msg && reply);

  /* Generate the per CPU adminPort (hard-wired port #0) */
  adminPort = _ICS_HANDLE(_ICS_TYPE_PORT, cpuNum, 0 /* version */, 0 /* index */);

  /* Create a temporary local port for the admin reply messages */
  err = ICS_port_alloc(NULL, NULL, NULL, ICS_PORT_MIN_NDESC, 0, &adminReply);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_ADMIN, "Failed to create a local port : %d\n", err);
    goto error;
  }

  ICS_ASSERT(adminReply != ICS_INVALID_HANDLE_VALUE);

  /* Fill out common section of the message */
  msg->type      = msgType;
  msg->replyPort = adminReply;

  /* We must be able to send these messages inline */
  ICS_ASSERT(sizeof(*msg) <= ICS_MSG_INLINE_DATA);

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);
  
  /* Call the internal send message API whilst holding the state lock 
   * This API doesn't require the target CPU to be in the connected state
   */
  err = _ics_msg_send(adminPort, msg, ICS_INLINE, sizeof(*msg), flags, cpuNum);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
  
  if (err != ICS_SUCCESS)
    goto error_closePort;

  /* Post a blocking receive for the reply */
  err = ICS_msg_recv(adminReply, &rdesc, _ICS_COMMS_TIMEOUT);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_ADMIN, "ICS_msg_recv : cpuNum %d adminReply 0x%x : %s (%d)\n",
		cpuNum, adminReply, ics_err_str(err), err);
    
    goto error_closePort;
  }
  
  ICS_PRINTF(ICS_DBG_ADMIN, "Reply received : mflags 0x%x size %d data %p\n",
	     rdesc.mflags, rdesc.size, rdesc.data);

  /* We expect an inline reply */
  ICS_ASSERT(rdesc.mflags & ICS_INLINE);
  ICS_ASSERT(rdesc.size == sizeof(*reply));

  /* Copy returned reply structure to caller */
  *reply = *((ics_admin_reply_t *) rdesc.data);

  ICS_PRINTF(ICS_DBG_ADMIN, "completed err %d\n", err);

  /* Free off the local port */
  (void) ICS_port_free(adminReply, 0);

  return err;

error_closePort:
  (void) ICS_port_free(adminReply, 0);
  
error:
  ICS_EPRINTF(ICS_DBG_ADMIN, "Failed : %s (%d)\n",
	      ics_err_str(err), err);

#ifdef ICS_DEBUG
  if (err == ICS_SYSTEM_TIMEOUT)
  {
    if (ics_state->cpuNum == 0)
      /* TIMEOUT ERROR: Dump out the log of the target CPU */
      ICS_debug_dump(cpuNum);
  }
#endif

  return err;
}

/*
 * Client: Map a memory region in the target cpu
 */
ICS_ERROR
ics_admin_map_region (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags, ICS_UINT cpuNum)
{
  ICS_ERROR         err;
  ics_admin_msg_t   msg;
  ics_admin_reply_t reply;

  ICS_ASSERT(ics_state);
  ICS_ASSERT(cpuNum < _ICS_MAX_CPUS);
  ICS_ASSERT(size);
  ICS_ASSERT(mflags);

  ICS_PRINTF(ICS_DBG_ADMIN, "paddr 0x%x size %d mflags 0x%x cpuNum %d\n",
	     paddr, size, mflags, cpuNum);
  
  msg.u.map.paddr  = paddr;
  msg.u.map.size   = size;
  msg.u.map.mflags = mflags;

  /* Send off the admin message and wait for a reply */
  err = ics_admin_send_msg(_ICS_ADMIN_MAP_REGION, &msg, &reply, cpuNum, ICS_MSG_CONNECT);
  if (err != ICS_SUCCESS)
  {
    /* Send message failed */
    return err;
  }

  /* Extract reply info */
  err = reply.err;

  ICS_PRINTF(ICS_DBG_ADMIN, "completed err %d\n", err);

  return err;
}

/*
 * Client: unmap a memory region in the target cpu
 */
ICS_ERROR
ics_admin_unmap_region (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags, ICS_UINT cpuNum)
{
  ICS_ERROR         err;
  ics_admin_msg_t   msg;
  ics_admin_reply_t reply;

  ICS_ASSERT(ics_state);
  ICS_ASSERT(cpuNum < _ICS_MAX_CPUS);
  ICS_ASSERT(size);
  ICS_ASSERT(mflags);

  ICS_PRINTF(ICS_DBG_ADMIN, "paddr 0x%x size %d mflags 0x%x cpuNum %d\n",
	     paddr, size, mflags, cpuNum);
  
  msg.u.map.paddr  = paddr;
  msg.u.map.size   = size;
  msg.u.map.mflags = mflags;

  /* Send off the admin message and wait for a reply */
  err = ics_admin_send_msg(_ICS_ADMIN_UNMAP_REGION, &msg, &reply, cpuNum, 0);
  if (err != ICS_SUCCESS)
  {
    /* Send message failed */
    return err;
  }

  /* Extract reply info */
  err = reply.err;

  ICS_PRINTF(ICS_DBG_ADMIN, "completed err %d\n", err);

  return err;
}

/*
 * Client: Load a dynamic module into the target cpu
 */
ICS_ERROR
ics_admin_dyn_load (ICS_CHAR *name, ICS_OFFSET paddr, ICS_SIZE size, ICS_UINT flags, ICS_UINT cpuNum,
		    ICS_DYN parent, ICS_DYN *handlep)
{
  ICS_ERROR         err = ICS_SUCCESS;
  ics_admin_msg_t   msg = { 0 };
  ics_admin_reply_t reply;

  ICS_ASSERT(ics_state);
  ICS_ASSERT(cpuNum < _ICS_MAX_CPUS);
  ICS_ASSERT(size);
  ICS_ASSERT(handlep);

  ICS_PRINTF(ICS_DBG_ADMIN, "name '%s' parent 0x%x paddr 0x%x size %d cpuNum %d\n",
	     name, parent, paddr, size, cpuNum);

  /* Fill out request structure */
  strncpy(msg.u.dyn_load.name, name, _ICS_MAX_PATHNAME_LEN);
  msg.u.dyn_load.paddr  = paddr;
  msg.u.dyn_load.size   = size;
  msg.u.dyn_load.flags  = flags;
  msg.u.dyn_load.parent = parent;

  /* Send off the admin message and wait for a reply */
  err = ics_admin_send_msg(_ICS_ADMIN_DYN_LOAD, &msg, &reply, cpuNum, ICS_MSG_CONNECT);
  if (err != ICS_SUCCESS)
  {
    /* Send message failed */
    return err;
  }
  
  /* Extract reply info */
  err = reply.err;
  if (err == ICS_SUCCESS)
    *handlep = reply.u.dyn_reply.handle;
  else
    *handlep = ICS_INVALID_HANDLE_VALUE;

  ICS_PRINTF(ICS_DBG_ADMIN, "completed err %d handle 0x%x\n", err, *handlep);

  return err;
}

/*
 * Client: Unload a dynamic module based on its handle
 */
ICS_ERROR
ics_admin_dyn_unload (ICS_DYN handle)
{
  ics_admin_msg_t   msg;
  ics_admin_reply_t reply;

  ICS_ERROR         err;
  int               type, cpuNum, ver, idx;

  ICS_ASSERT(ics_state);

  /* Decode the dynamic module handle */
  _ICS_DECODE_HDL(handle, type, cpuNum, ver, idx);

  if (type != _ICS_TYPE_DYN || cpuNum >= _ICS_MAX_CPUS || idx >= _ICS_MAX_DYN_MOD)
  {
    return ICS_INVALID_ARGUMENT;
  }

  ICS_PRINTF(ICS_DBG_ADMIN, "handle 0x%x cpuNum %d idx %d\n", handle, cpuNum, idx);
  
  /* Fill out request structure */
  msg.u.dyn_unload.handle = handle;

  /* Send off the admin message and wait for a reply */
  err = ics_admin_send_msg(_ICS_ADMIN_DYN_UNLOAD, &msg, &reply, cpuNum, 0);
  if (err != ICS_SUCCESS)
  {
    /* Send message failed */
    return err;
  }
  
  /* Extract reply info */
  err = reply.err;

  ICS_PRINTF(ICS_DBG_DYN, "completed err %d\n", err);

  return err;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
