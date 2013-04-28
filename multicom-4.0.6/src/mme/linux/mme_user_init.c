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

#include <mme.h>	/* External defines and prototypes */

#include "_mme_sys.h"	/* Internal defines and prototypes */

#include <mme/mme_ioctl.h>	/* External ioctl interface */

#include "_mme_user.h"		/* Internal definitions */

#include <asm/uaccess.h>

/*
 * Userspace->Kernel wrappers for MME calls
 */

int mme_user_mmeinit(mme_user_t *instance, void *arg)
{
	 int                    res = -ENODEV;
	 MME_ERROR              status = MME_INTERNAL_ERROR;

  mme_userinit_t *user_mmeinit = (mme_userinit_t *) arg;

  /* Init MME */
  status = MME_Init(); 
  if (put_user(status,&(user_mmeinit->status))) {
		  res = -EFAULT;
	 }
 return res;
}

int mme_user_mmeterm(mme_user_t *instance, void *arg)
{
	 int                    res = -ENODEV;
	 MME_ERROR              status = MME_INTERNAL_ERROR;

  mme_userterm_t *user_mmeterm = (mme_userterm_t *) arg;

  /* Init MME */
  status = MME_Term(); 
  if (put_user(status,&(user_mmeterm->status))) {
		  res = -EFAULT;
	 }
 return res;
}
