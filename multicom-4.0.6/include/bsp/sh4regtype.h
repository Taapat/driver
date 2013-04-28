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

#ifndef __SH4REGTYPE_H
#define __SH4REGTYPE_H

#ifndef _SH4REG_ASM_
typedef volatile unsigned char *const sh4_byte_reg_t;
typedef volatile unsigned short *const sh4_word_reg_t;
typedef volatile unsigned int *const sh4_dword_reg_t;
__extension__ typedef volatile unsigned long long int *const sh4_gword_reg_t;

#define SH4_BYTE_REG(address) ((sh4_byte_reg_t) (address))
#define SH4_WORD_REG(address) ((sh4_word_reg_t) (address))
#define SH4_DWORD_REG(address) ((sh4_dword_reg_t) (address))
#define SH4_GWORD_REG(address) ((sh4_gword_reg_t) (address))
#define SH4_PHYS_REG(address) (((((unsigned int) (address)) & 0xe0000000) == 0xe0000000) ? ((unsigned int) (address)) : (((unsigned int) (address)) & 0x1fffffff))
#else /* _SH4REG_ASM_ */
#define SH4_PHYS_REG(address) ((((address) & 0xe0000000) == 0xe0000000) ? (address) : ((address) & 0x1fffffff))
#define SH4_BYTE_REG(address) (address)
#define SH4_WORD_REG(address) (address)
#define SH4_DWORD_REG(address) (address)
#define SH4_GWORD_REG(address) (address)
#endif /* _SH4REG_ASM_ */

#endif /* __SH4REGTYPE_H */
