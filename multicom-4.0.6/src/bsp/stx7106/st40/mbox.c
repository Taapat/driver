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

/*
 * MB680/sti7106 st40
 */

#include <bsp/_bsp.h>

#define MBOX0_ADDR	0xfe211000
#define MBOX1_ADDR	0xfe212000

#ifdef __os21__

#include <os21.h>

/* #include <os21/st40/sti7106.h> */
extern interrupt_name_t OS21_INTERRUPT_MBOX_DMU_SH4;
extern interrupt_name_t OS21_INTERRUPT_MBOX_AUD_SH4;

#define VIDEO_MBOX_IRQ	((unsigned int) &OS21_INTERRUPT_MBOX_DMU_SH4)	/* MBOX0 SET2 */
#define AUDIO_MBOX_IRQ	((unsigned int) &OS21_INTERRUPT_MBOX_AUD_SH4)	/* MBOX1 SET2 */

#endif

#ifdef __KERNEL__

/* See ADCS XXXXXXXX (Table X) */

#define VIDEO_MBOX_IRQ	(136)						/* MBOX0 SET2 */
#define AUDIO_MBOX_IRQ	(137)						/* MBOX1 SET2 */

#endif

struct bsp_mbox_regs bsp_mailboxes[] = 
{
  { 
    .base	= (void *) (MBOX0_ADDR), 		/* Video LX Mailbox (MBOX0.1) */
    .interrupt	= 0,					/* Video owns SET1 */
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX0_ADDR+0x100), 		/* Video LX Mailbox (MBOX0.2) */
    .interrupt	= VIDEO_MBOX_IRQ,			/* *WE* own SET2 */
    .flags      = 0
  },


  {
    .base	= (void *) (MBOX1_ADDR),		/* Audio LX Mailbox (MBOX1.1) */
    .interrupt  = 0,					/* Audio owns SET1 */
    .flags      = 0
  },
  {
    .base	= (void *) (MBOX1_ADDR+0x100),		/* Audio LX Mailbox (MBOX1.2) */
    .interrupt  = AUDIO_MBOX_IRQ,			/* *WE* own SET2 */
    .flags      = 0
  },

};

unsigned int bsp_mailbox_count = sizeof(bsp_mailboxes)/sizeof(bsp_mailboxes[0]);

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
