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
 * Admin task code.
 *
 * The admin task implements a simple RPC mechanism, unpacking
 * remote API calls, executing the relevant function and then
 * sending back a response to the caller
 */

static
ICS_ERROR sendReply (ICS_UINT srcCpu, ICS_PORT replyPort, ics_admin_reply_t *reply, ICS_ERROR err)
{
  ICS_PRINTF(ICS_DBG_ADMIN, "send reply = %d to port 0x%x\n", err, replyPort);

  /* Fill out reply structure */
  reply->type = _ICS_ADMIN_REPLY;
  reply->err  = err;

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Call the internal send message API whilst holding the state lock 
   * This API doesn't require the target CPU to be in the connected state
   */
  err = _ics_msg_send(replyPort, (void *)reply, ICS_INLINE, sizeof(*reply), ICS_MSG_CONNECT, srcCpu);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
  
  return err;
}

/*
 * Map in a new region of virtual memory as requested by the
 * remote srcCpu
 */
static ICS_ERROR mapRegion (ICS_UINT srcCpu, ics_admin_msg_t *msg)
{
  ICS_ERROR        err = ICS_SUCCESS;

  ICS_OFFSET       paddr;
  ICS_SIZE         size;
  ICS_MEM_FLAGS    mflags;
  
  ICS_VOID         *map;

  ics_admin_reply_t reply;

  /* Unpack the request */
  paddr  = msg->u.map.paddr;
  size   = msg->u.map.size;
  mflags = msg->u.map.mflags;

  ICS_PRINTF(ICS_DBG_ADMIN, "srcCpu %d paddr 0x%x size %d mflags 0x%x\n", srcCpu, paddr, size, mflags);
  
  /* Map the region into our local VM */
  err = ics_region_map(srcCpu, paddr, size, mflags, &map);
  
  /* Send back a reply */
  err = sendReply(srcCpu, msg->replyPort, &reply, err);

  return err;
}

static
ICS_ERROR unmapRegion (ICS_UINT srcCpu, ics_admin_msg_t *msg)
{
  ICS_ERROR        err = ICS_SUCCESS;

  ICS_OFFSET       paddr;
  ICS_SIZE         size;
  ICS_MEM_FLAGS    mflags;

  ics_admin_reply_t reply;
  
  /* Unpack the request */
  paddr  = msg->u.map.paddr;
  size   = msg->u.map.size;
  mflags = msg->u.map.mflags;

  ICS_PRINTF(ICS_DBG_ADMIN, "srcCpu %d paddr 0x%x size %d mflags 0x%x\n", srcCpu, paddr, size, mflags);
  
  /* Remove the region from our local VM */
  err = ics_region_unmap(paddr, size, mflags);
  
  /* Send back a reply */
  err = sendReply(srcCpu, msg->replyPort, &reply, err);

  return err;
}

/*
 * Load a new dynamic module into this cpu
 *
 */
static 
ICS_ERROR dynLoad (ICS_UINT srcCpu, ics_admin_msg_t *msg)
{
  ICS_ERROR         err = ICS_SUCCESS;

  ICS_CHAR         *name;
  ICS_OFFSET        paddr;
  ICS_SIZE          size;
  ICS_UINT	    flags;
  ICS_DYN	    parent;

  ics_admin_reply_t reply;

  /* Unpack the request */
  name   = msg->u.dyn_load.name;
  paddr  = msg->u.dyn_load.paddr;
  size   = msg->u.dyn_load.size;
  flags  = msg->u.dyn_load.flags;
  parent = msg->u.dyn_load.parent;

  ICS_PRINTF(ICS_DBG_ADMIN, "srcCpu %d name '%s' paddr 0x%x size %d flags 0x%x parent 0x%x\n",
	     srcCpu, name, paddr, size, flags, parent);
  
  /* Load the module into our local OS */
  err = ics_dyn_load_image(name, paddr, size, flags, parent, &reply.u.dyn_reply.handle);

  /* Send back a reply */
  err = sendReply(srcCpu, msg->replyPort, &reply, err);

  return err;
}

/*
 * Load a new dynamic module into this cpu
 *
 */
static
ICS_ERROR dynUnload (ICS_UINT srcCpu, ics_admin_msg_t *msg)
{
  ICS_ERROR         err = ICS_SUCCESS;

  ICS_DYN           handle;

  ics_admin_reply_t reply;

  handle = msg->u.dyn_unload.handle;
  
  ICS_PRINTF(ICS_DBG_ADMIN, "handle 0x%x\n", handle);
  
  /* Unload the module from our local OS */
  err = ics_dyn_unload(handle);
  
  ICS_PRINTF(ICS_DBG_ADMIN, "send reply = %d to port 0x%x\n", err, msg->replyPort);

  /* Send back a reply */
  err = sendReply(srcCpu, msg->replyPort, &reply, err);

  return err;
}

/*
 * Main ICS Admin task which has to handle
 * all incoming administration requests
 */
