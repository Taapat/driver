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

#if 0
static 
int ValidatePagesContiguous (unsigned virtAddress, unsigned size, unsigned* physBase)
{
	/* Iterate over the pages ensuring they are physically contiguous */
        int res = 0;

	unsigned virtBase  = (virtAddress / PAGE_SIZE) * PAGE_SIZE;
	unsigned offset    = virtAddress - virtBase;
	unsigned range     = offset + size;
	unsigned pageCount = range/PAGE_SIZE + ((range%PAGE_SIZE)?1:0);
	unsigned lastPhys  = 0xffffffff;
	unsigned thisPage;

	for (thisPage=0; thisPage<pageCount; thisPage++, virtBase += PAGE_SIZE) {
		ICS_ERROR err;
		ICS_OFFSET phys;
		
		/* Perform a Virtual to physical translation of the user address */
		err = _ICS_OS_VIRT2PHYS((void *)virtBase, &phys);
		
		if (err == ICS_INVALID_ARGUMENT) {
			MME_EPRINTF(ICS_DBG_
                		(DEBUG_ERROR_STR "Page lookup failed for address 0x%08x\n", virtBase));
                	printk("Page lookup failed for address 0x%08x\n", virtBase);
			res = -EFAULT;
			break;
		}
		
		if (lastPhys == 0xffffffff) {
			lastPhys  = phys;
			*physBase = offset + lastPhys;
#ifdef NO_PAGE_CHECK
			return 0;
#endif
		} else if (phys == (lastPhys + PAGE_SIZE)) {
			lastPhys += PAGE_SIZE;		
		} else {
                	printk("Page discontinuous virtual 0x%08x, last phys 0x%08x, phys 0x%08x, pageCount %d\n", 
                	       virtBase, lastPhys, phys, pageCount);
                	
			res = -EFAULT;
			break;
		}
	}        

	return res;
}
#endif

static int CopyBuffer (MME_DataBuffer_t *dstBuffer, MME_DataBuffer_t *srcBuffer)
{
	int res = 0;
	MME_ScatterPage_t* scatterPages;
	
	/* Copy the MME_DataBuffer_t from user space */
	if (copy_from_user(dstBuffer, srcBuffer, sizeof(MME_DataBuffer_t))) {
		MME_EPRINTF(MME_DBG_COMMAND, "Failed to copy buffer\n"
			    "Kernel 0x%08x, User 0x%08x, Size %d\n",
			    (int)dstBuffer, (int)srcBuffer, sizeof(MME_DataBuffer_t));
		res = -EFAULT;
		goto exit;
	}
	MME_PRINTF(MME_DBG_COMMAND, "Copied buffer - Kernel 0x%08x, User 0x%08x, Num scatter pages %d\n",
		   (int)dstBuffer, (int)srcBuffer, dstBuffer->NumberOfScatterPages);
	
	/* Allocate space for the array of scatter page descriptors */
	scatterPages = _ICS_OS_MALLOC(sizeof(MME_ScatterPage_t) * dstBuffer->NumberOfScatterPages);
	if (!scatterPages) {
		res = -ENOMEM;
		goto exit;
	}

	/* Copy the array of scatter pages */
	if (copy_from_user(scatterPages, dstBuffer->ScatterPages_p, 
                           sizeof(MME_ScatterPage_t) * dstBuffer->NumberOfScatterPages)) {
		MME_EPRINTF(MME_DBG_COMMAND, "Failed to copy scatter pages\n"
			    "Kernel 0x%08x, User 0x%08x, Size %d\n",
			    (int)scatterPages, (int)dstBuffer->ScatterPages_p, 
			    sizeof(MME_ScatterPage_t) * dstBuffer->NumberOfScatterPages);
		res = -EFAULT;
		goto exitFree;
	}

        /* Point to the array of new pages in kernel space */
        dstBuffer->ScatterPages_p = scatterPages;

	/* Scatter pages are flushed from memory during command pack */

	return res;
	
exitFree:
	_ICS_OS_FREE(scatterPages);
exit:
	return res;
}

