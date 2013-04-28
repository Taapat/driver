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
 * Userspace <-> Kernel API for ICS library
 */

#ifndef _ICS_IOCTL_H
#define _ICS_IOCTL_H

typedef struct ics_load_elf_file {
        /* IN */
        const char			*fname;
	ICS_UINT			 fnameLen;
  	ICS_UINT			 flags;
	
        /* OUT */
	ICS_ERROR			 err;
        ICS_OFFSET			 entryAddr;
} ics_load_elf_file_t;

typedef struct ics_cpu_start {
	/* IN */
	ICS_UINT			 cpuNum;
	ICS_OFFSET			 entryAddr;	
        ICS_UINT			 flags;

        /* OUT */
	ICS_ERROR			 err;
} ics_cpu_start_t;

typedef struct ics_cpu_reset {
	/* IN */
	ICS_UINT			 cpuNum;
        ICS_UINT			 flags;

        /* OUT */
	ICS_ERROR			 err;
} ics_cpu_reset_t;

typedef struct ics_cpu_init {
	/* IN */
 ICS_UINT			 inittype;
 ICS_UINT			 flags;
	ICS_UINT			 cpuNum;
	ICS_ULONG			cpuMask;
 /* OUT */
	ICS_ERROR			 err;
} ics_cpu_init_t;


typedef struct ics_cpu_term {
	/* IN */
 ICS_UINT			 flags;
 /* OUT */
	ICS_ERROR			 err;
} ics_cpu_term_t;

typedef struct ics_cpu_info {
	/* IN */
        /* OUT */
	ICS_UINT			 cpuNum;
	ICS_ULONG			 cpuMask;
	ICS_ERROR			 err;
} ics_cpu_info_t;

typedef struct ics_cpu_bsp {
	/* IN */
	ICS_UINT			 cpuNum;
	
        /* OUT */
	char                             *name;
	char                             *type;
	ICS_ERROR			 err;
} ics_cpu_bsp_t;

typedef struct ics_user_region {
	/* IN */
 ICS_VOID   *map;
 ICS_OFFSET paddr;
 ICS_SIZE   size;
 ICS_MEM_FLAGS mflags;
	ICS_ULONG		cpuMask;
 ICS_REGION region;
 
 /* OUT */
	ICS_ERROR			 err;
} ics_user_region_t;

#define ICS_IOC_MAGIC 			'I'

#define ICS_IOC_LOADELFFILE		_IOWR(ICS_IOC_MAGIC, 0x1, ics_load_elf_file_t)
#define ICS_IOC_CPUSTART		_IOWR(ICS_IOC_MAGIC, 0x2, ics_cpu_start_t)
#define ICS_IOC_CPURESET		_IOWR(ICS_IOC_MAGIC, 0x3, ics_cpu_reset_t)
#define ICS_IOC_CPUINFO			_IOW (ICS_IOC_MAGIC, 0x4, ics_cpu_info_t)
#define ICS_IOC_CPUBSP			_IOWR(ICS_IOC_MAGIC, 0x5, ics_cpu_bsp_t)
#define ICS_IOC_CPULOOKUP		_IOWR(ICS_IOC_MAGIC, 0x6, ics_cpu_bsp_t)
#define ICS_IOC_CPUINIT			_IOW (ICS_IOC_MAGIC, 0x7, ics_cpu_init_t)
#define ICS_IOC_CPUTERM			_IOW (ICS_IOC_MAGIC, 0x8, ics_cpu_term_t)
#define ICS_IOC_REGIONADD		_IOW (ICS_IOC_MAGIC, 0x9, ics_user_region_t)
#define ICS_IOC_REGIONREMOVE    _IOW (ICS_IOC_MAGIC, 0x10, ics_user_region_t)

#endif /* _ICS_IOCTL_H */


/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
