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

#ifndef _ELF_TYPES_H
#define	_ELF_TYPES_H

/*
 * 32-bit OS Basic ELF types
 */

#if defined(__linux__) && !defined(__KERNEL__)

#include <elf.h>

#define EM_ST200	100		/* ST Micro ST200 cpu */

#elif defined(__KERNEL__)

#include <linux/elf.h>

#define EM_ST200	100		/* ST Micro ST200 cpu */
#define ELFOSABI_SYSV	0		/* UNIX System V ABI */

#else

typedef unsigned int	Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned int	Elf32_Off;
typedef int		Elf32_Sword;
typedef unsigned int	Elf32_Word;

/*
 * ELF header (at beginning of file)
 */
#define	EI_NIDENT	16

typedef struct {
  unsigned char	e_ident[EI_NIDENT];	/* magic bytes */
  Elf32_Half		e_type;		/* file type */
  Elf32_Half		e_machine;	/* target machine */
  Elf32_Word		e_version;	/* file version */
  Elf32_Addr		e_entry;	/* start address */
  Elf32_Off		e_phoff;	/* Program hdr file offset */
  Elf32_Off		e_shoff;	/* Section hdr file offset */
  Elf32_Word		e_flags;	/* file flags */
  Elf32_Half		e_ehsize;	/* sizeof Elf hdr */
  Elf32_Half		e_phentsize;	/* sizeof Program hdr */
  Elf32_Half		e_phnum;	/* number Program hdrs */
  Elf32_Half		e_shentsize;	/* sizeof Section hdr */
  Elf32_Half		e_shnum;	/* number Section hdrs */
  Elf32_Half		e_shstrndx;	/* Section hdr string index */
} Elf32_Ehdr;

#define	EV_NONE		0		/* e_version, EI_VERSION */
#define	EV_CURRENT	1
#define	EV_NUM		2

#define	EI_MAG0		0		/* e_ident[] indexes */
#define	EI_MAG1		1
#define	EI_MAG2		2
#define	EI_MAG3		3
#define	EI_CLASS	4
#define	EI_DATA		5
#define	EI_VERSION	6
#define	EI_OSABI	7
#define	EI_PAD		8

#define	ELFMAG0		0x7f		/* ELF magic */
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

#define	ELFCLASSNONE	0		/* e_ident[EI_CLASS] */
#define	ELFCLASS32	1
#define	ELFCLASS64	2
#define	ELFCLASSNUM	3

#define ELFDATANONE	0		/* e_ident[EI_DATA] */
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

#define ELFOSABI_NONE	0		/* e_ident[EI_OSABI] */
#define ELFOSABI_SYSV	0		/* UNIX System V ABI */

/*
 * Elf types (e_type)
 */
#define ET_NONE		0		/* No file type */
#define ET_REL		1		/* Relocatable file */
#define ET_EXEC		2		/* Executable file */
#define ET_DYN		3		/* Shared object file */
#define ET_CORE		4		/* Core file */
#define	ET_NUM		5		/* Number of defined types */

/*
 * Elf machine types (e_machine)
 * Supported types only
 */
#define EM_NONE		  0		/* No machine */
#define EM_SH		 42		/* ST Micro ST40/SuperH */
#define EM_ST200	100		/* ST Micro ST200 cpu */

/*
 * Program header
 */
typedef struct {
  Elf32_Word		p_type;		/* entry type */
  Elf32_Off		p_offset;	/* file offset */
  Elf32_Addr		p_vaddr;	/* virtual address */
  Elf32_Addr		p_paddr;	/* physical address */
  Elf32_Word		p_filesz;	/* file size */
  Elf32_Word		p_memsz;	/* memory size */
  Elf32_Word		p_flags;	/* entry flags */
  Elf32_Word		p_align;	/* memory/file alignment */
} Elf32_Phdr;

/*
 * Program header type (p_type)
 */
