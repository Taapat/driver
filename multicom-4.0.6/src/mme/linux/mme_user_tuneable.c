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

/* Called from userspace ioctl() */
int mme_user_set_tuneable(mme_user_t *instance, void *arg)
{
	int res = 0;
	MME_ERROR status = MME_INVALID_ARGUMENT;
	mme_tuneables_t *usertuneable = (mme_tuneables_t *) arg;

	mme_tuneables_t tuneable;

 if (copy_from_user(&tuneable,usertuneable,sizeof(mme_tuneables_t))) {
		res = -EFAULT;
		goto exit;
	}

 status = MME_ModifyTuneable(tuneable.key,tuneable.value);

 if (put_user(status, &(usertuneable->status))) {
		res = -EFAULT;
	}
exit:
 return res;
}

int mme_user_get_tuneable(mme_user_t *instance, void *arg)
{
	int res = 0;
	mme_tuneables_t *usertuneable = (mme_tuneables_t *) arg;

	mme_tuneables_t tuneable;

 if (get_user(tuneable.key,&(usertuneable->key))) {
		res = -EFAULT;
		goto exit;
	}

 tuneable.value = MME_GetTuneable(tuneable.key);

 if (put_user(tuneable.value, &(usertuneable->value))) {
		res = -EFAULT;
	}
exit:
 return res;
}
