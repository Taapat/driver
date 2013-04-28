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
 * b2000/stiH415 host
 */

/*
 * Mailbox allocation 
 * MBOX1_0x000 -> a9     [IRQ 1]
 * MBOX0_0x100 -> rt     [IRQ 0]
 * MBOX4_0x100 -> video0 [IRQ 0]
 * MBOX2_0x100 -> video1 [IRQ 0]
 * MBOX1_0x100 -> audio0 [IRQ 0]
 * MBOX5_0x100 -> audio1 [IRQ 0]
 * MBOX3_0x100 -> gp     [IRQ 0]
 
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

#define MBOX0_0_ADDR		0xfde0e000
#define MBOX1_0_ADDR		0xfde0e800
#define MBOX1_1_ADDR		0xfde0e900
#define MBOX2_0_ADDR		0xfde0f000
#define MBOX3_0_ADDR		0xfde0f800
#define MBOX4_0_ADDR		0xfde10000
#define MBOX5_0_ADDR		0xfde10800
#define MBOX6_0_ADDR		0xfde11000
#define MBOX7_0_ADDR	 0xfde11800


#ifdef __os21__

#include <os21.h>

/* #include <os21/st40/stiH415.h> */
extern interrupt_name_t OS21_INTERRUPT_MAILBOX_1; //MBOX0 ST40_IRQ
#define MBOX0_IRQ0	((unsigned int) &OS21_INTERRUPT_MAILBOX_1)

#else

#error Invalid OS type.

#endif

struct bsp_mbox_regs bsp_mailboxes[] = 
{
  { 
    .base	= (void *) (MBOX0_0_ADDR), 		    /* MBOX0.0 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX0_0_ADDR+0x100), 		/* MBOX0.1 Mailbox */
    .interrupt	= MBOX0_IRQ0,								      /* We own this*/
    .flags      = 0
  },

  { 
    .base	= (void *) (MBOX1_0_ADDR), 		    /* MBOX1.0 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX1_0_ADDR+0x100), 		/* MBOX1.1 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },

  { 
    .base	= (void *) (MBOX2_0_ADDR), 		    /* MBOX2.0 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX2_0_ADDR+0x100),		 /* MBOX2.1 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },

  {
    .base	= (void *) (MBOX3_0_ADDR),							/* MBOX3.0 Mailbox */
    .interrupt  = 0,
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX3_0_ADDR+0x100),		 /* MBOX3.1 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },

  {
    .base	= (void *) (MBOX4_0_ADDR),		      /* MBOX4.0 Mailbox */
    .interrupt  = 0,
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX4_0_ADDR+0x100),		  /* MBOX4.1 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },

  { 
    .base	= (void *) (MBOX5_0_ADDR),		      /* MBOX5.0 Mailbox */
    .interrupt  = 0,				
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX5_0_ADDR+0x100),		  /* MBOX5.1 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },

  { 
    .base	= (void *) (MBOX6_0_ADDR),		      /* MBOX6.0 Mailbox */
    .interrupt  = 0,				
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX6_0_ADDR+0x100),		  /* MBOX6.1 Mailbox */
    .interrupt	= 0,
    .flags      = 0
  },

  { 
    .base	= (void *) (MBOX7_0_ADDR),		      /* MBOX7.0 Mailbox */
    .interrupt  = 0,				
    .flags      = 0
  },
  { 
    .base	= (void *) (MBOX7_0_ADDR+0x100),		  /* MBOX7.1 Mailbox */
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
