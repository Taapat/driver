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

#ifndef _MME_HANDLE_H
#define _MME_HANDLE_H

/* Encode <type,cpu,ver,idx> into the 32-bit MME handle
 *
 * [31-29] Handle type (3-bit)
 * [28-24] Cpu         (5-bit)
 * [19-12] Version     (12-bit)
 * [11-00] Index       (12-bit)
 *
 */
#define _MME_HANDLE_TYPE_SHIFT 		(29)
#define _MME_HANDLE_TYPE_MASK 		(0x7)
#define _MME_HANDLE_CPU_SHIFT 		(24)
#define _MME_HANDLE_CPU_MASK 		(0x1f)
#define _MME_HANDLE_VER_SHIFT 		(12)
#define _MME_HANDLE_VER_MASK 		(0xfff)
#define _MME_HANDLE_IDX_SHIFT 		(0)
#define _MME_HANDLE_IDX_MASK 		(0xfff)

/* Generate a handle from the TYPE, CPU, VER and IDX
 * We assume that TYPE, CPU & IDX don't overflow their bitmask widths
 * but for VER we need to mask and wrap it
 */
#define _MME_HANDLE(TYPE, CPU, VER, IDX) ((TYPE) << _MME_HANDLE_TYPE_SHIFT | ((CPU) << _MME_HANDLE_CPU_SHIFT) | ((VER) & _MME_HANDLE_VER_MASK) << _MME_HANDLE_VER_SHIFT | ((IDX) & _MME_HANDLE_IDX_MASK) << _MME_HANDLE_IDX_SHIFT)

/* Extract the Type from a handle */
#define _MME_HDL2TYPE(HDL)	    	(((HDL) >> _MME_HANDLE_TYPE_SHIFT) & _MME_HANDLE_TYPE_MASK)

/* Extract the CPU number from a handle */
#define _MME_HDL2CPU(HDL)	    	(((HDL) >> _MME_HANDLE_CPU_SHIFT) & _MME_HANDLE_CPU_MASK)

/* Extract the Version number from a handle */
#define _MME_HDL2VER(HDL)	    	(((HDL) >> _MME_HANDLE_VER_SHIFT) & _MME_HANDLE_VER_MASK)
  
/* Extract the Index from a handle */
#define _MME_HDL2IDX(HDL)	    	(((HDL) >> _MME_HANDLE_IDX_SHIFT) & _MME_HANDLE_IDX_MASK)

/* Split a handle into its component parts */
#define _MME_DECODE_HDL(HDL, TYPE, CPU, VER, IDX) do { \
    (TYPE) = _MME_HDL2TYPE(HDL);		       \
    (CPU)  = _MME_HDL2CPU(HDL);			       \
    (VER)  = _MME_HDL2VER(HDL);			       \
    (IDX)  = _MME_HDL2IDX(HDL); } while (0)
			       
/*
 * MME Possible handle types
 */
#define _MME_TYPE_TRANSFORMER	(0x5)	/* TRANSFORMER instance handle */

#endif /* _MME_HANDLE_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
