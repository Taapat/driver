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

#ifdef ICS_DEBUG_MEM

/* 
 * 
 */ 

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

/*
 * Simple debug memory allocation wrapper to detect
 * memory leaks from within ICS
 */

static int initialised = 0;

static ICS_ULONG totalSize = 0;

static LIST_HEAD(_ics_debug_mem);
static _ICS_OS_MUTEX ics_debug_mem_lock;

typedef struct debug_mem
{
  struct list_head 	 list;		/* Doubly linked list */
  
  const char		*file;		/* Allocating file */
  const char		*caller;	/* Allocating function */
  int			 line;		/* Line number of function */
  _ICS_OS_TASK		*task;		/* Allocating task */
  
  size_t		 size;		/* Size of allocation */
  size_t		 align;		/* Alignment of allocation (contig only) */

  char			*buf;		/* Actual buffer returned */
} debug_mem_t;

void *_ics_debug_malloc (size_t size, size_t align, const char *file, const char *caller, int line, _ICS_OS_TASK *task)
{
  debug_mem_t *mem;

  if (!initialised)
    _ics_debug_mem_init();

  /* Allocate desc */
  mem = _ics_os_malloc(sizeof(debug_mem_t));
  if (mem == NULL)
    return NULL;
  
  /* If align is non zero then this is a request for contiguous memory */
  mem->buf = (align ? _ics_os_contig_alloc(size, align) : _ics_os_malloc(size));
  if (mem->buf == NULL)
  {
    _ics_os_free(mem);
    return NULL;
  }

  mem->file   = strrchr(file, '/') ? strrchr(file, '/') + 1 : file; 
  mem->caller = caller;
  mem->line   = line;
  mem->task   = task;
  mem->size   = size;
  mem->align  = align;

  _ICS_OS_MUTEX_TAKE(&ics_debug_mem_lock);
  
  list_add(&mem->list, &_ics_debug_mem);

  totalSize += size;

  _ICS_OS_MUTEX_RELEASE(&ics_debug_mem_lock);

  if (align)
    return (void *) ALIGNUP(&mem->buf[0], align);
  else
    return &mem->buf[0];
  
  /* NOTREACHED */
  return NULL;
}

void *_ics_debug_zalloc (size_t size, const char *file, const char *caller, int line, _ICS_OS_TASK *task)
{
  void *buf;

  if (!initialised)
    _ics_debug_mem_init();

  buf = _ics_debug_malloc(size, 0/*align*/, file, caller, line, task);

  if (buf == NULL)
    return NULL;
  
  _ICS_OS_MEMSET(buf, 0, size);

  return buf;
}

void _ics_debug_free (void *buf, const char *file, const char *caller, int line)
{
  debug_mem_t *mem, *tmp;

  if (!initialised)
    _ics_debug_mem_init();

  _ICS_OS_MUTEX_TAKE(&ics_debug_mem_lock);

  list_for_each_entry_safe(mem, tmp, &_ics_debug_mem, list)
  {
    if (buf == mem->buf)
    {
      /* Found a match, remove from the list */
      list_del_init(&mem->list);

      totalSize -= mem->size;

      _ICS_OS_MUTEX_RELEASE(&ics_debug_mem_lock);

      /* Free of real buffer */
      if (mem->align)
	_ics_os_contig_free(buf);
      else
	_ics_os_free(buf);

      /* Now free mem desc */
      _ics_os_free(mem);
      
      return;
    }
  }

  ICS_EPRINTF(ICS_DBG, "ERROR: Free buf %p not found caller %s:%s:%d\n", buf, file, caller, line);

  _ICS_OS_MUTEX_RELEASE(&ics_debug_mem_lock);

  return;
}

void _ics_debug_mem_init (void)
{
  initialised = 1;

  (void) _ICS_OS_MUTEX_INIT(&ics_debug_mem_lock);
}

size_t _ics_debug_mem_total (void)
{
  return totalSize;
}

/*
 * Dump out all the entries on the debug mem list
 */
ICS_UINT _ics_debug_mem_dump (void)
{
  debug_mem_t *mem;
  ICS_UINT total;

  if (!initialised)
    _ics_debug_mem_init();

  total = 0;

  _ICS_OS_MUTEX_TAKE(&ics_debug_mem_lock);

  if (!list_empty(&_ics_debug_mem))
  {
    ICS_EPRINTF(ICS_DBG, "DUMPING memory descs %p [%p.%p]\n", 
		&_ics_debug_mem,
		_ics_debug_mem.next, _ics_debug_mem.prev);
    
    list_for_each_entry(mem, &_ics_debug_mem, list)
    {
      total++;
      
      ICS_EPRINTF(ICS_DBG, "buf %p size %d caller %s:%s:%d task %p\n",
		  mem->buf,
		  mem->size,
		  mem->file,
		  mem->caller,
		  mem->line,
		  mem->task);
    }
    
    ICS_EPRINTF(ICS_DBG, "total %d totalSize %d\n", total, totalSize);
  }

  _ICS_OS_MUTEX_RELEASE(&ics_debug_mem_lock);
  
  return total;
}

#endif /* ICS_DEBUG_MEM */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
