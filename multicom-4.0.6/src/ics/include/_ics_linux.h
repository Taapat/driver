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
 * Linux Kernel OS Wrapper macros
 */

#ifndef _ICS_LINUX_SYS_H
#define _ICS_LINUX_SYS_H

#include <linux/kernel.h>
#include <linux/kthread.h>

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
#include <linux/config.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
#include <linux/slab.h>
#endif /*  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) */

#include <linux/platform_device.h>
#include <linux/firmware.h>

#include <linux/mm.h>    	/* vmalloc_to_page() */
#include <linux/delay.h>	/* msleep() */
#include <linux/interrupt.h>	/* request_irq(), free_irq() */

#include <linux/ioport.h>	/* Need "struct resource" for bpa2.h */

#if defined(__sh__)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
#include <linux/bigphysarea.h>
#else
#include <linux/bpa2.h>
#endif
#elif defined (__arm__)
#include <linux/bpa2.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include <asm/io.h>		/* sh4 dma_cache_wback() etc */
#include <asm/cacheflush.h>
#include <asm/unistd.h>  	/* __NR_sched_setscheduler */

#include <asm/io.h>		/* ioremap() etc */
#include "_ics_util.h"

#ifndef assert
#define assert(expr) \
					if(!(expr)) {							\
					  printk(KERN_ERR "ICS Assert failure '%s' @ %s:%s:%d\n", 	\
						 #expr,__FILE__,__FUNCTION__,__LINE__); 		\
					  ICS_debug_dump(0); \
					  BUG_ON(!(expr)); }
#endif

#define _ICS_OS_EXPORT 			extern
#define _ICS_OS_INLINE_FUNC_PREFIX 	static __inline__

#define _ICS_OS_MAX_TASK_NAME		16	/* OS specific maximum length of a task name */

/* Console/tty output function */
#define _ICS_OS_PRINTF			printk

/* 
 * Memory allocation
 *
 * Debug wrappers of these are defined in _ics_debug.h
 */
#define _ICS_OS_PAGESIZE		PAGE_SIZE
#define _ics_os_malloc(S)		kmalloc((S), GFP_KERNEL)
#define _ics_os_zalloc(P, S)		((P) = kzalloc((S), GFP_KERNEL))
#define _ics_os_free(P)			kfree((P))

#define _ICS_OS_MEM_AVAIL()		(0)
#define _ICS_OS_MEM_USED()		(0)

/* Data movement */
#define _ICS_OS_MEMSET(P, V, S)		memset((P), (V), (S))
#define _ICS_OS_MEMCPY(D, S, N)		memcpy((D), (S), (N))

/* 
 * Cache management
 */
#if defined (__sh__)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
/* SH4: These calls flush both the L1 and L2 caches (see asm-sh/io.h) */
#define _ICS_OS_CACHE_PURGE(V, P, S)	 	dma_cache_wback_inv((void *)(V), (S))
#define _ICS_OS_CACHE_FLUSH(V, P, S) 		dma_cache_wback((void *)(V), (S))
#define _ICS_OS_CACHE_INVALIDATE(V, P, S) 	dma_cache_inv((void *)(V), (S))
#else
/* SH4: These calls flush both the L1 and L2 caches (see asm-sh/cacheflush.h) */
#define _ICS_OS_CACHE_PURGE(V, P, S)	 	flush_ioremap_region((P), ((void *)(V)), 0, (S))
#define _ICS_OS_CACHE_FLUSH(V, P, S) 		writeback_ioremap_region((P), ((void *)(V)), 0, (S))
#define _ICS_OS_CACHE_INVALIDATE(V, P, S) 	invalidate_ioremap_region((P), ((void *)(V)), 0, (S))
#endif /*  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) */

#elif defined (__arm__)

/* The xxx_ioremap_region family are SH4 specific cache management functions.
 * These functions are not available in current STLinux/ARM kernels.
 * The ARM equivalents are the dmac_ family of functions.
 *
 * At present it is not clear whether the _ioremap functions will be added to
 * the ARM kernel. For that reason we deliberately reuse the SH4 names without
 * modification. This ensures that if the functions are added we will get a
 * compiler error (which should be solved by removing all the three functions
 * below).
 */
#define MC_CACHE_ALLIGN
static inline void purge_ioremap_region(unsigned long phys, void __iomem *virt,
       unsigned offset, size_t size)
{
       void *start = (void __force *)virt + offset;
       unsigned long phys_start = phys + offset;

#ifdef MC_CACHE_ALLIGN
       /* Align the address to cache line. */
       size       =  ALIGNUP(size +((unsigned long)start -((unsigned long)start & (~(ICS_CACHELINE_SIZE -1)))), ICS_CACHELINE_SIZE);
       start      = (void *)((unsigned long)start & (~(ICS_CACHELINE_SIZE -1)));
       phys_start &=  ~(ICS_CACHELINE_SIZE -1);
#endif /*MC_CACHE_ALLIGN*/

       /* Purge inner cache -l1*/   
       dmac_flush_range(start, start + size);
       smp_mb();

       /* Purge outer cache -l2*/  
       outer_flush_range(phys_start,phys_start+size);
       mb();
}
static inline void writeback_ioremap_region(unsigned long phys, void __iomem *virt,
                                           unsigned offset, size_t size)
{
       void *start = (void __force *)virt + offset;
       unsigned long phys_start = phys + offset;

#ifdef MC_CACHE_ALLIGN
       /* Align the address to cache line. */
       size       =  ALIGNUP(size +((unsigned long)start -((unsigned long)start & (~(ICS_CACHELINE_SIZE -1)))), ICS_CACHELINE_SIZE);
       start      = (void *)((unsigned long)start & (~(ICS_CACHELINE_SIZE -1)));
       phys_start &=  ~(ICS_CACHELINE_SIZE -1);
#endif /*MC_CACHE_ALLIGN*/

       /* Clean i.e flush the inner cache -l1*/
       dmac_clean_range(start, start + size);
       smp_mb();

       /* Clean outer cache -l2*/  
       outer_clean_range(phys_start,phys_start+size);
       mb();

}
static inline void invalidate_ioremap_region(unsigned long phys, void __iomem *virt,
                                            unsigned offset, size_t size)
{
       void *start = (void __force *)virt + offset;
       unsigned long phys_start = phys + offset;

#ifdef MC_CACHE_ALLIGN
       /* Align the address to cache line. */
       size       =  ALIGNUP(size +((unsigned long)start -((unsigned long)start & (~(ICS_CACHELINE_SIZE -1)))), ICS_CACHELINE_SIZE);
       start      = (void *)((unsigned long)start & (~(ICS_CACHELINE_SIZE -1)));
       phys_start &=  ~(ICS_CACHELINE_SIZE -1);
#endif /*MC_CACHE_ALLIGN*/
       
       /* Invalidate inner cache -l1*/   
       dmac_inv_range(start, start + size);
       smp_mb();

       /* Invalidate outer cache -l2*/  
       outer_inv_range(phys_start,phys_start+size);
       mb();

}

#define _ICS_OS_CACHE_PURGE(V, P, S)	 	purge_ioremap_region((P), ((void *)(V)), 0, (S))
#define _ICS_OS_CACHE_FLUSH(V, P, S) 		writeback_ioremap_region((P), ((void *)(V)), 0, (S))
#define _ICS_OS_CACHE_INVALIDATE(V, P, S) 	invalidate_ioremap_region((P), ((void *)(V)), 0, (S))
#else
#error Unknown CPU model
#endif /* __sh__ */

/*
 * Physically contiguous memory allocation
 *
 * Debug wrappers for these are defined in _ics_debug.h
 */
_ICS_OS_EXPORT void * _ics_os_contig_alloc (ICS_SIZE size, ICS_SIZE align);
_ICS_OS_EXPORT void _ics_os_contig_free (ICS_VOID *ptr);

/*
 * Virtual address mapping
 */
#ifndef ioremap_cache
/* XXXX What can we do in this case ? */
#if defined (__arm__)
#define ioremap_cache ioremap_cached
#else
#define ioremap_cache ioremap
#endif
#endif

_ICS_OS_EXPORT ICS_ERROR ics_os_virt2phys (ICS_VOID *vaddr, ICS_OFFSET *paddrp, ICS_MEM_FLAGS *mflagsp);

#define _ICS_OS_VIRT2PHYS(V, PP, MFP)	ics_os_virt2phys((V), (PP), (MFP))
#if defined (__sh__) || defined(__st200__)
#define _ics_os_mmap(P, S, C)		((C) ? ioremap_cache((unsigned long)(P), (S)) : ioremap_nocache((unsigned long)(P), (S)))
#define _ics_os_munmap(V)		(iounmap((V)), ICS_SUCCESS)
#elif defined(__arm__)
void * _ics_os_mmap(unsigned long phys_addr, size_t size, unsigned int cache);
ICS_ERROR _ics_os_munmap(void * vmem);
#endif /* defined (__sh__) || defined(__st200__) */


/* ST40 only - no speculation unit! */
#define _ICS_OS_SCU_ENABLE(A, S)	ICS_SUCCESS
#define _ICS_OS_SCU_DISABLE(A, S)	ICS_SUCCESS

/*
 * Mutual exclusion
 */
typedef struct mutex _ICS_OS_MUTEX;

/* The macro definition of mutex_init() needs wrapping */
_ICS_OS_INLINE_FUNC_PREFIX
void ics_os_mutex_init(_ICS_OS_MUTEX *mutex)
{
  mutex_init(mutex);
}
#define _ICS_OS_MUTEX_INIT(pMutex)	(ics_os_mutex_init((pMutex)), ICS_TRUE)
#define _ICS_OS_MUTEX_DESTROY(pMutex)
#define _ICS_OS_MUTEX_TAKE(pMutex)	mutex_lock((pMutex))
#define _ICS_OS_MUTEX_RELEASE(pMutex)	mutex_unlock((pMutex))
#define _ICS_OS_MUTEX_HELD(pMutex)	mutex_is_locked((pMutex))

/*
 * Blocking Events
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
/* 
 * In the absence of down_timeout() we emulate
 * the generic semaphore code as an ICS event
 */
typedef struct ics_os_event {
    spinlock_t		lock;
    unsigned int	count;
    struct list_head	wait_list;
} _ICS_OS_EVENT;

ICS_ERROR ics_os_event_init (_ICS_OS_EVENT *event);
ICS_ERROR ics_os_event_wait (_ICS_OS_EVENT *ev, ICS_ULONG timeout, ICS_BOOL interruptible);
void      ics_os_event_post (_ICS_OS_EVENT *event);

#define _ICS_OS_EVENT_INIT(pEvent)	(ics_os_event_init(pEvent) == ICS_SUCCESS)
#define _ICS_OS_EVENT_DESTROY(pEvent)	(void)(pEvent)
#define _ICS_OS_EVENT_COUNT(pEvent)	((pEvent)->count)
#define _ICS_OS_EVENT_READY(pEvent)	(_ICS_OS_EVENT_COUNT(pEvent) != 0)
#define _ICS_OS_EVENT_WAIT(pEvent,t,i)	ics_os_event_wait((pEvent), (t), (i))
#define _ICS_OS_EVENT_POST(pEvent)	ics_os_event_post(pEvent)

#else

typedef struct semaphore _ICS_OS_EVENT;

ICS_ERROR ics_os_event_wait (_ICS_OS_EVENT *ev, ICS_ULONG timeout, ICS_BOOL interruptible);

#define _ICS_OS_EVENT_INIT(pEvent)	(sema_init((pEvent), 0), ICS_TRUE)
#define _ICS_OS_EVENT_DESTROY(pEvent)	(void)(pEvent)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
/* XXXX Accessing semaphore count is naughty */
#define _ICS_OS_EVENT_COUNT(pEvent)	(atomic_read(&(pEvent)->count))
#else
/* XXXX Accessing semaphore count is naughty */
#define _ICS_OS_EVENT_COUNT(pEvent)	((pEvent)->count)
#endif
#define _ICS_OS_EVENT_READY(pEvent)	(_ICS_OS_EVENT_COUNT(pEvent) != 0)
#define _ICS_OS_EVENT_WAIT(pEvent,t,i)	ics_os_event_wait((pEvent), (t), (i))
#define _ICS_OS_EVENT_POST(pEvent)	up((pEvent))

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26) */

