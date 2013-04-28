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

#include <bsp/_bsp.h>

/* NMHDK/fli7610 */

/*
 * Logical CPU numbering
 *
 *    0 cortexa9	host (MASTER)
 *    1 st40 			rt
 *    2 st231 		video0
 *    3 st231 		video1
 *    4 st231 		audio
 *    5 st231 		gp0
 *    6 st231 		gp1
 *
 */

/*
 * BSP CPU info
 *
 * This table provides the complete list of all the CPUs
 * on the SoC.
 * Each CPU is assigned a logical CPU number which is
 * used by ICS to identify it. Logical CPU 0 is assumed
 * to be the MASTER.
 *
 * The order of the CPUs in this table must match that of
 * the other BSP tables (e.g. reset.c) so that the correct
 * CPU registers are used for load/reset
 */
struct bsp_cpu_info bsp_cpus[] =
{
  {
    .name 		= "host",		/* host */
    .type     = "cortexa9",
    .num	  	= 0,			/* MASTER */
    .flags		= 0,
  },

  {
    .name 		= "rt",			/* real time */
    .type     = "st40",
    .num		  = 1,			/* st40 */
    .flags		= 0,
  },

  {
    .name 		= "video0",		/* video */
    .type     = "st231",
    .num	  	= 2,
    .flags		= 0,
  },

  {
    .name 		= "video1", /* video 1 */
    .type     = "st231",
    .num		  = 3,
    .flags		= 0,
  },

  {
    .name 	  = "audio",/* audio */
    .type     = "st231",
    .num		  = 4,
    .flags		= 0,
  },

  {
    .name 		= "gp0",	 /* general purpose 0 */
    .type     = "st231",
    .num		  = 5,
    .flags		= 0,
  },

  {
    .name 		= "gp1",		/* general purpose 1 */
    .type     = "st231",
    .num		  = 6,
    .flags		= 0,
  },

};

unsigned int bsp_cpu_count = sizeof(bsp_cpus)/sizeof(bsp_cpus[0]);

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
