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

#include "flx7510reg.h"

struct bsp_reg_mask bsp_sys_reset_bypass[] = {};

/* Size of the above array */
unsigned int bsp_sys_reset_bypass_count = sizeof(bsp_sys_reset_bypass)/sizeof(bsp_sys_reset_bypass[0]);

struct bsp_boot_address_reg bsp_sys_periph_registers[] = {
  {0, 0, 0},					/* st40 */
  {FLI7510_ST231_DRA2_PERIPH, 0, 0xFFF00000},	/* video */
  {FLI7510_ST231_AUD1_PERIPH, 0, 0xFFF00000},	/* audio0 */
  {FLI7510_ST231_AUD2_PERIPH, 0, 0xFFF00000}	/* audio1 */
};

struct bsp_boot_address_reg bsp_sys_boot_registers[] = {
  {0, 0, 0},					/* st40 */
  {FLI7510_ST231_DRA2_BOOT, 0, 0xFFFFFFC0},	/* video */
  {FLI7510_ST231_AUD1_BOOT, 0, 0xFFFFFFC0},	/* audio0 */
  {FLI7510_ST231_AUD2_BOOT, 0, 0xFFFFFFC0}	/* audio1 */
};

struct bsp_reg_mask bsp_sys_boot_enable[] = {
  {NULL, 0, 0},					/* st40 */
  {FLI7510_BOOT_CTRL, ~0x04, 0x04},		/* video */
  {FLI7510_BOOT_CTRL, ~0x08, 0x08},		/* audio0 */
  {FLI7510_BOOT_CTRL, ~0x10, 0x10}		/* audio1 */
};

struct bsp_reg_mask bsp_sys_reset_registers[] = {
  {NULL, 0, 0},					/* st40 */
  {FLI7510_RESET_CTRL, ~0x10, 0x10},		/* video */
  {FLI7510_RESET_CTRL, ~0x20, 0x20},		/* audio0 */
  {FLI7510_RESET_CTRL, ~0x40, 0x40}		/* audio1 */
};


/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
