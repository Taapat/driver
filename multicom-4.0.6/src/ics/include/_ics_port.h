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

#ifndef _ICS_PORT_SYS_H
#define _ICS_PORT_SYS_H

typedef enum ics_port_state
{
  _ICS_PORT_FREE = 0,
  _ICS_PORT_OPEN,
  _ICS_PORT_CLOSING
} ics_port_state_t;

/* 
 * Internal Port descriptor
 */
typedef struct ics_port
{
  ics_port_state_t	state;		/* Port state */

  ICS_CHAR		name[ICS_NSRV_MAX_NAME+1];	/* Port name (may be empty for anonymous ports) */

  ICS_PORT		handle;		/* Port handle for this port */
  ICS_UINT 		version;	/* Incarnation version number of this port */
  ICS_NSRV_HANDLE	nhandle;	/* Namserver registration handle */

  ics_mq_t	       *mq;		/* Local incoming message queue */

  struct list_head	postedRecvs;	/* List of posted receive events */

  ICS_PORT_CALLBACK	callback;	/* User defined callback fn */
  ICS_VOID	       *callbackParam;	/* User defined callback parameter */
  ICS_BOOL		callbackBlock;	/* Set to TRUE when ICS_FULL returned by callback fn */
  
  ICS_UINT		blockedChans;	/* Count of blocked channels */
  ICS_CHANNEL		blockedChan[_ICS_MAX_CPUS];	/* Blocked channel handles */

  ICS_UINT		matched;	/* STATISTICS: Count of matched msgs */
  ICS_UINT		unmatched;	/* STATISTICS: Count of unmatched msgs */

} ics_port_t;

/* Exported internal APIs */
extern ICS_ERROR ics_port_init (void);
extern void      ics_port_term (void);

#endif /* _ICS_PORT_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

