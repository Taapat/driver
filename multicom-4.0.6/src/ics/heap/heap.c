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

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

#include "_ics_shm.h"	/* SHM transport specific headers */

/*
 * Simple heap management system
 *
 * This uses a very simple heap management system to manage
 * a physically contiguous region of memory. 
 * Buffers can be allocated and freed back to the heap, but
 * the heap is never grown. 
 *
 * Once created, both cached and uncached mappings of the
 * heap memory are made available. For this reason all
 * buffers allocated are ICS_CACHELINE_SIZE aligned and
 * multiples of size
 */

#define ICS_BLOCK_MAGIC_ALLOC	0xa110caed
#define ICS_BLOCK_MAGIC_FREE	0xaddef4ee

typedef struct ics_block
{
  struct ics_block	*next;	/* Singly linked list */

  ICS_SIZE		 size;	/* Size of this block (including this header) */
  
  ICS_UINT		 magic;	/* It's a kind of */

  ICS_CHAR		 pad[_ICS_CACHELINE_PAD(1, 2)];

} ics_block_t;

typedef struct
{
  _ICS_OS_MUTEX		 lock;		/* lock to protect freelist */
  struct ics_block       listhead;	/* Head of block list */
  struct ics_block	*freelist;	/* Free list search start point */

  void     		*heapBase;	/* Supplied heap base (may be NULL) */
  ICS_BOOL  		 heapCached;	/* Is the heap memory cached ? */
  ICS_OFFSET		 heapPaddr;	/* Needed for cache flush/purge */

  void  		*memBase;	/* Heap memory segment base */
  size_t    		 memSize;	/* Size of heap memory segment */
  void     		*map;		/* [Un]cached mapping of heap memory segment */

  size_t		 allocated;	/* Number of bytes allocated */

} ics_heap_t;

/* Translate a heap memory address */	
#define _ICS_HEAP_TRANSLATE(HEAP, MEM)	((void *) ((ICS_OFFSET)(HEAP)->map + ((ICS_OFFSET)(MEM) - (ICS_OFFSET)(HEAP)->memBase)))

/* Macro to detect bogus heap handles being passed in */
#define BOGUS_HANDLE(hdl, heap) (hdl == ICS_INVALID_HANDLE_VALUE || ((int)hdl & 3) || 		\
				 heap == NULL || heap->memSize == 0 || heap->memBase == NULL ||	\
				 !ALIGNED(heap->map, _ICS_OS_PAGESIZE))

