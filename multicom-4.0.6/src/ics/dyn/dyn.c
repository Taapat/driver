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

#if defined(__os21__)

#include <rl_lib.h>	/* The OS21 dynamic loader system */

#if 0
/* XXXX This does not work as weak symbols are not overriden when
 * linked against libraries which declare the same symbols
 */
/*
 * Declare weak stubs for all the rl_lib calls we make so that
 * users can link against ICS without needing to use the --rmain option
 * if they don't require the dynamic module support
 */
extern rl_handle_t *rl_handle_new(const rl_handle_t *parent, int mode) __attribute__((weak));
extern int rl_handle_delete(rl_handle_t *handle)  __attribute__((weak));
extern rl_handle_t *rl_this_ __attribute__((weak));

extern void *rl_sym(rl_handle_t *handle, const char *name) __attribute__((weak));
extern int rl_load_buffer(rl_handle_t *handle, const char *image) __attribute__((weak));
extern int rl_unload(rl_handle_t *handle) __attribute__((weak));
extern int rl_set_file_name(rl_handle_t *handle, const char *f_name) __attribute__((weak));
extern int rl_errno(rl_handle_t *handle) __attribute__((weak));
extern char *rl_errstr(rl_handle_t *handle) __attribute__((weak));

rl_handle_t *rl_this_;

rl_handle_t *rl_handle_new (const rl_handle_t *parent, int mode)
{
  return NULL;
}

extern int rl_handle_delete (rl_handle_t *handle)
{
  return 0;
}

int rl_load_buffer (rl_handle_t *handle, const char *image)
{
  return 0;
}

int rl_unload (rl_handle_t *handle)
{
  return 0;
}

void *rl_sym (rl_handle_t *handle, const char *name)
{
  return NULL;
}

int rl_set_file_name(rl_handle_t *handle, const char *f_name)
{
  return 0;
}

int rl_errno(rl_handle_t *handle)
{
  return 0;
}

char *rl_errstr(rl_handle_t *handle)
{
  return NULL;
}
#endif

/*
 * Companion side interface to the Dynamic module loader system
 * Called by the admin task in response to load/unload requests
 *
 * OS21 only
 *
 */

typedef ICS_ERROR (PF)(void);

/* ISO C does not allow a void * to be converted to a function pointer */
typedef union
{
  void *entry;
  PF   *function;
} iso_wrapper_t;

/*
 * Lookup the module_init() symbol and execute it if present
 */
static 
ICS_ERROR moduleInit (ics_dyn_mod_t *dmod)
{
  iso_wrapper_t dsym;

  dsym.entry = rl_sym(dmod->handle, _ICS_DYN_INIT_NAME);
  if (dsym.entry == NULL)
  {
    /* We assume this is because it doesn't have a module init function 
     * and hence don't treat it as an error
     */
    return ICS_SUCCESS;
  }

  ICS_PRINTF(ICS_DBG_DYN, 
	     "Calling module fn '%s' @ %p\n", _ICS_DYN_INIT_NAME, dsym.entry);

  /* Call the dynamic function */
  return (*dsym.function)();
}

static 
ICS_ERROR moduleTerm (ics_dyn_mod_t *dmod)
{
  iso_wrapper_t dsym;

  dsym.entry = rl_sym(dmod->handle, _ICS_DYN_TERM_NAME);
  if (dsym.entry == NULL)
  {
    /* We assume this is because it doesn't have a module init function 
     * and hence don't treat it as an error
     */
    return ICS_SUCCESS;
  }

  ICS_PRINTF(ICS_DBG_DYN, 
	     "Calling module fn '%s' @ %p\n", _ICS_DYN_TERM_NAME, dsym.entry);

  return (*dsym.function)();
}

/*
 * Find a free slot in the dynamic module array
 */
static
int findFreeDmod (void)
{
  int i;

  ICS_assert(ics_state);

  for (i = 0; i < _ICS_MAX_DYN_MOD; i++)
  {
    /* Look for either a free slot or a free desc */
    if (ics_state->dyn[i].handle == NULL)
      return i;
  }

  /* No free slot found */
  return -1;
}

/*
 * Load a module into the OS using the rl module system
 *
 * We decode the supplied parent handle to find the rl_handle of the
 * parent module if it is supplied. This allows the symbols to be
 * be inherited from the parent to the child. For example the parent
 * could be the MME shared module or a dynamic library shared by
 * multiple transformers
 *
 * The new rl_handle is returned in rlhandep
 */
