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

/* MME_AllocDataBuffer()
 * Allocate a data buffer that is optimal for the transformer instantiation
 * to pass between a host and companion
 */
MME_ERROR MME_AllocDataBuffer (MME_TransformerHandle_t handle, MME_UINT size,
			       MME_AllocationFlags_t flags, MME_DataBuffer_t **dataBufferp) 
{
  MME_ERROR     res;
  ICS_MEM_FLAGS mflags;
  
  mme_transformer_t *transformer;
  mme_buffer_t      *buf;
  
  MME_AllocationFlags_t illegalFlags = ~(MME_ALLOCATION_PHYSICAL | MME_ALLOCATION_CACHED | MME_ALLOCATION_UNCACHED);
  
  if (!mme_state)
    return MME_DRIVER_NOT_INITIALIZED;

  MME_PRINTF(MME_DBG_BUFFER, "handle 0x%x size %d flags 0x%x dataBufferp %p\n",
	     handle, size, flags, dataBufferp);

  /* Validate parameters */
  if (size == 0 || dataBufferp == NULL || (flags & illegalFlags))
  {
    return MME_INVALID_ARGUMENT;
  }

  /* Disallow very large allocations (size becomes -ve) */
  if (0 > (int) size)
  {
    return MME_NOMEM;
  }

  /* Validate transformer handle */
  if (handle == 0)
  {
    return MME_INVALID_HANDLE;
  }

  /* Lookup the transformer instance (takes lock on success) */
  transformer = mme_transformer_instance(handle);
  if (transformer == NULL)
  {
    return MME_INVALID_HANDLE;
  }

  /* Don't use the transformer so drop the lock */
  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

  /* Allocate local buffer descriptor */
  _ICS_OS_ZALLOC(buf, sizeof(*buf));

  /* Fill out MME_DataBuffer_t struct */
  buf->buffer.StructSize           = sizeof(MME_DataBuffer_t);
  buf->buffer.NumberOfScatterPages = 1;
  buf->buffer.ScatterPages_p       = buf->pages;
  buf->buffer.TotalSize            = size;

  buf->flags                       = flags;
  buf->pages[0].Size               = size;

  /* DEBUG: Stash owner of buffer */
  buf->owner                       = RETURN_ADDRESS(0);

  /* Translate the MME buffer allocation flags */
  if (flags & MME_ALLOCATION_CACHED)
    mflags = ICS_CACHED;
  else if (flags & MME_ALLOCATION_UNCACHED)
    mflags = ICS_UNCACHED;
  else
    /* Set default/affinity memory allocation flags if non supplied */
    mflags = _MME_BUF_CACHE_FLAGS;
  
  MME_ASSERT(mflags);

  /* Now allocate the Companion mapped memory from the ICS heap */
  buf->pages[0].Page_p = ics_heap_alloc(mme_state->heap, size, mflags);
  if (buf->pages[0].Page_p == NULL)
  {
    MME_EPRINTF(MME_DBG_BUFFER, "Failed to allocate %d bytes mflags 0x%x from ICS heap %p\n",
		size, mflags, mme_state->heap);

    res = MME_NOMEM;
    goto error_free;
  }
  
  MME_PRINTF(MME_DBG_BUFFER, "Successfully allocated buf %p size %d Page_p %p (%s)\n",
	     buf, size, buf->pages[0].Page_p,
	     (mflags & ICS_CACHED) ? "CACHED" : "UNCACHED");
    
  /* Return MME_DataBuffer_t pointer to caller */
  *dataBufferp = &buf->buffer;

  return MME_SUCCESS;

error_free:
  _ICS_OS_FREE(buf);
  
  return res;
}


/* MME_FreeDataBuffer()
 * Free a buffer previously allocated with MME_AllocDataBuffer 
 */
MME_ERROR MME_FreeDataBuffer (MME_DataBuffer_t *buffer)
{
  mme_buffer_t *buf = (mme_buffer_t *) buffer;

  if (mme_state == NULL)
    return MME_DRIVER_NOT_INITIALIZED;
  
  if (buf == NULL)
    return MME_INVALID_ARGUMENT;
  
  if (buf->buffer.ScatterPages_p != buf->pages)
    return MME_INVALID_ARGUMENT;

  /* Free off the Companion mapped memory */
  ics_heap_free(mme_state->heap, buf->pages[0].Page_p);

  /* Free of Buf desc */
  _ICS_OS_MEMSET(buf, 0, sizeof(*buf));
  _ICS_OS_FREE(buf);

  return MME_SUCCESS;
}

  
/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