void
ics_admin_task (void *param)
{
  ICS_ERROR     err = ICS_SUCCESS;
  ICS_MSG_DESC  rdesc;
  ICS_PORT      port;

  ICS_ASSERT(ics_state);

  /* The local port was created in ICS_Init() */
  port = ics_state->adminPort;
  
  ICS_PRINTF(ICS_DBG_ADMIN, "Starting: state %p port 0x%x\n", ics_state, port);

  /*
   * Loop forever, servicing requests
   */
  while (1)
  {
    ics_admin_msg_t *msg;

    ics_admin_reply_t reply;
    
    /* 
     * Post a blocking receive
     */
    err = ICS_msg_recv(port, &rdesc, ICS_TIMEOUT_INFINITE);

    if (err != ICS_SUCCESS)
    {
      /* Don't report PORT_CLOSED as errors */
      if (err != ICS_PORT_CLOSED)
	ICS_EPRINTF(ICS_DBG_ADMIN, "ICS_msg_receive returned error : %s(%d)\n", 
		    ics_err_str(err), err);

      /* Get outta here */
      break;
    }

    ICS_assert(rdesc.data);
    ICS_assert(rdesc.size);

    msg = (ics_admin_msg_t *) rdesc.data;

    ICS_assert(rdesc.size >= sizeof(ics_admin_msg_t));

    switch (msg->type)
    {
    case _ICS_ADMIN_MAP_REGION:
      err = mapRegion(rdesc.srcCpu, msg);
      break;

    case _ICS_ADMIN_UNMAP_REGION:
      err = unmapRegion(rdesc.srcCpu, msg);
      break;

    case _ICS_ADMIN_DYN_LOAD:
      err = dynLoad(rdesc.srcCpu, msg);
      break;

    case _ICS_ADMIN_DYN_UNLOAD:
      err = dynUnload(rdesc.srcCpu, msg);
      break;

    default:
      ICS_EPRINTF(ICS_DBG_ADMIN, "Incorrect msg %p size %d req 0x%x\n",
		  msg, rdesc.size, msg->type);
      
      /* Send a NACK back to sender */
      err = sendReply(rdesc.srcCpu, msg->replyPort, &reply, ICS_INVALID_ARGUMENT);
      break;
    }

    ICS_ASSERT(err == ICS_SUCCESS);

    ICS_PRINTF(ICS_DBG_ADMIN, "completed req 0x%x err %d\n", msg->type, err);
  }

  ICS_PRINTF(ICS_DBG_ADMIN, "exit : %s(%d)\n",
	     ics_err_str(err), err);

  return;
}

void ics_admin_term (void)
{
  ICS_ASSERT(ics_state);

  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_ADMIN, "Shutting down cpu %d\n", ics_state->cpuNum);

  /* Closing this port will wake the admin task */
  if (ics_state->adminPort != ICS_INVALID_HANDLE_VALUE)
  {
    ICS_port_free(ics_state->adminPort, 0);
    ics_state->adminPort = ICS_INVALID_HANDLE_VALUE;

    /* Delete the admin task */
    _ICS_OS_TASK_DESTROY(&ics_state->adminTask);
  }
}

/*
 * Create and start the Admin task 
 * Runs on all CPUs
 */
static
ICS_ERROR createAdminTask (void)
{
  ICS_ERROR err;
  
  char taskName[_ICS_OS_MAX_TASK_NAME];

  ICS_ASSERT(ics_state);

  snprintf(taskName, _ICS_OS_MAX_TASK_NAME, "ics_admin");

  /* Create a high priority thread for servicing these messages */
  err = _ICS_OS_TASK_CREATE(ics_admin_task, NULL, _ICS_OS_TASK_DEFAULT_PRIO, taskName, &ics_state->adminTask);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "Failed to create Admin task %s : %d\n", taskName, err);

    return ICS_NOT_INITIALIZED;
  }
  
  ICS_PRINTF(ICS_DBG_INIT, "created Admin task %p\n", ics_state->adminTask.task);

  return ICS_SUCCESS;
}


/*
 * Create and start the Admin Server task
 */
ICS_ERROR ics_admin_init (void)
{
  ICS_ERROR err;

  ICS_ASSERT(ics_state);

  /* Create a local port for the admin messages */
  err = ICS_port_alloc(NULL, NULL, NULL, _ICS_ADMIN_PORT_NDESC, 0, &ics_state->adminPort);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_ADMIN, "Failed to create a local port : %s (%d)\n",
		ics_err_str(err), err);
    goto error;
  }

  ICS_assert(ics_state->adminPort != ICS_INVALID_HANDLE_VALUE);

  /* Must be first Port allocated as we generate the handle in other CPUs */
  ICS_assert(_ICS_HDL2IDX(ics_state->adminPort) == 0);

  /* Create the Admin task */
  err = createAdminTask();
  if (err != ICS_SUCCESS)
  {
    /* Failed to create the Admin task */
    ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_ADMIN, "Failed to create Admin task : %d\n", 
		err);
    goto error;
  }

  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_ADMIN, "adminPort 0x%x\n",
	     ics_state->adminPort);

  return ICS_SUCCESS;

error:
  /* Tidy up */
  ics_admin_term();

  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_ADMIN, "Failed : %s (%d)\n",
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
