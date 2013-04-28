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

#ifndef _ICS_DEBUG_SYS_H
#define _ICS_DEBUG_SYS_H

#define _ICS_DEBUG_LINE_SIZE	(128)		/* Fixed line size for cyclic debug log */

/* Global debug logging flags */
extern ICS_DBG_FLAGS _ics_debug_flags;
extern ICS_DBG_CHAN  _ics_debug_chan;

extern void ics_debug_msg (ICS_CHAR *msg, ICS_UINT len);
extern void ics_debug_printf (const char *fmt, const char *fn, int line, ...);

#define ICS_DEBUG_MSG(MSG, LEN)	do { if ((_ics_debug_chan & ICS_DBG_LOG)) ics_debug_msg((MSG), (LEN)); } while(0)

/*
 * Memory debugging 
 */
void   *_ics_debug_malloc (size_t size, size_t align, const char *file, const char *caller, int line, _ICS_OS_TASK *task);
void   *_ics_debug_zalloc (size_t size, const char *file, const char *caller, int line, _ICS_OS_TASK *task);
void    _ics_debug_free (void *buf, const char *file, const char *caller, int line);

void    _ics_debug_mem_init (void);
ICS_UINT _ics_debug_mem_dump (void);
size_t  _ics_debug_mem_total (void);

/*
 * MMAP debugging 
 */
void   *_ics_debug_mmap (ICS_OFFSET paddr, size_t size, ICS_UINT flags, const char *file, const char *caller, int line, _ICS_OS_TASK *task);
int     _ics_debug_munmap (void *addr, const char *file, const char *caller, int line);

void    _ics_debug_mmap_init (void);
ICS_UINT _ics_debug_mmap_dump (void);
size_t  _ics_debug_mmap_total (void);

#define __STRING(x)	#x

/* Make sure we log the assertion message into the cyclic buffer */
#define _ics_assert(expr)	do { if (!(expr)) {							\
				 	ics_debug_printf("assertion \"%s\" failed file %s\n", 		\
							 __func__, __LINE__, __STRING(expr), __FILE__); \
					assert(expr); }							\
				} while (0)

#if defined(ICS_DEBUG) || defined(ICS_DEBUG_ASSERT)

#define ICS_ASSERT(expr) 	_ics_assert(expr)

#else

#define ICS_ASSERT(expr)

#endif

#if defined(ICS_DEBUG)
/* Compile in all DEBUG messages */
#define ICS_PRINTF(FLAGS, FMT, ...)	if (((FLAGS) == ICS_DBG) || ((FLAGS) & _ics_debug_flags))	\
    						ics_debug_printf(FMT, __FUNCTION__, __LINE__, ## __VA_ARGS__)

#elif defined(ICS_DEBUG_FLAGS)
/* Only compile in DEBUG messages specified by ICS_DEBUG_FLAGS (and ICS_DBG messages) */
#define ICS_PRINTF(FLAGS, FMT, ...)	if (((FLAGS) == ICS_DBG) || ((FLAGS) & ICS_DEBUG_FLAGS)) 	\
    						ics_debug_printf(FMT, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
/* Only compile in ICS_DBG messages */
#define ICS_PRINTF(FLAGS, FMT, ...)	if (((FLAGS) == ICS_DBG))					\
    						ics_debug_printf(FMT, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#endif

#if 1
/* This assert macro is always enabled and should be used
 * for non performance critical assertion checks
 */
#define ICS_assert(expr)	_ics_assert(expr)

#else

#define ICS_assert(expr)

#endif

#if 1
/* ERROR printf 
 * Will cause all error messages to be displayed if 
 * _ics_debug_flags has ICS_DBG_ERR bit set or equals ICS_DBG
 */
#define ICS_EPRINTF(FLAGS, FMT, ...)	if (((FLAGS) == ICS_DBG || (((FLAGS)|ICS_DBG_ERR) & _ics_debug_flags))) \
    						ics_debug_printf(FMT, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else

#define ICS_EPRINTF(FLAGS, FMT, ...)

#endif

#ifdef ICS_DEBUG_MEM
/*
 * Install logging versions of the memory allocation
 * routines that will detect invalid frees and memory leaks
 */
#define _ICS_OS_MALLOC(S)		_ics_debug_malloc((S), 0, __FILE__, __FUNCTION__, __LINE__, _ICS_OS_TASK_SELF())
#define _ICS_OS_ZALLOC(P, S)		((P) = _ics_debug_zalloc((S), __FILE__, __FUNCTION__, __LINE__, _ICS_OS_TASK_SELF()))
#define _ICS_OS_FREE(P)			_ics_debug_free((P), __FILE__, __FUNCTION__, __LINE__)

#define _ICS_OS_CONTIG_ALLOC(S, A)	_ics_debug_malloc((S), (A), __FILE__, __FUNCTION__, __LINE__, _ICS_OS_TASK_SELF())
#define _ICS_OS_CONTIG_FREE(P)		_ics_debug_free((P), __FILE__, __FUNCTION__, __LINE__)

#define _ICS_DEBUG_MEM_INIT()		_ics_debug_mem_init()
#define _ICS_DEBUG_MEM_DUMP()		_ics_debug_mem_dump()
#define _ICS_DEBUG_MEM_TOTAL()		_ics_debug_mem_total()

#else

#define _ICS_OS_MALLOC(S)		_ics_os_malloc((S))
#define _ICS_OS_ZALLOC(P, S)		_ics_os_zalloc((P), (S))
#define _ICS_OS_FREE(P)			_ics_os_free((P))

#define _ICS_OS_CONTIG_ALLOC(S, A)	_ics_os_contig_alloc((S),(A))
#define _ICS_OS_CONTIG_FREE(P)		_ics_os_contig_free((P))

#define _ICS_DEBUG_MEM_INIT()		
#define _ICS_DEBUG_MEM_DUMP()		(0)
#define _ICS_DEBUG_MEM_TOTAL()		(0)

#endif /* ICS_DEBUG_MEM */

#ifdef ICS_DEBUG_MMAP
#define _ICS_OS_MMAP(P, S, F)		_ics_debug_mmap((P), (S), (F), __FILE__, __FUNCTION__, __LINE__, _ICS_OS_TASK_SELF())
#define _ICS_OS_MUNMAP(A)		_ics_debug_munmap((A), __FILE__, __FUNCTION__, __LINE__)

#define _ICS_DEBUG_MMAP_INIT()		_ics_debug_mmap_init()
#define _ICS_DEBUG_MMAP_DUMP()		_ics_debug_mmap_dump()
#define _ICS_DEBUG_MMAP_TOTAL()		_ics_debug_mmap_total()

#else

#define _ICS_OS_MMAP(P, S, F)		_ics_os_mmap((P), (S), (F))
#define _ICS_OS_MUNMAP(A)		_ics_os_munmap((A))

#define _ICS_DEBUG_MMAP_INIT()		
#define _ICS_DEBUG_MMAP_DUMP()		(0)
#define _ICS_DEBUG_MMAP_TOTAL()		(0)

#endif

#endif /* _ICS_DEBUG_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