static
ICS_ERROR loadModule (ICS_CHAR *name, ICS_VOID *image, ICS_SIZE imageSize, 
		      ICS_DYN parent, rl_handle_t **rlhandlep)
{
  ICS_ERROR    err = ICS_SUCCESS;

  rl_handle_t *rlhandle, *rlparent = NULL;

  /* Decode and lookup parent handle if supplied */
  if (parent == 0)
  {
    /* Parent is the main ICS module */
    rlparent = rl_this();
  }
  else
  {
    int            type, cpuNum, ver, pidx;
    ics_dyn_mod_t *pmod;

    /* Decode the dynamic module handle */
    _ICS_DECODE_HDL(parent, type, cpuNum, ver, pidx);
    
    if (type != _ICS_TYPE_DYN || cpuNum >= _ICS_MAX_CPUS || pidx >= _ICS_MAX_DYN_MOD)
    {
      err = ICS_HANDLE_INVALID;
      goto error;
    }
    
    if (cpuNum != ics_state->cpuNum)
    {
      /* Can only specify local modules */
      err = ICS_HANDLE_INVALID;
      goto error;
    }

    pmod = &ics_state->dyn[pidx];

    if (pmod->handle == NULL)
    {
      /* Parent module is not valid */
      err = ICS_HANDLE_INVALID;
      goto error;
    }
    
    rlparent = pmod->handle;
  }

  ICS_assert(rlparent);

  /* Allocate an rl handle for this image */
  rlhandle = rl_handle_new(rlparent, 0);
  if (rlhandle == NULL)
  {
    err = ICS_ENOMEM;
    goto error;
  }

  ICS_PRINTF(ICS_DBG_DYN, "Created rlhandle %p parent %p\n", rlhandle, rlparent);

  /* Use the supplied module name as the debugger filename (may be '\0') */
  ICS_PRINTF(ICS_DBG_DYN, "Setting debugger filename to '%s'\n", name);
  rl_set_file_name(rlhandle, name);

  /* Perform the dynamic load */
  ICS_PRINTF(ICS_DBG_DYN, "Calling rl_load_buffer(0x%x, %p)\n",
	     rlhandle, image);

  err = rl_load_buffer(rlhandle, image);
  if (err != 0)
  {
    ICS_EPRINTF(ICS_DBG_DYN, 
		"rl_load_buffer() failed : %s (%d)\n",
		rl_errstr(rlhandle), rl_errno(rlhandle));

    /* Could be due to a missing symbol, perhaps be more elaborate */
    err = ICS_SYSTEM_ERROR;
    goto error_release;
  }

  /* Return allocated rl_handle to caller */
  *rlhandlep = rlhandle;
  
  return ICS_SUCCESS;

error_release:

  /* Release the rl handle */
  (void) rl_handle_delete(rlhandle);

error:
  
  ICS_EPRINTF(ICS_DBG_DYN, "Failed image %p imageSize %d parent 0x%x : %s (%d)\n",
	      image, imageSize, parent,
	      ics_err_str(err), err);
  
  return err;
}

/*
 * Dynamically load the module supplied in the physical address segment
 *
 * The physical memory is temporarily mapped into virtual address space to
 * perform the dynamic load. It is unmapped on completion.
 * This means that the physical address supplied need not be part of one
 * of the registered ICS memory regions which makes sizing of the ICS heaps
 * a little easier.
 *
 * MULTITHREAD SAFE: Single threaded - called from admin thread
 */
