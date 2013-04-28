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

/**************************************************************
 *
 * File: ics_region.c
 *
 * Description
 *    Routines to manage the ICS regions and address 
 *    translations
 *
 **************************************************************/

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

/* 
 * Region management code
 *
 * A table of memory regions is maintained for performing
 * address translation between physical and virtual addresses
 *
 * When a user adds a region it is added to the table and also
 * communicated to all the connected CPUs so they can then perform
 * the physical to virtual translation of message buffer addresses
 * sent by this CPU.
 *
 * The same table is used to hold local & remote map requests to provide
 * as single address translation interface. Remote mappings are 
 * distinguished by the cpuNum stored in the region desc
 */

/* This structre is used to remove regions from ics_region_cpu_down*/
static  struct
{
   ICS_OFFSET base; 
   ICS_SIZE size;
   ICS_MEM_FLAGS mflags;
} rgns[_ICS_MAX_REGIONS];

/*
 * Find a free region desc
 *
 * Returns NULL if non found
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static
ics_region_t *findFreeRegion (void)
{
  int idx;

  ICS_assert(ics_state);

  /* XXXX Should this be a dynamic table ? */
  for (idx = 0; idx < _ICS_MAX_REGIONS; idx++)
  {
    if (ics_state->rgn[idx].map == NULL)
      return &ics_state->rgn[idx];
  }

  /* No free slot found */
  return NULL;
}

/*
 * Free off a region desc
 *
 * MULTITHREAD SAFE: Called holding the state lock from task ctxt
 * The table is also searched/read from IRQ context to make that
 * safe we rely on write ordering and set the size to zero first
 */
static
void freeRegion (ics_region_t *rgn)
{
  ICS_assert(rgn->refCount == 0);

  /* Mark a region as being free */
  rgn->size = 0;

  rgn->map  = rgn->umap = NULL;
  rgn->base = 0;
  
  /* Increment (and wrap) the incarnation version for future allocations */
  rgn->version = (++rgn->version & _ICS_HANDLE_VER_MASK);
  /* Generate the new handle */
  rgn->handle = _ICS_HANDLE(_ICS_TYPE_REGION, ics_state->cpuNum, rgn->version, rgn->handle);

  return;
}

/*
 * Look for a matching region for a given physical address range
 * Returns a region address if found, NULL otherwise
 *
 * MULTITHREAD SAFE: Called holding the state lock from task ctxt
 * The table is also searched/read from IRQ context to make that
 * safe we rely on write ordering and set the size to zero first
 */
static
ics_region_t *lookupRegion (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags)
{
  int idx;

  /* XXXX Should this be a binary tree or an ordered list ? */
  for (idx = 0; idx < _ICS_MAX_REGIONS; idx++)
  {
    ics_region_t *rgn;
    
    rgn = &ics_state->rgn[idx];

    /* Test for an invalidated slot first */
    if (rgn->size == 0)
      continue;

    /* Find a matching region including the cache flags */
    if ((mflags == rgn->mflags) &&
	(paddr >= rgn->base) && paddr < (rgn->base + rgn->size) &&
	(paddr + size) <= (rgn->base + rgn->size))
    {
      /* Found a matching region which overlaps all of range (plus the same flags) */
      return rgn;
    }
  }

  return NULL;
}

/*  
 *  Look for a matching region for a given physical address range
 *  Returns a region address if found, NULL otherwise
 *  
 *  MULTITHREAD SAFE: Called holding the state lock from task ctxt
 *  The table is also searched/read from IRQ context to make that
 *  safe we rely on write ordering and set the size to zero first
 */
static
ics_region_t *lookupRemoteRegion (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags)
{   
  int idx;
  
  /* XXXX Should this be a binary tree or an ordered list ? */
  for (idx = 0; idx < _ICS_MAX_REGIONS; idx++)
  {
    ics_region_t *rgn;
     
    rgn = &ics_state->rgn[idx];
    
    /* Test for an invalidated slot first */
    if (rgn->size == 0)
      continue;

    /* Find a matching region including the cache flags */
    if ((mflags == rgn->mflags) &&
        (paddr == rgn->base)  &&
        (paddr + size) <= (rgn->base + rgn->size))
    {
      /* Found a matching region which overlaps all of range (plus the same flags) */
      return rgn;
    }
  }

  return NULL;
}