static int CopyBuffers (MME_Command_t* commandInfo)
{
	int res = 0;
	int i;

	unsigned nbufs         = commandInfo->NumberInputBuffers + commandInfo->NumberOutputBuffers;
	unsigned bufferPtrSize = nbufs * sizeof(MME_DataBuffer_t*);

	MME_DataBuffer_t **dataBufferPtrs = NULL;
	MME_DataBuffer_t  *dataBuffers    = NULL;
 		                 
	if (nbufs == 0) {
		/* No buffers with this command */
		goto error;
	}

	/* Allocate a kernel array of buffer pointers */
	dataBufferPtrs = _ICS_OS_MALLOC(bufferPtrSize);
        if (!dataBufferPtrs) {
                res = -ENOMEM;
                goto error;
        }

	/* Allocate a kernel array of buffer descriptors */
	dataBuffers = _ICS_OS_MALLOC(sizeof(MME_DataBuffer_t) * nbufs);
        if (!dataBuffers) {
                res = -ENOMEM;
                goto error_free;
        }
	
	/* Get the array of pointers to data buffers */
	if (copy_from_user(dataBufferPtrs, commandInfo->DataBuffers_p, bufferPtrSize)) {
		MME_EPRINTF(MME_DBG_COMMAND, "Failed to copy buffer ptrs \n"
			    "Kernel 0x%08x, User 0x%08x, Size %d\n", 
			    (int)dataBufferPtrs, (int)commandInfo->DataBuffers_p, (int)bufferPtrSize);
		res = -EFAULT;
		goto error_free;
	}

	/* Point to the array in kernel space */
	commandInfo->DataBuffers_p = dataBufferPtrs;
	MME_PRINTF(MME_DBG_COMMAND, "Buffer ptrs at 0x%08x\n", (int)commandInfo->DataBuffers_p);

	/* Iterate over all input and output buffers */
	for (i = 0; i < nbufs; i++) {
		res = CopyBuffer(&dataBuffers[i], dataBufferPtrs[i]);
		if (res) {
			goto error_freeBufs;
		}
		
		/* Point to the kernel copy */
		dataBufferPtrs[i] = (MME_DataBuffer_t*)(&dataBuffers[i]);
		MME_PRINTF(MME_DBG_COMMAND, "Done buffer[%d] at 0x%08x\n", 
			   i, (int)dataBufferPtrs[i]);
	}

	/* TODO - error cases */
	/* TODO - deallocation */
	/* TODO - remove hack */

	return res;

error_freeBufs:
	for ( i -= 1 ; i >= 0; i--)
		_ICS_OS_FREE(dataBuffers[i].ScatterPages_p);
	
error_free:
	if (dataBuffers)
		_ICS_OS_FREE(dataBuffers);
	if (dataBufferPtrs)
		_ICS_OS_FREE(dataBufferPtrs);
error:
	return res;
}

