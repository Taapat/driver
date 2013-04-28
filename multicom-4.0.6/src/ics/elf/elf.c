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

/* Define this so we can find the ".boot" section */
#define SECTION_DRIVEN

/* 
 * 
 */ 

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

#include "elf_types.h"


#define FOREACH_PHDR(elfh) { \
  const Elf32_Ehdr *ELFH = (elfh); \
  int PHNUM = Elf32Half(ELFH->e_phnum);	\
  int PHOFF = Elf32Off(ELFH->e_phoff);	\
  int PHENTSIZE = Elf32Half(ELFH->e_phentsize);	\
  int NUM; \
  for (NUM = 0; NUM < PHNUM; NUM++) { \
    const Elf32_Phdr *PHDR = (const Elf32_Phdr *)((const char *)ELFH+PHOFF+NUM*PHENTSIZE); \
    {

#define ENDEACH_PHDR \
    } \
  } \
}

#define FOREACH_SHDR(elfh) { \
  const Elf32_Ehdr *ELFH = (elfh); \
  int SHNUM = Elf32Half(ELFH->e_shnum);		\
  int SHOFF = Elf32Off(ELFH->e_shoff);		\
  int SHENTSIZE = Elf32Half(ELFH->e_shentsize);	\
  int NUM; \
  for (NUM = 0; NUM < SHNUM; NUM++) { \
    const Elf32_Shdr *SHDR = (const Elf32_Shdr *)((const char *)ELFH+SHOFF+NUM*SHENTSIZE); \
    {

#define ENDEACH_SHDR \
    } \
  } \
}

#ifdef ICS_DEBUG
/*
 * Dump out the Elf header 
 */
static void 
dumpElfHdr (Elf32_Ehdr *ehdr)
{
  ICS_PRINTF(ICS_DBG_LOAD, "Elf header %p\n", ehdr);
  ICS_PRINTF(ICS_DBG_LOAD, "  Class:\t%s (%d)\n",
	     (ehdr->e_ident[EI_CLASS] == ELFCLASS32) ? "ELF32" : "Not supported",
	     ehdr->e_ident[EI_CLASS]);
  
  ICS_PRINTF(ICS_DBG_LOAD, "  Data: \t%s (%d)\n",
	     (ehdr->e_ident[EI_DATA] == ELFDATA2LSB) ? "Little Endian" : "Not supported",
	     ehdr->e_ident[EI_DATA]);

  ICS_PRINTF(ICS_DBG_LOAD, "  Machine:\t%s (%d)\n",
	     ((Elf32Half(ehdr->e_machine) == EM_SH) ? "ST/SuperH SH" :
	      ((Elf32Half(ehdr->e_machine) == EM_ST200) ? "ST ST200" : "Not supported")),
	     ehdr->e_machine);
  
  ICS_PRINTF(ICS_DBG_LOAD, "  Type: \t%s (%d)\n",
	     (Elf32Word(ehdr->e_type) == ET_EXEC) ? "Executable" : "Not supported",
	     Elf32Word(ehdr->e_type));
  
  ICS_PRINTF(ICS_DBG_LOAD, "  Entry point:\t0x%x\n",
	     Elf32Addr(ehdr->e_entry));

  ICS_PRINTF(ICS_DBG_LOAD, "  Flags:\t0x%x\n",
	     Elf32Word(ehdr->e_flags));
  
  ICS_PRINTF(ICS_DBG_LOAD, "  Num phdrs:\t%d Num shdrs:\t%d\n",
	     Elf32Half(ehdr->e_phnum), Elf32Half(ehdr->e_shnum));
}
#endif /* DEBUG */

ICS_ERROR ics_elf_check_magic (ICS_CHAR *hdr)
{
  if (hdr[EI_MAG0] != ELFMAG0 ||
      hdr[EI_MAG1] != ELFMAG1 ||
      hdr[EI_MAG2] != ELFMAG2 ||
      hdr[EI_MAG3] != ELFMAG3) {
    return ICS_INVALID_ARGUMENT;
  }

  return ICS_SUCCESS;
}

