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

void mme_user_command_free (mme_user_t *instance, mme_user_command_t *intCommand)
{
	MME_Command_t *command = &intCommand->command;
	int i;

	/* Only free the data structure once the refCount hits zero */
	if (!atomic_dec_and_test(&intCommand->refCount))
		return;
	
	MME_PRINTF(MME_DBG_COMMAND, "Freeing internal command %p userCommand %p\n",
		   intCommand, intCommand->userCommand);

	MME_ASSERT(atomic_read(&intCommand->refCount) == 0);

	_ICS_OS_MUTEX_TAKE(&instance->ulock);

	/* Should be on the issued command list */
	MME_ASSERT(!list_empty(&intCommand->list));

	/* Remove from issuedCmds list */
	list_del_init(&intCommand->list);

	_ICS_OS_MUTEX_RELEASE(&instance->ulock);

	for (i=0; i<command->NumberInputBuffers + command->NumberOutputBuffers; i++) {
		MME_DataBuffer_t* dataBuffer = command->DataBuffers_p[i];

        	/* Free the array of kernel scatter pages */
		_ICS_OS_FREE(dataBuffer->ScatterPages_p);
	}

	if (i) {
		/* There are some data buffers */

		/* Free the data buffer - the first ptr has it's start address */
		_ICS_OS_FREE(command->DataBuffers_p[0]);

		/* Free the buffer pointers */
		_ICS_OS_FREE(command->DataBuffers_p);
	}

        /* Free the user data */
 	if (command->Param_p && command->ParamSize) {
		_ICS_OS_FREE(command->Param_p);
        }
	
	if (command->CmdStatus.AdditionalInfo_p && command->CmdStatus.AdditionalInfoSize) {
		_ICS_OS_FREE(command->CmdStatus.AdditionalInfo_p);
        }

	/* Free the command */
	_ICS_OS_FREE(intCommand);

	return;
}

/*
 * Called from userspace transformer helper task and from 
 * MME_WaitCommand completion
 */
int mme_user_command_complete (mme_user_t *instance, mme_user_command_t *intCommand, MME_Event_t event)
{
	int res=0;

	MME_Command_t*      userCommand;
	unsigned int	    nBufs;

	MME_assert(intCommand);
	userCommand = intCommand->userCommand;	/* Original user command address */

	MME_PRINTF(MME_DBG_COMMAND,
		   "Copying event %d command %p status to userCommand %p\n",
		   event, intCommand, userCommand);

	MME_ASSERT(atomic_read(&intCommand->refCount) >= 1);

	/* Update userCommand status */
	if (put_user(intCommand->command.CmdStatus.State, &(userCommand->CmdStatus.State)) ||
	    put_user(intCommand->command.CmdStatus.Error, &(userCommand->CmdStatus.Error))) {
		MME_EPRINTF(MME_DBG_COMMAND,
			    "Failed to copy to user command status %p\n", userCommand);
		res = -EFAULT;
		goto exit;
        }

        /* Copy the private status data */
	if (intCommand->command.CmdStatus.AdditionalInfo_p && 
	    intCommand->command.CmdStatus.AdditionalInfoSize &&
	    copy_to_user(intCommand->userAdditionalInfo, 
			 intCommand->command.CmdStatus.AdditionalInfo_p,
			 intCommand->command.CmdStatus.AdditionalInfoSize)) {
		MME_EPRINTF(MME_DBG_COMMAND,
			    "Failed to copy AdditionInfo back %p %d\n",
			    intCommand->userAdditionalInfo, 
			    intCommand->command.CmdStatus.AdditionalInfoSize);
		res = -EFAULT;
		goto exit;
	}

	/* 
	 * Copy scatter page meta data back into the userspace buffer
	 */
	nBufs = intCommand->command.NumberInputBuffers + intCommand->command.NumberOutputBuffers;
	if (nBufs) {
		MME_DataBuffer_t **userBufs;
		unsigned int iBuf;

		if (get_user(userBufs, &(userCommand->DataBuffers_p))) {
			MME_EPRINTF(MME_DBG_COMMAND,
				    "Could not follow user supplied pointer (DataBuffers_p) %p\n",
				    &(userCommand->DataBuffers_p));
			res = -EFAULT;
			goto exit;
		}

		for (iBuf=0; iBuf<nBufs; iBuf++) {
			MME_DataBuffer_t *driverBuf = intCommand->command.DataBuffers_p[iBuf];
			MME_DataBuffer_t *userBuf;
			MME_ScatterPage_t *userPages;
			unsigned int iPage;

			if (get_user(userBuf, &(userBufs[iBuf])) ||
			    get_user(userPages, &(userBuf->ScatterPages_p))) {
				MME_EPRINTF(MME_DBG_COMMAND,
					    "Could not follow user supplied pointer (*DataBuffers_p %p or ScatterPages_p %p)\n",
					    &(userBufs[iBuf]),
					    &(userBuf->ScatterPages_p));
				res = -EFAULT;
				goto exit;
			}
		
			for (iPage=0; iPage<driverBuf->NumberOfScatterPages; iPage++) {
				MME_ScatterPage_t *driverPage = driverBuf->ScatterPages_p + iPage;
				MME_ScatterPage_t *userPage = userPages + iPage;

				if (put_user(driverPage->BytesUsed, &(userPage->BytesUsed)) |
				    put_user(driverPage->FlagsOut, &(userPage->FlagsOut))) {
					MME_EPRINTF(MME_DBG_COMMAND,
						    "Could not copy scatter page meta data to user memory %p\n",
						    &(userPage->FlagsOut));
					res = -EFAULT;
					goto exit;
				}
			}
		}
	}

	if (event == MME_COMMAND_COMPLETED_EVT || event == MME_TRANSFORMER_TIMEOUT) {
		/* Do cleanup for this command */
		mme_user_command_free(instance, intCommand);
	}
	

exit:
	return res;
}

int mme_user_abort_command (mme_user_t *instance, void *arg)
{
	int res = 0;
	MME_ERROR status = MME_SUCCESS;

	mme_abort_command_t *abort = (mme_abort_command_t *)arg;

	MME_CommandId_t cmdId;
	MME_TransformerHandle_t handle;

	/* Copy handle in from userspace */
	if (get_user(handle, &(abort->handle))) {
		res = -EFAULT;
		goto exit;
	}

	/* Copy command handle in from user space */
	if (get_user(cmdId, &(abort->cmdId))) {
		res = -EFAULT;
		goto exit;
	}

	/* Abort the command (asynchronous) */
	status = MME_AbortCommand(handle, cmdId);

	/* Pass back MME status code */
        if (put_user(status, &(abort->status))) {
		res = -EFAULT;
		goto exit;
	}

exit:
	return res;
}

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */

