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

#ifndef _ICS_REGION_H
#define _ICS_REGION_H

typedef ICS_HANDLE   ICS_REGION;

/*
 * Address translation & Region management
 */
ICS_EXPORT ICS_ERROR ICS_region_add (ICS_VOID *map, ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS flags,
				     ICS_ULONG cpuMask, ICS_REGION *regionp);
ICS_EXPORT ICS_ERROR ICS_region_remove (ICS_REGION region, ICS_UINT flags);

/* Convert virtual addresses to and from physical ones based on the registered regions */
ICS_EXPORT ICS_ERROR ICS_region_virt2phys (ICS_VOID *address, ICS_OFFSET *paddrp, ICS_MEM_FLAGS *mflagsp);
ICS_EXPORT ICS_ERROR ICS_region_phys2virt (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags, ICS_VOID **addressp);

#endif /* _ICS_REGION_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
