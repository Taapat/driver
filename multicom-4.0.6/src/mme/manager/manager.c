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
 * MME Manager task code.
 *
 */

static
MME_ERROR sendReply (mme_manager_msg_t *message, ICS_UINT messageSize, MME_ERROR res)
{
  ICS_ERROR err;

  MME_PRINTF(MME_DBG_MANAGER, "send reply %p res=%d to port 0x%x\n", message, res, message->replyPort);

  /* Update res code in response message */
  message->res = res;

  /* Send back original message buffer */
  err = ICS_msg_send(message->replyPort, message, _MME_MSG_CACHE_FLAGS, messageSize, ICS_MSG_CONNECT);

  return (err == ICS_SUCCESS) ? MME_SUCCESS : MME_ICS_ERROR;
}

/* Instantiate a transformer */
static
MME_ERROR initTransformer (ICS_UINT srcCpu, ICS_UINT messageSize, mme_manager_msg_t *message)
{
  MME_ERROR              res;

  mme_init_trans_t      *init = &message->u.init;

  mme_receiver_t        *receiver;
  mme_transformer_reg_t *treg;

  MME_GenericParams_t   *params;
  MME_UINT               paramsSize;

  MME_PRINTF(MME_DBG_MANAGER, "srcCpu %d messageSize %d message %p\n",
	     srcCpu, messageSize, message);

  MME_PRINTF(MME_DBG_MANAGER, "init %p name '%s'\n",
	     init, init->name);

  MME_assert(strlen(init->name) > 0);

  /* First lookup the corresponding Registered transformer */
  treg = mme_transformer_lookup(init->name);
  if (treg == NULL)
  {
    MME_EPRINTF(MME_DBG_MANAGER, "Failed to find transformer '%s'\n",
		init->name);
    
    res = MME_UNKNOWN_TRANSFORMER;
    goto error;
  }

  /* Parameter array (if there is one) begins after control message */
  paramsSize = messageSize - sizeof(*message);
  params = (MME_GenericParams_t*) (paramsSize ? (message + 1) : NULL);

  /* Now create a Receiver object, port and task
   */
  res = mme_receiver_create(treg, paramsSize, params, &receiver);
  if (res != MME_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_MANAGER, "Failed to create receiver for treg %p : %d\n",
		treg, res);
    goto error;
  }

  /* Add this receiver to the corresponding execution task */
  res = mme_execution_add(receiver, init->priority);
  if (res != MME_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_MANAGER, "Failed to add receiver %p to execution task @ priority %d : %d\n",
		receiver, init->priority, res);
    goto error_free;
  }
  
  /* Fill out the response components of the message */
  init->handle = (MME_TransformerHandle_t) receiver;	/* Use receiver address as a unique handle */
  init->port   = receiver->port;

  MME_PRINTF(MME_DBG_MANAGER, "Success : receiver %p port 0x%x\n",
	     receiver, receiver->port);

  /* Send back a response to the source */
  res = sendReply(message, messageSize, MME_SUCCESS);
  
  return res;

error_free:
  mme_receiver_free(receiver);

error:

  MME_EPRINTF(MME_DBG_MANAGER, "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  /* Send back a response to the source */
  res = sendReply(message, messageSize, res);

  return res;
}


/* Terminate a transformer instantiation */
static
MME_ERROR termTransformer (ICS_UINT srcCpu, ICS_UINT messageSize, mme_manager_msg_t *message)
{
  MME_ERROR         res;

  mme_term_trans_t *term = &message->u.term;
  mme_receiver_t   *receiver;

  MME_PRINTF(MME_DBG_MANAGER, "srcCpu %d messageSize %d message %p\n",
	     srcCpu, messageSize, message);

  receiver = (mme_receiver_t *) term->handle;

  /* Do some simple sanity checks on the receiver */
  if (receiver == NULL || list_empty(&receiver->list) || receiver->exec == NULL)
    return MME_INVALID_ARGUMENT;

  MME_PRINTF(MME_DBG_MANAGER, "Terminating receiver %p\n", receiver);

  /* Call the transformer termination callback */
  res = mme_receiver_term(receiver);
  if (res != MME_SUCCESS)
    return res;

  /* Now remove the receiver from the execution task list */
  mme_execution_remove(receiver);

  /* Now we can free off the receiver desc and task */
  mme_receiver_free(receiver);

  MME_PRINTF(MME_DBG_MANAGER, "Successfully terminated receiver %p\n", receiver);
  
  /* Send back a response to the source */
  res = sendReply(message, messageSize, MME_SUCCESS);

  return res;
}

