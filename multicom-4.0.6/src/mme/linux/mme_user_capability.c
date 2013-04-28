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

int mme_user_get_capability (mme_user_t *instance, void *arg)
{
	int res = 0;
	MME_ERROR status = MME_INTERNAL_ERROR;

	mme_get_capability_t *cap = (mme_get_capability_t *) arg;

	char                         name[MME_MAX_TRANSFORMER_NAME+1];
	unsigned int                 length;
	char*                        namePtr;

	MME_TransformerCapability_t  capability;
	MME_TransformerCapability_t* capPtr;
	MME_GenericParams_t          paramsPtr;  

	if (get_user(length, &(cap->length))) {
		res = -EFAULT;
		goto exit;
	}
	if (get_user(namePtr, &(cap->name))) {
		res = -EFAULT;
		goto exit;
	}
	if (get_user(capPtr, &(cap->capability))) {
		res = -EFAULT;
		goto exit;
	}

	/* Don't allow excessive length names through */
	length = min(MME_MAX_TRANSFORMER_NAME, length);
	
	/* Copy the name */
	/* Copy the capability */
	if (copy_from_user(name, namePtr, length) || 
	    copy_from_user(&capability, capPtr, sizeof(capability))) {
		res = -EFAULT;
		goto exit;
        }
	name[length] = '\0';

	/* Allocate the params space in kernel memory */
	paramsPtr = capability.TransformerInfo_p;
	if (capability.TransformerInfoSize) {
		capability.TransformerInfo_p = _ICS_OS_MALLOC(capability.TransformerInfoSize);
		if (!capability.TransformerInfo_p) {
			status = MME_NOMEM;
			res = -ENOMEM;
			goto exit;
                }
        }

	status = MME_GetTransformerCapability(name, &capability);

	/* Copy the data back to user space */
	if (status == MME_SUCCESS) {
		if (copy_to_user(paramsPtr, capability.TransformerInfo_p, capability.TransformerInfoSize)        ||
		    copy_to_user(&(capPtr->Version), &(capability.Version), sizeof(capability.Version))          ||
		    copy_to_user(&(capPtr->InputType), &(capability.InputType), sizeof(capability.InputType))    ||
		    copy_to_user(&(capPtr->OutputType), &(capability.OutputType), sizeof(capability.OutputType))) {
			res = -EFAULT;
			goto free;
		}
        }

	/* FALLTHRU */
free:
	if (capability.TransformerInfoSize)
		_ICS_OS_FREE(capability.TransformerInfo_p);

exit:
	/* Pass back MME status code */
        if (put_user(status, &(cap->status))) {
		res = -EFAULT;
	}

	return res;
}

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
