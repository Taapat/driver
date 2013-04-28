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
 * Userspace->Kernel wrappers for MME calls
 */

int mme_user_init_transformer (mme_user_t *instance, void *arg)
{
	int                    res = -ENODEV;
	MME_ERROR              status = MME_INTERNAL_ERROR;

        mme_init_transformer_t *init = (mme_init_transformer_t *) arg;

	char                   name[MME_MAX_TRANSFORMER_NAME+1];
	unsigned int           nameLength;
        MME_TransformerInitParams_t params;
	MME_GenericParams_t    localParams = NULL;

	MME_TransformerHandle_t handle;

	mme_user_trans_t      *trans = NULL;
	int                    idx;

	/* copy the init params and the name */
	if (copy_from_user(&params, &init->params, sizeof(params)) || 	
	    copy_from_user(&nameLength, &init->nameLength, sizeof(nameLength))) {
		res = -EFAULT;
		goto errorExit;
	}
	
	/* Don't allow excessive length names through */
	nameLength = min(MME_MAX_TRANSFORMER_NAME, nameLength);
	
	if (copy_from_user(name, init->name, nameLength)) {
		res = -EFAULT;
		goto errorExit;
	}
	name[nameLength] = '\0';

	MME_PRINTF(MME_DBG_INIT, "name '%s' InitParams_p %p Callback %p\n",
		   name, params.TransformerInitParams_p, params.Callback);

	/* Deal with init parameters - copy to kernel space */
	if (params.TransformerInitParams_p && params.TransformerInitParamsSize) {
		localParams = _ICS_OS_MALLOC(params.TransformerInitParamsSize);
		if (!localParams) {
			res = -ENOMEM;
			status = MME_NOMEM;
			goto errorExit;
                }

		if (copy_from_user(localParams, params.TransformerInitParams_p,
				   params.TransformerInitParamsSize)) {
			res = -EFAULT;
			goto errorFree;
                }
		params.TransformerInitParams_p = localParams;
        }
	
	/* Allocate a local instantiation structure */
	_ICS_OS_ZALLOC(trans, sizeof(*trans));
	if (trans == NULL) {
			res = -ENOMEM;
			status = MME_NOMEM;
			goto errorFree;
	}

	MME_PRINTF(MME_DBG_INIT, "instance %p trans %p\n",
		   instance, trans);

	INIT_LIST_HEAD(&trans->issuedCmds);

	/* If the user requires a callback, replace the supplied Callback
	 * function with our internal one. This will in turn schedule wakeups
	 * of the user Pthread which will do the callback in the correct user context
	 */
	{
		params.Callback = mme_user_help_callback;
		params.CallbackUserData = trans;
	}

	/* Instantiate the transformer */
	status = MME_InitTransformer(name, &params, &handle);

	MME_PRINTF(MME_DBG_TRANSFORMER,
		   "Transformer handle 0x%08x, status %d\n",
		   (int)handle, (int)status);

	if (status != MME_SUCCESS) {
		res = -ENODEV;
		goto errorFree;
	}
	
	if (params.Callback) {
		MME_assert(trans->handle == 0);

		/* Allocate a callback completion message queue */
		trans->mq = _ICS_OS_MALLOC(_ICS_MQ_SIZE(_MME_MAX_COMMANDS+1, sizeof(mme_user_callback_t)));
		if (trans->mq == NULL)
		{
			res = -ENOMEM;
			status = MME_NOMEM;
			goto errorTerm;
		}
		
		/* Initialise the message queue */
		ics_mq_init(trans->mq, _MME_MAX_COMMANDS+1, sizeof(mme_user_callback_t));

		/* Allocate an event to block/sleep on */
		MME_assert(_ICS_OS_EVENT_INIT(&trans->event));
	} 

	/* Associate the allocated handle with the local instantiation */
	trans->handle = handle;

	/* copy the allocated transformer handle back */
	if (put_user(handle, &(init->handle))) {
		res = -EFAULT;
		goto errorTerm;
	}

	/* Write back status code to caller */
        if (put_user(status, &(init->status))) {
		res = -EFAULT;
		goto errorTerm;
	}

	/* Decode the allocated transformer handle */
	idx = _MME_HDL2IDX(handle);
	
	MME_ASSERT(instance->insTrans[idx] == NULL);

	/* Insert into the local transformer table */
	instance->insTrans[idx] = trans;
	
	if (localParams) {
		_ICS_OS_FREE(localParams);
        }

	return 0;

errorTerm:
	(void) MME_TermTransformer(handle);
	
errorFree:
	if (localParams) {
		_ICS_OS_FREE(localParams);
        }
	
	if (trans) {
		if (trans->mq)
			_ICS_OS_FREE(trans->mq);
			
		_ICS_OS_FREE(trans);
	}

errorExit:
        (void) put_user(status, &(init->status));
			
	return res;
}

