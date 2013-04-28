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


int mme_user_wait_command (mme_user_t *instance, void *arg)
{
	int res = 0;
        MME_ERROR status = MME_INTERNAL_ERROR;

	MME_TransformerHandle_t handle;
	MME_CommandId_t         cmdId;
	MME_Time_t		timeout;
	MME_Event_t             event;
	mme_wait_command_t     *waitCommand = (mme_wait_command_t *)arg;

	mme_user_command_t     *intCommand = NULL;

	mme_user_trans_t       *trans;
	int                     type, cpuNum, ver, idx;

	if (get_user(handle, &(waitCommand->handle)) ||
	    get_user(cmdId, &(waitCommand->cmdId)) ||
	    get_user(timeout, &(waitCommand->timeout))) {
		res = -EFAULT;
		goto exit;
	}
	
	/* Decode the supplied transformer handle */
	_MME_DECODE_HDL(handle, type, cpuNum, ver, idx);

	if (handle == 0 || type != _MME_TYPE_TRANSFORMER || idx >= _MME_TRANSFORMER_INSTANCES) {
		res = -EINVAL;
		status = MME_INVALID_HANDLE;
		goto exit;
	}
	
	_ICS_OS_MUTEX_TAKE(&instance->ulock);

	/* Get a handle into the local transformer table */
	trans = instance->insTrans[idx];
	
	if (trans == NULL) {
		res = -EINVAL;
		status = MME_INVALID_HANDLE;
		goto exit_release;
	}
		
	/* Find the matching internal command desc */
	list_for_each_entry(intCommand, &trans->issuedCmds, list)
	{
#ifdef DEBUG
		MME_PRINTF(MME_DBG_COMMAND, 
			   "Compare cmdId 0x%x with intCommand %p 0x%x\n",
			   cmdId, intCommand, intCommand->command.CmdStatus.CmdId);
#endif
		if (intCommand->command.CmdStatus.CmdId == cmdId)
			break;
	}

	_ICS_OS_MUTEX_RELEASE(&instance->ulock);

	if (intCommand == NULL)
	{
		res = -EINVAL;
		status = MME_INVALID_ARGUMENT;
		goto exit;
	}

	status = MME_WaitCommand(handle, cmdId, &event, timeout);
	
	if (status == MME_SUCCESS) {
	
		/* Now complete the userspace command (freeing off intCommand as necessary) */
		res = mme_user_command_complete(instance, intCommand, event);
		
		if (res != 0) {
			status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : MME_INTERNAL_ERROR;
			goto exit;
		}

		if (put_user(event, &(waitCommand->event)))
		{
			res = -EFAULT;
			goto exit;
		}
	}

exit_release:
	_ICS_OS_MUTEX_RELEASE(&instance->ulock);

exit:
	/* Update caller's status */
	if (put_user(status, &(waitCommand->status)))
		res = -EFAULT;
	
	return res;
}
	
/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
