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


/*
 * Client side interface to the Dynamic moduler loader system
 *
 */


/* 
 * Send off a request to dynamically load the module located in a memory region
 *
 * The physical address of the memory is supplied to the remote cpu who will
 * dynamically map it into their address space in order to load the module
 * hence it does not need to be from a registered region
 */
static
ICS_ERROR loadImage (ICS_UINT cpuNum, ICS_CHAR *name, ICS_CHAR *image, ICS_SIZE imageSize, ICS_UINT flags,
		     ICS_DYN parent, ICS_DYN *handlep)
{
  ICS_ERROR      err;
  ICS_OFFSET     paddr;
  ICS_MEM_FLAGS  mflags;

  ICS_assert(image);
  ICS_assert(imageSize);
  ICS_assert(ICS_PAGE_ALIGNED(image));
  ICS_assert(ICS_PAGE_ALIGNED(imageSize));

  ICS_assert(handlep);
  ICS_assert(cpuNum < _ICS_MAX_CPUS);
  ICS_assert(name);

  ICS_PRINTF(ICS_DBG_DYN, "cpuNum %d name '%s'\n",
	     cpuNum, name);

  ICS_PRINTF(ICS_DBG_DYN, "image %p imageSize %d flags 0x%x parent 0x%x\n",
	     image, imageSize, flags, parent);

  /* Convert the supplied image address to a physical one */
  err = _ICS_OS_VIRT2PHYS(image, &paddr, &mflags);
  if (err != ICS_SUCCESS)
    goto error;

  if ((mflags & ICS_CACHED))
  {
    /* Flush out the cached memory image */
    _ICS_OS_CACHE_FLUSH(image, paddr, imageSize);
  }

  /* Fire off a request to the target cpu to load the image */
  err = ics_admin_dyn_load(name, paddr, imageSize, flags, cpuNum, parent, handlep);

  if (err != ICS_SUCCESS)
    goto error;
  
  return ICS_SUCCESS;
  
error:
  ICS_EPRINTF(ICS_DBG_DYN, "Failed : %s (%d)\n",
	      ics_err_str(err), err);

  return err;
}

/*
 * Dynamically load a module from a local filename 
 */
ICS_ERROR ICS_dyn_load_file (ICS_UINT cpuNum, ICS_CHAR *fname, ICS_UINT flags,
			     ICS_DYN parent, ICS_DYN *handlep)
{
  ICS_ERROR       err = ICS_SUCCESS;

  ICS_UINT        validFlags = 0;

  ICS_CHAR       *image;
  ICS_SIZE        fsize, imageSize;

  _ICS_OS_FHANDLE file;
  ICS_SIZE        sz;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;
  
  if (fname == NULL || strlen(fname) > _ICS_MAX_PATHNAME_LEN || handlep == NULL ||  flags & ~validFlags)
    return ICS_INVALID_ARGUMENT;

  /* Target cpu must be present in the bitmask */
  if (cpuNum >= _ICS_MAX_CPUS || !((1UL << cpuNum) & ics_state->cpuMask))
    return ICS_INVALID_ARGUMENT;
    
  ICS_PRINTF(ICS_DBG_DYN, "fname '%s'\n", fname);
    
  ICS_PRINTF(ICS_DBG_DYN, "cpuNum %d flags 0x%x parent 0x%x\n",
	     cpuNum, flags, parent);

  /* Open the file on the local OS */
  if (_ICS_OS_FOPEN(fname, &file) != 0)
  {
    ICS_EPRINTF(ICS_DBG_DYN,
		"Failed to open file '%s'\n", fname);

    return ICS_NAME_NOT_FOUND;
  }

  /* Determine the total file size */
  _ICS_OS_FSIZE(file, &fsize);
  if (fsize == 0)
  {
    ICS_EPRINTF(ICS_DBG_DYN,
		"Failed to find file size of '%s'\n",
		fname);

    err = ICS_INVALID_ARGUMENT;
    goto error_close;
  }

  ICS_PRINTF(ICS_DBG_DYN, "Filesize is %d\n", fsize);

  /* Allocate a page aligned memory region to hold the image file
   * Always allocate whole numbers of ICS pages as this memory
   * region will be remapped & invalidated during load
   */
  imageSize = ALIGNUP(fsize, ICS_PAGE_SIZE);
  image = _ICS_OS_CONTIG_ALLOC(imageSize, ICS_PAGE_SIZE);
  if (image == NULL)
  {
    ICS_EPRINTF(ICS_DBG_DYN,
		"Failed to allocate image memory %d bytes\n", imageSize);

    err = ICS_ENOMEM;
    goto error_close;
  }

  ICS_PRINTF(ICS_DBG_DYN, "Loading fh 0x%x size %d into %p\n", (int) file, fsize, image);
  
  sz = _ICS_OS_FREAD(image, file, fsize);
  if (sz != fsize)
  {
    ICS_EPRINTF(ICS_DBG_DYN,
		"Failed to read file '%s' : %d\n", fname, sz);

    err = ICS_SYSTEM_ERROR;
    goto error_free;
  }

  err = loadImage(cpuNum, fname, image, imageSize, flags, parent, handlep);
  if (err != ICS_SUCCESS)
    goto error_free;

  /* Can now release the image memory */
  _ICS_OS_CONTIG_FREE(image);
  
  /* Must close the file too */
  _ICS_OS_FCLOSE(file);

  return ICS_SUCCESS;
  
error_free:
  _ICS_OS_CONTIG_FREE(image);

error_close:
  _ICS_OS_FCLOSE(file);

  ICS_EPRINTF(ICS_DBG_DYN,
	      "Failed to load '%s' : %s (%d)\n", fname,
	      ics_err_str(err), err);

  return err;
}

/*
 * Dynamically load a module from a user supplied memory buffer
 */

ICS_ERROR ICS_dyn_load_image (ICS_UINT cpuNum, ICS_CHAR *image, ICS_SIZE imageSize, ICS_UINT flags,
			      ICS_DYN parent, ICS_DYN *handlep)
{
  ICS_UINT validFlags = 0;

  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  ICS_PRINTF(ICS_DBG_DYN, "cpuNum %d image %p imageSize %d flags 0x%x parent 0x%x\n",
	     cpuNum, image, imageSize, flags, parent);

  if (image == NULL || imageSize == 0 || handlep == NULL || flags & ~validFlags)
    return ICS_INVALID_ARGUMENT;

  /* Code image and size should be page aligned */
  if (!ICS_PAGE_ALIGNED(image) || !ICS_PAGE_ALIGNED(imageSize))
    return ICS_INVALID_ARGUMENT;

  /* Target cpu must be present in the bitmask */
  if (cpuNum >= _ICS_MAX_CPUS || !((1UL << cpuNum) & ics_state->cpuMask))
    return ICS_INVALID_ARGUMENT;

  return loadImage(cpuNum, ""/*must be non NULL*/, image, imageSize, flags, parent, handlep);
}

/*
 * Unload a dynamic module
 */
ICS_ERROR ICS_dyn_unload (ICS_DYN handle)
{
  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;

  if (handle == ICS_INVALID_HANDLE_VALUE || _ICS_HDL2TYPE(handle) != _ICS_TYPE_DYN)
    return ICS_HANDLE_INVALID;

  /* Fire off a remote dynamic module unload request */
  return ics_admin_dyn_unload(handle);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