/*
 * Spinlocks (IRQ and SMP safe)
 */
typedef spinlock_t _ICS_OS_SPINLOCK;
#if 1
#define _ICS_OS_SPINLOCK_INIT(PLOCK)		spin_lock_init((PLOCK))
#define _ICS_OS_SPINLOCK_ENTER(PLOCK, FLAGS)	spin_lock_irqsave((PLOCK), (FLAGS))
#define _ICS_OS_SPINLOCK_EXIT(PLOCK, FLAGS)	spin_unlock_irqrestore((PLOCK), (FLAGS));
#else

static inline void _sp_lock(raw_spinlock_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
"	wfene\n"
"	strexeq	%0, %2, [%1]\n"
"	teqeq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");

	smp_mb();
}

static inline int _sp_unlock(raw_spinlock_t *lock)
{
	smp_mb();

	__asm__ __volatile__(
"	str	%1, [%0]\n"
"	mcr	p15, 0, %1, c7, c10, 4\n" /* DSB */
"	sev"
	:
	: "r" (&lock->lock), "r" (0)
	: "cc");
	smp_mb();
}

static inline void _spin_unlock_irqrestore_local(spinlock_t *lock,
					    unsigned long flags)
{
	_sp_unlock(lock);
	local_irq_restore(flags);
 mb();
	preempt_enable();
 mb();
}
static inline unsigned long _spin_lock_irqsave_local(spinlock_t *lock)
{
	unsigned long flags;

	local_irq_save(flags);
	preempt_disable();
 mb();
_sp_lock(lock);
 mb();
	//spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	/*
	 * On lockdep we dont want the hand-coded irq-enable of
	 * _raw_spin_lock_flags() code, because lockdep assumes
	 * that interrupts are not re-enabled during lock-acquire:
	 */
	return flags;
}


