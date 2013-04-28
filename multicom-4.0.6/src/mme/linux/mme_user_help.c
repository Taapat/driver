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

#include <mme/mme_ioctl.h>	/* External ioctl interface */

#include "_mme_user.h"		/* Internal definitions */

#include <asm/uaccess.h>

/*
 * Called from userspace transformer helper task
 */
int mme_user_help_task (mme_user_t *instance, void *arg)
{
	ICS_ERROR           err;
	int                 res=0;

	mme_help_task_t *help = (mme_help_task_t *) arg;
	
	mme_user_command_t  *intCommand;
	MME_Command_t*       userCommand;

	MME_TransformerHandle_t handle;
	MME_Event_t             event;

	mme_user_callback_t *callback;

	mme_user_trans_t    *trans;
	int                  type, cpuNum, ver, idx;

	if (copy_from_user(&handle, &(help->handle), sizeof(MME_TransformerHandle_t)))
		return -EFAULT;

	MME_PRINTF(MME_DBG_COMMAND, "transformer handle 0x%x\n", handle);
	
	/* Decode the allocated transformer handle */
	_MME_DECODE_HDL(handle, type, cpuNum, ver, idx);

	if (handle == 0 || type != _MME_TYPE_TRANSFORMER || idx >= _MME_TRANSFORMER_INSTANCES)
		return -EINVAL;
	
	/* Get a handle into the instantiation table */
	trans = instance->insTrans[idx];

	/* Double check that this transformer is expected to
	 * have a callback task 
	 */
	if (trans == NULL || trans->mq == NULL)
		return -EINVAL;
	
	MME_PRINTF(MME_DBG_COMMAND,
		   "Waiting for event - transformer handle 0x%x trans %p[%d]\n", handle, trans, idx);

	/* Blocking waiting for a callback event (interruptible) */
	err = _ICS_OS_EVENT_WAIT(&trans->event, ICS_TIMEOUT_INFINITE, ICS_TRUE);

	if (err != ICS_SUCCESS) {
		MME_PRINTF(MME_DBG_COMMAND, "trans %p helper task interrupted : %d\n", trans, err);
		res = (err == ICS_SYSTEM_INTERRUPT ? -EINTR : -ENOTTY);
		goto exit;
	}
		
	/* Check to see if we are being terminated (trans may now be free) */
	if (instance->insTrans[idx] == NULL) {
		MME_PRINTF(MME_DBG_COMMAND, "trans %p helper task exit request\n", trans);
		
		/* Signal exit/abort */
		res = -ENODEV;
		goto exit;
	}

	/* Get the callback event from the MQ */
	callback = ics_mq_recv(trans->mq);
	MME_assert(callback != NULL);

	/* The MME_Command is embedded at beginning of the internal data structure */
	intCommand = (mme_user_command_t *) callback->command;
	event      = callback->event;

	/* Release MQ slot */
	ics_mq_release(trans->mq);

	/* The original user's command desc */
	userCommand = intCommand->userCommand;

	MME_PRINTF(MME_DBG_COMMAND, "wakeup callback %p command %p userCommand %p event %d\n",
		   callback, intCommand, userCommand, event);

	/* Now complete the userspace command (freeing off intCommand as necessary) */
	res = mme_user_command_complete(instance, intCommand, event);

	if (res == 0) {
		/* Update event and userCommand info for caller */
		if (put_user(event, &(help->event)) ||
		    put_user(userCommand, &(help->command)))
			res = -EFAULT;
	}

exit:

	return res;
}

/*
 * IRQ callback handler for userspace instantiated transformers
 *
 * Simply enqueues a callback desc and signals the user helper task
 */
void mme_user_help_callback (MME_Event_t event, MME_Command_t *command, void *param)
{
	ICS_ERROR err;
	
	mme_user_trans_t *trans = (mme_user_trans_t *) param;

	mme_user_callback_t callback;

	/* Formulate a callback event */
	callback.command = command;
	callback.event   = event;

	MME_assert(trans->mq);

	/* Enqueue callback event */
	err = ics_mq_insert(trans->mq, &callback, sizeof(callback));
	MME_assert(err == ICS_SUCCESS);

	/* Signal helper task */
	_ICS_OS_EVENT_POST(&trans->event);

	return;           ;
}

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */

