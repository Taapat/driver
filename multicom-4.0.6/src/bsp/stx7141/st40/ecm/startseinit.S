/*
 * File     : startseinit.S
 * Synopsis : Code to transition an ST40 from 29-bit mode to 32-bit SE mode,
 *            configuring the PMBs, before jumping to normal application
 *            entrypoint.
 *
 * Copyright (c) 2008-2010 STMicroelectronics Limited.  All right reserved.
 *
 * For ST40 applications which need to run in SE mode with configured PMBs we
 * can link the following code and use __start_se_init as the executable entry-
 * point.
 *
 * This code will check whether SE mode is on, and if not configure the PMBs and
 * enable it.  It is PIC code so that even after linking it can be run from any
 * address (e.g. it can be copied around and still work, or it can be run from
 * a 29-bit BART address equivalent to the 32-bit address it is linked for.
 *
 * If AT is on, we disable it and run from P0 while turning SE on as we know we
 * can access the 29-bit RAM via P0 and it will not disappear when SE mode is
 * enabled.  If we were running from P1 or P2 we would need to know the PMBs
 * were going to identity map the RAM to the same address, and we cannot
 * guarantee that (indeed it is not the case for the default way the toolset
 * configures the PMBs).
 *
 * This code uses pokeloop.S and pmbconfig_<platform>.S (or the filename
 * in the PMBCONFIGINCLUDE macro).
 */

/*
 * This code is directly lifted from the OS21 VOB ; examples/st40/os21/romloader/startseinit.S
 */
	
#define _SH4REG_ASM_
#include <asmdefs.h>

/* Make this code specific to the ecm on stx7141 */
#define PMBCONFIGINCLUDE "pmbconfig_mb628.s"
#include "../sti7141reg.h"

/* This is the symbol we will jump to after enabling SE mode - normally start
   for the standard C runtime initialisation.
 */
#ifndef CRT_ENTRYPOINT
#define CRT_ENTRYPOINT			start
#endif /* CRT_ENTRYPOINT */

	.section	.alt0text, "ax", @progbits
	.org	0
	.global	__start_se_init
	.import	CRT_ENTRYPOINT
	.balign	2

__start_se_init:
	/* First check whether SE mode is already on (both MMUCR and PASCR) */
#ifndef __SH4_300__
	MOV_CONST32_R0	SH4_CCN_MMUCR		/* R0 = MMUCR address */
	mov.l	@r0, r1
	mov	#(1 << 4), r0			/* R0 = MMUCR SE bit */
	tst	r0, r1				/* In SE mode? */
	bf	crtjmp				/* Skip straight to normal entrypoint */
#endif /* !__SH4_300__ */

#ifndef __SH4_200__
	MOV_CONST32_R0	SH4_CCN_PASCR		/* R0 = PASCR address */
	mov.l	@r0, r1
	MOV_CONST32_R0	(1 << 31)		/* R0 = PASCR SE bit */
	tst	r0, r1				/* In SE mode? */
	bf	crtjmp				/* Skip straight to normal entrypoint */
#endif /*!__SH4_200__ */

	/*
	 * We're not in SE mode.  Ensure we're running from P0 before
	 * continuing.  We need to disable AT (address translation in P0 and P3)
	 * if it is on.
	 */
	MOV_CONST32_R0	SH4_CCN_MMUCR		/* R0 = MMUCR address */
	mov	r0, r2				/* R2 = MMUCR address */
	mov.l	@r2, r1
	mov	#(1 << 0), r0			/* R0 = MMUCR AT bit */
	not	r0, r0
	and	r1, r0				/* Force AT bit to 0 */
	mov.l	r0, @r2				/* AT mode off (if it was on) */

	/* Jump to P0 using an rte */
	stc	sr, r1				/* Ensure SR register remains the same after rte */
	ldc	r1, ssr
	mova	1f, r0				/* Use rte to jump to P0 address */
	mov	r0, r1
	MOV_CONST32_R0	0x1FFFFFFF		/* R0 = P0 mask */
	and	r0, r1
	ldc	r1, spc
	rte
	  nop

	.balign	4
	/*
	 * Enable I-cache & D-cache; D-cache in copy-back mode if not already
	   enabled.
	 */
1:	MOV_CONST32_R0	SH4_CCN_CCR		/* R0 = CCR address */
	mov	r0, r2				/* R2 = CCR address */
	mov.l	@r2, r1
	MOV_CONST32_R0	((1 << 8) | (1 << 0))	/* Bits 0 & 8 for OC & IC enable */
	and	r0, r1
	cmp/eq	r1, r0				/* Both caches enabled? */
	bt	1f
	MOV_CONST32_R0	0x8000090D		/* Caches on, D-cache copy-back,
						   enhanced mode on SH4-200
						   series cores */
	mov.l	r0, @r2

	/* Ensure instruction coherency using an rte */
	INSTRUCTION_COHERE_TO	1f

	.balign	4
	/*
	 * We now call the poke loop with the __pmb_setup_table address in R4 to
	 * configure the PMBs as required for 32-bit 'space enhancement' mode
	 * and set the SE bit.
	 */
1:	PIC_MOV32	__pmb_setup_table, r4	/* R4 = PMB pokes start address */
	PIC_CALL	__pokeLoop

	/*
	 * Ensure cache coherency using an rte at the same time as jumping to
	 * the C runtime entrypoint.
	 */
crtjmp:	stc	sr, r0				/* Ensure SR register remains the same after rte */
	ldc	r0, ssr
	mov.l	crt_entrypoint_p, r0		/* Use rte to jump to the normal entrypoint */
	ldc	r0, spc
	rte
	  nop

	.balign	4
PIC_LABEL	__pmb_setup_table
PIC_LABEL	__pokeLoop

crt_entrypoint_p:
	.long	CRT_ENTRYPOINT

/* Include the poke loop code and poke table so we know the poke table code and
 * pmbconfig_*.S file are consistently preprocessed (they have the same
 * poketable.h).
 */
#define MUST_RUN_FROM_ICACHE		0
#define POKELOOPSECTION			.alt0text
#define PMB_CONFIG_TABLE_ONLY
#include "../pokeloop.S"
	.balign	4
__pmb_setup_table:
#ifdef PMBCONFIGINCLUDE
#include PMBCONFIGINCLUDE
#else /* !PMBCONFIGINCLUDE */
#error No PMB configuration specified for startseinit.S - define PMBCONFIGINCLUDE
#endif /* PMBCONFIGINCLUDE */

	END_MARKER
__end_pmb_init:
	.end
