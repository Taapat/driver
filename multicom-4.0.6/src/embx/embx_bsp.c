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

#include <bsp/_bsp.h>

/*
 * Dummy BSP for EMBXSHM emulation library
 */

const char * bsp_cpu_name;

struct bsp_cpu_info bsp_cpus[1];

unsigned int bsp_cpu_count = 0;

struct bsp_mbox_regs bsp_mailboxes[1];

unsigned int bsp_mailbox_count = 0;

/* 
 * ST40 only
 * reset.c dummies
 */
struct bsp_reg_mask bsp_sys_reset_bypass[1];
unsigned int bsp_sys_reset_bypass_count = 0;

struct bsp_boot_address_reg bsp_sys_boot_registers[1];
struct bsp_reg_mask bsp_sys_reset_registers[1];
struct bsp_reg_mask bsp_sys_boot_enable[1];

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
