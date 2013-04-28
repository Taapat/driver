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

#ifndef _ICS_HANDLE_H
#define _ICS_HANDLE_H

/* Encode <type,cpu,ver,idx> into the 32-bit ICS handle
 *
 * [31-29] Handle type (3-bit)
 * [28-24] Cpu         (5-bit)
 * [19-12] Version     (12-bit)
 * [11-00] Index       (12-bit)
 *
 */
#define _ICS_HANDLE_TYPE_SHIFT 		(29)
#define _ICS_HANDLE_TYPE_MASK 		(0x7)
#define _ICS_HANDLE_CPU_SHIFT 		(24)
#define _ICS_HANDLE_CPU_MASK 		(0x1f)
#define _ICS_HANDLE_VER_SHIFT 		(12)
#define _ICS_HANDLE_VER_MASK 		(0xfff)
#define _ICS_HANDLE_IDX_SHIFT 		(0)
#define _ICS_HANDLE_IDX_MASK 		(0xfff)

/* Generate a handle from the TYPE, CPU, VER and IDX
 * We assume that TYPE, CPU & IDX don't overflow their bitmask widths
 * but for VER we need to mask and wrap it
 */
#define _ICS_HANDLE(TYPE, CPU, VER, IDX) ((TYPE) << _ICS_HANDLE_TYPE_SHIFT | ((CPU) << _ICS_HANDLE_CPU_SHIFT) | ((VER) & _ICS_HANDLE_VER_MASK) << _ICS_HANDLE_VER_SHIFT | ((IDX) & _ICS_HANDLE_IDX_MASK) << _ICS_HANDLE_IDX_SHIFT)

/* Extract the Type from a handle */
#define _ICS_HDL2TYPE(HDL)	    	(((HDL) >> _ICS_HANDLE_TYPE_SHIFT) & _ICS_HANDLE_TYPE_MASK)

/* Extract the CPU number from a handle */
#define _ICS_HDL2CPU(HDL)	    	(((HDL) >> _ICS_HANDLE_CPU_SHIFT) & _ICS_HANDLE_CPU_MASK)

/* Extract the Version number from a handle */
#define _ICS_HDL2VER(HDL)	    	(((HDL) >> _ICS_HANDLE_VER_SHIFT) & _ICS_HANDLE_VER_MASK)
  
/* Extract the Index from a handle */
#define _ICS_HDL2IDX(HDL)	    	(((HDL) >> _ICS_HANDLE_IDX_SHIFT) & _ICS_HANDLE_IDX_MASK)

/* Split a handle into its component parts */
#define _ICS_DECODE_HDL(HDL, TYPE, CPU, VER, IDX) do { \
    (TYPE) = _ICS_HDL2TYPE(HDL);		       \
    (CPU)  = _ICS_HDL2CPU(HDL);			       \
    (VER)  = _ICS_HDL2VER(HDL);			       \
    (IDX)  = _ICS_HDL2IDX(HDL); } while (0)
			       
/*
 * ICS Possible handle types (3-bit field)
 */
#define _ICS_TYPE_PORT		(0x1)	/* PORT handle */
#define _ICS_TYPE_CHANNEL	(0x2)	/* CHANNEL handle */
#define _ICS_TYPE_DYN		(0x3)	/* DYNAMIC module handle */
#define _ICS_TYPE_REGION	(0x4)	/* REGION handle */
#define _ICS_TYPE_NSRV		(0x5)	/* NSRV entry handle */

#endif /* _ICS_HANDLE_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