/*
 * Look for a matching region for a given virtual addres
 * Returns a region address if found, NULL otherwise
 *
 * NB: Lookup is performed without holding the state lock
 */
static
ics_region_t *lookupAddr (ICS_VOID *addr)
{
  int idx;

  /* XXXX Should this be a binary tree or an ordered list ? */
  for (idx = 0; idx < _ICS_MAX_REGIONS; idx++)
  {
    ics_region_t *rgn;
    
    rgn = &ics_state->rgn[idx];

    /* Test for an invalidated slot first */
    if (rgn->size == 0)
      continue;
    
    if (((ICS_OFFSET)addr >= (ICS_OFFSET)rgn->map && (ICS_OFFSET)addr < ((ICS_OFFSET)rgn->map + rgn->size)))
    {
      /* Found a matching mapped region */
      return rgn;
    }
  }
  
  return NULL;
}


/* 
 * Add a new region to the region table. Returns the newly created region desc
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static 
ics_region_t *addRegion (ICS_VOID *map, ICS_OFFSET paddr, ICS_SIZE size,
			 ICS_MEM_FLAGS mflags, ICS_UINT cpuNum, ICS_ULONG cpuMask)
{
  ics_region_t *rgn;

  ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "map %p paddr 0x%x size %d mflags 0x%x cpuNum %d\n",
	     map, paddr, size, mflags, cpuNum);

  /* Grab a new region desc */
  rgn = findFreeRegion();
  if (rgn == NULL)
  {
    /* No free regions */
    goto error;
  }

  if (map == NULL)
  {
    /* XXXX Need to translate all flag bits down into ICS/OS equivalents */
    ICS_BOOL cached = ((mflags & ICS_CACHED) ? ICS_TRUE : ICS_FALSE);

    /* Map in [UN]CACHED virtual memory corresponding to the physical range */
    rgn->map = _ICS_OS_MMAP(paddr, size, cached);
    if (rgn->map == NULL)
    {
      /* Memory map failed */
      goto error_free;
    }

    ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT,
	       "mapped paddr 0x%x @ %p %s\n",
	       paddr, rgn->map, (cached ? "(CACHED)" : "(UNCACHED)"));
    
    /* Purge any old cache mappings */
    _ICS_OS_CACHE_PURGE(rgn->map, paddr, size);
  }
  else
    rgn->map = map;

  /* Stash the supplied user map address so we can
   * distinguish the case where we created the mapping
   * i.e. rgn->umap == NULL
   */
  rgn->umap     = map;
  rgn->cpuNum   = cpuNum;
  rgn->cpuMask  = cpuMask;	/* Will be 0 for remote map requests */

  /* Fill out the new region info */
  rgn->base     = paddr;
  rgn->mflags   = mflags;

  /*
   * The table is also searched/read from IRQ context to make that
   * safe we rely on write ordering and set the size to non-zero last
   */
  rgn->size     = size;

  rgn->refCount = 1;

  ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT,
	     "Successfully added rgn %p ver 0x%x map %p umap %p base 0x%x mflags 0x%x\n",
	     rgn, rgn->version, rgn->map, rgn->umap, rgn->base, rgn->mflags);

  /* Success - return region ptr to caller */
  return rgn;

error_free:
  freeRegion(rgn);

error:

  return NULL;
}

/*
 * Remove a region from the region table, unmapping any
 * mapping we created
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static 
ICS_ERROR removeRegion (ics_region_t *rgn)
{
  if (rgn->umap == NULL)
  {
    /* Purge the cache before unmapping */
    _ICS_OS_CACHE_PURGE(rgn->map, rgn->base, rgn->size);

    /* We created this mapping so must remove it */
    if (_ICS_OS_MUNMAP(rgn->map))
    {
      return ICS_SYSTEM_ERROR;
    }
  }

  /* Free the region entry */
  freeRegion(rgn);

  return ICS_SUCCESS;
}

