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

#ifndef _ICS_CPU_H
#define _ICS_CPU_H

/*
 * Routines to start and reset companions CPUs
 */
ICS_EXPORT ICS_ERROR   ics_cpu_start (ICS_OFFSET entryAddr, ICS_UINT cpuNum, ICS_UINT flags);
ICS_EXPORT ICS_ERROR   ics_cpu_reset (ICS_UINT cpuNum, ICS_UINT flags);

/*
 * Routines to query the BSP
 */
ICS_EXPORT ICS_INT     ics_cpu_self (void);
ICS_EXPORT ICS_ULONG   ics_cpu_mask (void);

/*
 * Routines to query the BSP, translating between logical CPU numbers and BSP CPU names
 */
ICS_EXPORT int         ics_cpu_lookup (const ICS_CHAR *cpuName);
ICS_EXPORT const char *ics_cpu_name (ICS_UINT cpuNum);
ICS_EXPORT const char *ics_cpu_type (ICS_UINT cpuNum);
ICS_EXPORT int         ics_cpu_index (ICS_UINT cpuNum);

#endif /* _ICS_CPU_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
