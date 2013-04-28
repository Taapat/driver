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

#ifndef _ICS_H
#define _ICS_H

#define __ICS_VERSION__		"1.0.2 : Final"
#define __ICS_VERSION_MAJOR__	1
#define __ICS_VERSION_MINOR__	0
#define __ICS_VERSION_PATCH__	2

/* Version macros like the ones found in Linux */
#define ICS_VERSION(MA,MI,PA)	(((MA) << 16) + ((MI) << 8) + (PA))
#define ICS_VERSION_CODE	ICS_VERSION(__ICS_VERSION_MAJOR__, __ICS_VERSION_MINOR__, __ICS_VERSION_PATCH__)

#include <ics/ics_os.h>		/* OS specific headers */
#include <ics/ics_types.h>	/* ICS base types */
#include <ics/ics_cache.h>	/* ICS Cacheline/Page Size defines */
#include <ics/ics_init.h>	/* Initialisation functions */
#include <ics/ics_debug.h>	/* Debug logging and control */

#include <ics/ics_port.h>	/* Port management */
#include <ics/ics_msg.h>	/* Message send & receive */
#include <ics/ics_nsrv.h>	/* Nameserver API */
#include <ics/ics_region.h>	/* Memory region management */
#include <ics/ics_heap.h>	/* Heap creation */

#include <ics/ics_channel.h>	/* Channel API */
#include <ics/ics_connect.h>	/* CPU connection API */

#include <ics/ics_load.h>	/* Companion firmware loader API */
#include <ics/ics_cpu.h>	/* Companion cpu control API */
#include <ics/ics_dyn.h>	/* Dynamic module loader API */
#include <ics/ics_watchdog.h>	/* Watchdog API */
#include <ics/ics_stats.h>	/* Statistics API */

#endif /* _ICS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
