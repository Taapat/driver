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

#ifndef _ICS_STATS_H
#define _ICS_STATS_H

#define ICS_STATS_MAX_ITEMS		16	/* Maximum number of stats monitored per CPU */

#define ICS_STATS_MAX_NAME		31	/* Max stat name string len (not including the \0) */

typedef ICS_HANDLE ICS_STATS_HANDLE;
typedef ICS_UINT64 ICS_STATS_VALUE;
typedef ICS_UINT64 ICS_STATS_TIME;

/*
 * Statistic item types
 * Used to display them correctly
 */
typedef enum
{
  ICS_STATS_COUNTER		= 0x1,		/* Display value as a simple count (with delta) */
  ICS_STATS_RATE		= 0x2,		/* Display value as rate per second */
  ICS_STATS_PERCENT		= 0x4,		/* Display value as percentage of time */
  ICS_STATS_PERCENT100		= 0x8,		/* Display value at 100 - percentage of time */

} ICS_STATS_TYPE;

typedef void (*ICS_STATS_CALLBACK)(ICS_STATS_HANDLE handle, ICS_VOID *param);

/*
 * Array item returned by stats sample
 */
typedef struct ics_stats_item
{
  ICS_STATS_VALUE	value[2];
  ICS_STATS_TIME	timestamp[2];
  ICS_STATS_TYPE	type;
  ICS_CHAR              name[ICS_STATS_MAX_NAME+1];
} ICS_STATS_ITEM;

ICS_ERROR ICS_stats_sample (ICS_UINT cpuNum, ICS_STATS_ITEM *stats, ICS_UINT *nstatsp);
ICS_ERROR ICS_stats_update (ICS_STATS_HANDLE handle, ICS_STATS_VALUE value, ICS_STATS_TIME timestamp);
ICS_ERROR ICS_stats_add (const ICS_CHAR *name, ICS_STATS_TYPE type, 
			 ICS_STATS_CALLBACK callback, ICS_VOID *param, ICS_STATS_HANDLE *handlep);
ICS_ERROR ICS_stats_remove (ICS_STATS_HANDLE handle);

#endif /* _ICS_STATS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