#define spin_unlock_irqrestore_local(lock, flags)		\
	do {						\
		typecheck(unsigned long, flags);	\
		_spin_unlock_irqrestore_local(lock, flags);	\
	} while (0)

#define spin_lock_irqsave_local(lock, flags)			\
	do {						\
		typecheck(unsigned long, flags);	\
		flags = _spin_lock_irqsave_local(lock);	\
	} while (0)
#define _ICS_OS_SPINLOCK_INIT(PLOCK)		spin_lock_init((PLOCK))
#define _ICS_OS_SPINLOCK_ENTER(PLOCK, FLAGS)	spin_lock_irqsave_local((PLOCK), (FLAGS))
#define _ICS_OS_SPINLOCK_EXIT(PLOCK, FLAGS)	spin_unlock_irqrestore_local((PLOCK), (FLAGS));

#endif


/* 
 * Thread/Task functions
 */
typedef struct task_struct _ICS_OS_TASK;

typedef struct ics_task_info
{
  _ICS_OS_TASK	     *task;
  void              (*entry)(void *);
  void               *param;
  ICS_INT             priority;
  _ICS_OS_EVENT       event;     
} _ICS_OS_TASK_INFO;

#define _ICS_OS_DEFAULT_STACK_SIZE	(0)
#define _ICS_OS_TASK_DEFAULT_PRIO	(-(20 + 70))		/* RT - see also mme_tune.c */

