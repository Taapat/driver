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

static uint  debug_flags;
static int   init=1;
static ulong pool_size = 0;

#if defined(__KERNEL__) && defined(MODULE)

MODULE_DESCRIPTION("MME: Multi Media Encoder");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("GPL");

module_param(debug_flags, int, 0);
MODULE_PARM_DESC(debug_flags, "Bitmask of MME debugging flags");

module_param(init, int, S_IRUGO);
MODULE_PARM_DESC(init, "Do not initialize MME if set to 0");

module_param(pool_size, ulong, 0);
MODULE_PARM_DESC(pool_size, "Size of the MME_DataBuffer buffer pool");

/*
 * Export all the MME module symbols
 */

EXPORT_SYMBOL( MME_SendCommand );
EXPORT_SYMBOL( MME_AbortCommand );
EXPORT_SYMBOL( MME_NotifyHost );

EXPORT_SYMBOL( MME_AllocDataBuffer );
EXPORT_SYMBOL( MME_FreeDataBuffer );

EXPORT_SYMBOL( MME_RegisterTransport );
EXPORT_SYMBOL( MME_DeregisterTransport );

EXPORT_SYMBOL( MME_Init );
EXPORT_SYMBOL( MME_Term );
EXPORT_SYMBOL( MME_Run );		/* DEPRECATED */

EXPORT_SYMBOL( MME_RegisterTransformer );
EXPORT_SYMBOL( MME_DeregisterTransformer );
EXPORT_SYMBOL( MME_IsTransformerRegistered );
EXPORT_SYMBOL( MME_TermTransformer );
EXPORT_SYMBOL( MME_InitTransformer );

EXPORT_SYMBOL( MME_KillTransformer );
EXPORT_SYMBOL( MME_KillCommand );
EXPORT_SYMBOL( MME_KillCommandAll );

EXPORT_SYMBOL( MME_ModifyTuneable );
EXPORT_SYMBOL( MME_GetTuneable );

EXPORT_SYMBOL( MME_IsStillAlive );
EXPORT_SYMBOL( MME_GetTransformerCapability );

/*
 * MME4 Extensions
 */
EXPORT_SYMBOL( MME_Version );
EXPORT_SYMBOL( MME_WaitCommand );
EXPORT_SYMBOL( MME_RegisterMemory );
EXPORT_SYMBOL( MME_DeregisterMemory );
EXPORT_SYMBOL( MME_DebugFlags );
EXPORT_SYMBOL( MME_ErrorStr );

/*
 * Internal APIs exported for mme_user module
 */
EXPORT_SYMBOL( mme_debug_flags );

static int mme_probe (struct platform_device *pdev)
{
	MME_ERROR res;

  	printk("MME module init=%d debug_flags=0x%x pool_size=%ld\n", init, debug_flags, pool_size);
  
	MME_DebugFlags(debug_flags);

	if (pool_size)
		MME_ModifyTuneable(MME_TUNEABLE_BUFFER_POOL_SIZE, pool_size);

	if (!init)
	  return 0;

	res = MME_Init();
	
	if (res != MME_SUCCESS)
	  return -ENODEV;

	return 0;
}

static int mme_remove (struct platform_device *pdev) 
{
	MME_ERROR status;
	
	status = MME_Term();

	/* XXXX Do we need to force shutdown in this case ? */
	if (status != MME_SUCCESS)
		printk("MME_Term() failed : %s (%d)\n",
		       MME_ErrorStr(status), status);
	
	return (status == MME_SUCCESS) ? 0 : -EBUSY;
}

static struct platform_driver mme_driver = {
	.driver.name  = "mme",
	.driver.owner = THIS_MODULE,
	.probe        = mme_probe,
	.remove       = mme_remove,
};

static void mme_release (struct device *dev) {}

struct platform_device mme_pdev = {
	.name           = "mme",
	.id             = -1,
	.dev.release    = mme_release,
};


int __init mme_mod_init( void )
{

	platform_driver_register(&mme_driver);

	platform_device_register(&mme_pdev);

	
	return 0;
}

void __exit mme_mod_exit( void )
{
	platform_device_unregister(&mme_pdev);
	
	platform_driver_unregister(&mme_driver);

	return;
}

module_init(mme_mod_init);
module_exit(mme_mod_exit);

#endif /* defined(__KERNEL__) && defined(MODULE) */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
