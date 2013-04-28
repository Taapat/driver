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
 * File     : asmdefs.h
 * Synopsis : Macros to support assembly programming
 *
 *
 */
#ifndef ASMDEFS_H
#define ASMDEFS_H

/* This is defined by the compiler to be a prefix added to all C namespace symbols */
#ifndef __USER_LABEL_PREFIX__
/* If not defined, assume it should be the empty string */
#define __USER_LABEL_PREFIX__
#endif

/* Use this to ensure a global assembly symbol is visible in C with the same name */
#define __ASMDEFS_CONCAT2(a, b)	a ## b
#define __ASMDEFS_CONCAT(a, b)	__ASMDEFS_CONCAT2(a, b)
#define CDECL(symname)		__ASMDEFS_CONCAT(__USER_LABEL_PREFIX__, symname)

#if defined(__ASSEMBLER__) || defined(_SH4REG_ASM_)
#ifndef NO_ASM_MACROS

	/* Build up a 32-bit constant into R0 */
.macro MOV_CONST32_R0 p1:req
	mov	#(\p1 >> 24), r0
	shll8	r0
	.ifne	((\p1 >> 16) & 0xFF)
	or	#((\p1 >> 16) & 0xFF), r0
	.endif
	shll8	r0
	.ifne	((\p1 >> 8) & 0xFF)
	or	#((\p1 >> 8) & 0xFF), r0
	.endif
	shll8	r0
	.ifne	(\p1 & 0xFF)
	or	#(\p1 & 0xFF), r0
	.endif
.endm

	/* Build up a 16-bit constant into R0 */
.macro MOV_CONST16_R0 p1:req
	mov	#((\p1 >> 8) &0xFF), r0
	shll8	r0
	.ifne	(\p1 & 0xFF)
	or	#(\p1 & 0xFF), r0
	.endif
.endm

	/* Unset top 3 bits of PC */
.macro ENTER_P0
	mova	1f, r0
	mov	#0xE0, r1
	shll16	r1
	shll8	r1
	not	r1, r1		/* MASK is 0x1fffffff */
	and	r1, r0		/* unset top 3-bits */
	jmp	@r0
	  nop
.balign 4
1:
.endm

	/* OR 0x80 into top byte of PC */
.macro ENTER_P1
	mova	1f, r0
	mov	#0xE0, r1
	shll16	r1
	shll8	r1
	not	r1, r1		/* MASK is 0x1fffffff */
	and	r1, r0		/* unset top 3-bits */
	mov	#0x80, r1
	shll16	r1
	shll8	r1		/* MASK is 0x80000000 */
	or	r1, r0		/* put PC in P1 */
	jmp	@r0
	  nop
.balign 4
1:
.endm

	/* OR 0xA0 into top byte of PC */
.macro ENTER_P2
	mova	1f, r0
	mov	#0xE0, r1
	shll16	r1
	shll8	r1
	not	r1, r1		/* MASK is 0x1fffffff */
	and	r1, r0		/* unset top 3-bits */
	mov	#0xA0, r1
	shll16	r1
	shll8	r1		/* MASK is 0xA0000000 */
	or	r1, r0		/* put PC in P2 */
	jmp	@r0
	  nop
.balign 4
1:
.endm

/*
 * Write out a series of bytes, monotonically increasing
 * in value from "first" to "last" (inclusive).
 *
 * Usage:	BYTES <first> <last>
 * where <first> is the first byte to generate
 * where <last>  is the last byte to generate
 *
 * Note: this macro uses recursion (one level per byte)
 */
.macro BYTES first=0, last=63
	.byte \first
	.if \last-\first
	BYTES "(\first+1)",\last	/* note: recursion */
	.endif
.endm

	/*
	 * Create a PIC function label
	 */
.macro PIC_LABEL symbol:req, instance=0
	.L_\symbol\()_pic\instance:	.long \symbol - .
.endm

	/*
	 * Load a PIC symbol address into a register
	 * Note: Clobbers r0
	 */
.macro PIC_MOV32 symbol:req, reg:req, instance=0
	mova	.L_\symbol\()_pic\instance, r0
	mov.l	@r0, \reg
	add	r0, \reg
.endm

	/*
	 * Call a PIC function
	 */
.macro PIC_CALL func:req, instance=0
	mova	.L_\func\()_pic\instance, r0
	mov.l	@r0, r1
	add	r1, r0
	jsr	@r0
	  nop
.endm

	/*
	 * Ensure instruction coherency using an rte instruction
	 */
.macro INSTRUCTION_COHERE_TO label:req
	stc	sr, r0			/* Ensure SR register remains the same after rte */
	ldc	r0, ssr
	mova	\label, r0		/* Use rte to skip the data */
	ldc	r0, spc
	rte
	  nop
.endm

#endif /* NO_ASM_MACROS */
#endif /* defined(__ASSEMBLER__) || defined(_SH4REG_ASM_) */

#endif /* ASMDEFS_H */
