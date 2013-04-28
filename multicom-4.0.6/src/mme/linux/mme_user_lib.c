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

/*
 * Linux Userspace MME library
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#include <ics.h>

#include <mme.h>
#include <mme/mme_ioctl.h>

/* Selected internal MME header files */
#include "_mme_handle.h"
#include "_mme_limits.h"
#include "_mme_debug.h"

#include "_ics_os.h"		/* OS wrappers */
#include "_ics_util.h"		/* ICS misc macros */
#include "_ics_debug.h"

#define MME_DEV_NAME	"/dev/mme"

/* Need to emulate this for the _mme_debug.h macros */
#define ics_debug_printf(FMT, FN, LINE, ...)	do { _ICS_OS_PRINTF("%04d:%s:%d ", getpid(), FN, LINE); \
				       		     _ICS_OS_PRINTF(FMT, ## __VA_ARGS__); } while (0)

#define DEVICE_CLOSED ((int)-1)

static int deviceFd = DEVICE_CLOSED;

static _ICS_OS_MUTEX apiLock;

/* Internal transformer instantiation desc */
typedef struct {
	MME_TransformerHandle_t handle;
	MME_GenericCallback_t	callback;
	void*			callbackUserData;

	_ICS_OS_TASK_INFO	task;
} transformer_t;

/* Internal MME_DataBuffer_t allocation desc */
typedef struct buffer {
	MME_DataBuffer_t      buffer;  		/* Embbeded MME_DataBuffer_t */
	void*                 virtAddress;	/* The virtual address mapped to physAddress */
	unsigned long         physAddress;	/* The page aligned phys address */
	unsigned              mapSize;		/* The size of the virt mapping in bytes */
	MME_ScatterPage_t     pages[1];		/* Scatter page array for MME_DataBuffer_t */
} buffer_t;

/* Instantiated transformers */
static transformer_t insTrans[_MME_TRANSFORMER_INSTANCES];

MME_ERROR MME_Init (void)
{
  int res;
  MME_ERROR status;
  mme_userinit_t usermmeinit;

  if (deviceFd == DEVICE_CLOSED) {
    deviceFd = open(MME_DEV_NAME, O_RDWR);
    if (deviceFd < 0) {
      MME_EPRINTF(MME_DBG_INIT, "Failed to open /dev/mme - errno %d\n", errno);
      status =  MME_DRIVER_NOT_INITIALIZED;
      goto exit;
    }
	
    if (!_ICS_OS_MUTEX_INIT(&apiLock)) {
      status = MME_NOMEM;
      goto exit;
    } 
  }
  
	 /* Now init mme */
	 res = ioctl(deviceFd, MME_IOC_MMEINIT, &usermmeinit);
	 status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : usermmeinit.status;

exit:
 	return status;
}

MME_ERROR MME_Term (void)
{
  int res;
	 int idx;
  mme_userterm_t usermmeterm;

        if (deviceFd == DEVICE_CLOSED) {
	        return MME_DRIVER_NOT_INITIALIZED;
	}

	/* Check for any currently instantiated transformers */
	for (idx = 0; idx < _MME_TRANSFORMER_INSTANCES; idx++)
		if (insTrans[idx].handle != 0)
			return MME_HANDLES_STILL_OPEN;


	  res = ioctl(deviceFd, MME_IOC_MMETERM, &usermmeterm);

        res = close(deviceFd);
        if (res != 0) {
	        return MME_HANDLES_STILL_OPEN;
        }
        deviceFd = DEVICE_CLOSED;

        _ICS_OS_MUTEX_DESTROY(&apiLock);

	return MME_SUCCESS;
}

MME_ERROR MME_AbortCommand (MME_TransformerHandle_t handle, MME_CommandId_t CmdId)
{
	int res;
	MME_ERROR status;

	mme_abort_command_t abort;

	if (deviceFd == DEVICE_CLOSED) {
	        return MME_DRIVER_NOT_INITIALIZED;
        }
	
	abort.cmdId  = CmdId;
	abort.handle = handle;
	
	res = ioctl(deviceFd, MME_IOC_ABORTCOMMAND, &abort);
	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : abort.status;
	
	return status;
}

MME_ERROR MME_SendCommand (MME_TransformerHandle_t handle, MME_Command_t * command)
{
	int res;
	MME_ERROR status = MME_SUCCESS;

	mme_send_command_t sendCommand;

	if (deviceFd == DEVICE_CLOSED) {
		MME_EPRINTF(MME_DBG_COMMAND, "MME Device %s not open\n", "/dev/mme");
	        return MME_DRIVER_NOT_INITIALIZED;
        }

        if (0 == handle) {
		MME_EPRINTF(MME_DBG_COMMAND, "Invalid handle 0x%x\n", handle);
                return MME_INVALID_HANDLE;
        }

        if (NULL == command ||
            command->StructSize != sizeof(MME_Command_t)) {
		MME_EPRINTF(MME_DBG_COMMAND, "Invalid command %p\n", command);
                return MME_INVALID_ARGUMENT;
        }

        if (command->CmdCode != MME_SET_GLOBAL_TRANSFORM_PARAMS &&
            command->CmdCode != MME_TRANSFORM &&
            command->CmdCode != MME_SEND_BUFFERS) {
		MME_EPRINTF(MME_DBG_COMMAND, "Invalid command code %d\n", command->CmdCode);
                return MME_INVALID_ARGUMENT;
        }

        if (command->CmdEnd > MME_COMMAND_END_RETURN_WAKE) {
		MME_EPRINTF(MME_DBG_COMMAND, "Invalid command notify type %d\n", command->CmdEnd);
                return MME_INVALID_ARGUMENT;
        }
	
	sendCommand.handle  = handle;
	sendCommand.command = command;
	
	res = ioctl(deviceFd, MME_IOC_SENDCOMMAND, &sendCommand);
	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : sendCommand.status;

	if (0 != res)
		MME_EPRINTF(MME_DBG_BUFFER, "ioctl error - driver code %d, status %s (%d)\n",
			    res, MME_ErrorStr(status), status);

	return status;
}

MME_ERROR MME_WaitCommand (MME_TransformerHandle_t handle,
			   MME_CommandId_t cmdId,
			   MME_Event_t *eventp,
			   MME_Time_t timeout)
{
	int res;
	MME_ERROR status = MME_SUCCESS;

	mme_wait_command_t waitCommand;

	if (deviceFd == DEVICE_CLOSED) {
		MME_EPRINTF(MME_DBG_COMMAND, "%s device not open\n", MME_DEV_NAME);
	        return MME_DRIVER_NOT_INITIALIZED;
        }

        if (0 == handle) {
		MME_EPRINTF(MME_DBG_COMMAND, "Invalid handle 0x%x\n", handle);
                return MME_INVALID_HANDLE;
        }

	if (eventp == NULL) {
		MME_EPRINTF(MME_DBG_COMMAND, "Invalid eventp %p\n", eventp);
                return MME_INVALID_ARGUMENT;
        }

	waitCommand.handle  = handle;
	waitCommand.cmdId   = cmdId;
	waitCommand.timeout = timeout;

	res = ioctl(deviceFd, MME_IOC_WAITCOMMAND, &waitCommand);

	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : waitCommand.status;

	if (0 != res) {
		MME_EPRINTF(MME_DBG_BUFFER, "ioctl error - driver code %d, status %s (%d)\n",
			    res, MME_ErrorStr(status), status);
	}
	else
		*eventp = waitCommand.event;
		
	return status;
}

/* Per transformer helper task
 * Calls an ioctl() into the MME device which blocks until
 * there is a command completion event
 *
 * Call the relevant callback function when this occurs
 */
static
void helperTask (void *context)
{
	transformer_t *trans = (transformer_t *)context;
	
	MME_GenericCallback_t   callback = trans->callback;
	void *		        userData = trans->callbackUserData;

	int res = 0;

	MME_PRINTF(MME_DBG_TRANSFORMER, "helper task start trans %p handle 0x%x callback %p\n",
		   trans, trans->handle, callback);

	MME_ASSERT(trans->handle != 0);

        while (1) {

		mme_help_task_t help;

		help.handle = trans->handle;

		/* Wait for an error or for a callback request (retrying on signal delivery) */
                do {
                	res = ioctl(deviceFd, MME_IOC_HELPTASK, &help);
                } while (res != 0 && errno == EINTR);

                if (res != 0)
	                break;
		
		MME_PRINTF(MME_DBG_COMMAND, "Wakeup command %p State %d Error %d CmdEnd %d\n",
			   help.command,
			   help.command->CmdStatus.State,
			   help.command->CmdStatus.Error,
			   help.command->CmdEnd);
		
                /* MME_SUCCESS - do callback */
                if (MME_COMMAND_END_RETURN_NOTIFY == help.command->CmdEnd) {
 		        MME_PRINTF(MME_DBG_COMMAND,
				   "Callback %p event %d command 0x%08x\n",
				   callback, help.event, (int) help.command);
			(callback)(help.event, help.command, userData);
		}
        }
	
	MME_PRINTF(MME_DBG_TRANSFORMER, "helper task trans %p exit res %d errno %d\n", trans, res, errno);
}

MME_ERROR MME_InitTransformer (const char *name,
        		       MME_TransformerInitParams_t *params,
			       MME_TransformerHandle_t     *handle)
{ 
	int res;
	ICS_ERROR err;
	MME_ERROR status;
	
	mme_init_transformer_t  init;
	
	int type, cpuNum, ver, idx;
	transformer_t *trans;

	if (deviceFd == DEVICE_CLOSED) {
		MME_EPRINTF(MME_DBG_COMMAND, "%s device not open\n", MME_DEV_NAME);
	        return MME_DRIVER_NOT_INITIALIZED;
        }

        if (NULL == name || NULL == params || NULL == handle ||
            params->StructSize != sizeof(MME_TransformerInitParams_t) ||
	    strlen(name) > MME_MAX_TRANSFORMER_NAME) {
                return MME_INVALID_ARGUMENT;
        }

	init.name       = name;
	init.nameLength = strlen(name);
	init.params     = *params;
	init.handle     = 0;
	init.status     = MME_INTERNAL_ERROR;
	
	/* Now do the initialization */
	res = ioctl(deviceFd, MME_IOC_INITTRANSFORMER, &init);

	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : init.status;

	if (0 != res || MME_SUCCESS != status) {
		MME_EPRINTF(MME_DBG_TRANSFORMER, "ioctl error - driver code %d, status %d \n", 
			    res, status);
		goto error;
	}

	MME_PRINTF(MME_DBG_TRANSFORMER, "instantiated transformer handle 0x%x\n", init.handle);

	/* Decode the allocated transformer handle */
	_MME_DECODE_HDL(init.handle, type, cpuNum, ver, idx);
	
	/* Get a handle into the local transformer table */
	trans = &insTrans[idx];
	
	MME_ASSERT(trans->handle == 0);
	
	/* Fill out the local instantiation table */
	trans->handle = init.handle;
	trans->callback = params->Callback;
	trans->callbackUserData = params->CallbackUserData;
	
	/* Create the transformer helper task */
	{
		MME_PRINTF(MME_DBG_TRANSFORMER, "creating helper task trans %p callback %p\n",
			   trans, trans->callback);
		
		/* Create the helper task (passing it the trans pointer) */
		err = _ICS_OS_TASK_CREATE(helperTask, trans, 0, "helper", &trans->task);
		if (err != ICS_SUCCESS) {
			status = MME_NOMEM;
			goto error_term;
		}
	}

	/* Pass back handle to caller */
	*handle = init.handle;
	
	return MME_SUCCESS;

error_term:
	trans->handle = 0;

	(void) MME_TermTransformer(init.handle);

error:
	return status;
}

MME_ERROR MME_TermTransformer (MME_TransformerHandle_t handle)
{
	int res;
	MME_ERROR status;

	mme_term_transformer_t term;

	int type, cpuNum, ver, idx;
	transformer_t *trans;
	
	if (deviceFd == DEVICE_CLOSED) {
		MME_EPRINTF(MME_DBG_COMMAND, "%s device not open\n", MME_DEV_NAME);
	        return MME_DRIVER_NOT_INITIALIZED;
        }

	MME_PRINTF(MME_DBG_TRANSFORMER, "Terminating transformer 0x%x\n", handle);

	if (handle == 0 || _MME_HDL2TYPE(handle) != _MME_TYPE_TRANSFORMER)
		return MME_INVALID_HANDLE;

	/* First terminate the MME kernel transformer */
	term.handle = handle;

	res = ioctl(deviceFd, MME_IOC_TERMTRANSFORMER, &term);
	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : term.status;

	if (0 != res || status != MME_SUCCESS) {
		goto exit;
	}

	/* Decode transformer handle */
	_MME_DECODE_HDL(handle, type, cpuNum, ver, idx);
	
	/* Get a handle into the local transformer table */
	trans = &insTrans[idx];
	
	MME_ASSERT(trans->handle == handle);
	
	/* Terminate the transformer helper task (if there is one) */
	if (trans->callback) {
		MME_PRINTF(MME_DBG_TRANSFORMER, "Waiting for trans %p helper task exit\n", trans);
		
		/* Wait for the task to terminate */
		_ICS_OS_TASK_DESTROY(&trans->task);
	}
	
	/* Clear down the local instantiation */
	trans->handle = 0;

exit:
	MME_PRINTF(MME_DBG_TRANSFORMER, "Terminated transformer 0x%x status %d\n", handle, status);

	return status;
}

MME_ERROR MME_GetTransformerCapability (const char *name, MME_TransformerCapability_t * capability)
{
	int res;
	MME_ERROR status;

	mme_get_capability_t cap;

	if (deviceFd == DEVICE_CLOSED) {
		MME_EPRINTF(MME_DBG_COMMAND, "%s device not open\n", MME_DEV_NAME);
	        return MME_DRIVER_NOT_INITIALIZED;
        }

        if (NULL == name || NULL == capability ||
            capability->StructSize != sizeof(MME_TransformerCapability_t) ||
	    strlen(name) > MME_MAX_TRANSFORMER_NAME) {
		return MME_INVALID_ARGUMENT;
        }

	cap.name = (char*) name;
	cap.length = strlen(name);
	cap.capability = capability;

	res = ioctl(deviceFd, MME_IOC_GETCAPABILITY, &cap);
	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : cap.status;

	return status;
}

MME_ERROR MME_AllocDataBuffer ( MME_TransformerHandle_t handle,
				MME_UINT size,
				MME_AllocationFlags_t flags,
				MME_DataBuffer_t ** dataBuffer_p)
{
	int res;
	MME_ERROR status;
	
	buffer_t *buf;
	void* pageBuffer;

	static const MME_AllocationFlags_t illegalFlags = (MME_AllocationFlags_t)
		~(MME_ALLOCATION_PHYSICAL | MME_ALLOCATION_CACHED | MME_ALLOCATION_UNCACHED);

	mme_alloc_buffer_t allocBuf;
	
	if (deviceFd == DEVICE_CLOSED) {
		MME_EPRINTF(MME_DBG_COMMAND, "%s device not open\n", MME_DEV_NAME);
	        return MME_DRIVER_NOT_INITIALIZED;
        }

	if (0 == size || NULL == dataBuffer_p || (flags & illegalFlags)) {
		return MME_INVALID_ARGUMENT;
	}

	if ((int) size < 0) {
		return MME_NOMEM;
	}

	if (handle == 0 || _MME_HDL2TYPE(handle) != _MME_TYPE_TRANSFORMER)
		return MME_INVALID_HANDLE;
	
	/* The following code gets the driver to allocate page-aligned memory
	 * This is mapped to user space with mmap(). 
	 */

	/* Allocate the internal buffer structure */
	buf = _ICS_OS_MALLOC(sizeof(*buf));
	if (NULL == buf) {
		return MME_NOMEM;
	}
	memset(buf, 0, sizeof(*buf));
	
	allocBuf.handle = handle;
	allocBuf.size = size;
	allocBuf.flags = flags;

	res = ioctl(deviceFd, MME_IOC_ALLOCBUFFER, &allocBuf);

	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : allocBuf.status;

	if (0 != res || status != MME_SUCCESS) {
		MME_EPRINTF(MME_DBG_BUFFER, "ioctl error - driver code %d, status %s (%d)\n",
			    res, MME_ErrorStr(allocBuf.status), allocBuf.status);

		_ICS_OS_FREE(buf);

		return status;
	}
	
	MME_PRINTF(MME_DBG_BUFFER,
		   "Calling mmap(0, size 0x%08x, prot READ|WRITE, PRIVATE, device %d, offset 0x%08x)\n",
		   (int)allocBuf.mapSize, deviceFd, (int)allocBuf.offset);
	
	/* Map the allocated memory into user space */
	pageBuffer = mmap(0, allocBuf.mapSize, PROT_READ | PROT_WRITE, MAP_SHARED,
			  deviceFd, allocBuf.offset);

	if (MAP_FAILED == pageBuffer) {
		/* TODO undo allocation */
		MME_EPRINTF(MME_DBG_BUFFER, "mmap failed - errno  %d\n", errno);
		*dataBuffer_p = NULL;

		_ICS_OS_FREE(buf);

		return MME_NOMEM;
	}
	
	/* Fill out internal buffer desc */
	buf->virtAddress = pageBuffer;			/* The virtual address assigned */
	buf->physAddress = allocBuf.offset;		/* The aligned physical address */
	buf->mapSize = allocBuf.mapSize;		/* Bytes mapped */
	buf->pages[0].Size = size;			/* Scatter page array */
	buf->pages[0].Page_p = pageBuffer;

	/* Fill out the MME_DataBuffer_t */
	buf->buffer.StructSize = sizeof(MME_DataBuffer_t);
	buf->buffer.NumberOfScatterPages = 1;
	buf->buffer.ScatterPages_p = &buf->pages[0];
	buf->buffer.TotalSize = size;
	*dataBuffer_p = &(buf->buffer);
	
	MME_PRINTF(MME_DBG_BUFFER, "Data buffer at 0x%08x\n", (int) (*dataBuffer_p));

	return MME_SUCCESS;
}

MME_ERROR MME_FreeDataBuffer (MME_DataBuffer_t * DataBuffer_p)
{
	int res;
	MME_ERROR status;
	buffer_t* buf = (buffer_t *)DataBuffer_p;

	mme_free_buffer_t freeBuf;

	if (deviceFd == DEVICE_CLOSED) {
		MME_EPRINTF(MME_DBG_COMMAND, "%s device not open\n", MME_DEV_NAME);
	        return MME_DRIVER_NOT_INITIALIZED;
        }

	if (NULL == DataBuffer_p)
		return MME_INVALID_ARGUMENT;

	if (buf->virtAddress == NULL || buf->physAddress == 0 || buf->mapSize < _ICS_OS_PAGESIZE) {
		return MME_INVALID_HANDLE;
	}

	/* First unmap the corresponding physical memory */
	if (munmap(buf->virtAddress, buf->mapSize)) {
		status = MME_INVALID_ARGUMENT;
		goto exit;
	}

	/* Identify the MME_DataBuffer_t by its physical address */
	freeBuf.offset = buf->physAddress;

	/* Now free the correspondig kernel MME_DataBuffer_t */
	res = ioctl(deviceFd, MME_IOC_FREEBUFFER, &freeBuf);
	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : freeBuf.status;
	
	if (0 != res || status != MME_SUCCESS) {
		MME_EPRINTF(MME_DBG_BUFFER,
			    "ioctl error - driver code %d, status %d\n", 
			    res, freeBuf.status);
		goto exit;
	}
	
	_ICS_OS_FREE(buf);

exit:
	return status;
}

MME_ERROR MME_RegisterMemory (MME_TransformerHandle_t handle,
			      void *base,
			      MME_SIZE size,
			      MME_MemoryHandle_t *handlep)
{
	/* XXXX Do we need to implement this ? */
	return MME_NOT_IMPLEMENTED;
}

MME_ERROR MME_DeregisterMemory (MME_MemoryHandle_t handle)
{
	/* XXXX Do we need to implement this ? */
	return MME_NOT_IMPLEMENTED;
}


const char *MME_Version (void)
{
	return __MULTICOM_VERSION__;
}

MME_ERROR MME_ModifyTuneable (MME_Tuneable_t key, MME_UINT value)
{
	 int res;
	 MME_ERROR status;
  mme_tuneables_t tuneable;

  if (deviceFd == DEVICE_CLOSED) {
    deviceFd = open(MME_DEV_NAME, O_RDWR);
    if (deviceFd < 0) {
      MME_EPRINTF(MME_DBG_INIT, "Failed to open /dev/mme - errno %d\n", errno);
      status =  MME_DRIVER_NOT_INITIALIZED;
      goto exit;
    }
	
    if (!_ICS_OS_MUTEX_INIT(&apiLock)) {
      status = MME_NOMEM;
      goto exit;
    } 
  }

	/* Fill out tuneable stucture */
	tuneable.key   =  key;
	tuneable.value =  value;

	/* Set tuneable value */
	res = ioctl(deviceFd,MME_IOC_SETTUNEABLE, &tuneable);
	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : tuneable.status;
exit:
	return status;
}

MME_UINT MME_GetTuneable (MME_Tuneable_t key)
{
	 int res;
	 MME_ERROR status;
  mme_tuneables_t tuneable;

  if (deviceFd == DEVICE_CLOSED) {
    deviceFd = open(MME_DEV_NAME, O_RDWR);
    if (deviceFd < 0) {
      MME_EPRINTF(MME_DBG_INIT, "Failed to open /dev/mme - errno %d\n", errno);
      status =  MME_DRIVER_NOT_INITIALIZED;
      goto exit;
    }
	
    if (!_ICS_OS_MUTEX_INIT(&apiLock)) {
      status = MME_NOMEM;
      goto exit;
    } 
  }

 	/* Fill out tuneable stucture */
	 tuneable.key   =  key;

	 /* Get tuneable value */
	 res = ioctl(deviceFd,MME_IOC_GETTUNEABLE, &tuneable);
	 tuneable.value = (res == -EFAULT) ? MME_INVALID_ARGUMENT : tuneable.value;
 	return tuneable.value;

exit:
  return status;
}

MME_ERROR MME_RegisterTransformer (const char *name,
				   MME_AbortCommand_t abortFunc,
				   MME_GetTransformerCapability_t getTransformerCapabilityFunc,
				   MME_InitTransformer_t initTransformerFunc,
				   MME_ProcessCommand_t processCommandFunc,
				   MME_TermTransformer_t termTransformerFunc)
{
	/* Local transformers are not implemented in MME4 */
	return MME_NOT_IMPLEMENTED;
}

MME_ERROR MME_IsTransformerRegistered (const char *name)
{
	 int res;
	 MME_ERROR status;
  mme_is_transformer_reg_t regtfr;


	 if (deviceFd == DEVICE_CLOSED) {
		MME_EPRINTF(MME_DBG_COMMAND, "%s device not open\n", MME_DEV_NAME);
	        return MME_DRIVER_NOT_INITIALIZED;
        }
  if (NULL == name ) 
		return MME_INVALID_ARGUMENT;

	regtfr.name = (char*) name;
	regtfr.length = strlen(name);

	res = ioctl(deviceFd,MME_IOC_ISTFRREG, &regtfr);
	status = (res == -EFAULT) ? MME_INVALID_ARGUMENT : regtfr.status;

	return status;

}

MME_ERROR MME_DeregisterTransformer (const char *name)
{
	/* Local transformers are not implemented in MME4 */
	return MME_NOT_IMPLEMENTED;
}

MME_ERROR MME_NotifyHost (MME_Event_t event, MME_Command_t *command, MME_ERROR error)
{
	if (deviceFd == DEVICE_CLOSED) {
		MME_EPRINTF(MME_DBG_COMMAND, "%s device not open\n", MME_DEV_NAME);
	        return MME_DRIVER_NOT_INITIALIZED;
        }

	/* Local transformers are not implemented in MME4 */
	return MME_NOT_IMPLEMENTED;
}

MME_ERROR MME_Run (void)
{
	/* Local transformers are not implemented in MME4 */
	return MME_NOT_IMPLEMENTED;
}

MME_ERROR MME_RegisterTransport (const char *name)
{
	return MME_NOT_IMPLEMENTED;
}

MME_ERROR MME_DeregisterTransport (const char *name)
{
	return MME_NOT_IMPLEMENTED;
}


/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
