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

#ifndef _ICS_INIT_H
#define _ICS_INIT_H

/*
 * ICS_cpu_init flags
 */
typedef enum
{
  ICS_INIT_CONNECT_ALL       = 0x1,		/* Connect to all CPUs during init */
  ICS_INIT_WATCHDOG          = 0x2,		/* Install a CPU watchdog */

} ICS_INIT_FLAGS;

ICS_EXPORT ICS_ERROR        ICS_cpu_init (ICS_UINT flags);
ICS_EXPORT void             ICS_cpu_term (ICS_UINT flags);

ICS_EXPORT ICS_ERROR        ics_cpu_init (ICS_UINT cpuNum, ICS_ULONG cpuMask, ICS_UINT flags);

ICS_EXPORT ICS_ERROR        ICS_cpu_info (ICS_UINT *cpuNump, ICS_ULONG *cpuMaskp);

ICS_EXPORT const ICS_CHAR * ics_cpu_version (void);

ICS_EXPORT ICS_ERROR        ICS_cpu_version (ICS_UINT cpuNum, ICS_ULONG *versionp);

#endif /* _ICS_INIT_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