/*
 * Send an admin message to all connected CPUs informing them
 * of the new local memory region we have just added or removed
 *
 * NB: Some of the functions below will grab the state lock 
 */
static 
ICS_ERROR updateCpus (ICS_OFFSET base, ICS_SIZE size, ICS_MEM_FLAGS mflags, ICS_BOOL add, ICS_ULONG cpuMask)
{
  ICS_ERROR err = ICS_SUCCESS;
  
  int       cpuNum;

  for (cpuNum = 0; cpuNum < _ICS_MAX_CPUS; cpuNum++)
  {
    /* Don't update ourself or CPUs which are not in the bitmask */
    if (cpuNum == ics_state->cpuNum || !(cpuMask & (1UL << cpuNum)))
      continue;
    
    if (ics_state->cpu[cpuNum].state == _ICS_CPU_CONNECTED)
    {
      ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT,
		 "%s base %p size %d mflags 0x%x cpuNum %d\n",
		 (add ? "add" : "remove"),
		 base, size, mflags, cpuNum);

      if (add == ICS_TRUE)
	err = ics_admin_map_region(base, size, mflags, cpuNum);
      else
	err = ics_admin_unmap_region(base, size, mflags, cpuNum);

      if (err)
	break;
    }
  }

  if (err != ICS_SUCCESS)
    ICS_EPRINTF(ICS_DBG_REGION, "Failed : %s (%d)\n",
		ics_err_str(err), err);

  return err;
}

/* Dump out all the region table entries.
 * Called when we detect an error
 */
void ics_region_dump (void)
{
  int       idx;

  for (idx = 0; idx < _ICS_MAX_REGIONS; idx++)
  {
    ics_region_t *rgn = &ics_state->rgn[idx];

    /* Skip empty region table slots */
    if (rgn->size == 0)
      continue;

    ICS_EPRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "rgn %d : base 0x%x size 0x%x map 0x%x mflags 0x%x\n",
		idx, rgn->base, rgn->size, rgn->map, rgn->umap, rgn->mflags);
    
    ICS_EPRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "rgn %d : cpuMask 0x%x cpuNum %d version %d handle 0x%x\n",
		idx, rgn->cpuMask, rgn->cpuNum, rgn->version, rgn->handle);
  }
}

/*
 * Add a region descriptor for a local memory range
 *
 * The caller supplies the physical address and size of the range to be added.
 *
 * They can also supply the virtual address mapping of the region,
 * and if a NULL mapping is provided then one is created using the supplied
 * memory cache flags
 *
 * The region info is then communication to all CPUs in the bitmask allowing
 * them to map in the same physical region
 */