static __inline__
MME_ERROR termTransformer (mme_user_t *instance, int idx, ICS_BOOL kill)
{
	MME_ERROR status;
	
	mme_user_trans_t *trans = instance->insTrans[idx];
	
	/* Terminate the transformer */
	status = MME_TermTransformer(trans->handle);

	MME_PRINTF(MME_DBG_TRANSFORMER,
		   "Terminated Transformer handle 0x%08x status %d\n",
		   trans->handle, status);

	if ((status != MME_SUCCESS) && kill) {
		/* Kill all outstanding commands */
		status = MME_KillCommandAll(trans->handle);

		/* Kill transformer instantiation */
		status = MME_KillTransformer(trans->handle);
	}
		
	/* On success, clear down the local transformer info */
	if (status == MME_SUCCESS) {
		trans->handle = 0;
	
		/* Clear down the corresponding transformer info */
		instance->insTrans[idx] = NULL;
	
		/* Free off the callback task message queues etc */
		if (trans->mq) {
			MME_PRINTF(MME_DBG_TRANSFORMER,
				   "wake the helper task event %p\n",
				   &trans->event);

			/* Wakeup the user helper task */
			_ICS_OS_EVENT_POST(&trans->event);

			_ICS_OS_FREE(trans->mq);
			trans->mq = NULL;
			
			_ICS_OS_EVENT_DESTROY(&trans->event);
		}

		/* Finally free off the local instantiation */
		_ICS_OS_FREE(trans);
	}

	return status;
}

/* Called from userspace ioctl() */
int mme_user_term_transformer (mme_user_t *instance, void *arg)
{
	int res = 0;
	MME_ERROR status = MME_INTERNAL_ERROR;

	mme_term_transformer_t *term = (mme_term_transformer_t *) arg;

	MME_TransformerHandle_t handle;

	int                     type, cpuNum, ver, idx;

	/* Copy transformer handle in from userspace */
	if (get_user(handle, &(term->handle))) {
		res = -EFAULT;
		goto exit;
	}

	MME_PRINTF(MME_DBG_TRANSFORMER, "Terminating Transformer handle 0x%08x\n",
		   handle);

	/* Decode the allocated transformer handle */
	_MME_DECODE_HDL(handle, type, cpuNum, ver, idx);
	
	if (handle == 0 || type != _MME_TYPE_TRANSFORMER || idx >= _MME_TRANSFORMER_INSTANCES) {
		res = -EINVAL;
		status = MME_INVALID_HANDLE;
		goto exit;
	}

	/* Terminate the transformer */
	status = termTransformer(instance, idx, ICS_FALSE);

exit:
	/* Write back status code to caller */
        if (put_user(status, &(term->status))) {
		res = -EFAULT;
	}

	return res;
}

/* User process is exiting - abort all issued commands */
static
void abortCommands (mme_user_t *instance, mme_user_trans_t *trans)
{
	mme_user_command_t *intCommand;

	/* Iterate over all issued command descs */
	list_for_each_entry(intCommand, &trans->issuedCmds, list)
	{
		MME_ERROR status = MME_SUCCESS;

		MME_PRINTF(MME_DBG_TRANSFORMER, 
			   "trans %p (0x%x) Aborting CmdId 0x%x CmdEnd %d\n",
			   trans, trans->handle, intCommand->command.CmdStatus.CmdId, intCommand->command.CmdEnd);

		/* Abort the command */
		status = MME_AbortCommand(trans->handle, intCommand->command.CmdStatus.CmdId);
#if 0
		if (status != MME_SUCCESS)
			printk("mme_user: Failed to abort handle 0x%x CmdId 0x%x : %s (%d)\n",
			       trans->handle, intCommand->command.CmdStatus.CmdId, MME_ErrorStr(status), status);
#endif
	}
}