/* Validate some of the Elf header values */
ICS_ERROR ics_elf_check_hdr (ICS_CHAR *image)
{
  Elf32_Ehdr *ehdr;

  ehdr = (Elf32_Ehdr *) image;

  if (ics_elf_check_magic(image) != ICS_SUCCESS)
    goto error_magic;

#ifdef ICS_DEBUG
  dumpElfHdr(ehdr);
#endif

  /* 32-bit binaries only */
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
    goto error_class;

  /* Check we have a little endian file */
  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    goto error_endian;

  /* Check ELF version */
  if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
    goto error_version;

  /* Check OS/ABI */
  if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    goto error_osabi;

  /* Check for support machine types */
  if ((Elf32Half(ehdr->e_machine) != EM_SH) && (Elf32Half(ehdr->e_machine) != EM_ST200))
    goto error_machine;

  /* We only support exectutables files */
  if (Elf32Half(ehdr->e_type) != ET_EXEC)
    goto error_type;

  return ICS_SUCCESS;

error_magic:
error_class:
error_endian:
error_machine:
error_type:
error_version:
error_osabi:
  
  return ICS_INVALID_ARGUMENT;
}

ICS_OFFSET ics_elf_entry (ICS_CHAR *image)
{
  Elf32_Ehdr *ehdr;

  ehdr = (Elf32_Ehdr *) image;

  return Elf32Addr(ehdr->e_entry);
}

/* Lookup a string from the section header string table ".shstrtab" */
const char *ics_elf_shstr (Elf32_Ehdr *ehdr, Elf32_Word off)
{
  int        shstrndx    = Elf32Half(ehdr->e_shstrndx);		/* shstrtab section number */
  Elf32_Off  shoff       = Elf32Off(ehdr->e_shoff);		/* section table offset */
  Elf32_Word shentsize   = Elf32Half(ehdr->e_shentsize);	/* size of each shdr entry */

  const Elf32_Shdr *shstrsec;
  const char       *shstrtab;

  /* Calculate the base of the shstrtab section header */
  shstrsec        = (const Elf32_Shdr *)((const char *)ehdr + shoff + (shstrndx * shentsize));

  /* Calculate the base of the ststrtab string table */
  shstrtab        = (const char *)ehdr + Elf32Off(shstrsec->sh_offset);
  
  /* Return the requested string from within the table */
  return shstrtab + off;
}

ICS_ERROR ics_elf_load_size (ICS_CHAR *image, ICS_OFFSET *basep, ICS_SIZE *sizep, ICS_SIZE *alignp, ICS_OFFSET *bootp)
{
  Elf32_Ehdr *ehdr;
  ICS_OFFSET  baseAddr = -1, topAddr = 0, bootAddr = -1;
  
  ICS_SIZE    align = 0;

  ehdr = (Elf32_Ehdr *) image;

#ifdef SECTION_DRIVEN

  FOREACH_SHDR(ehdr) {
    Elf32_Addr sh_addr  = Elf32Addr(SHDR->sh_addr);
    Elf32_Word sh_size  = Elf32Word(SHDR->sh_size);
    Elf32_Word sh_align = Elf32Word(SHDR->sh_addralign);

    Elf32_Word sh_addr_align;

    if (!(Elf32Word(SHDR->sh_flags) & SHF_ALLOC) || Elf32Word(sh_size) == 0)
      /* Only consider non-zero SHF_ALLOC sections */
      continue;
    
    /* Find the lowest (aligned) section address */
    sh_addr_align = sh_align ? sh_addr & ~(sh_align-1) : sh_addr;
    
    if (sh_addr_align < baseAddr)
      baseAddr = sh_addr_align;
    
    /* Calculate the highest section end address */
    if (sh_addr + sh_size > topAddr)
      topAddr = sh_addr + sh_size;
    
    /* Find the largest alignment constraint */
    if (sh_align > align)
      align = sh_align;

    /* Hack to support older toolkits where the ELF start
     * address is invalid and instead we need to jump to the
     * ".boot" section start address
     */
    if (strcmp(".boot", ics_elf_shstr(ehdr, Elf32Off(SHDR->sh_name))) == 0)
	bootAddr = sh_addr;

  } ENDEACH_SHDR;
  
#else

  FOREACH_PHDR(ehdr) {
    Elf32_Addr ph_paddr = Elf32Addr(PHDR->p_paddr);
    Elf32_Word ph_memsz = Elf32Word(PHDR->p_memsz);
    Elf32_Word ph_align = Elf32Word(PHDR->p_align); 
    
    if (Elf32Word(PHDR->p_type) != PT_LOAD || ph_memsz == 0)
      /* Only consider non-zero sized PT_LOAD segments */
      continue;

    /* Find the lowest section address */
    if (ph_paddr < baseAddr)
      baseAddr = ph_paddr;
    
    /* Calculate the highest section end address */
    if (ph_paddr + ph_memsz > topAddr)
      topAddr = ph_paddr + ph_memsz;
    
    /* Find the largest alignment constraint */
    if (ph_align > align)
      align = ph_align;
    
  } ENDEACH_PHDR;

#endif /* SECTION_DRIVEN */

  ICS_assert(baseAddr != -1);
  ICS_assert(topAddr > baseAddr);

  *sizep  = topAddr - baseAddr;
  *alignp = align;

  /* On 29-bit sh4 we need to convert P1 addresses
   * to be true physical ones
   */
#if defined(__sh__)
  if (Elf32Half(ehdr->e_machine) == EM_SH)
  {
    ICS_ERROR     err;
    ICS_MEM_FLAGS mflags;
    ICS_OFFSET    paddr;
    
    err = _ICS_OS_VIRT2PHYS((void *)baseAddr, &paddr, &mflags);
    if (err != ICS_SUCCESS)
    {
      ICS_EPRINTF(ICS_DBG_LOAD, "Failed to convert addr 0x%lx to physical\n",
		  baseAddr);
      return err;
    }

    baseAddr = paddr;
  }
#endif /* __sh__ */

  *basep  = baseAddr;

  /* Return bootAddr from ".boot" section (may be -1) */
  *bootp  = bootAddr;

  ICS_PRINTF(ICS_DBG_LOAD, "image %p : base 0x%x size %d align %d boot 0x%x\n",
	     image, *basep, *sizep, *alignp, *bootp);

  return ICS_SUCCESS;
}