ICS_ERROR ICS_region_add (ICS_VOID *map, ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags, ICS_ULONG cpuMask,
			  ICS_REGION *regionp)
{
  ICS_ERROR     err = ICS_SYSTEM_ERROR;

  ICS_UINT      validFlags = (ICS_CACHED|ICS_UNCACHED|ICS_WRITE_BUFFER);

  ics_region_t *rgn;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "map %p paddr 0x%x size %d mflags 0x%x\n",
	     map, paddr, size, mflags);

  if (mflags == 0 || (mflags & ~validFlags))
    return ICS_INVALID_ARGUMENT;
    
  if (size == 0 || paddr == ICS_BAD_OFFSET || regionp == NULL || cpuMask == 0)
    return ICS_INVALID_ARGUMENT;

  /* Always register whole pages */
  size = ALIGNUP(size, ICS_PAGE_SIZE);

  /* Has the user supplied a virtual address mapping ? 
   * If so, do some checks on it
   */
  if (map)
  {
    ICS_OFFSET    base;
    ICS_MEM_FLAGS mmflags;
  
   /* Can't find physical address on ARM uniprocessor-kernel if the memory is section
    * mapped.
    */
#if !defined (__arm__) || defined (CONFIG_SMP) 

    /* Convert supplied virtual address to physical to generate the
     * region base offset
     */
    err = _ICS_OS_VIRT2PHYS(map, &base, &mmflags);
    if (err != ICS_SUCCESS || base != paddr)
    {
      ICS_EPRINTF(ICS_DBG_REGION|ICS_DBG_INIT, 
		  "Failed to convert map %p to physical (base 0x%x paddr 0x%x) : %d\n",
		  map, base, paddr, err);
      goto error;
    }
#endif /* !defined (__arm__) || defined (CONFIG_SMP) */

#if 0
    /* XXXX Disabled this code as it can't reliably check
     * the cache attributes on newer Linux kernels and also
     * on older mmap based OS21 we can also fail due to there
     * being multiple translations of each physical address
     * (see vmem_virt_mode() in vmemadapt.c)
     */
   
    /* Check the supplied vaddr matches the supplied cache flags too */
    if (((mflags & ICS_CACHED) && (mmflags & ICS_UNCACHED)) || ((mflags & ICS_UNCACHED) && (mmflags & ICS_CACHED)))
    {
      ICS_EPRINTF(ICS_DBG_REGION|ICS_DBG_INIT,
		  "Cache flags mismatch: map %p mflags 0x%x mmflags 0x%x\n", map, mflags, mmflags);
      
      err = ICS_INVALID_ARGUMENT;
      goto error;
    }
#endif
    
    ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "%s map %p base 0x%x size %d mflags 0x%x\n",
	       ((mflags & ICS_CACHED) ? "CACHED" : "UNCACHED"), map, base, size, mflags);
  }

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Add the region (mapping in a new one if map is NULL) */
  rgn = addRegion(map, paddr, size, mflags, ics_state->cpuNum, cpuMask);
  if (rgn == NULL)
  {
    /* Failed to allocate a new region */
    err = ICS_ENOMEM;
    goto error_release;
  }

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  /* Inform all connected cpus about the newly added local region */
  err = updateCpus(rgn->base, rgn->size, rgn->mflags, ICS_TRUE /* add */, cpuMask);
  if (err != ICS_SUCCESS)
    /* XXXX should we remove the region ? */
    goto error;

  /* Return a region handle to the caller */
  *regionp = rgn->handle;
  
  ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "Successfully added region handle=0x%x\n",
	     *regionp);

  return ICS_SUCCESS;

error_release:
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

error:
  
  ICS_EPRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "Failed : %s (%d)\n",
	      ics_err_str(err), err);
  
  return err;
}

/* 
 * Remove a previously added region
 *
 * This will unmap any local mappings and also inform all
 * connected CPUs of the region's removal
 */
ICS_ERROR ICS_region_remove (ICS_REGION region, ICS_UINT flags)
{
  ICS_ERROR     err;

  ICS_UINT      validFlags = 0;

  int           type, cpuNum, ver, idx;

  ics_region_t *rgn;
  ICS_OFFSET    base;
  ICS_SIZE      size;
  ICS_MEM_FLAGS mflags;
  ICS_ULONG     cpuMask;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "region 0x%x flags 0x%x\n",
	     region, flags);

  if (flags & ~validFlags)
  {
   err = ICS_INVALID_ARGUMENT;
   goto error;
  }

  /* Decode the region handle */
  _ICS_DECODE_HDL(region, type, cpuNum, ver, idx);

  /* Check the region info */
  if (type != _ICS_TYPE_REGION || cpuNum >= _ICS_MAX_CPUS || idx >= _ICS_MAX_REGIONS)
  {
    err = ICS_HANDLE_INVALID;
    goto error;
  }

  if (cpuNum != ics_state->cpuNum)
  {
    /* Can only remove local regions */
    err = ICS_HANDLE_INVALID;
    goto error;
  }

  ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "region 0x%x -> type 0x%x cpuNum %d ver %d idx %d\n",
	     region, type, cpuNum, ver, idx);

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Get a handle onto the region desc */
  rgn = &ics_state->rgn[idx];

  ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "Found rgn %p version 0x%x base 0x%x map %p umap %p\n",
	     rgn, rgn->version, rgn->base, rgn->map, rgn->umap);

  /* Check this is the correct incarnation */
  if (rgn->version != ver)
  {
    err = ICS_HANDLE_INVALID;
    goto error_release;
  }

  ICS_assert(rgn->refCount > 0);

  if (--rgn->refCount == 0)
  {
    /* Extract region info */
    base    = rgn->base;
    size    = rgn->size;
    mflags  = rgn->mflags;
    cpuMask = rgn->cpuMask;
    
    /* Now remove the region, unmapping any mappings */
    err = removeRegion(rgn);
    ICS_assert(err == ICS_SUCCESS);

    /* Must drop this lock before calling updateCpus() */
    _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
    
    /* Inform all connected cpus that this region has been removed */
    err = updateCpus(base, size, mflags, ICS_FALSE /* remove */, cpuMask);
    /* XXXX What can be done about errors here ? */
    ICS_assert(err == ICS_SUCCESS || err == ICS_SYSTEM_TIMEOUT);
  }
  else
    _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return ICS_SUCCESS;