/* Query a Registered transformer's capabilities */
static
MME_ERROR getCapability (ICS_UINT srcCpu, ICS_UINT messageSize, mme_manager_msg_t *message)
{
  MME_ERROR              res;

  mme_capability_t      *get = &message->u.get;

  mme_transformer_reg_t *treg;


  MME_PRINTF(MME_DBG_MANAGER, "srcCpu %d messageSize %d message %p\n",
	     srcCpu, messageSize, message);
  
  MME_assert(strlen(get->name) > 0);

  MME_PRINTF(MME_DBG_MANAGER, "get %p name '%s'\n",
	     get, get->name);

  /* First lookup the corresponding Registered transformer */
  treg = mme_transformer_lookup(get->name);
  if (treg == NULL)
  {
    MME_EPRINTF(MME_DBG_MANAGER, "Failed to find transformer '%s'\n",
		get->name);
    
    res = MME_INVALID_ARGUMENT;
    goto error;
  }

  /* Point the TransformerInfo_p at the correct place in the message */
  get->capability.TransformerInfo_p = (get->capability.TransformerInfoSize ? message + 1 : NULL);

  /* Call the Registered Transformer Get Capability function */
  res = (treg->funcs.getTransformerCapability) (&get->capability);

  MME_PRINTF(MME_DBG_MANAGER, "transformer function %p returned : %s (%d)\n",
	     treg->funcs.getTransformerCapability,
	     MME_Error_Str(res), res);

  /* Send back a response to the source */
  res = sendReply(message, messageSize, res);

  return res;

error:

  MME_EPRINTF(MME_DBG_MANAGER, "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  /* Send back a response to the source */
  res = sendReply(message, messageSize, res);
  
  return res;
}

/*
 * Main MME Manager task which has to handle all incoming 
 * transformer management requests such as creating transformer
 * instantiations etc.
 *
 * May be run from the MME_Run() call
 */
void
mme_manager_task (void *param)
{
  MME_ERROR         res = MME_INTERNAL_ERROR;

  ICS_PORT          port;
  ICS_ERROR         err = ICS_SYSTEM_ERROR;

  MME_assert(mme_state);

  /* The local port was created in MME_Init() */
  port = mme_state->managerPort;
  MME_assert(port != ICS_INVALID_HANDLE_VALUE);

  MME_PRINTF(MME_DBG_MANAGER, "Starting: state %p port 0x%x\n", mme_state, port);

  /*
   * Loop forever, servicing requests
   */
  while (1)
  {
    ICS_MSG_DESC       rdesc;
    mme_manager_msg_t *message;
    
    /* 
     * Post a blocking receive
     */
    err = ICS_msg_recv(port, &rdesc, ICS_TIMEOUT_INFINITE);

    if (err != ICS_SUCCESS)
    {
      /* Don't report PORT_CLOSED as errors */
      if (err == ICS_PORT_CLOSED)
      {
	res = MME_SUCCESS;
      }
      else
      {
	MME_EPRINTF(MME_DBG_MANAGER, "ICS_msg_receive returned error : %s (%d)\n", 
		    ics_err_str(err), err);
	res = MME_ICS_ERROR;
      }

      /* Get outta here */
      break;
    }

    MME_assert(rdesc.data);
    MME_assert(rdesc.size);
    MME_assert(!(rdesc.mflags & ICS_INLINE));

    message = (mme_manager_msg_t *) rdesc.data;

    MME_PRINTF(MME_DBG_MANAGER, "received request: data %p size %d type 0x%x\n",
	       rdesc.data, rdesc.size, message->type);

    MME_assert(rdesc.size >= sizeof(mme_manager_msg_t));

    switch (message->type)
    {
    case _MME_MANAGER_TRANS_INIT:
      /* Instantiate a transformer instance */
      res = initTransformer(rdesc.srcCpu, rdesc.size, message);
      break;

    case _MME_MANAGER_TRANS_TERM:
      /* Terminate a transformer instance */
      res = termTransformer(rdesc.srcCpu, rdesc.size, message);
      break;

    case _MME_MANAGER_GET_CAPABILITY:
      /* Query a transformer capability */
      res = getCapability(rdesc.srcCpu, rdesc.size, message);
      break;

    default:
      MME_EPRINTF(MME_DBG_MANAGER, "port 0x%x Incorrect message %p size %d type 0x%x received\n",
		  port, rdesc.data, rdesc.size, message->type);

      MME_assert(0);

      /* Send a NACK to srcCpu */
      res = sendReply(message, rdesc.size, MME_INVALID_ARGUMENT);

      break;
    }

    MME_ASSERT(res == MME_SUCCESS);

    MME_PRINTF(MME_DBG_MANAGER, "completed req 0x%x res %d\n", message->type, res);

  } /* while(1) */

  /* Inform MME_Run() we are now exiting */
  _ICS_OS_EVENT_POST(&mme_state->managerExit);

  MME_PRINTF(MME_DBG_MANAGER, "exit : %s (%d)\n",
	     MME_Error_Str(res), res);

  return;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
