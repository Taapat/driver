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

#ifndef _ICS_OS_H
#define _ICS_OS_H

#if defined __os21__
#include <stddef.h>
#endif

#if defined __linux__

#if defined __KERNEL__
/*
 * Linux kernel space
 */
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/semaphore.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
#include <linux/slab.h>
#endif /*  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) */

#else

/*
 * Linux user space
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#endif

#endif /* __linux__ */

#endif /* _ICS_OS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
