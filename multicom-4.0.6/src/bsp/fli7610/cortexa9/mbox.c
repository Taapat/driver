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
 * fli7610 arm cortexa9
 */

/*
 * Mailbox allocation
 * MBOX1_0x000 -> a9     [IRQ 1]
 * MBOX0_0x100 -> rt     [IRQ 0]
 * MBOX4_0x100 -> video0 [IRQ 0]
 * MBOX2_0x100 -> video1 [IRQ 0]
 * MBOX1_0x100 -> audio  [IRQ 0]
 * MBOX5_0x100 -> gp0    [IRQ 0]
 * MBOX3_0x100 -> gp1    [IRQ 0]

 * MBOX0_0x000 ->  ???
 * MBOX2_0x000 ->  ???
 * MBOX3_0x000 ->  ???
 * MBOX4_0x000 ->  ???
 * MBOX5_0x000 ->  ???
 * MBOX6_0x000 ->  ???
 * MBOX6_0x100 ->  ???
 * MBOX7_0x000 ->  ???
 * MBOX7_0x100 ->  ???
 *
 * IRQ line naming
 * IRQ0 ---> ST40_IRQ
 * IRQ1 ---> ST200_IRQ
 */

#include <bsp/_bsp.h>

#define MBOX0_ADDR	0xfde0e000
#define MBOX1_ADDR	0xfde0e800
#define MBOX2_ADDR	0xfde0f000
#define MBOX3_ADDR	0xfde0f800
#define MBOX4_ADDR	0xfde10000
#define MBOX5_ADDR	0xfde10800
#define MBOX6_ADDR	0xfde11000
#define MBOX7_ADDR	0xfde11800

#ifdef __os21__
#error OS21 not supported on fli7610 arm cortexa9 core
#endif

#ifdef __KERNEL__

/* Based on an IRQ base of 32 + offsets in ADCS xxxxx (Table xxxxx) */
#define IRQ_BASE	(32)
#define MBOX0_IRQ1 (IRQ_BASE + 1)
#define MBOX1_IRQ1	(IRQ_BASE + 2)
#define MBOX2_IRQ1	(IRQ_BASE + 3)
#define MBOX3_IRQ1	(IRQ_BASE + 4)
#define MBOX4_IRQ1	(IRQ_BASE + 5)
#define MBOX5_IRQ1	(IRQ_BASE + 6)
#define MBOX6_IRQ1	(IRQ_BASE + 7)
#define MBOX7_IRQ1	(IRQ_BASE + 8)

#endif

struct bsp_mbox_regs bsp_mailboxes[] =
{
  {
    .base	= (void *) (MBOX0_ADDR), 								/*  Mailbox (MBOX0.1) */
    .interrupt	= 0,
    .flags      = 0
  },
  {
    .base	= (void *) (MBOX0_ADDR+0x100), 		/* Mailbox (MBOX0.2) */
    .interrupt	= 0,
    .flags      = 0
  },

  {
    .base	= (void *) (MBOX1_ADDR), 							/* Mailbox (MBOX1.1) */
    .interrupt	= MBOX1_IRQ1,																/* *WE* own this one */
    .flags      = 0
  },
  {
    .base	= (void *) (MBOX1_ADDR+0x100), 		/* Mailbox (MBOX1.2) */
    .interrupt	= 0,
    .flags      = 0
  },


  { .base	= (void *) (MBOX2_ADDR),									/* Mailbox (MBOX2.1) */
    .interrupt  = 0,
    .flags      = 0
  },
  { .base	= (void *) (MBOX2_ADDR+0x100),			/* Mailbox (MBOX2.2) */
    .interrupt	= 0,
    .flags      = 0
  },


  { .base	= (void *) (MBOX3_ADDR),								/* Mailbox (MBOX3.1) */
    .interrupt  = 0,
    .flags      = 0
  },
  { .base	= (void *) (MBOX3_ADDR+0x100),		/* Mailbox (MBOX3.2) */
    .interrupt	= 0,
    .flags      = 0
  },


  {
    .base	= (void *) (MBOX4_ADDR), 							/* Mailbox (MBOX4.1) */
    .interrupt	= 0,
    .flags      = 0
  },
  {
    .base	= (void *) (MBOX4_ADDR+0x100), 	/* Mailbox (MBOX4.2) */
    .interrupt	= 0,
    .flags      = 0
  },


  {
    .base	= (void *) (MBOX5_ADDR), 							/* Mailbox (MBOX5.1) */
    .interrupt	= 0,
    .flags      = 0
  },
  {
    .base	= (void *) (MBOX5_ADDR+0x100), 	/* Mailbox (MBOX5.2) */
    .interrupt	= 0,
    .flags      = 0
  },


  { .base	= (void *) (MBOX6_ADDR),								/* Mailbox (MBOX6.1) */
    .interrupt  = 0,
    .flags      = 0
  },
  { .base	= (void *) (MBOX6_ADDR+0x100),		/* Mailbox (MBOX6.2) */
    .interrupt	= 0,
    .flags      = 0
  },


  { .base	= (void *) (MBOX7_ADDR),								/* Mailbox (MBOX7.1) */
    .interrupt  = 0,
    .flags      = 0
  },
  { .base	= (void *) (MBOX7_ADDR+0x100),		/* Mailbox (MBOX7.2) */
    .interrupt	= 0,
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
