/*******************************************************************/
/* Copyright 2008 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_os21.c                                               */
/*                                                                 */
/* Description:                                                    */
/*         Operating system abstraction implementation             */
/*                                                                 */
/*******************************************************************/

#include "embxP.h"
#include "embx_osinterface.h"
#include "debug_ctrl.h"

/*
 * Define key vmem flags if not already defined.
 * Lib could be built with old or new toolset
 */
#ifndef VMEM_CREATE_CACHED

#define VMEM_CREATE_CACHED          (1 << 0)
#define VMEM_CREATE_UNCACHED        (1 << 1)
#define VMEM_CREATE_WRITE_BUFFER    (1 << 2)
#define VMEM_CREATE_NO_WRITE_BUFFER (1 << 3)
#define VMEM_CREATE_READ            (1 << 4)
#define VMEM_CREATE_WRITE           (1 << 5)
#define VMEM_CREATE_EXECUTE         (1 << 6)
#define VMEM_CREATE_LOCK            (1 << 7)
#define VMEM_CREATE_EXCL            (1 << 8)

#endif

/*
 * Declare vmem() interface as weak - we could be linked with old or new OS21
 */
extern void *vmem_create     (void * pAddr, unsigned int size, void * vAddr, unsigned int mode) __attribute__ ((weak));
extern int vmem_delete       (void * vAddr) __attribute__ ((weak));
extern int vmem_virt_to_phys (void * vAddr, void ** pAddrp) __attribute__ ((weak));
extern int vmem_virt_mode    (void * vAddr, unsigned int * modep) __attribute__ ((weak));

#if defined(__ST231__)

/* Define in these OS21 V2.X defines if we haven't included the original header file */
#if !defined _OS21_ST200_MMAP_H_

/*
 * Memory protection attributes
 */
typedef enum
{
  mmap_protect_none = 0x00,
  mmap_protect_x    = 0x01,
  mmap_protect_r    = 0x02,
  mmap_protect_rx   = 0x03,
  mmap_protect_w    = 0x04,
  mmap_protect_wx   = 0x05,
  mmap_protect_rw   = 0x06,
  mmap_protect_rwx  = 0x07

} mmap_protection_t;

/*
 * Memory cachability attributes
 */
typedef enum
{
  mmap_uncached      = 0x00,
  mmap_cached        = 0x01,
  mmap_writecombined = 0x02

} mmap_cache_policy_t;

#endif /* ! _OS21_ST200_MMAP_H_ */

/*
 * Declare mmap() interface as weak - we could be linked with old or new OS21
 */
extern void *mmap_translate_cached (void *mapped_address) __attribute__ ((weak));
extern void *mmap_translate_physical (void *mapped_address) __attribute__ ((weak));
extern void *mmap_translate_uncached (void *mapped_address) __attribute__ ((weak));
extern void *mmap_translate_virtual (void *physical_address) __attribute__ ((weak));
extern void *mmap_create (void *pAddr, unsigned int size, mmap_protection_t prot, mmap_cache_policy_t policy,
			  unsigned int page_size)  __attribute__ ((weak));

extern int mmap_enable_speculation (void *pAddr, unsigned int length) __attribute__ ((weak));
extern int mmap_disable_speculation (void *pAddr)  __attribute__ ((weak));

extern int scu_enable_range (void * startAddr, unsigned int length) __attribute__ ((weak));
extern int scu_disable_range (void * startAddr) __attribute__ ((weak));

#endif

#ifdef __sh__
extern int _st40_vmem_enhanced_mode (void) __attribute__ ((weak));
#endif

/*
 * This header is needed to collect the prototype for memalign.
 */
#include <malloc.h>

/*----------------------------- MEMORY ALLOCATION ---------------------------*/

EMBX_VOID *EMBX_OS_ContigMemAlloc(EMBX_UINT size, EMBX_UINT align)
{
    void **alignedAddr = NULL;

    EMBX_Info(EMBX_INFO_OS, (">>>>ContigMemAlloc(size=%u, align=%u)\n",size,align));

    /* allocate an carefully aligned block of memory */
    alignedAddr = memalign(align, size);

#if defined(__sh__)
    if (alignedAddr) {
	/* ensure there are no cache entries covering this address */
	cache_invalidate_data(alignedAddr, size);
    }
#endif

    EMBX_Info(EMBX_INFO_OS, ("<<<<ContigMemAlloc = 0x%08x\n", (unsigned) alignedAddr));
    return (EMBX_VOID *)alignedAddr;
}

