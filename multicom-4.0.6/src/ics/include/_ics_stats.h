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

#ifndef _ICS_SYS_STATS_H
#define _ICS_SYS_STATS_H

/*
 * Internal stats state
 */
typedef struct
{ 
  ICS_STATS_HANDLE	 handle;			/* Transport handle */

  ICS_STATS_CALLBACK	 callback;			/* User callback & param */
  ICS_VOID		*param;

  struct list_head       list;				/* Doubly linked list */
  struct list_head       tlist;				/* Doubly linked trigger list */

} ics_stats_t;

typedef ICS_STATS_ITEM __attribute__ ((aligned(8))) ics_stats_item_t;

/* 
 * Internal APIs
 */
ICS_ERROR ics_stats_init (void);
void ics_stats_term (void);
void ics_stats_trigger (void);

void ics_stats_mem (ICS_STATS_HANDLE handle, void *param);
void ics_stats_mem_sys (ICS_STATS_HANDLE handle, void *param);
void ics_stats_cpu (ICS_STATS_HANDLE handle, void *param);

#endif /* _ICS_SYS_STATS_H */
 
/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