ICS_ERROR ics_heap_create (ICS_VOID *heapBase, ICS_SIZE heapSize, ICS_UINT flags, ICS_HEAP *heapp)
{
  ICS_ERROR     err = ICS_SUCCESS;
  ICS_UINT      validFlags = 0;
  ics_heap_t   *heap;

  ICS_OFFSET    paddr;
  ICS_MEM_FLAGS mflags;
  ICS_BOOL	cached;
    
  void         *memBase = NULL, *map = NULL;
  size_t        memSize = 0;
#ifdef CONFIG_SMP   
    unsigned long  iflags;
#endif

  ICS_PRINTF(ICS_DBG_HEAP, "heapBase %p heapSize %d flags 0x%x heapp %p\n",
	     heapBase, heapSize, flags, heapp);

  if (heapSize == 0 || !ICS_PAGE_ALIGNED(heapSize) || heapp == NULL || flags & ~validFlags)
    return ICS_INVALID_ARGUMENT;

  if (heapBase && !ICS_PAGE_ALIGNED(heapBase))
    return ICS_INVALID_ARGUMENT;

  /* Allocate a new heap desc */
  _ICS_OS_ZALLOC(heap, sizeof(*heap));
  if (heap == NULL)
  {
    err = ICS_ENOMEM;
    goto error;
  }

  if (heapBase == NULL)
  {
    /* Guaranteed to be a whole number of OS pages */
    memSize = heapSize;
    
    /* Allocate a chunk of physically contiguous memory
     * that can be mapped into other CPUs memory systems.
     */
    memBase = _ICS_OS_CONTIG_ALLOC(memSize, ICS_PAGE_SIZE);
    if (memBase == NULL)
    {
      err = ICS_ENOMEM;
      goto error_free_desc;
    }
  }
  else
  {
    ICS_OFFSET start, end;

    /* Get physical address of start and end of heap region */
    err = _ICS_OS_VIRT2PHYS(heapBase, &start, &mflags);
    if (err != ICS_SUCCESS)
      goto error;

    err = _ICS_OS_VIRT2PHYS(heapBase+(heapSize-1), &end, &mflags);
    if (err != ICS_SUCCESS)
      goto error;

    /* Check memory region is physically contiguous */
    if ((end - start) != (heapSize-1))
    {
      ICS_EPRINTF(ICS_DBG_HEAP, "Non-contiguous heapBase %p heapSize %d : 0x%lx->0x%lx\n",
		  heapBase, heapSize, start, end);

      err = ICS_INVALID_ARGUMENT;
      goto error;
    }

    /* User supplied heap base address */
    memBase = heapBase;
    memSize = heapSize;
  }	
  
  ICS_assert(memBase && ICS_PAGE_ALIGNED(memBase));
  ICS_assert(memSize && ICS_PAGE_ALIGNED(memSize));

  /* Determine the cache attribute of the memory region */
  err = _ICS_OS_VIRT2PHYS(memBase, &paddr, &mflags);
  if (err != ICS_SUCCESS)
    goto error_free_mem;

#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Clear memory and then purge it out of the cache */
  _ICS_OS_MEMSET(memBase, 0x0, memSize);
  /* Need to PURGE here to be L2 safe on SH4 */
  _ICS_OS_CACHE_PURGE(memBase, paddr, memSize);

#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  cached = ((mflags & ICS_CACHED) ? ICS_TRUE : ICS_FALSE);

#if defined (__sh__) || defined(__st200__)   
  /* Create an [un]cached mapping of the heap */
  map = _ICS_OS_MMAP(paddr, memSize, !cached);
  if (map == NULL)
  {
    err = ICS_SYSTEM_ERROR;
    goto error_free_mem;
  }
#elif defined(__arm__) /* arm core */
  /* ARM doesn't support cached and uncached transaltions
     for a same physical location. */
  map = memBase;
#else
#error "Undeinfed CPU type"
#endif /* defined (__sh__) || defined(__st200__) */

#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Purge any old cache mappings */
  _ICS_OS_CACHE_PURGE(map, paddr, memSize);
  
  {
    ics_block_t *block;

#ifndef CONFIG_SMP   
    (void) _ICS_OS_MUTEX_INIT(&heap->lock);
#endif
    /* Create the first free block */
    block = (ics_block_t *) memBase;
    block->magic = ICS_BLOCK_MAGIC_FREE;
    block->size  = memSize;
    block->next  = &heap->listhead;
    
    /* Initialise 'dummy' list head */
    heap->listhead.next  = block;
    heap->listhead.magic = ICS_BLOCK_MAGIC_FREE;
    heap->listhead.size  = 0;		/* Head is an empty block */
    
    heap->freelist = &heap->listhead;	/* Search starts here */
  }

  /* Stash info in heap desc */
  heap->memBase   = memBase;
  heap->memSize   = memSize;
  heap->map       = map;

  heap->heapCached = cached;
  heap->heapBase   = heapBase;	/* Stash the supplied heapBase so we know who created the heap */
  heap->heapPaddr  = paddr;

  heap->allocated  = 0;

  ICS_PRINTF(ICS_DBG_HEAP,
	     "created heap %p: memBase %p memSize %d map %p cached %d heapBase %p\n",
	     heap, heap->memBase, heap->memSize, heap->map, heap->heapCached, heap->heapBase);

  /* Pass back the heap handle */
  *heapp = (ICS_HEAP) heap;
  
#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  return ICS_SUCCESS;

error_free_mem:
  /* Free off memory segment if we created it */
  if (heap->heapBase == NULL)
    _ICS_OS_CONTIG_FREE(memBase);

error_free_desc:
  _ICS_OS_FREE(heap);

error:
  ICS_EPRINTF(ICS_DBG_HEAP, "Failed : %s (%d)\n",
	      ics_err_str(err), err);

  return err;
}


ICS_ERROR ics_heap_destroy (ICS_HEAP hdl, ICS_UINT flags)
{
  ICS_UINT validFlags = 0;
 #ifdef CONFIG_SMP   
    unsigned long  iflags;
#endif 
  ics_heap_t *heap = (ics_heap_t *) hdl;

  if (flags & ~validFlags)
    return ICS_INVALID_ARGUMENT;
  
  /* Try and detect bogus handles */
  if (BOGUS_HANDLE(hdl, heap))
    return ICS_HANDLE_INVALID;

#ifndef CONFIG_SMP   
  _ICS_OS_MUTEX_DESTROY(&heap->lock);
#endif

#if defined (__sh__) || defined(__st200__)   
  /* Unmap the [un]cached mapping of the heap */
  if (heap->map)
  {
    /* Need to PURGE here to be L2 safe on SH4 */
    _ICS_OS_CACHE_PURGE(heap->map, heap->heapPaddr, heap->memSize);
    _ICS_OS_MUNMAP(heap->map);
  }
#endif

  /* Free off memory segment if we created it */
  if (heap->heapBase == NULL)
  {
    /* Need to PURGE here to be L2 safe on SH4 */
    _ICS_OS_CACHE_PURGE(heap->memBase, heap->heapPaddr, heap->memSize);
    _ICS_OS_CONTIG_FREE(heap->memBase);
  }

#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Free off heap desc */
  _ICS_OS_MEMSET(heap, 0, sizeof(*heap));
#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  _ICS_OS_FREE(heap);

  return ICS_SUCCESS;
}

/*
 * First fit algorithm
 * Find first free block with sufficient size to contain allocation
 * Split block if not completely consumed
 *
 * Size passed in already includes extra needed for a block header
 * and is also ICS_CACHELINE_SIZE aligned
 */
static
void *findFreeBlock (ics_heap_t *heap, ICS_SIZE size)
{
  ics_block_t *prev, *curr;

#ifndef CONFIG_SMP   
  _ICS_OS_MUTEX_TAKE(&heap->lock);
#endif

  ICS_PRINTF(ICS_DBG_HEAP, "heap %p freelist %p\n", heap, heap->freelist);

  ICS_ASSERT(heap->freelist);
  ICS_ASSERT(heap->freelist->magic == ICS_BLOCK_MAGIC_FREE);
  ICS_ASSERT(heap->freelist->next);

  prev = heap->freelist;
  for (curr = prev->next; ; prev = curr, curr = curr->next)
  {
    ICS_ASSERT(curr);
    ICS_ASSERT(curr->magic == ICS_BLOCK_MAGIC_FREE);

    /* Does it fit ? */
    if (curr->size >= size) 
    {
      ICS_PRINTF(ICS_DBG_HEAP, "Found block %p size %d next %p\n",
		 curr, curr->size, curr->next);

      if (curr->size == size)
      {
	/* an exact fit - remove curr */
	prev->next = curr->next;
      }
      else
      {
	/* split block */
	ics_block_t *new = (ics_block_t *)((ICS_OFFSET)curr + size);
	
	prev->next = new;
	new->size = curr->size - size;
	new->next = curr->next;
	new->magic = ICS_BLOCK_MAGIC_FREE;

	/* Shrink current */
	curr->size = size;
      }

      curr->magic = ICS_BLOCK_MAGIC_ALLOC;

      /* Improve fragment distribution and reduce our average
       * search time by starting our next search here. (see
       * Knuth vol 1, sec 2.5, pg 449 (Exercise 6 answer))
       */
      heap->freelist = prev->next;

      heap->allocated += size;

#ifndef CONFIG_SMP   
      _ICS_OS_MUTEX_RELEASE(&heap->lock);
#endif

      /* Return memory base */
      return curr+1;
    }

    /* Have we reached the starting point ? */
    if (curr == heap->freelist)
    {
#ifndef CONFIG_SMP     
      _ICS_OS_MUTEX_RELEASE(&heap->lock);
#endif

      /* Nothing found */
      return NULL;
    }
  }
}

/*
 * Return the correct virtual address based on the requested cache flags
 *
 * NB: This function is also called from ics_heap_base() with a 0 size
 * when we are translating the base of the heap to either cached or uncached
 */