void EMBX_OS_ContigMemFree(EMBX_VOID *addr, EMBX_UINT size)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>ContigMemFree\n"));

    free(addr);

    EMBX_Info(EMBX_INFO_OS, ("<<<<ContigMemFree\n"));
}



EMBX_VOID *EMBX_OS_MemAlloc(EMBX_UINT size)
{
    void *pAddr;
    
    pAddr = (EMBX_VOID *)malloc(size);
    
    return pAddr;
}



/* This is an ANSI C like memory deallocate. 
 *
 * Because it is ANSI C like it is defined that this function will ignore
 * a NULL argument and return immediately.
 */
void EMBX_OS_MemFree(EMBX_VOID *addr)
{
    free((void *)addr);
}


/* this API provides a simple cache of initialized OS21 semaphores
 * because deleting them is expensive on this OS (all objects are
 * dynamically allocated and free() is too slow to appear on critical
 * paths).
 *
 * in order to do this we use a custom allocator that allocates an
 * extra word that we can use as a linkage pointer without having
 * to know anything about the internal structure of the semaphore.
 * this technique while somewhat clumsy uses only published OS21
 * APIs and it therefore future proof.
 */

static semaphore_t *cacheHead = NULL;

/*
 * The following partition pointer is volatile to ensure the allocate/backoff
 * logic in EMBX_OS_EventCreate doesn't get optimized away. 
 */
static volatile partition_t *cachePartition = NULL;

static void *cacheAlloc  (void *state, size_t size);
static void  cacheFree   (void *state, void *ptr);
static void *cacheRealloc(void *state, void *ptr, size_t size);
static int   cacheStatus (void *state, partition_status_t *status, partition_status_flags_t flags);


semaphore_t *EMBX_OS_EventCreate(void)
{
    semaphore_t *sem = 0;

    task_lock();

    if (cacheHead != 0)
    {
        sem = cacheHead;

        EMBX_Assert(0 == semaphore_value(sem));
        cacheHead = (semaphore_t *) ((void **) cacheHead)[-1];

	task_unlock();
    }
    else
    {
        /* just in time initialization of the cachePartition
         * (our custom memory allocator)
         */
        if (0 == cachePartition)
        {
	    /* Try to create a partition, this may block in the OS allowing
	     * another task to run.
             */
            partition_t *p = partition_create_any(0, cacheAlloc,   cacheFree,
                                                     cacheRealloc, cacheStatus);


            /* Since we may have allowed another tasks to run it is possible
             * another task may have beaten us to it and already set
             * a partition. If this is the case then we release the
             * partition we just got and use the one that is now there.
             */
            if (0 == cachePartition)
            {
                EMBX_Assert(p != 0);

                cachePartition = p;
            }
            else
            {
                /* This is a really unlikely corner case, but we can
                 * still continue even if our own partition create failed!
		 * (because 0 == cachePartition the semaphore will be
		 * created from the C library heap)
                 */
                if(p != 0)
                {
                    partition_delete(p);
                }
            }
        }


	task_unlock();
        sem = semaphore_create_fifo_p((partition_t *)cachePartition, 0);
    }

    return sem;
}


void EMBX_OS_EventDelete(semaphore_t *sem)
{
    EMBX_Assert(sem);

    /* This should assert if there are waiters on the semaphore
     * but OS21 does not give us an interface to find this out
     * at the moment.
     */

    /* Drain the semaphore count to 0 so that we can
     * re-allocate it from the cache in a default state.
     */
    while(semaphore_value(sem) != 0)
    {
        semaphore_wait(sem);
    }


    task_lock();

    ((void **) sem)[-1] = (void *) cacheHead;
    cacheHead = sem;

    task_unlock();
}


static void * cacheAlloc(void *state, size_t size)
{
    return ((void **) memory_allocate(NULL, size+sizeof(void *))) + 1;
}


