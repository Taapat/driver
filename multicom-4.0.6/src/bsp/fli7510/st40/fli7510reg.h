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
 */

#ifndef __FLI7510REG_H
#define __FLI7510REG_H

#include "sh4regtype.h"

/*----------------------------------------------------------------------------*/
#define FLI7510_VDEC_PU_CONFIG_REG1	0xFD7C0000
#define FLI7510_PRB_PU_CONFIG_1		0xFD220000

/*
 * Base addresses for control register banks.
 */
#define FLI7510_ST231_DRA2_PERIPH	SH4_DWORD_REG(FLI7510_VDEC_PU_CONFIG_REG1 + 0x0000)
#define FLI7510_ST231_DRA2_BOOT		SH4_DWORD_REG(FLI7510_VDEC_PU_CONFIG_REG1 + 0x0004)

#define FLI7510_ST231_AUD1_PERIPH	SH4_DWORD_REG(FLI7510_VDEC_PU_CONFIG_REG1 + 0x0008)
#define FLI7510_ST231_AUD1_BOOT		SH4_DWORD_REG(FLI7510_VDEC_PU_CONFIG_REG1 + 0x000C)

#define FLI7510_ST231_AUD2_PERIPH	SH4_DWORD_REG(FLI7510_VDEC_PU_CONFIG_REG1 + 0x0010)
#define FLI7510_ST231_AUD2_BOOT		SH4_DWORD_REG(FLI7510_VDEC_PU_CONFIG_REG1 + 0x0014)

#define FLI7510_RESET_CTRL		SH4_DWORD_REG(FLI7510_PRB_PU_CONFIG_1 + 0x0000)
#define FLI7510_BOOT_CTRL		SH4_DWORD_REG(FLI7510_PRB_PU_CONFIG_1 + 0x0004)


/*----------------------------------------------------------------------------*/


#endif /* __FLI7510REG_H */
