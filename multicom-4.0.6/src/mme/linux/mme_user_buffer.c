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


int mme_user_alloc_buffer (mme_user_t *instance, void *arg)
{
	int res = 0;
	ICS_ERROR err;
	MME_ERROR status = MME_NOMEM;
	
	mme_alloc_buffer_t *alloc = (mme_alloc_buffer_t *) arg;

	MME_TransformerHandle_t handle;
	unsigned long           size;
	MME_AllocationFlags_t   flags;
	MME_DataBuffer_t       *mmeBuf = NULL;

	void*                   allocAddress = NULL;
	void*                   alignedBuffer;
	
	mme_user_buf_t*         allocBuf = NULL;
	int                     allocSize;
	unsigned long           mapsize;
	unsigned long           offset;
	
	ICS_MEM_FLAGS           mflags;

        if (get_user(handle, &alloc->handle)) {
		res = -EFAULT;
		goto exit;
	}
        if (get_user(size, &alloc->size)) {
		res = -EFAULT;
		goto exit;
	}
        if (get_user(flags, &alloc->flags)) {
		res = -EFAULT;
		goto exit;
	}

	/* returned buf must be page-aligned for mmap and be an exact number of pages
	 * over-allocate then round up to page boundary 
	 */
	allocSize = (size <= _ICS_OS_PAGESIZE) ? (2*_ICS_OS_PAGESIZE) : (2*_ICS_OS_PAGESIZE + size);
	
	/* Allocate a buffer descriptor */
	allocBuf = _ICS_OS_MALLOC(sizeof(*allocBuf));
	if (allocBuf == NULL)
	{
		MME_EPRINTF(MME_DBG_BUFFER, "Failed to allocate buf desc size %d\n", sizeof(*allocBuf));
		res =  -ENOMEM;
		goto exit;
	}

	/* Allocate the MME DataBuffer (using page-aligned size) */
	status = MME_AllocDataBuffer(handle, allocSize, flags, &mmeBuf);

	if (status != MME_SUCCESS) {
		MME_EPRINTF(MME_DBG_BUFFER, "Failed to allocate MME DataBuffer of size %d : %s (%d)\n",
			    allocSize,
			    MME_ErrorStr(status), status);
		res =  -ENOMEM;
		goto exit;
	}

	/* Assume we get a single contiguous buffer */
	MME_ASSERT(mmeBuf->NumberOfScatterPages == 1);
	allocAddress = mmeBuf->ScatterPages_p[0].Page_p;

	if (0 == ((unsigned long)allocAddress & (_ICS_OS_PAGESIZE-1))) {
		/* Already aligned */
		alignedBuffer = allocAddress;
	        mapsize = allocSize;
	} else {
		/* Round up to next page */
		alignedBuffer = (void*)(PAGE_SIZE + ((unsigned long)allocAddress & ~(_ICS_OS_PAGESIZE-1)));
		mapsize = allocSize - ((unsigned long)alignedBuffer - (unsigned long)allocAddress);
        }

        /* And make the mapping size a multiple of page size - the nasty stuff below
           should boil down to bit shifting!
         */
	mapsize = (mapsize / _ICS_OS_PAGESIZE) * _ICS_OS_PAGESIZE;

	MME_PRINTF(MME_DBG_BUFFER, "Allocated MME data buffer %p flags 0x%x\n", 
		   alignedBuffer, flags);

	/* Convert the buffer address to a physical one 
	 * Also determines the memory caching flags
	 */
	err = ICS_region_virt2phys(alignedBuffer, &offset, &mflags);
	if (err != ICS_SUCCESS)
	{
		res = -ENOMEM;
		goto exit;
	}

	MME_PRINTF(MME_DBG_BUFFER, "Converted MME data addr %p, flags 0x%x to offset 0x%08x\n", 
		   alignedBuffer, flags, offset);

	/* Pass back info to caller */
        if (put_user(offset,             &alloc->offset)       || 
            put_user(mapsize,            &alloc->mapSize)) {
		res = -EFAULT;
		goto exit;
	}

	MME_PRINTF(MME_DBG_BUFFER, "offset 0x%08x, mapsize 0x%08x\n",
		   (int)offset, (int)mapsize);

	/* Stash the allocated buffer info and save it on a linked list */
	allocBuf->offset       = offset;
	allocBuf->mmeBuf       = mmeBuf;
	allocBuf->size         = mapsize;
	allocBuf->mflags       = mflags;

	INIT_LIST_HEAD(&allocBuf->list);
	list_add(&allocBuf->list, &instance->allocatedBufs);

	status = MME_SUCCESS;
	
exit:
	if (res) {
		/* Failure cases */
		if (mmeBuf) {
			MME_FreeDataBuffer(mmeBuf);
		}
		
		if (allocBuf)
			_ICS_OS_FREE(allocBuf);
	}

	/* Update caller's status word */
	if (put_user(status, &alloc->status))
		res = -EFAULT;

	return res;
}

static __inline__
void freeBuffer (mme_user_buf_t *buf)
{
	MME_PRINTF(MME_DBG_BUFFER, "freeing buf %p offset %lx mmeBuf %p\n",
		   buf, buf->offset, buf->mmeBuf);

	/* Remove from the allocated buffer list */
	list_del_init(&buf->list);

	/* Free off the MME DataBuffer */
	MME_FreeDataBuffer(buf->mmeBuf);
	
	/* Free buf desc */
	_ICS_OS_FREE(buf);
}

int mme_user_free_buffer (mme_user_t *instance, void *arg)
{
	int res = 0;
	MME_ERROR status = MME_SUCCESS;

	mme_free_buffer_t *free = (mme_free_buffer_t *)arg;

	mme_user_buf_t    *buf, *tmp;
	
	unsigned long 	   offset;

        if (get_user(offset, &free->offset)) {
		res = -EFAULT;
		goto exit;
	}

	MME_PRINTF(MME_DBG_BUFFER, "offset %lx\n", offset);

	_ICS_OS_MUTEX_TAKE(&instance->ulock);
	
	/* Find the specified buffer on the allocated list */
	list_for_each_entry_safe(buf, tmp, &instance->allocatedBufs, list)
	{
		if (buf->offset == offset)
		{
			/* Unlink & free off the local and MME data buffers */
			freeBuffer(buf);

			break;
		}
	}

	_ICS_OS_MUTEX_RELEASE(&instance->ulock);
	
	/* Buffer not found */
	if (buf == NULL) {
		res = -EINVAL;
		status = MME_INVALID_HANDLE;
	}

exit:	
	/* Pass back status to caller */
	if (put_user(status, &(free->status))) {
		res = -EFAULT;
	}

        return res;
}

/* Called as the user process terminates */
void mme_user_buffer_release (mme_user_t *instance)
{
	mme_user_buf_t    *buf, *tmp;

	_ICS_OS_MUTEX_TAKE(&instance->ulock);
	
	/* Find the specified buffer on the allocated list */
	list_for_each_entry_safe(buf, tmp, &instance->allocatedBufs, list)
	{
		/* Unlink & free off the local and MME data buffers */
		freeBuffer(buf);
	}

	_ICS_OS_MUTEX_RELEASE(&instance->ulock);
}

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */

