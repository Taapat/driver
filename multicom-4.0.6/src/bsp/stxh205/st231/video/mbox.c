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
 * 
 */ 

/*
 * stih205 ST231 Video
 */


/*
 * Mailbox allocation (ST231 only see even mbox IRQs)
 *
 * MBOX0 -> host
 * MBOX1
 * MBOX2 -> audio
 * MBOX3
 * MBOX4 -> video
 * MBOX5
 *
 */

#include <bsp/_bsp.h>

#define MBOX0_ADDR	0xfdabf000
#define MBOX1_ADDR	0xfdabf100
#define MBOX2_ADDR	0xfdabf800
#define MBOX3_ADDR	0xfdabf900
#define MBOX4_ADDR	0xfdac0000
#define MBOX5_ADDR	0xfdac0100

#include <os21/st200.h>

/* #include <os21/st200/sti7108.h> */
extern interrupt_name_t OS21_INTERRUPT_MAILBOX_0;
extern interrupt_name_t OS21_INTERRUPT_MAILBOX_2;
extern interrupt_name_t OS21_INTERRUPT_MAILBOX_4;

struct bsp_mbox_regs bsp_mailboxes[] = 
{
  { 
    .base	= (void *) (MBOX0_ADDR), 		/* MBOX0 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX1_ADDR), 		/* MBOX1 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },


  { 
    .base	= (void *) (MBOX2_ADDR), 		/* MBOX2 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX3_ADDR),		/* MBOX3 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },


  {
    .base	= (void *) (MBOX4_ADDR),		/* MBOX4 Mailbox */
    .interrupt  = (unsigned int) &OS21_INTERRUPT_MAILBOX_4,/* *WE* own this one */
    .flags      = 0
  },
  {
    .base	= (void *) (MBOX5_ADDR),		/* MBOX5 Mailbox */
    .interrupt  = 0,
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
