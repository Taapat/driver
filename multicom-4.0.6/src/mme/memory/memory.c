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
 * This function will determine the physical address of the supplied
 * buffer and it's memory caching attributes.
 * It also determines the CPU location of the target transformer so
 * that the supplied memory region can be mapped in there via ICS
 *
 * MULTITHREAD SAFE: Called holding the transformer lock
 */
static MME_ERROR 
memoryRegister (mme_transformer_t *transformer, void *base, MME_SIZE size, MME_MemoryHandle_t *handlep)
{
  MME_ERROR res = MME_ICS_ERROR;

  ICS_ERROR err;

  ICS_MEM_FLAGS mflags;
  ICS_OFFSET paddr;
  
  ICS_UINT  cpuNum;
  ICS_ULONG cpuMask;

  /* Convert supplied virtual address to a physical one 
   * and also get its cache attributes too
   */
  err = _ICS_OS_VIRT2PHYS(base, &paddr, &mflags);
  if (err != ICS_SUCCESS)
    goto error;

  /* Determine the target CPU location of this transformer */
  err = ICS_port_cpu(transformer->mgrPort, &cpuNum);
  if (err != ICS_SUCCESS)
    goto error;

  /* Generate the correct cpuMask */
  cpuMask = (1UL << cpuNum);

  err = ICS_region_add(base, paddr, size, mflags, cpuMask, handlep);
  if (err != ICS_SUCCESS)
    goto error;

  MME_PRINTF(MME_DBG_TRANSFORMER, "added region (%p,0x%x,%d,%x,0x%lx) handle=0x%x\n",
	     base, paddr, size, mflags, cpuMask, *handlep);
  
  return MME_SUCCESS;

error:
  MME_EPRINTF(MME_DBG_TRANSFORMER, "base %p size %d mflags 0x%x failed : %s (%d)\n",
	      base, size, mflags, ics_err_str(err), err);

  return res;
}

/*
 * Register a memory region against the specified transformer handle
 *
 * It will then register that region in the remote CPU where the transformer
 * is registered, hence allowing buffers from within that region to be used
 * in transformation commands
 */
MME_ERROR MME_RegisterMemory (MME_TransformerHandle_t handle,
			      void *base,
			      MME_SIZE size,
			      MME_MemoryHandle_t *handlep)
{
  MME_ERROR res = MME_INTERNAL_ERROR;

  mme_transformer_t *transformer;

  /* 
   * Sanity check the arguments
   */
  if (mme_state == NULL)
  {
    res = MME_DRIVER_NOT_INITIALIZED;
    goto error;
  }

  MME_PRINTF(MME_DBG_TRANSFORMER, "handle 0x%x base %p size %d handlep %p\n",
	     handle, base, size, handlep);
  
  if (handle == 0 || size == 0 || handlep == NULL)
  {
    res = MME_INVALID_ARGUMENT;
    goto error;
  }

  /* Lookup the transformer instance */
  transformer = mme_transformer_instance(handle);
  if (transformer == NULL)
  {
    res = MME_INVALID_HANDLE;
    goto error;
  }

  /* Do the memory region registration */
  res = memoryRegister(transformer, base, size, handlep);

  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

  return MME_SUCCESS;

error:
  MME_EPRINTF(MME_DBG_INIT, 
	      "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  return res;
}


/*
 * Remove a previously registered memory region
 */
MME_ERROR MME_DeregisterMemory (MME_MemoryHandle_t handle)
{
  ICS_ERROR err;

  /* 
   * Sanity check the arguments
   */
  if (mme_state == NULL)
  {
    return MME_DRIVER_NOT_INITIALIZED;
  }

  if (handle == 0)
    return MME_INVALID_HANDLE;
  
  err = ICS_region_remove(handle, 0 /* flags */);
  if (err != ICS_SUCCESS)
    return MME_ICS_ERROR;

  return MME_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
