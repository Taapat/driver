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

#ifndef _ICS_NSRV_SYS_H
#define _ICS_NSRV_SYS_H

#define _ICS_NSRV_CPU		0			/* The CPU which hosts the nsrv task */
#define _ICS_NSRV_PORT_NAME	"ics_nsrv"		/* Name of the nameserver admin port */
#define _ICS_NSRV_PORT_NDESC	16			/* Number of descs in the Port mq */

/*
 * Nameserver message types 
 */
typedef enum ics_nsrv_msg_type
{
  _ICS_NSRV_REPLY         = (0xdd << 24) | 0x00,		/* Reply message */

  _ICS_NSRV_LOOKUP	  = (0xdd << 24) | 0x01,		/* Lookup handle */
  _ICS_NSRV_REG	  	  = (0xdd << 24) | 0x02,		/* Register name & handle */
  _ICS_NSRV_UNREG	  = (0xdd << 24) | 0x03,		/* Unregister name */
  _ICS_NSRV_CPU_DOWN      = (0xdd << 24) | 0x04			/* CPU is going down/crashed */

} ics_nsrv_msg_type_t;


typedef struct ics_nsrv_msg
{
  ics_nsrv_msg_type_t		type;				/* Msg type */
  ICS_PORT			replyPort;			/* Reply Port */
  ICS_CHAR			name[ICS_NSRV_MAX_NAME+1];	/* Name */

  union
  {
    struct					/* _ICS_NSRV_LOOKUP request */
    {
      ICS_ULONG			timeout;	/* Blocking timeout */
      ICS_UINT			flags;		/* e.g. ICS_BLOCK */
    } lookup;

    struct					/* _ICS_NSRV_REG/UNREG request */
    {
      ICS_HANDLE		handle;		/* Handle to unregister */
      ICS_UINT			flags;		/* extra flags */
    } reg;

    struct					/* _ICS_NSRV_CPU_DOWN request */
    {
      ICS_UINT 			cpuNum;		/* CPU which is down */
      ICS_UINT			flags;		/* extra flags */
    } cpu_down;

  } u;

  ICS_CHAR			data[ICS_NSRV_MAX_DATA];	/* Object data */

} ics_nsrv_msg_t;

typedef struct ics_nsrv_reply
{
  ics_nsrv_msg_type_t		type;		/* Msg type */
  ICS_ERROR			err;		/* Operation result code */

  ICS_NSRV_HANDLE		handle;		/* Associated handle */

  ICS_CHAR 			data[ICS_NSRV_MAX_DATA];	/* Associated nameserver object data */

} ics_nsrv_reply_t;

/* Exported internal APIs */
extern void ics_nsrv_task (void *param);
extern ICS_ERROR ics_nsrv_init (ICS_UINT cpuNum);
extern void ics_nsrv_term (void);

extern ICS_ERROR ics_nsrv_cpu_down (ICS_UINT cpuNum, ICS_UINT flags);

#endif /* _ICS_NSRV_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