error_release:
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

error:
  ICS_EPRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "Failed : %s (%d)\n",
	      ics_err_str(err), err);
    
  return err;
}

/*
 * Map in a new virtual memory region for the supplied physical address range
 * This is called by the admin task in response to remote map requests.
 */
ICS_ERROR ics_region_map (ICS_UINT cpuNum, ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags, ICS_VOID **map)
{
  ICS_ERROR err;

  ics_region_t *rgn;

  ICS_ASSERT(ics_state);
  ICS_ASSERT(size);
  ICS_ASSERT(map);

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Lookup region in our local region table */
  rgn = lookupRemoteRegion(paddr, size, mflags);
  if (rgn != NULL && (rgn->cpuNum == cpuNum)) 
  {
    /* Region matches or is a subset of previously registered one from the same CPU.
     * This can happen due to the mappings being rounded to pageSize boundaries
     */
    ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "map 0x%lx-%lx : Found existing rgn %p base 0x%x-0x%x cpuNum %d\n",
	       paddr, paddr+size, rgn, rgn->base, rgn->base+rgn->size, cpuNum);

    /* Simply reference count */
    rgn->refCount++;
    
    _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
    
    return ICS_SUCCESS;
  }
  
  /* Map in a new memory region and add it as a region */
  rgn = addRegion(NULL, paddr, size, mflags, cpuNum, 0 /*cpuMask*/);
  if (rgn == NULL)
  {
    err = ICS_ENOMEM;
    goto error_release;
  }

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  /* Returned mapped vaddr to caller */
  *map = rgn->map;

  ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "base 0x%x size %d mflags 0x%x -> map %p\n",
	     paddr, size, mflags, *map);

  return ICS_SUCCESS;

error_release:
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
  
  return err;
}

/*
 * Unmap a virtual mapping of a region.
 * This is called by the admin task in response to remote unmap requests.
 *
 * It does exactly the same as ICS_region_unmap() except it doesn't call updateCpus()
 */
ICS_ERROR ics_region_unmap (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags)
{
  ICS_ERROR     err;
  ics_region_t *rgn;

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Lookup region in our local region table */
  rgn = lookupRemoteRegion(paddr, size, mflags);
  if (rgn == NULL)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error_release;
  }

  ICS_PRINTF(ICS_DBG_REGION|ICS_DBG_INIT, "Found rgn %p base 0x%x map %p umap %p refCount %d\n",
	     rgn, rgn->base, rgn->map, rgn->umap, rgn->refCount);

  /* Cannot remove locally added regions */
  if (rgn->cpuNum == ics_state->cpuNum)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error_release;
  }

  ICS_assert(rgn->refCount > 0);

  /* Decrement refCount and free if 0 */
  if (--rgn->refCount == 0)
  {
    /* Now remove the region, unmapping any mappings */
    err = removeRegion(rgn);
    ICS_assert(err == ICS_SUCCESS);	/* XXXX Cannot recover */
  }

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return ICS_SUCCESS;

error_release:
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
  
  return err;
}

/*
 * Inform the specified CPU of all our current local regions
 *
 * Called when a cpu is first (re)-connected to so it can map
 * in all our current memory regions
 */
