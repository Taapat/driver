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
 * Linux Kernel module header file
 */

#ifndef _ICS_LINUX_MODULE_H
#define _ICS_LINUX_MODULE_H

#define MODULE_NAME	"ics"

/* Procfs utilities */
extern int ics_create_procfs (ICS_ULONG cpuMask);
extern void ics_remove_procfs (ICS_ULONG cpuMask);
extern struct bpa2_part *ics_bpa2_part;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30)
#define SET_PROCFS_OWNER(D)	(D)->owner = THIS_MODULE
#else
#define SET_PROCFS_OWNER(D)
#endif

#endif /* _ICS_LINUX_MODULE_H */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
