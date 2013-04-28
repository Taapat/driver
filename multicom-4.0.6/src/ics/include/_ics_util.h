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

#ifndef _ICS_UTIL_SYS_H
#define _ICS_UTIL_SYS_H

/*
 * A collection of useful macros
 */

#define ALIGNUP(x,a)    (((unsigned long)(x) + ((a)-1UL)) & ~((a)-1UL)) /* 'a' must be a power of 2 */
#define ALIGNED(x,a)	(!((unsigned long)(x) & ((a)-1UL)))		/* 'a' must be a power of 2 */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)	(sizeof(a)/sizeof((a)[0]))
#endif

/* Determine whether a positive integer is a power of 2 or not */
#define powerof2(N)	((N) && ((N-1) & (N)) == 0)

#ifndef min
#define min(_a, _b) 	(((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef max
#define max(_a, _b) 	(((_a) > (_b)) ? (_a) : (_b))
#endif

#ifdef __GNUC__
#define log2i(X)	__builtin_ctz(X)
#else
static inline 
unsigned int log2i(unsigned int x)
{
  unsigned int level = 0;

  while (x)
  {
    x >>= 1;
    level++;
  }

  return level;
}
#endif /* __builtin_ctz */

#ifdef __GNUC__
#define RETURN_ADDRESS(N)	__builtin_return_address((N))
#else
#define RETURN_ADDRESS(N)	NULL
#endif

#endif /* _ICS_UTIL_SYS_H */