static void cacheFree(void *state, void *ptr)
{
    memory_deallocate(NULL, ((void **) ptr) - 1);
}


static void * cacheRealloc(void *state, void *ptr, size_t size)
{
    return ((void **) memory_reallocate(NULL, ((void **) ptr) - 1, size+sizeof(void *))) + 1;
}


static int cacheStatus(void *state, partition_status_t      *status,
                                    partition_status_flags_t flags)
{
    return partition_status(NULL, status, flags);
}


/*------------------------ MEMORY ADDRESS TRANSLATION -----------------------*/

EMBX_VOID *EMBX_OS_PhysMemMap(EMBX_UINT pMem, int size, int cached)
{
    EMBX_VOID *vaddr = NULL;

    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemMap(0x%08x, %d)\n", (unsigned int) pMem, size));

    /* Test the weak symbol for being non NULL, if true we are linked against a vmem capable OS21 */
    if (vmem_create)
    {
      unsigned mode;

      mode = VMEM_CREATE_READ|VMEM_CREATE_WRITE;

      if (cached)
	  mode |= VMEM_CREATE_CACHED;
      else
	  mode |= VMEM_CREATE_UNCACHED | VMEM_CREATE_NO_WRITE_BUFFER;
      
      vaddr = vmem_create((EMBX_VOID *)pMem, size, NULL, mode);

    }
    else
    {
#if defined __ST231__
	  /* This assumes that pMem is a true physical address */
	  vaddr = mmap_translate_virtual((EMBX_VOID *)pMem);
	  vaddr = (cached ? mmap_translate_cached(vaddr) : mmap_translate_uncached(vaddr));
	  
	  if (!vaddr)
	  {
	      /* Failed to find a current translation, so create our own */

	      EMBX_UINT page_size = 0x10000000; /* Map 256MB pages unconditionally */
	      EMBX_UINT pMem_base = pMem & ~(page_size-1);
	      EMBX_UINT pMem_size = (size + (page_size-1)) & ~(page_size-1);

	      vaddr = mmap_create((void *)pMem_base, 
				  pMem_size,
				  mmap_protect_rwx, 
				  (cached ? mmap_cached : mmap_uncached),
				  page_size);
	      
	      /* Adjust the returned vaddr accordingly */
	      if (vaddr)
		  vaddr = (void *) ((EMBX_UINT) vaddr + (pMem - pMem_base));
	  }

#elif defined __sh__

      if (cached)
	  vaddr = ST40_P1_ADDR(pMem);
      else
	  vaddr = ST40_P2_ADDR(pMem);

#endif /* defined __ST231__ */
      
      if (NULL == vaddr) {
	  EMBX_DebugMessage(("PhysMemMap: pMem %p size %d cached %d failed\n", pMem, size, cached));
      }
    }

    EMBX_Info(EMBX_INFO_OS, ("PhysMemMap: *vMem = %p\n", vaddr));
    
    EMBX_Info(EMBX_INFO_OS, ("<<<<PhysMemMap\n"));
    
    return vaddr;
}



void EMBX_OS_PhysMemUnMap(EMBX_VOID *vMem)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemUnMap\n"));

    if (vmem_delete)
	vmem_delete(vMem);

    EMBX_Info(EMBX_INFO_OS, ("<<<<PhysMemUnMap\n"));
}

/* 
 * Translate a kernel virtual address to a phyiscal one 
 *
 * Returns either EMBX_SUCESS or EMBX_INCOHERENT_MEMORY when translation succeeds
 * Returns EMBX_INVALID_ARGUMENT otherwise
 */