/* User process is exiting - wait for any issued commands */
static
void waitCommands (mme_user_t *instance, mme_user_trans_t *trans)
{
	mme_user_command_t *intCommand, *tmp;

	/* Iterate over all issued command descs */
	list_for_each_entry_safe(intCommand, tmp, &trans->issuedCmds, list)
	{
		MME_ERROR status = MME_SUCCESS;
		
		/* Does this command complete with MME_WaitCommand() ? */
		if (intCommand->command.CmdEnd == MME_COMMAND_END_RETURN_WAKE) {
			MME_Event_t event;
			
			MME_PRINTF(MME_DBG_TRANSFORMER, 
				   "trans %p (0x%x) Waiting CmdId 0x%x CmdEnd %d\n",
				   trans, trans->handle, intCommand->command.CmdStatus.CmdId, intCommand->command.CmdEnd);
			
			status = MME_WaitCommand(trans->handle, intCommand->command.CmdStatus.CmdId, &event,
						 _MME_COMMAND_TIMEOUT);
			
			if (status != MME_SUCCESS)
				printk("mme_user: Failed WaitCommand handle 0x%x command 0x%x : %s (%d)\n",
				       trans->handle, intCommand->command.CmdStatus.CmdId, MME_ErrorStr(status), status);
			else
				/* Do cleanup for this command */
				mme_user_command_free(instance, intCommand);
		}
	}


}

/* Called as the user process terminates
 */
void mme_user_transformer_release (mme_user_t *instance)
{
	MME_ERROR               status;

	int                     idx;

	/* Find all instantiated transformers */
	for (idx = 0; idx < _MME_TRANSFORMER_INSTANCES; idx++) {
		/* Terminate transformer */
		if (instance->insTrans[idx]) {
			int count;
			
			/* Abort any issued commands (asynchronous) */
			abortCommands(instance, instance->insTrans[idx]);

			count = 0;
			do {
				/* Attempt to terminate the transformer */
				status = termTransformer(instance, idx, ICS_FALSE);

				if (status != MME_SUCCESS) {
#if 0
					printk("mme_user: Failed to terminate transformer %p : %s (%d)\n",
					       instance->insTrans[idx], MME_ErrorStr(status), status);
#endif
				
					/* Attempt to wait for any WaitCommand commands */
					waitCommands(instance, instance->insTrans[idx]);

					_ICS_OS_TASK_DELAY(100);
				}

			} while ((status == MME_COMMAND_STILL_EXECUTING) && (count++ < 50));
			
			if (status == MME_COMMAND_STILL_EXECUTING) {
				/* Force kill of transformer (remote must be dead) */
				status = termTransformer(instance, idx, ICS_TRUE);
			}

#if 0
			if (status != MME_SUCCESS) 
				printk("mme_user: Failed to terminate transformer %p : %s (%d)\n",
				       instance->insTrans[idx], MME_ErrorStr(status), status);
#endif
		}
	}

}

/* Called from userspace ioctl() */
int mme_user_is_transformer_registered(mme_user_t *instance, void *arg)
{
	int res = 0;
	MME_ERROR status = MME_INTERNAL_ERROR;

	mme_is_transformer_reg_t *regtfr = (mme_is_transformer_reg_t *) arg;

	char                         name[MME_MAX_TRANSFORMER_NAME+1];
	unsigned int                 length;
	char*                        namePtr;

	if (get_user(length, &(regtfr->length))) {
		res = -EFAULT;
		goto exit;
	}
	if (get_user(namePtr, &(regtfr->name))) {
		res = -EFAULT;
		goto exit;
	}

	/* Don't allow excessive length names through */
	length = min(MME_MAX_TRANSFORMER_NAME, length);
	
	/* Copy the name */
	if (copy_from_user(name, namePtr, length)) {
		res = -EFAULT;
		goto exit;
 }
	name[length] = '\0';

	status = MME_IsTransformerRegistered(name);

exit:
	/* Pass back MME status code */
  if (put_user(status, &(regtfr->status))) {
		res = -EFAULT;
	}
	return res;
}


/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