_ICS_OS_INLINE_FUNC_PREFIX
void *heapTranslate (ics_heap_t *heap, void *mem, ICS_MEM_FLAGS mflags, ICS_SIZE size)
{
  /* Supply the correct virtual address based on the requested cache flags */
  if (mflags & ICS_CACHED)
  { 
    if (heap->heapCached)
      /* Return a cached memory address */
      return mem;
    else
      /* Translate to a cached address */
      return _ICS_HEAP_TRANSLATE(heap, mem);
  }
  else if (mflags & ICS_UNCACHED)
  {
    ICS_OFFSET offset;
    void *cached;

    /* Generate the physical address offset */
    ICS_assert(((ICS_OFFSET)mem >= (ICS_OFFSET)heap->memBase));
    offset = (ICS_OFFSET)mem - (ICS_OFFSET)heap->memBase;

    /*
     * Generate the cached address so we can purge any
     * old entries from the cache. This is especially
     * important for the stx7108 L2 cache as it can start
     * fulfilling requests to uncached addresses!
     */
    if (heap->heapCached)
    {
      /*
       * Heap memBase is cached, hence so is mem
       */
      cached = mem;
      
      /* Translate to an uncached memory address */
      mem = _ICS_HEAP_TRANSLATE(heap, mem);
    }
    else
    {
      /*
       * Heap memBase is uncached, hence so is mem
       */
      
      /* Translate to a cached address */
      cached = _ICS_HEAP_TRANSLATE(heap, mem);
    }
    
    if (size)
    {
      /* Purge any old cache entries out */
      ICS_PRINTF(ICS_DBG_HEAP, "Purge cache %p paddr %lx size %d\n",
		 cached, heap->heapPaddr + offset, size);
      
      /* Need to PURGE here to be L2 safe on SH4 */
      _ICS_OS_CACHE_PURGE(cached, heap->heapPaddr + offset, size);
    }

    /* Return an uncached memory address */
    return mem;
  }

  ICS_assert(0);

  /* NOTREACHED */
  return NULL;
}

/*
 * Allocate a buffer from the heap
 * As we support CACHED and UNCACHED mappings of the buffers they
 * will be ICS_CACHELINE_SIZE aligned and sized or else we end up
 * with cache issues when accessing them from different CPUs.
 */
void *ics_heap_alloc (ICS_HEAP hdl, ICS_SIZE size, ICS_MEM_FLAGS mflags)
{
  ics_heap_t *heap = (ics_heap_t *) hdl;
  ICS_UINT    validFlags = (ICS_CACHED|ICS_UNCACHED);
  void       *mem;
  void       *translatedmem;
#ifdef CONFIG_SMP   
    unsigned long  iflags;
#endif
  if (hdl == ICS_INVALID_HANDLE_VALUE || heap == NULL)
    return NULL;

  if (mflags == 0 || (mflags & ~validFlags))
    return NULL;
    
#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Try and detect bogus handles */
  if (BOGUS_HANDLE(hdl, heap))
  {
#ifdef CONFIG_SMP   
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    return NULL;
  }
  /* Always have enough room for the block header */
  size += sizeof(ics_block_t);
  
  /* Always allocate whole numbers of cache lines */
  size = ALIGNUP(size, ICS_CACHELINE_SIZE);
  
  mem = findFreeBlock(heap, size);
  if (mem == NULL)
  {
    ICS_EPRINTF(ICS_DBG_HEAP, "heap %p Failed to allocate %d bytes\n", heap, size);
#ifdef CONFIG_SMP   
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    return NULL;
  }

  ICS_ASSERT(ICS_CACHELINE_ALIGNED(mem));

#if defined(__arm__)    
  translatedmem = mem;
#else
  /* Supply the correct virtual address based on the requested cache flags */
  translatedmem = heapTranslate(heap, mem, mflags, size);
#endif
#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  return translatedmem;
}

/* 
 * Insert a block back into the freelist
 * We keep this in address order so that during the free
 * operation we can merge neighbouring blocks together
 */