ICS_ERROR ics_elf_load_image (ICS_CHAR *image, ICS_VOID *mem, ICS_SIZE memSize,
			      ICS_OFFSET baseAddr)
{
  Elf32_Ehdr *ehdr;

  ICS_PRINTF(ICS_DBG_LOAD, "image %p mem %p memSize %d baseAddr 0x%x\n",
	     image, mem, memSize, baseAddr);
  
  if (image == NULL || mem == NULL || memSize == 0)
    return ICS_INVALID_ARGUMENT;
  
  ehdr = (Elf32_Ehdr *) image;

  /*
   * Drive the load from the program header table
   */
  FOREACH_PHDR(ehdr) {
    Elf32_Addr ph_paddr  = Elf32Addr(PHDR->p_paddr);
    Elf32_Addr ph_offset = Elf32Addr(PHDR->p_offset);
    Elf32_Word ph_memsz  = Elf32Word(PHDR->p_memsz);
    Elf32_Word ph_filesz = Elf32Word(PHDR->p_filesz); 
    
    ICS_OFFSET offset;

    if (Elf32Word(PHDR->p_type) != PT_LOAD || ph_memsz == 0)
      /* Only consider non-zero sized PT_LOAD segments */
      continue;

    /* On 29-bit sh4 we need to convert P1 addresses
     * to true physical ones
     */
#if defined(__sh__)
    if (Elf32Half(ehdr->e_machine) == EM_SH)
    {
      ICS_ERROR     err;
      ICS_MEM_FLAGS mflags;
      ICS_OFFSET    paddr;
      
      err = _ICS_OS_VIRT2PHYS((void *)ph_paddr, &paddr, &mflags);
      if (err != ICS_SUCCESS)
      {
	ICS_EPRINTF(ICS_DBG_LOAD, "Failed to convert addr 0x%lx to physical\n",
		    ph_paddr);
	return err;
      }
      
      ph_paddr = paddr;
    }
#endif

    /* Now generate the memory load offset */
    offset = ph_paddr - baseAddr;

    ICS_PRINTF(ICS_DBG_LOAD, "Code: off 0x%x ph_offset 0x%x filesz %d\n", offset, ph_offset, ph_filesz);
    
    ICS_assert(offset < memSize);

    /* Copy the code from the image to the memory region */
    Elf32Copy((void *)((ICS_OFFSET)mem + offset), (void *)((ICS_OFFSET)image + ph_offset), ph_filesz);

#ifdef ZERO_BSS
    /* XXXX Do we need to do this or does crt0 do it for us ? */
    /* Zero any BSS regions */
    if (ph_filesz < ph_memsz)
    {
      Elf32_Word bss = ph_memsz - ph_filesz;

      /* Skip over the code bit */
      offset += ph_filesz;

      ICS_assert(offset+bss <= memSize);
      
      ICS_PRINTF(ICS_DBG_LOAD, "BSS section to %p-%p\n", mem + offset, mem + offset + bss);
      _ICS_OS_MEMSET((void *)((ICS_OFFSET)mem + offset), 0, bss);
    }
#endif
    
  }  ENDEACH_PHDR;

  return ICS_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