ICS_ERROR ics_region_cpu_up (ICS_UINT cpuNum)
{
  ICS_ERROR  err = ICS_SUCCESS;
  int        idx;

  for (idx = 0; idx < _ICS_MAX_REGIONS; idx++)
  {
    ics_region_t *rgn = &ics_state->rgn[idx];

    /* Only send info on locally created regions (i.e. ones created with ICS_region_add) */
    if (rgn->size && (rgn->cpuNum == ics_state->cpuNum) && (rgn->cpuMask & (1UL << cpuNum)))
    {
      /* Local region */
      err = ics_admin_map_region(rgn->base, rgn->size, rgn->mflags, cpuNum);
      if (err != ICS_SUCCESS)
      {
	ICS_EPRINTF(ICS_DBG_REGION, "admin map region %p size %d cpuNum %d failed : %d\n",
		    rgn->base, rgn->size, cpuNum, err);
	break;
      }
    }

    /* XXXX What do we do if there is an error ? */
    ICS_assert(err == ICS_SUCCESS);
  }

  return err;
}

/*
 * Remove any remote and local memory regions for the
 * cpu from which we are disconnecting
 *
 * flags can be set to ICS_CPU_DEAD when the cpu has failed
 * which will prevent unmap messages being sent to it
 *
 */
ICS_ERROR ics_region_cpu_down (ICS_UINT cpuNum, ICS_UINT flags)
{
  int i, idx;
  

  /* Protect the region table */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);
  
  for (i = 0, idx = 0; i < _ICS_MAX_REGIONS; i++)
  {
    ics_region_t *rgn = &ics_state->rgn[i];

    /* Skip empty region table slots */
    if (rgn->size == 0)
      continue;

    /* Send unmap requests for locally created regions (i.e. ones created with ICS_region_add) */
    if (!(flags & ICS_CPU_DEAD) && (rgn->cpuNum == ics_state->cpuNum))
    {
      /* Stash these for below */
      rgns[idx].base = rgn->base;
      rgns[idx].size = rgn->size;
      rgns[idx++].mflags = rgn->mflags;
    }

    /* Find local regions matching the supplied cpuNum */
    if (rgn->cpuNum == cpuNum)
    {
      /* Force immediate removal */
      rgn->refCount = 0;

      /* Now remove the region, unmapping any mappings */
      (void) removeRegion(rgn);
    }
  }

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  if (!(flags & ICS_CPU_DEAD))
  {
    /* Send unmap requests now not holding state lock */
    for (i = 0; i < idx; i++)
    {
      (void) ics_admin_unmap_region(rgns[i].base, rgns[i].size, rgns[i].mflags, cpuNum);
    }
  }

  return ICS_SUCCESS;
}

/* 
 * Initialise the region table
 */
ICS_ERROR ics_region_init (void)
{
  int idx;
  
  for (idx = 0; idx < _ICS_MAX_REGIONS; idx++)
  {
    ics_region_t *rgn = &ics_state->rgn[idx];

    /* Generate an initial region handle */
    rgn->handle = _ICS_HANDLE(_ICS_TYPE_REGION, ics_state->cpuNum, rgn->version, idx);
  }

  return ICS_SUCCESS;
}

/* 
 * Remove all regions, called during ICS shutdown
 */
void ics_region_term (void)
{
  ICS_ERROR err;

  int       idx;
  
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  for (idx = 0; idx < _ICS_MAX_REGIONS; idx++)
  {
    ics_region_t *rgn = &ics_state->rgn[idx];

    /* XXXX should we inform remote CPUs ? */

    /* Now remove the region, unmapping any mappings */
    if (rgn->map != NULL)
    {
      err = removeRegion(rgn);
      ICS_assert(err == ICS_SUCCESS);
    }
  }

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return;
}