EMBX_ERROR EMBX_OS_VirtToPhys(EMBX_VOID *vaddr, EMBX_UINT *paddrp)
{
    if (vmem_virt_to_phys)
    {
	unsigned mode;
	
	/* OS21: This should work in both 29-bit and 32-bit modes */
	if (vmem_virt_to_phys(vaddr, (EMBX_VOID *)paddrp))
	    return EMBX_INVALID_ARGUMENT;
	
	if (vmem_virt_mode(vaddr, &mode))
	    /* Lookup failed - assume incoherent */
	    return EMBX_INCOHERENT_MEMORY;
	
	if ((mode & VMEM_CREATE_CACHED) || (mode & VMEM_CREATE_WRITE_BUFFER))
	    return EMBX_INCOHERENT_MEMORY;
	
	/* Success and memory is coherent */
	return EMBX_SUCCESS;
    }

#if defined __ST231__
    /* OS21/ST231: use the old mmap interface */
    *(paddrp)= (EMBX_UINT) mmap_translate_physical(vaddr);
    
    return ((vaddr == mmap_translate_uncached(vaddr)) ? EMBX_SUCCESS : EMBX_INCOHERENT_MEMORY);
#else 
    /* Assume SH4 in 29-bit mode OS21 */
    *(paddrp)= (void *)ST40_PHYS_ADDR((EMBX_UINT)vaddr & 0x1fffffff);
    
    return ((vaddr == ST40_P2_ADDR(vaddr)) ? EMBX_SUCCESS : EMBX_INCOHERENT_MEMORY);
#endif /* defined __ST231__ */
}

#ifdef __ST231__
/*
 * ST200 specific functions to enable and disable the memory speculation feature
 */
EMBX_ERROR EMBX_OS_EnableSpeculation(EMBX_VOID *pAddr, EMBX_UINT size)
{
    int res;

    if (scu_enable_range)
	res = scu_enable_range(pAddr, size);
    else
	res = mmap_enable_speculation(pAddr, size);

    return (res ? EMBX_SUCCESS : EMBX_SYSTEM_ERROR);
}

EMBX_ERROR EMBX_OS_DisableSpeculation(EMBX_VOID *pAddr)
{
    int res;
    
    if (scu_disable_range)
	res = scu_disable_range(pAddr);
    else
	res = mmap_disable_speculation(pAddr);

    return (res ? EMBX_SUCCESS : EMBX_SYSTEM_ERROR);
}

#endif /* __ST231__ */

#ifdef __sh__
/*
 * SH4 specific code which detects whether SE mode (32-bit) is in operation
 */
EMBX_BOOL EMBX_OS_SEMode(void)
{
    if (_st40_vmem_enhanced_mode)
	return _st40_vmem_enhanced_mode() ? EMBX_TRUE : EMBX_FALSE;
    
    /* No _st40_vmem_enhanced_mode symbol so we must be linked against
     * an old 29-bit only capable OS21
     */
    return EMBX_FALSE;
}
#endif /* __sh__ */

/*--------------------------- THREAD CREATION -------------------------------*/

static char *_default_name = "embx";

static char *get_default_name(void)
{
    /* Trivial for now, think about adding an incrementing
     * number to the name, but how do we deal with mutual
     * exclusion, or don't we care for this?
     */
    return _default_name;
}

EMBX_THREAD EMBX_OS_ThreadCreate(void (*thread)(void *), void *param, EMBX_INT priority, const EMBX_CHAR *name)
{
    task_t *t;

    EMBX_Info(EMBX_INFO_OS, (">>>>ThreadCreate\n"));

    if(name == EMBX_DEFAULT_THREAD_NAME)
    {
        name = get_default_name();
    }

    t = task_create(thread, param, EMBX_DEFAULT_THREAD_STACK_SIZE, priority, name, 0);
    if(t == EMBX_INVALID_THREAD)
    {
        EMBX_DebugMessage(("ThreadCreate: task_create failed.\n"));
    }

    EMBX_Info(EMBX_INFO_OS, ("<<<<ThreadCreate\n"));

    return t;
}

EMBX_ERROR EMBX_OS_ThreadDelete(EMBX_THREAD thread)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>EMBX_OS_ThreadDelete\n"));

    if(thread == EMBX_INVALID_THREAD) {
	EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS (invalid task)\n"));
        return EMBX_SUCCESS;
    }

    if(task_wait(&thread, 1, TIMEOUT_INFINITY) != 0)
    {
        EMBX_DebugMessage(("EMBX_OS_ThreadDelete: task_wait failed.\n"));
        return EMBX_SYSTEM_INTERRUPT;        
    }

    task_delete(thread);

    EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}



/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