static void
freeBlock (ics_heap_t *heap, ics_block_t *block)
{
  ics_block_t *curr;

  
#ifndef CONFIG_SMP   
  _ICS_OS_MUTEX_TAKE(&heap->lock);
#endif

  ICS_PRINTF(ICS_DBG_HEAP, "Free block heap %p block %p size %d\n",
	     heap, block, block->size);

  ICS_ASSERT(block->magic == ICS_BLOCK_MAGIC_ALLOC);
  ICS_ASSERT(ICS_CACHELINE_ALIGNED(block->size));

  heap->allocated -= block->size;

  block->magic = ICS_BLOCK_MAGIC_FREE;

  /* Find reinsertion point */
  for (curr = heap->freelist; !(block > curr && block < curr->next); curr = curr->next)
  { 
    ICS_ASSERT(curr->magic == ICS_BLOCK_MAGIC_FREE);

    /* Cope with wrap condition */
    if (curr >= curr->next && (block > curr || block < curr->next))
      break;
  }

  ICS_ASSERT(curr->magic == ICS_BLOCK_MAGIC_FREE);

  ICS_PRINTF(ICS_DBG_HEAP, "Insert block at curr %p size %d next %p\n",
	     curr, curr->size, curr->next);

  if ((ics_block_t *)((ICS_OFFSET)block + block->size) == curr->next)
  {
    ICS_ASSERT(curr->next->magic == ICS_BLOCK_MAGIC_FREE);

    /* Merge with next block */
    block->size += curr->next->size;
    block->next = curr->next->next;

    ICS_ASSERT(ICS_CACHELINE_ALIGNED(block->size));
  } 
  else
    block->next = curr->next;
  
  if ((ics_block_t *)((ICS_OFFSET)curr + curr->size) == block)
  {
    /* Merge with previous block */
    curr->size += block->size;
    curr->next = block->next;
  } 
  else
    curr->next = block;

  /* Start search here next time */
  heap->freelist = curr;

  ICS_ASSERT(curr->magic == ICS_BLOCK_MAGIC_FREE);
  ICS_ASSERT(ICS_CACHELINE_ALIGNED(curr->size));
#ifndef CONFIG_SMP   
  _ICS_OS_MUTEX_RELEASE(&heap->lock);
#endif
}

ICS_ERROR ics_heap_free (ICS_HEAP hdl, void *mem)
{
  ics_heap_t *heap = (ics_heap_t *) hdl;
#ifdef CONFIG_SMP   
    unsigned long  iflags;
#endif

  ics_block_t *block;

#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Try and detect bogus handles */
  if (BOGUS_HANDLE(hdl, heap))
  {
#ifdef CONFIG_SMP   
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    return ICS_HANDLE_INVALID;
  }

  /* We expect CACHLINE aligned buffers */
  if (!ICS_CACHELINE_ALIGNED(mem))
  {
#ifdef CONFIG_SMP   
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    return ICS_INVALID_ARGUMENT;
  }   
  /* Allow free of a NULL pointer */
  if (mem == NULL)
  {
#ifdef CONFIG_SMP   
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    return ICS_SUCCESS;
  }

#if defined (__sh__) || defined(__st200__)    
  /* We may have given the caller a differently mapped virtual address */
  if (((ICS_OFFSET)mem >= (ICS_OFFSET)heap->map) && ((ICS_OFFSET)mem < ((ICS_OFFSET)heap->map + heap->memSize)))
  {
    /* Convert back to a memory ptn address */
    mem = (void *)((ICS_OFFSET) heap->memBase + ((ICS_OFFSET) mem - (ICS_OFFSET) heap->map));
  }
#endif
  /* Access the block header */
  block = ((ics_block_t *)mem) - 1;
  
  /* Free block */
  freeBlock(heap, block);

#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  return ICS_SUCCESS;
}

/* Return the virtual address of the heap base
 * 
 * Returns either the Cached or Uncached address based
 * on the supplied mflags
 */
void *ics_heap_base (ICS_HEAP hdl, ICS_MEM_FLAGS mflags)
{
  ics_heap_t *heap = (ics_heap_t *) hdl;
  void       *translatedmem;
#ifdef CONFIG_SMP   
    unsigned long  iflags;
#endif

  ICS_UINT    validFlags = (ICS_CACHED|ICS_UNCACHED);

  /* Try and detect bogus handles */
  if (BOGUS_HANDLE(hdl, heap))
    return NULL;

  if (mflags == 0 || (mflags & ~validFlags))
    return NULL;

#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
#if defined(__arm__)    
  translatedmem = heap->memBase;
#else
  /* Return the [un]cached heap memory base */
  translatedmem = heapTranslate(heap, heap->memBase, mflags, 0);
#endif
#ifdef CONFIG_SMP   
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  return translatedmem;
}

/* Return the physical address of the heap base */
ICS_OFFSET ics_heap_pbase (ICS_HEAP hdl)
{
  ics_heap_t   *heap = (ics_heap_t *) hdl;

  /* Try and detect bogus handles */
  if (BOGUS_HANDLE(hdl, heap))
    return ICS_BAD_OFFSET;

  return heap->heapPaddr;
}

/* Return the actual size of the mapped heap */
ICS_SIZE ics_heap_size (ICS_HEAP hdl)
{
  ics_heap_t *heap = (ics_heap_t *) hdl;

  /* Try and detect bogus handles */
  if (BOGUS_HANDLE(hdl, heap))
    return ICS_HANDLE_INVALID;

  return heap->memSize;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