/* Internal virtual to physical translation function
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
ICS_ERROR _ics_region_virt2phys (ICS_VOID *address, ICS_OFFSET *paddrp, ICS_MEM_FLAGS *mflagsp)
{
  ics_region_t *rgn;

  /* Lookup region in our local region table */
  rgn = lookupAddr(address);

  if (rgn == NULL)
  {
    /* Failed to find a mapping */
    ICS_PRINTF(ICS_DBG_REGION, "address %p - mapping not found\n", address);

    /* Fallback to OS Virt To Phys lookup */
    return _ICS_OS_VIRT2PHYS(address, paddrp, mflagsp);
  }

  /* Return the physical address of the supplied address */
  *paddrp = rgn->base + ((ICS_OFFSET) address - (ICS_OFFSET) rgn->map);

  ICS_PRINTF(ICS_DBG_REGION, "address %p rgn %p base 0x%x map %p -> paddr 0x%x\n",
	     address, rgn, rgn->base, rgn->map, *paddrp);

  /* Return region memory flags */
  *mflagsp = rgn->mflags;

  return ICS_SUCCESS;
}

/*
 * Convert the supplied virtual address into a physical one
 * by looking it up in the table of regions.
 *
 * If no matching region is found, then a call to the OS specific
 * VIRT2PHYS routine is made. This allows addresses which are not
 * described by any local region to be converted to physical.
 *
 * On success ICS_SUCCESS is returned and the physical address and cache flags
 * are return via the supplied parameters
 *
 * On failure to find a region, ICS_HANDLE_INVALID is returned
 */
ICS_ERROR ICS_region_virt2phys (ICS_VOID *address, ICS_OFFSET *paddrp, ICS_MEM_FLAGS *mflagsp)
{
  ics_region_t *rgn;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  if (paddrp == NULL || mflagsp == NULL)
    return ICS_INVALID_ARGUMENT;

#ifndef CONFIG_SMP   
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);
#endif   


  /* Lookup region in our local region table */
  rgn = lookupAddr(address);

#ifndef CONFIG_SMP   
  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
#endif   

  if (rgn == NULL)
  {
    /* Failed to find a mapping */
    ICS_PRINTF(ICS_DBG_REGION, "address %p - mapping not found\n", address);

    /* Pass this through as a Physical address */
    *mflagsp = ICS_PHYSICAL;

    /* Fallback to OS Virt To Phys lookup */
    return _ICS_OS_VIRT2PHYS(address, paddrp, mflagsp);
  }

  /* Return the physical address of the supplied address */
  *paddrp = rgn->base + ((ICS_OFFSET) address - (ICS_OFFSET) rgn->map);

  ICS_PRINTF(ICS_DBG_REGION, "address %p rgn %p base 0x%x map %p -> paddr 0x%x\n",
	     address, rgn, rgn->base, rgn->map, *paddrp);

  /* Return region memory flags */
  *mflagsp = rgn->mflags;

  return ICS_SUCCESS;
}

/*
 * Convert the supplied physical address into a virtual one
 * by looking for it in the table of mapped regions.
 * The flags parameter will determine what translation is matched, i.e. CACHED or UNCACHED
 *
 * If no matching region is found, then ICS_HANDLE_INVALID is returned
 *
 * Can be called directly from the message receive handler in IRQ context
 * hence the lookup is performed without taking a MUTEX lock
 *
 */
ICS_ERROR ICS_region_phys2virt (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags, ICS_VOID **addressp)
{
  ics_region_t *rgn;

  ICS_UINT      validFlags = (ICS_CACHED|ICS_UNCACHED|ICS_WRITE_BUFFER);  /* ICS_PHYSICAL doesn't make sense */

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  if (addressp == NULL)
    return ICS_INVALID_ARGUMENT;

  if (mflags & ~validFlags)
    return ICS_INVALID_ARGUMENT;
    
  /* Lookup region in our local region table */
  rgn = lookupRegion(paddr, size, mflags);
  if (rgn == NULL)
  {
    ICS_PRINTF(ICS_DBG_REGION, "paddr 0x%x mflags 0x%x - Not found\n", paddr, mflags);
    
    /* Failed to find a mapped region */
    return ICS_HANDLE_INVALID;
  }

  /* Return the corresponding virtual address mapping */
  *addressp = (ICS_VOID *) ((ICS_OFFSET) rgn->map + (paddr - rgn->base));

  ICS_PRINTF(ICS_DBG_REGION, "paddr 0x%x mflags %d rgn %p base 0x%x map %p -> addr %p\n",
	     paddr, mflags, rgn, rgn->base, rgn->map, *addressp);

  return ICS_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
