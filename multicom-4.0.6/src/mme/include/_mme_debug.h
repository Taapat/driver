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

#ifndef _MME_DEBUG_SYS_H
#define _MME_DEBUG_SYS_H

/* _ICS_OS_EXPORT void mme_debug_printf (const char *fmt, const char *fn, int line, ...); */
#define mme_debug_printf	ics_debug_printf

/* Global debug logging flags */
extern MME_DBG_FLAGS mme_debug_flags;

/* Make sure we log the assertion message into the cyclic buffer */
#define _mme_assert(expr)	do { if (!(expr)) {							\
      					mme_debug_printf("assertion \"%s\" failed file %s\n",		\
							 __func__, __LINE__, #expr, __FILE__); 		\
       }							\
				} while (0)

#if defined(MME_DEBUG) || defined(MME_DEBUG_ASSERT)

#define MME_ASSERT(expr) 	_mme_assert(expr)

#else

#define MME_ASSERT(expr)

#endif

#if defined(MME_DEBUG)
/* Compile in all DEBUG messages */
#define MME_PRINTF(FLAGS, FMT, ...)	if (((FLAGS) == MME_DBG) || ((FLAGS) & mme_debug_flags))	\
    						mme_debug_printf(FMT, __FUNCTION__, __LINE__, ## __VA_ARGS__)

#elif defined(MME_DEBUG_FLAGS)
/* Only compile in DEBUG messages specified by MME_DEBUG_FLAGS (and MME_DBG messages) */
#define MME_PRINTF(FLAGS, FMT, ...)	if (((FLAGS) == MME_DBG) || ((FLAGS) & MME_DEBUG_FLAGS))	\
    						mme_debug_printf(FMT, __FUNCTION__, __LINE__, ## __VA_ARGS__)

#else
/* Only compile in MME_DBG messages */
#define MME_PRINTF(FLAGS, FMT, ...)	if (((FLAGS) == MME_DBG))					\
    						mme_debug_printf(FMT, __FUNCTION__, __LINE__, ## __VA_ARGS__)

#endif

/* This assert macro is always enabled and should be used
 * for non performance critical assertion checks
 */
#define MME_assert(expr)	_mme_assert(expr)

/* ERROR printf 
 * Will cause all error messages to be displayed if 
 * mme_debug_flags has MME_DBG_ERR bit set
 */
#define MME_EPRINTF(FLAGS, FMT, args...) if ((FLAGS|MME_DBG_ERR) & mme_debug_flags)	\
    						mme_debug_printf(FMT, __FUNCTION__, __LINE__, args)


#endif /* _MME_DEBUG_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
