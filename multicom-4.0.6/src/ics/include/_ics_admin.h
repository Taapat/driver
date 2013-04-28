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

#ifndef _ICS_ADMIN_SYS_H
#define _ICS_ADMIN_SYS_H

#define _ICS_ADMIN_PORT_NAME	"ics_admin_%02d"	/* Name (prefix) of per CPU admin port */
#define _ICS_ADMIN_PORT_NDESC	16			/* Number of Admin Port mq descs */

/*
 * Admin message types 
 */
typedef enum ics_admin_msg_type
{
  _ICS_ADMIN_REPLY         = (0xee << 24) | 0x00,	/* Reply message */

  _ICS_ADMIN_MAP_REGION    = (0xee << 24) | 0x01,	/* Map memory region */
  _ICS_ADMIN_UNMAP_REGION  = (0xee << 24) | 0x02,	/* Unmap memory region */

  _ICS_ADMIN_DYN_LOAD      = (0xee << 24) | 0x03,	/* Load a dynamic module */
  _ICS_ADMIN_DYN_UNLOAD    = (0xee << 24) | 0x04	/* Unload a dynamic module */

} ics_admin_msg_type_t;


typedef struct ics_admin_msg
{
  ics_admin_msg_type_t		type;		/* Msg type */
  ICS_PORT			replyPort;	/* Port to send result to */

  union
  {
    struct					/* _ICS_ADMIN_MAP_REGION */
    {
      ICS_OFFSET		paddr;		/* Base memory physical address */
      ICS_SIZE			size;		/* Size of memory segment */
      ICS_MEM_FLAGS		mflags;		/* Memory mapping flags for remote segment */
    } map;
    
    struct					/* _ICS_ADMIN_DYN_LOAD */
    {
      ICS_CHAR			name[_ICS_MAX_PATHNAME_LEN+1]; /* Module name (used for debugging only) */

      ICS_OFFSET		paddr;		/* Image memory physical address */
      ICS_SIZE			size;		/* Size of memory segment */
      ICS_UINT			flags;		/* Associated flags */
      ICS_DYN			parent;		/* Parent module of the new module being loaded */
    } dyn_load;

    struct					/* _ICS_ADMIN_DYN_UNLOAD */
    {
      ICS_DYN			handle;		/* Associated dynamic object handle */
    } dyn_unload;

  } u;

} ics_admin_msg_t;

typedef struct ics_admin_reply
{
  ics_admin_msg_type_t		type;		/* Msg type */
  ICS_ERROR			err;		/* Operation result code */

  union
  {
    struct					/* _ICS_ADMIN_DYN_LOAD reply */
    {
      ICS_DYN			handle;		/* Associated dynamic object handle */
    } dyn_reply;

  } u;

} ics_admin_reply_t;

/* Exported internal APIs */
extern void ics_admin_task (void *param);
extern ICS_ERROR ics_admin_init (void);
extern void ics_admin_term (void);

/* Memory region management */
extern ICS_ERROR ics_admin_map_region (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS flags, ICS_UINT cpuNum);
extern ICS_ERROR ics_admin_unmap_region (ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS flags, ICS_UINT cpuNum);

/* Dynamic loader */
extern ICS_ERROR ics_admin_dyn_load (ICS_CHAR *name, ICS_OFFSET paddr, ICS_SIZE size, ICS_UINT flags,
				     ICS_UINT cpuNum, ICS_DYN parent, ICS_DYN *handlep);
extern ICS_ERROR ics_admin_dyn_unload (ICS_DYN handle);

#endif /* _ICS_ADMIN_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