#define	PT_NULL		0		/* Program header table entry unused */
#define PT_LOAD		1		/* Loadable program segment */
#define PT_DYNAMIC	2		/* Dynamic linking information */
#define PT_INTERP	3		/* Program interpreter */
#define PT_NOTE		4		/* Auxiliary information */
#define PT_SHLIB	5		/* Reserved */
#define PT_PHDR		6		/* Entry for header table itself */
#define PT_TLS		7		/* Thread-local storage segment */
#define	PT_NUM		8		/* Number of defined types */

/*
 * Section header
 */
typedef struct {
  Elf32_Word		sh_name;	/* section name */
  Elf32_Word		sh_type;	/* section type (SHT_)*/
  Elf32_Word		sh_flags;	/* section flags (SHF_) */
  Elf32_Addr		sh_addr;	/* virtual address */
  Elf32_Off		sh_offset;	/* file offset */
  Elf32_Word		sh_size;	/* section size */
  Elf32_Word		sh_link;	/* link to another section */
  Elf32_Word		sh_info;	/* additional info */
  Elf32_Word		sh_addralign;	/* section alignment */
  Elf32_Word		sh_entsize;	/* entry size if table */
} Elf32_Shdr;

/*
 * Section type values (sh_type)
 */
#define SHT_NULL	  0		/* Section header table entry unused */
#define SHT_PROGBITS	  1		/* Program data */
#define SHT_SYMTAB	  2		/* Symbol table */
#define SHT_STRTAB	  3		/* String table */
#define SHT_RELA	  4		/* Relocation entries with addends */
#define SHT_HASH	  5		/* Symbol hash table */
#define SHT_DYNAMIC	  6		/* Dynamic linking information */
#define SHT_NOTE	  7		/* Notes */
#define SHT_NOBITS	  8		/* Program space with no data (bss) */
#define SHT_REL		  9		/* Relocation entries, no addends */
#define SHT_SHLIB	  10		/* Reserved */
#define SHT_DYNSYM	  11		/* Dynamic linker symbol table */

/*
 * Section flags (sh_flags)
 */
#define SHF_WRITE	 (1 << 0)	/* Writable */
#define SHF_ALLOC	 (1 << 1)	/* Occupies memory during execution */
#define SHF_EXECINSTR	 (1 << 2)	/* Executable */

#endif /* __KERNEL__ */

#define FLIP16(value)		((((value) & 0xFF00) >> 8) | (((value) & 0x00FF) << 8))
#define FLIP32(value)		((((value) & 0xFF000000) >> 24) | (((value) & 0x00FF0000) >> 8) | \
				 (((value) & 0x0000FF00) << 8)  | (((value) & 0x000000FF) << 24))

/* Check if the target CPU is little endian */
#if defined(__sh__) || defined(__st200__) || defined(__arm__)
#if !defined(__LITTLE_ENDIAN__)
#define __LITTLE_ENDIAN__
#endif
#endif /* (__sh__) || defined(__st200__) || defined(__arm__) */

#ifdef __LITTLE_ENDIAN__

/* All ST Elf files are Little Endian (ELFDATA2LSB), so no flip is required */

#define Elf32Addr(V)	((V))
#define Elf32Half(V)	((V))
#define Elf32Off(V)	((V))
#define Elf32Word(V)	((V))
#define Elf32Sword(V)	((value))
#define Elf32Copy(DST, SRC, SIZE) _ICS_OS_MEMCPY((DST), (SRC), (SIZE))

#else

/* This will hopefully help those MIPs guys ... */

#define Elf32Addr(V)	(FLIP32(V))
#define Elf32Half(V)	(FLIP16(V))
#define Elf32Off(V)	(FLIP32(V))
#define Elf32Word(V)	(FLIP32(V))
#define Elf32Sword(V)	(FLIP32(V))
#define Elf32Copy(DST, SRC, SIZE) do {int i; Elf32_Word *src = (SRC), *dst = (DST); for (i = 0; i < (SIZE)/sizeof(Elf32_Word); i++, src++) *dst++ = Elf32Word(*src);} while (0)

#endif /* __LITTLE_ENDIAN__ */

#endif /* _ELF_TYPES_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
