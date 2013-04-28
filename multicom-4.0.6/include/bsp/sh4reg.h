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

#ifndef __SH4REG_H
#define __SH4REG_H

#include "sh4regtype.h"

/*----------------------------------------------------------------------------*/

/*
 * Core SH4 control registers
 */

/* Core control registers (common to all SH4 variants) */
#define SH4_CCN_PTEH SH4_DWORD_REG(0xff000000)
#define SH4_CCN_PTEL SH4_DWORD_REG(0xff000004)
#define SH4_CCN_TTB SH4_DWORD_REG(0xff000008)
#define SH4_CCN_TEA SH4_DWORD_REG(0xff00000c)
#define SH4_CCN_MMUCR SH4_DWORD_REG(0xff000010)
#define SH4_CCN_BASRA SH4_BYTE_REG(0xff000014)
#define SH4_CCN_BASRB SH4_BYTE_REG(0xff000018)
#define SH4_CCN_CCR SH4_DWORD_REG(0xff00001c)
#define SH4_CCN_TRA SH4_DWORD_REG(0xff000020)
#define SH4_CCN_EXPEVT SH4_DWORD_REG(0xff000024)
#define SH4_CCN_INTEVT SH4_DWORD_REG(0xff000028)
#define SH4_CCN_PVR SH4_DWORD_REG(0xff000030)
#define SH4_CCN_PTEA SH4_DWORD_REG(0xff000034)
#define SH4_CCN_QACR0 SH4_DWORD_REG(0xff000038)
#define SH4_CCN_QACR1 SH4_DWORD_REG(0xff00003c)
#define SH4_CCN_CVR SH4_DWORD_REG(0xff000040)
#define SH4_CCN_PRR SH4_DWORD_REG(0xff000044)

/* Core control registers (SH4-300 only) */
#define SH4_CCN_PASCR SH4_DWORD_REG(0xff000070)
#define SH4_CCN_IRMCR SH4_DWORD_REG(0xff000078)

/* User Break Controller control registers (common to all SH4 variants) */
#define SH4_UBC_BARA SH4_DWORD_REG(0xff200000)
#define SH4_UBC_BAMRA SH4_BYTE_REG(0xff200004)
#define SH4_UBC_BBRA SH4_WORD_REG(0xff200008)
#define SH4_UBC_BASRA SH4_BYTE_REG(0xff000014)
#define SH4_UBC_BARB SH4_DWORD_REG(0xff20000c)
#define SH4_UBC_BAMRB SH4_BYTE_REG(0xff200010)
#define SH4_UBC_BBRB SH4_WORD_REG(0xff200014)
#define SH4_UBC_BASRB SH4_BYTE_REG(0xff000018)
#define SH4_UBC_BDRB SH4_DWORD_REG(0xff200018)
#define SH4_UBC_BDMRB SH4_DWORD_REG(0xff20001c)
#define SH4_UBC_BRCR SH4_WORD_REG(0xff200020)

/* User Debug Interface control registers (common to all SH4 variants) */
#define SH4_UDI_SDIR SH4_WORD_REG(0xfff00000)
#define SH4_UDI_SDDR SH4_DWORD_REG(0xfff00008)
#define SH4_UDI_SDDRH SH4_WORD_REG(0xfff00008)
#define SH4_UDI_SDDRL SH4_WORD_REG(0xfff0000a)
#define SH4_UDI_SDINT SH4_WORD_REG(0xfff00014)

/* Advanced User Debugger control registers (common to all SH4 variants) */
#define SH4_AUD_AUCSR SH4_WORD_REG(0xff2000cc)
#define SH4_AUD_AUWASR SH4_DWORD_REG(0xff2000d0)
#define SH4_AUD_AUWAER SH4_DWORD_REG(0xff2000d4)
#define SH4_AUD_AUWBSR SH4_DWORD_REG(0xff2000d8)
#define SH4_AUD_AUWBER SH4_DWORD_REG(0xff2000dc)

/*
 * Generic SH4 control registers
 */

