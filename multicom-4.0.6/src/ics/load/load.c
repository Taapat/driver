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
 * Host side code for loading Elf firmware
 */


/*
 * Take an Elf image in memory and unpack it according to the Elf header info
 */
static
ICS_ERROR loadElfImage (ICS_CHAR *image, ICS_UINT flags, ICS_OFFSET *entryAddrp)
{
  ICS_ERROR  err;

  ICS_OFFSET baseAddr, bootAddr, entryAddr;
  ICS_SIZE   loadSize, loadAlign;
  ICS_VOID  *loadBase = NULL;

  ICS_assert(image);

  /* Check the validity of the Elf image */
  if ((err = ics_elf_check_magic(image)) != ICS_SUCCESS)
  {
    goto error;
  }

  /* Is the Elf header valid and compatible? */
  if ((err = ics_elf_check_hdr(image)) != ICS_SUCCESS)
  {
    goto error;
  }

  /* Determine the base physical address and memory size to load this image file */
  if ((err = ics_elf_load_size(image, &baseAddr, &loadSize, &loadAlign, &bootAddr)) != ICS_SUCCESS)
  {
    goto error;
  }

  /* Map memory (cached) at the correct address for loading the image */
  loadBase = _ICS_OS_MMAP(baseAddr, loadSize, ICS_TRUE);
  if (loadBase == NULL)
  {
    ICS_EPRINTF(ICS_DBG_LOAD, "OS_MMAP(0x%lx, %d) failed\n",
		baseAddr, loadSize);
    err = ICS_SYSTEM_ERROR;
    goto error;
  }

  /* Purge any old cache entries for this newly mapped memory */
  _ICS_OS_CACHE_PURGE(loadBase, baseAddr, loadSize);

  /* Load the Elf image into the target memory */
  if ((err = ics_elf_load_image(image, loadBase, loadSize, baseAddr)) != ICS_SUCCESS)
  {
    goto error_unmap;
  }

  /* Purge the cache of the loaded code image */
  _ICS_OS_CACHE_PURGE(loadBase, baseAddr, loadSize);

  /* Unmap the target memory */
  _ICS_OS_MUNMAP(loadBase);

  /* Grab the Elf entry address */
  if (bootAddr != -1)
    /* Hack to support older compilers which have an invalid ELF entry address */
    entryAddr = bootAddr;
  else
    entryAddr = ics_elf_entry(image);

  ICS_assert(entryAddr);

  ICS_PRINTF(ICS_DBG_LOAD, "loadBase %p loadSize %d baseAddr 0x%x entryAddr 0x%x\n",
	     loadBase, loadSize, baseAddr, entryAddr);

  /* Pass back the entry address to caller */
  *entryAddrp = entryAddr;
  
  return ICS_SUCCESS;

error_unmap:
  _ICS_OS_MUNMAP(loadBase);
  
error:
  ICS_EPRINTF(ICS_DBG_LOAD, "Failed : %s(%d)\n",
	      ics_err_str(err), err);

  return err;
}

/*
 * Load an Elf image from a file
 */
ICS_ERROR ics_load_elf_file (const ICS_CHAR *fname, ICS_UINT flags, ICS_OFFSET *entryAddrp)
{
  ICS_ERROR       err = ICS_INVALID_ARGUMENT;

  ICS_UINT        validFlags = 0;

  ICS_CHAR       *image;
  ICS_SIZE        fsize;    

  _ICS_OS_FHANDLE file;
  ICS_SIZE        sz;
  
  int             res;

  if (fname == NULL || entryAddrp == NULL || (flags & ~validFlags))
    return ICS_INVALID_ARGUMENT;

  ICS_PRINTF(ICS_DBG_LOAD, "fname '%s'\n", fname);
    
  ICS_PRINTF(ICS_DBG_LOAD, "flags 0x%x\n", flags);
  
  /* Open the file on the local OS */
  if ((res = _ICS_OS_FOPEN(fname, &file)) != 0)
  {
    ICS_EPRINTF(ICS_DBG_LOAD,
		"Failed to open file '%s' : %d\n", fname, res);

    return ICS_NAME_NOT_FOUND;
  }

  /* Determine the total file size */
  _ICS_OS_FSIZE(file, &fsize);
  if (fsize == 0)
  {
    ICS_EPRINTF(ICS_DBG_LOAD,
		"Failed to find file size of '%s'\n",
		fname);

    err = ICS_INVALID_ARGUMENT;
    goto error_close;
  }

  ICS_PRINTF(ICS_DBG_LOAD, "Filesize is %d\n", fsize);

  /* Allocate some memory to hold the Elf file image */
  image = _ICS_OS_MALLOC(fsize);

  if (image == NULL)
  {
    ICS_EPRINTF(ICS_DBG_LOAD,
		"Failed to allocate image memory %d bytes\n", fsize);

    err = ICS_ENOMEM;
    goto error_close;
  }

  ICS_PRINTF(ICS_DBG_LOAD, "Loading fh 0x%x size %d into %p\n", (int) file, fsize, image);
  
  sz = _ICS_OS_FREAD(image, file, fsize);
  if (sz != fsize)
  {
    ICS_EPRINTF(ICS_DBG_LOAD,
		"Failed to read file '%s' : %d\n", fname, sz);
    
    err = ICS_SYSTEM_ERROR;
    goto error_free;
  }

  /* Now load the Elf image from the memory image, returning the entry address */
  err = loadElfImage(image, flags, entryAddrp);

  /* Don't need these anymore */
  _ICS_OS_FREE(image);
  _ICS_OS_FCLOSE(file);
  
  return err;

error_free:
  _ICS_OS_FREE(image);

error_close:
  _ICS_OS_FCLOSE(file);
    
  return err;
}

/*
 * Load an Elf image from a memory region
 */
ICS_ERROR ics_load_elf_image (ICS_CHAR *image, ICS_UINT flags, ICS_OFFSET *entryAddrp)
{
  ICS_UINT validFlags = 0;

  if (image == NULL)
    return ICS_INVALID_ARGUMENT;
  
  if (entryAddrp == NULL || (flags & ~validFlags))
    return ICS_INVALID_ARGUMENT;

  /* Load the Elf image from a memory image, returning the entry address */
  return loadElfImage(image, flags, entryAddrp);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

