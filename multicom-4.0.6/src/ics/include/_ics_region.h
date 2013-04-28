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

#ifndef _ICS_REGION_SYS_H
#define _ICS_REGION_SYS_H

typedef struct ics_region
{
  ICS_OFFSET		 base;		/* Physical base of the region */
  ICS_SIZE		 size;		/* Size of the region */

  ICS_VOID		*map;		/* Virtual memory mapping of region */
  ICS_VOID		*umap;		/* User Virtual memory mapping of region */
  ICS_MEM_FLAGS		 mflags;	/* Memory region flags */
  ICS_ULONG		 cpuMask;	/* Bitmask of requested CPUs */

  ICS_UINT		 cpuNum;	/* Owning cpu number */

  ICS_UINT 		 version;	/* Incarnation version number of this region */
  ICS_HANDLE		 handle;	/* ICS_REGION handle */

  ICS_UINT		 refCount;	/* Reference count */

} ics_region_t;

/* Exported internal APIs */
ICS_ERROR ics_region_init (void);
void      ics_region_term (void);

ICS_ERROR ics_region_map (ICS_UINT cpuNum, ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS flags, ICS_VOID **map);
ICS_ERROR ics_region_unmap (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS flags);

ICS_ERROR ics_region_cpu_up (ICS_UINT cpuNum);
ICS_ERROR ics_region_cpu_down (ICS_UINT cpuNum, ICS_UINT flags);

ICS_ERROR _ics_region_virt2phys (ICS_VOID *address, ICS_OFFSET *paddrp, ICS_MEM_FLAGS *mflagsp);

void      ics_region_dump (void);

#endif /* _ICS_REGION_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