/* Timer Unit control registers (common to all SH4 variants) */
#define SH4_TMU_TOCR SH4_BYTE_REG(SH4_TMU_REGS_BASE + 0x00)
#define SH4_TMU_TSTR SH4_BYTE_REG(SH4_TMU_REGS_BASE + 0x04)
#define SH4_TMU_TCOR0 SH4_DWORD_REG(SH4_TMU_REGS_BASE + 0x08)
#define SH4_TMU_TCNT0 SH4_DWORD_REG(SH4_TMU_REGS_BASE + 0x0c)
#define SH4_TMU_TCR0 SH4_WORD_REG(SH4_TMU_REGS_BASE + 0x10)
#define SH4_TMU_TCOR1 SH4_DWORD_REG(SH4_TMU_REGS_BASE + 0x14)
#define SH4_TMU_TCNT1 SH4_DWORD_REG(SH4_TMU_REGS_BASE + 0x18)
#define SH4_TMU_TCR1 SH4_WORD_REG(SH4_TMU_REGS_BASE + 0x1c)
#define SH4_TMU_TCOR2 SH4_DWORD_REG(SH4_TMU_REGS_BASE + 0x20)
#define SH4_TMU_TCNT2 SH4_DWORD_REG(SH4_TMU_REGS_BASE + 0x24)
#define SH4_TMU_TCR2 SH4_WORD_REG(SH4_TMU_REGS_BASE + 0x28)
#define SH4_TMU_TCPR2 SH4_DWORD_REG(SH4_TMU_REGS_BASE + 0x2c)

/* Real Time Clock control registers (common to all SH4 variants) */
#define SH4_RTC_R64CNT SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x00)
#define SH4_RTC_RSECCNT SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x04)
#define SH4_RTC_RMINCNT SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x08)
#define SH4_RTC_RHRCNT SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x0c)
#define SH4_RTC_RWKCNT SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x10)
#define SH4_RTC_RDAYCNT SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x14)
#define SH4_RTC_RMONCNT SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x18)
#define SH4_RTC_RYRCNT SH4_WORD_REG(SH4_RTC_REGS_BASE + 0x1c)
#define SH4_RTC_RSECAR SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x20)
#define SH4_RTC_RMINAR SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x24)
#define SH4_RTC_RHRAR SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x28)
#define SH4_RTC_RWKAR SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x2c)
#define SH4_RTC_RDAYAR SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x30)
#define SH4_RTC_RMONAR SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x34)
#define SH4_RTC_RCR1 SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x38)
#define SH4_RTC_RCR2 SH4_BYTE_REG(SH4_RTC_REGS_BASE + 0x3c)

/*
 * UTLB address and data array access (not on SH4-400).
 */
#define SH4_UTLB_ADDR_ARRAY 0xf6000000
#define SH4_UTLB_DATA_ARRAY 0xf7000000

#define SH4_UTLB_ADDR_ARRAY_ENTRY(n) SH4_DWORD_REG(SH4_UTLB_ADDR_ARRAY + (n << 8))
#define SH4_UTLB_ADDR_ARRAY_ENTRY_ASSOC(n) SH4_DWORD_REG((SH4_UTLB_ADDR_ARRAY + (n << 8)) | (1 << 7))
#define SH4_UTLB_DATA_ARRAY_ENTRY(n) SH4_DWORD_REG(SH4_UTLB_DATA_ARRAY + (n << 8))

/*
 * PMB address and data array access (not on SH4-100 or SH4-400).
 */
#define SH4_PMB_ADDR_ARRAY 0xf6100000
#define SH4_PMB_DATA_ARRAY 0xf7100000

#define SH4_PMB_ADDR_ARRAY_ENTRY(n) SH4_DWORD_REG(SH4_PMB_ADDR_ARRAY + (n << 8))
#define SH4_PMB_DATA_ARRAY_ENTRY(n) SH4_DWORD_REG(SH4_PMB_DATA_ARRAY + (n << 8))

/*----------------------------------------------------------------------------*/

#endif /* __SH4REG_H */
