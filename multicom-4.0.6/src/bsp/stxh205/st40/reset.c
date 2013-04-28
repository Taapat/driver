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

/* 
 * 
 */ 

#include <bsp/_bsp.h>

#include "stxh205reg.h"

struct bsp_reg_mask bsp_sys_reset_bypass[] = {
  {STXH205_SYSCONF0_SYS_CFG01, ~0, (1 << 1)},
  {STXH205_SYSCONF1_SYS_CFG00, ~0, (1 << 1)}, 				/* enable BART for RT core */
  {STXH205_SYSCONF1_SYS_CFG01, ~(0xff >> 18), (0x44000000 >> 18)},	/* RT core BART loads from 0x44000000) */
};

/* Size of the above array */
unsigned int bsp_sys_reset_bypass_count = sizeof(bsp_sys_reset_bypass)/sizeof(bsp_sys_reset_bypass[0]);

struct bsp_boot_address_reg bsp_sys_boot_registers[] = {
  {STXH205_SYSCONF0_SYS_CFG04, 0, 0xFFFFFFFC},   /* host */
  {STXH205_SYSCONF0_SYS_CFG05, 0, 0xFFFFFFFC},   /* rt */
  {STXH205_SYSCONF0_SYS_CFG06, 0, 0xFFFFFFFC},   /* video */
  {STXH205_SYSCONF0_SYS_CFG07, 0, 0xFFFFFFFC},   /* audio */
  {STXH205_SYSCONF0_SYS_CFG08, 0, 0xFFFFFFFC}    /* gp */
};


/* STXH205 reset bits are active low - mask and ~mask will be used and value
   should be 0.
 */
struct bsp_reg_mask bsp_sys_reset_registers[] = {
  {STXH205_SYSCONF0_SYS_CFG00, ~(1 << 0), 0},	/* host */
  {STXH205_SYSCONF0_SYS_CFG00, ~(1 << 1), 0},	/* rt */
  {STXH205_SYSCONF0_SYS_CFG00, ~(1 << 4), 0},	/* video */
  {STXH205_SYSCONF0_SYS_CFG00, ~(1 << 5), 0},	/* audio */
  {STXH205_SYSCONF0_SYS_CFG00, ~(1 << 6), 0},	/* gp */
};

/* STXH205 has no boot enables - purely controlled by reset bits */
struct bsp_reg_mask bsp_sys_boot_enable[] = {
  {NULL, 0, 0},
  {NULL, 0, 0},
  {NULL, 0, 0},
  {NULL, 0, 0},
  {NULL, 0, 0}
};


/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
