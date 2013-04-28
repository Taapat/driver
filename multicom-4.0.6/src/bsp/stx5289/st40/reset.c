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

#include "stx5289reg.h"

struct bsp_reg_mask bsp_sys_reset_bypass[] = {
  {STX5289_SYSCONF_SYS_CFG09, ~(1 << 28), 1 << 28}
};

/* Size of the above array */
unsigned int bsp_sys_reset_bypass_count = sizeof(bsp_sys_reset_bypass)/sizeof(bsp_sys_reset_bypass[0]);

struct bsp_boot_address_reg bsp_sys_boot_registers[] = {
  {0, 0, 0},									/* st40 */
  {STX5289_SYSCONF_SYS_CFG28, 0, 0xFFFFFF00},		/* video */
  {STX5289_SYSCONF_SYS_CFG26, 0, 0xFFFFFF00}		/* audio */
};

struct bsp_reg_mask bsp_sys_boot_enable[] = {
  {NULL, 0, 0},								/* st40 */
  {STX5289_SYSCONF_SYS_CFG28, ~1, 1},			/* video */
  {STX5289_SYSCONF_SYS_CFG26, ~1, 1}			/* audio */
};

struct bsp_reg_mask bsp_sys_reset_registers[] = {
  {NULL, 0, 0},								/* st40 */
  {STX5289_SYSCONF_SYS_CFG29, ~1, 1},			/* video */
  {STX5289_SYSCONF_SYS_CFG27, ~1, 1}			/* audio */
};

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
