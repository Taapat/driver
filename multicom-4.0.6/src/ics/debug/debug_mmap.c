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

#ifdef ICS_DEBUG_MMAP

/* 
 * 
 */ 

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

/*
 * Simple debug mmap allocation wrapper to detect
 * memory leaks from within ICS
 */

static int initialised = 0;

static ICS_ULONG totalSize = 0;

static LIST_HEAD(_ics_debug_mmap_list);
static _ICS_OS_MUTEX ics_debug_mmap_lock;

typedef struct debug_mmap
{
  struct list_head 	 list;		/* Doubly linked list */
  
  const char		*file;		/* Allocating file */
  const char		*caller;	/* Allocating function */
  int			 line;		/* Line number of function */
  _ICS_OS_TASK		*task;		/* Allocating task */
  
  ICS_OFFSET		 paddr;		/* Physical base of mmapping */
  size_t		 size;		/* Size of allocation */

  char			*mem;		/* Actual address returned */
} debug_mmap_t;

void *_ics_debug_mmap (ICS_OFFSET paddr, size_t size, ICS_UINT flags, 
		       const char *file, const char *caller, int line, _ICS_OS_TASK *task)
{
  debug_mmap_t *mmap;

  if (!initialised)
    _ics_debug_mmap_init();

  /* Allocate desc */
  mmap = _ICS_OS_MALLOC(sizeof(debug_mmap_t));
  if (mmap == NULL)
    return NULL;
  
  mmap->mem = _ics_os_mmap(paddr, size, flags);
  if (mmap->mem == NULL)
  {
    _ICS_OS_FREE(mmap);
    return NULL;
  }

  mmap->file   = strrchr(file, '/') ? strrchr(file, '/') + 1 : file; 
  mmap->caller = caller;
  mmap->line   = line;
  mmap->task   = task;
  mmap->paddr  = paddr;
  mmap->size   = size;

  _ICS_OS_MUTEX_TAKE(&ics_debug_mmap_lock);
  
  list_add(&mmap->list, &_ics_debug_mmap_list);

  totalSize += size;

  _ICS_OS_MUTEX_RELEASE(&ics_debug_mmap_lock);

  return &mmap->mem[0];
}

int _ics_debug_munmap (void *mem, const char *file, const char *caller, int line)
{
  debug_mmap_t *mmap, *tmp;

  if (!initialised)
    _ics_debug_mmap_init();

  _ICS_OS_MUTEX_TAKE(&ics_debug_mmap_lock);

  list_for_each_entry_safe(mmap, tmp, &_ics_debug_mmap_list, list)
  {
    if (mem == mmap->mem)
    {
      /* Found a match, remove from the list */
      list_del_init(&mmap->list);

      totalSize -= mmap->size;

      _ICS_OS_MUTEX_RELEASE(&ics_debug_mmap_lock);

      /* Free of mmapped memory */
      _ics_os_munmap(mem);

      /* Now free mmap desc */
      _ICS_OS_FREE(mmap);
      
      return 0;
    }
  }

  ICS_EPRINTF(ICS_DBG, "ERROR: Free mem %p not found caller %s:%s:%d\n", mem, file, caller, line);

  _ICS_OS_MUTEX_RELEASE(&ics_debug_mmap_lock);

  return 1;
}

void _ics_debug_mmap_init (void)
{
  initialised = 1;

  (void) _ICS_OS_MUTEX_INIT(&ics_debug_mmap_lock);
}

size_t _ics_debug_mmap_total (void)
{
  return totalSize;
}

/*
 * Dump out all the entries on the debug mem list
 */
ICS_UINT _ics_debug_mmap_dump (void)
{
  debug_mmap_t *mmap;
  ICS_UINT total;

  if (!initialised)
    _ics_debug_mmap_init();

  total = 0;

  _ICS_OS_MUTEX_TAKE(&ics_debug_mmap_lock);

  if (!list_empty(&_ics_debug_mmap_list))
  {
    ICS_EPRINTF(ICS_DBG, "DUMPING mmap descs %p [%p.%p]\n", 
		&_ics_debug_mmap_list,
		_ics_debug_mmap_list.next, _ics_debug_mmap_list.prev);
    
    list_for_each_entry(mmap, &_ics_debug_mmap_list, list)
    {
      total++;
      
      ICS_EPRINTF(ICS_DBG, "mem %p paddr %lx size %zi caller %s:%s:%d task %p\n",
		  mmap->mem,
		  mmap->paddr,
		  mmap->size,
		  mmap->file,
		  mmap->caller,
		  mmap->line,
		  mmap->task);
    }
    
    ICS_EPRINTF(ICS_DBG, "total %d totalSize %ld\n", total, totalSize);
  }

  _ICS_OS_MUTEX_RELEASE(&ics_debug_mmap_lock);
  
  return total;
}

#endif /* ICS_DEBUG_MMAP */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
