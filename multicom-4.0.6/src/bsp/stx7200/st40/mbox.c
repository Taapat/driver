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
 * MB519/sti7200 ST40
 */

#include <bsp/_bsp.h>

#define MBOX0_ADDR	0xfd800000
#define MBOX1_ADDR	0xfd801000
#define MBOX2_ADDR	0xfd802000
#define MBOX3_ADDR	0xfd803000

#ifdef __os21__

#include <os21.h>

/* #include <os21/st40/sti7200.h> */
extern interrupt_name_t OS21_INTERRUPT_MBOX_SH4_AUD0;
extern interrupt_name_t OS21_INTERRUPT_MBOX_SH4_AUD1;
extern interrupt_name_t OS21_INTERRUPT_MBOX_SH4_DMU0;
extern interrupt_name_t OS21_INTERRUPT_MBOX_SH4_DMU1;

#define AUDIO0_MBOX_IRQ	((unsigned int) &OS21_INTERRUPT_MBOX_SH4_AUD0)
#define AUDIO1_MBOX_IRQ	((unsigned int) &OS21_INTERRUPT_MBOX_SH4_AUD1)
#define VIDEO0_MBOX_IRQ	((unsigned int) &OS21_INTERRUPT_MBOX_SH4_DMU0)
#define VIDEO1_MBOX_IRQ	((unsigned int) &OS21_INTERRUPT_MBOX_SH4_DMU1)

#endif

#ifdef __KERNEL__

/* Based on an ILC3 base of 44 + offsets in ADCS 7870016 (Table 6) */

#define AUDIO0_MBOX_IRQ	(44)
#define AUDIO1_MBOX_IRQ	(45)
#define VIDEO0_MBOX_IRQ	(46)
#define VIDEO1_MBOX_IRQ	(47)

#endif

struct bsp_mbox_regs bsp_mailboxes[] = 
{
  { 
    .base	= (void *) (MBOX0_ADDR), 		/* audio0 Mailbox (MBOX0.1) */
    .interrupt	= 0,
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX0_ADDR+0x100), 		/* audio0 Mailbox (MBOX0.2) */
    .interrupt	= AUDIO0_MBOX_IRQ,			/* *WE* own this one */
    .flags      = 0
  },


  { 
    .base	= (void *) (MBOX1_ADDR), 		/* video0 Mailbox (MBOX1.1) */
    .interrupt	= 0,
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX1_ADDR+0x100), 		/* video0 Mailbox (MBOX1.2) */
    .interrupt	= VIDEO0_MBOX_IRQ,			/* *WE* own this one */
    .flags      = 0
  },


  { .base	= (void *) (MBOX2_ADDR),		/* audio1 Mailbox (MBOX2.1) */
    .interrupt  = 0,
    .flags      = 0
  },
  { .base	= (void *) (MBOX2_ADDR+0x100),		/* audio1 Mailbox (MBOX2.2) */
    .interrupt  = AUDIO1_MBOX_IRQ,			/* *WE* own this one */
    .flags      = 0
  },


  { .base	= (void *) (MBOX3_ADDR),		/* video1 Mailbox (MBOX3.1) */
    .interrupt  = 0,
    .flags      = 0
  },
  { .base	= (void *) (MBOX3_ADDR+0x100),		/* video1 Mailbox (MBOX3.2) */
    .interrupt  = VIDEO1_MBOX_IRQ,			/* *WE* own this one */
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
