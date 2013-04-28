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
 * Private header file for ICS implementation
 *
 */ 

#ifndef _BSP_H
#define _BSP_H

#ifndef NULL
#define NULL ((void *)0)
#endif

/*
 * BSP tables structure definitions
 */ 

/*
 * CPU table info
 */
struct bsp_cpu_info 
{
  char 			*name;			/* CPU name e.g. "audio", "video" */
  char 			*type;			/* CPU type e.g. "st40", "st231" */
  int	 		 num;			/* CPU logical number (-ve to disable) */
  unsigned int		 flags;			/* CPU specific load flags */
};

/*
 * Hardware mailbox info 
 */

/* Mailbox flag bits */
#define _BSP_MAILBOX_LOCKS	0x01

struct bsp_mbox_regs
{
  void 			*base;
  unsigned int  	 interrupt;
  unsigned int           flags;
};

/* Boot address info */
struct bsp_boot_address_reg
{
  volatile unsigned int *address;
  int 			 leftshift;
  unsigned int 		 mask;
};

/* Address/Bitmask entry */
struct bsp_reg_mask
{
  volatile unsigned int *address;
  unsigned int 		 mask;
  unsigned int           value;
};

/*
 * The per platform BSP tables and info
 */
extern unsigned int                bsp_cpu_count;
extern const char *		   bsp_cpu_name;
extern struct bsp_cpu_info	   bsp_cpus[];

extern unsigned int                bsp_mailbox_count;
extern struct bsp_mbox_regs        bsp_mailboxes[];
 
extern struct bsp_reg_mask         bsp_sys_reset_bypass[];
extern unsigned int                bsp_sys_reset_bypass_count;
extern struct bsp_boot_address_reg bsp_sys_boot_registers[];
extern struct bsp_reg_mask         bsp_sys_boot_enable[];
extern struct bsp_reg_mask         bsp_sys_reset_registers[];

#endif /* _BSP_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