ICS_ERROR ics_os_task_create (void (*entry)(void *), void *param, ICS_INT priority, const ICS_CHAR *name,
			      _ICS_OS_TASK_INFO *task);
ICS_ERROR ics_os_task_destroy (_ICS_OS_TASK_INFO *task);

int ics_os_set_priority (struct task_struct *kt, ICS_INT prio);

#define _ICS_OS_TASK_CREATE(FUNC, PARAM, PRIO, NAME, T)	ics_os_task_create((FUNC), (PARAM), (PRIO), (NAME), (T))
#define _ICS_OS_TASK_DESTROY(T)		ics_os_task_destroy((T))

#define _ICS_OS_TASK_NAME()		current->comm
#define _ICS_OS_TASK_SELF()		current
#define _ICS_OS_TASK_PRIORITY_SET(PRIO)	ics_os_set_priority(NULL, (PRIO))
#define _ICS_OS_TASK_DELAY(M)		msleep((M))
#define _ICS_OS_TASK_YIELD()		schedule()
#define _ICS_OS_TASK_INTERRUPT()	in_interrupt()

/*
 * Clock functions
 */
#define _ICS_OS_TIME			unsigned long
#define _ICS_OS_TIME_NOW()		jiffies
#define _ICS_OS_TIME_TICKS2MSEC(T)	jiffies_to_msecs((T))
#define _ICS_OS_TIME_MSEC2TICKS(M)	msecs_to_jiffies((M))
#define _ICS_OS_TIME_AFTER(A,B)		time_after((A),(B))
#define _ICS_OS_TIME_EXPIRED(S,T)	time_after(jiffies, (S)+_ICS_OS_TIME_MSEC2TICKS((T)))
#define _ICS_OS_TIME_IDLE()		(0)	/* XXXX Not yet written */
#define _ICS_OS_HZ			HZ

/*
 * File/Firmware loading
 */
extern struct platform_device ics_pdev;	/* See linux/module.c */

#define _ICS_OS_FHANDLE			const struct firmware *
#define _ICS_OS_FOPEN(N, FHP)		request_firmware((FHP), (N), &ics_pdev.dev)
#define _ICS_OS_FREAD(M, FH, S)		(_ICS_OS_MEMCPY((M), (FH)->data, (S)), (S))
#define _ICS_OS_FCLOSE(FH)		release_firmware((FH))
#define _ICS_OS_FSIZE(FH, SP)		{ *(SP) = (FH)->size; }

#endif /* _ICS_LINUX_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
