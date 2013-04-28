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

#include "stxh415reg.h"

struct bsp_reg_mask bsp_sys_reset_bypass[] = {
  {STXH415_SYSCONF3_SYS_CFG629, ~0, (1 << 1)},
  {STXH415_SYSCONF3_SYS_CFG629, ~0, (1 << 5)}, 				/* enable BART for RT core */
//  {STXH415_SYSCONF3_SYS_CFG629, ~(0xF >> 18), (0x44000000 >> 18)},	/* RT core BART loads from 0x44000000) */
  {STXH415_SYSCONF3_SYS_CFG629, ~(0xF >> 18), (0x80000000 >> 18)},	/* RT core BART loads from 0x44000000) */
};

/* Size of the above array */
unsigned int bsp_sys_reset_bypass_count = sizeof(bsp_sys_reset_bypass)/sizeof(bsp_sys_reset_bypass[0]);

struct bsp_boot_address_reg bsp_sys_boot_registers[] = {
  {STXH415_SYSCONF3_SYS_CFG647, 0, 0x00000007},   /* host 														*/
  {STXH415_SYSCONF3_SYS_CFG648, 0, 0xFFFFFFFE},   /* rt 																*/
  {STXH415_SYSCONF3_SYS_CFG649, 0, 0xFFFFFFE0},   /* video_0								 			*/
  {STXH415_SYSCONF3_SYS_CFG652, 0, 0xFFFFFFE0},   /* video_1 											*/
  {STXH415_SYSCONF3_SYS_CFG650, 0, 0xFFFFFFE0},   /* audio_0 -aud 						*/
  {STXH415_SYSCONF3_SYS_CFG651, 0, 0xFFFFFFE0},   /* audio_1 -sec0 -gp0	*/
  {STXH415_SYSCONF3_SYS_CFG653, 0, 0xFFFFFFE0},   /* sec1-gp1										 */  
};
/* Size of the above array */
unsigned int bsp_sys_boot_registers_count = sizeof(bsp_sys_boot_registers)/sizeof(bsp_sys_boot_registers[0]);

/* STXH415 reset bits are active low - mask and ~mask will be used and value
   should be 0.
 */
struct bsp_reg_mask bsp_sys_reset_registers[] = {
  {STXH415_SYSCONF3_SYS_CFG644, ~(1 << 0), 0},	 /* host 														*/
  {STXH415_SYSCONF3_SYS_CFG644, ~(1 << 2), 0},	 /* rt 																*/
  {STXH415_SYSCONF3_SYS_CFG659, ~(1 << 27), 0},	/* video_0 											*/
  {STXH415_SYSCONF3_SYS_CFG659, ~(1 << 29), 0},	/* video_1 											*/
  {STXH415_SYSCONF3_SYS_CFG659, ~(1 << 26), 0},	/* audio_0 -aud 						*/
  {STXH415_SYSCONF3_SYS_CFG659, ~(1 << 28), 0},	/* audio_1 -sec0 -gp0	*/
  {STXH415_SYSCONF3_SYS_CFG659, ~(1 << 30), 0},	/* sec1-gp1										 */
};
/* Size of the above array */
unsigned int bsp_sys_reset_registers_count = sizeof(bsp_sys_reset_registers)/sizeof(bsp_sys_reset_registers[0]);

/* STXH415 has no boot enables - purely controlled by reset bits */
struct bsp_reg_mask bsp_sys_boot_enable[] = {
  {NULL, 0, 0},
  {NULL, 0, 0},
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