int mme_user_send_command (mme_user_t *instance, void *arg)
{
	int res = 0;
        MME_ERROR status = MME_INTERNAL_ERROR;

	MME_TransformerHandle_t handle;
	mme_send_command_t     *sendCommand = (mme_send_command_t *)arg;

	mme_user_command_t     *intCommand;
	MME_Command_t*          command;

	mme_user_trans_t       *trans;
	int                     type, cpuNum, ver, idx;

	/* Allocate an internal Command desc */
	_ICS_OS_ZALLOC(intCommand, sizeof(*intCommand));
	if (intCommand == NULL) {
		status = MME_NOMEM;
		res = -ENOMEM;
		goto exit;
	}
	
	/* Get address of embedded MME_Command_t */
	command = &intCommand->command;

	/* Bugzilla 4956 
	 * We set the reference count of this data structure to 2 so that it
	 * will not be freed by on completion until we have done the
	 * copy_to_user() below
	 */
	atomic_set(&intCommand->refCount, 2);

	INIT_LIST_HEAD(&intCommand->list);

	/* Copy in transformer handle and CommandInfo from userspace */
	if (get_user(handle, &(sendCommand->handle)) ||
	    get_user(intCommand->userCommand, &(sendCommand->command)) || 
	    copy_from_user(command, intCommand->userCommand, sizeof(*command))) {
		res = -EFAULT;
		goto errorFreeCmd;
	}

	/* Decode the supplied transformer handle */
	_MME_DECODE_HDL(handle, type, cpuNum, ver, idx);

	if (handle == 0 || type != _MME_TYPE_TRANSFORMER || idx >= _MME_TRANSFORMER_INSTANCES) {
		res = -EINVAL;
		status = MME_INVALID_HANDLE;
		goto errorFreeCmd;
	}

	/* Get a handle into the local transformer table */
	trans = instance->insTrans[idx];
	
	if (trans == NULL) {
		res = -EINVAL;
		status = MME_INVALID_HANDLE;
		goto errorFreeCmd;
	}

	if (sizeof(MME_Command_t) != command->StructSize) {
		res = -EINVAL;
		status = MME_INVALID_HANDLE;
		goto errorFreeCmd;
	}

        /* Allocate kernel mem for the user private data and copy across */
	if (command->Param_p && command->ParamSize) {
                void* userParam_p = command->Param_p;
		command->Param_p = _ICS_OS_MALLOC(command->ParamSize);

		if (NULL == command->Param_p) {
			status = MME_NOMEM;
			res = -ENOMEM;
			goto errorFreeCmd;
		}
		if (copy_from_user(command->Param_p, userParam_p, 
				   command->ParamSize)) {
			res = -EFAULT;
			goto errorFreeParam;
                }
       	}

        /* Allocate kernel mem for the AddionalInfo data and copy across */
	if (command->CmdStatus.AdditionalInfo_p && command->CmdStatus.AdditionalInfoSize) {
                /* Need the user address to copy back when the command completes */
                intCommand->userAdditionalInfo = command->CmdStatus.AdditionalInfo_p;
		command->CmdStatus.AdditionalInfo_p = _ICS_OS_MALLOC(command->CmdStatus.AdditionalInfoSize);

		if (NULL == command->CmdStatus.AdditionalInfo_p) {
			status = MME_NOMEM;
			res = -ENOMEM;
			goto errorFreeParam;
		}
		if (copy_from_user(command->CmdStatus.AdditionalInfo_p, intCommand->userAdditionalInfo, 
				   command->CmdStatus.AdditionalInfoSize)) {
			res = -EFAULT;
			goto errorFree;
                }
       	}

	/* Copy in all the data buffer information from userspace */
	res = CopyBuffers(command);

	if (0 != res) {
                if (-ENOMEM == res) {
        	        status = MME_NOMEM;
                } 
		goto errorFree;
	}

	/*
	 * Unless we are using MME_WaitCommand() then we require
	 * the internal Command callback to occur (mme_user_help_callback)
	 */
	if (command->CmdEnd != MME_COMMAND_END_RETURN_WAKE)
		command->CmdEnd = MME_COMMAND_END_RETURN_NOTIFY;
	
	/* Call the MME api */
	status = MME_SendCommand(handle, command);
        if (MME_SUCCESS != status) {
                goto errorFree;
        }

	/* Copy the command status structure back - contains command id, state etc */
	if (copy_to_user(&(intCommand->userCommand->CmdStatus), 
                         &(command->CmdStatus), 
			 offsetof(MME_CommandStatus_t, AdditionalInfo_p))) { /* Yuk - need to improve this */
		res = -EFAULT;
        }

	_ICS_OS_MUTEX_TAKE(&instance->ulock);

	/* Add to list of currently executing commands */
	list_add_tail(&intCommand->list, &trans->issuedCmds);

	_ICS_OS_MUTEX_RELEASE(&instance->ulock);

	/* Now call command_free which will 
	 * only free the structure when the refCount hits 0
	 * Command completion will also call it and the winner will
	 * free off the memory
	 */
	mme_user_command_free(instance, intCommand);

        goto exit;

errorFree:
	if (command->CmdStatus.AdditionalInfo_p && command->CmdStatus.AdditionalInfoSize) {
		_ICS_OS_FREE(command->CmdStatus.AdditionalInfo_p);
        }

errorFreeParam:
 	if (command->Param_p && command->ParamSize) {
		_ICS_OS_FREE(command->Param_p);
        }

errorFreeCmd:
        _ICS_OS_FREE(intCommand);

exit:
	/* Write back MME status to caller */
	if (put_user(status, &(sendCommand->status))) {
		res = -EFAULT;
	}

	return res;
}

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