ICS_ERROR ics_dyn_load_image (ICS_CHAR *name, ICS_OFFSET paddr, ICS_SIZE imageSize, ICS_UINT flags,
			      ICS_DYN parent, ICS_DYN *handlep)
{
  ICS_ERROR      err = ICS_SUCCESS;
  void          *image;
  
  ics_dyn_mod_t *dmod;

  int            idx;
  rl_handle_t   *rlhandle;

  ICS_assert(ics_state);
  ICS_assert(paddr);
  ICS_assert(imageSize);
  ICS_assert(handlep);
  ICS_assert(name && strlen(name) <= _ICS_MAX_PATHNAME_LEN);

  ICS_PRINTF(ICS_DBG_DYN, "name '%s'\n", name);

  ICS_PRINTF(ICS_DBG_DYN,
	     "paddr 0x%x imageSize %d flags 0x%x parent 0x%x\n",
	     paddr, imageSize, flags, parent);

  /* Find a free dynamic module slot */
  idx = findFreeDmod();
  if (idx == -1)
    return ICS_ENOMEM;

  ICS_ASSERT(idx < _ICS_MAX_DYN_MOD);
 
  dmod = &ics_state->dyn[idx];

  /* Always attempt to map in whole number of pages */
  imageSize = ALIGNUP(imageSize, ICS_PAGE_SIZE);

  /* Create a cached mapping of the image segment */
  image = _ICS_OS_MMAP(paddr, imageSize, ICS_TRUE);
  if (image == NULL)
  {
    ICS_EPRINTF(ICS_DBG_DYN,
		"Failed to map in paddr 0x%x size %d\n", paddr, imageSize);

    return ICS_SYSTEM_ERROR;
  }
  ICS_PRINTF(ICS_DBG_DYN, "Mapped in image @ %p size %d\n", image, imageSize);

  /* Purge any old cache entries for this newly mapped image */
  _ICS_OS_CACHE_PURGE(image, paddr, imageSize);

  err = loadModule(name, image, imageSize, parent, &rlhandle);
  if (err != ICS_SUCCESS)
    goto error_unmap;

  /* We can now unmap the region */
  (void) _ICS_OS_MUNMAP(image);

  ICS_PRINTF(ICS_DBG_DYN,
	     "loaded '%s' rlhandle %p idx %d\n",
	     name, rlhandle, idx);

  /* Fill out (and claim) the dmod entry */
  strcpy(dmod->name, name); /* Will copy the '\0' */
  dmod->handle = rlhandle;
  dmod->parent = parent;

  /* Now run any module init routines */
  err = moduleInit(dmod);

  /* Generate a new dynamic module handle */
  *handlep = _ICS_HANDLE(_ICS_TYPE_DYN, ics_state->cpuNum, 0, idx);

  ICS_PRINTF(ICS_DBG_DYN,
	     "Returning handle 0x%x : err %d\n", *handlep, err);

  /* Return err code from moduleInit() */
  return err;

error_unmap:
  ICS_EPRINTF(ICS_DBG_DYN,
	      "Failed to load '%s' parent 0x%x : %s (%d)\n",
	      name, parent,
	      ics_err_str(err), err);

  (void) _ICS_OS_MUNMAP(image);

  return err;
}

/* 
 * Unload a dynamic module based on the supplied handle
 *
 * MULTITHREAD SAFE: Single threaded - called from admin thread
 */
ICS_ERROR
ics_dyn_unload (ICS_DYN handle)
{
  int            err;

  int            type, cpuNum, ver, idx;
  ics_dyn_mod_t *dmod;

  ICS_assert(ics_state);
 
  /* Decode the dynamic module handle */
  _ICS_DECODE_HDL(handle, type, cpuNum, ver, idx);

  if (type != _ICS_TYPE_DYN || cpuNum >= _ICS_MAX_CPUS || idx >= _ICS_MAX_DYN_MOD)
    return ICS_HANDLE_INVALID;

  if (cpuNum != ics_state->cpuNum)
    /* Can only unload local modules */
    return ICS_HANDLE_INVALID;
  
  /* Used idx to reference dynamic module table */
  dmod = &ics_state->dyn[idx];

  if (dmod->handle == NULL)
    return ICS_HANDLE_INVALID;

  /* First run any module term routines */
  err = moduleTerm(dmod);

  /* XXXX Should we ignore this user return code and unload anyway ? */
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_DYN, 
		"moduleTerm() failed : %d\n", err);
    return err;
  }

  /* unload the dynamic module */
  err = rl_unload(dmod->handle);
  if (err != 0)
  {
    ICS_EPRINTF(ICS_DBG_DYN, 
		"rl_unload(%p) failed : %s (%d)\n",
		dmod->handle, rl_errstr(dmod->handle), rl_errno(dmod->handle));
    return ICS_SYSTEM_ERROR;
  }

  ICS_PRINTF(ICS_DBG_DYN, "unloaded '%s' handle %p\n",
	     dmod->name, dmod->handle);

  /* Release the rl handle */
  (void) rl_handle_delete(dmod->handle);

  /* Mark module slot as being free */
  dmod->name[0] = '\0';
  dmod->handle = NULL;
  dmod->parent = ICS_INVALID_HANDLE_VALUE;

  return ICS_SUCCESS;
}

#else

/* 
 * Dynamic modules not supported
 */

ICS_ERROR ics_dyn_load_image (ICS_CHAR *name, ICS_OFFSET paddr, ICS_SIZE imageSize, ICS_UINT flags,
			      ICS_DYN parent, ICS_DYN *handlep)
{
  return ICS_NOT_INITIALIZED;
}

ICS_ERROR ics_dyn_unload (ICS_DYN handle)
{
  return ICS_NOT_INITIALIZED;
}

#endif

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
