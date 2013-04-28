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

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

#include <embx.h>	/* External defines and prototypes */

#include "_embx_sys.h"

/*
 * Tuneable parameters and their default values
 */
static int   debug_flags = 0;
static int   debug_chan  = ICS_DBG_STDOUT|ICS_DBG_LOG;

#if defined(__KERNEL__) && defined(MODULE)

MODULE_DESCRIPTION("EMBX: SHIM module");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("GPL");

module_param(debug_flags, int, 0);
MODULE_PARM_DESC(debug_flags, "Bitmask of ICS debugging flags");

module_param(debug_chan, int, 0);
MODULE_PARM_DESC(debug_chan, "Bitmask of ICS debugging channels");

/*
 * Export all the EMBX module symbols
 */

/* Public API exports */
EXPORT_SYMBOL( EMBX_Init );
EXPORT_SYMBOL( EMBX_Deinit );
EXPORT_SYMBOL( EMBX_ModifyTuneable );

EXPORT_SYMBOL( EMBX_FindTransport );
EXPORT_SYMBOL( EMBX_GetFirstTransport );
EXPORT_SYMBOL( EMBX_GetNextTransport );
EXPORT_SYMBOL( EMBX_OpenTransport );
EXPORT_SYMBOL( EMBX_CloseTransport );
EXPORT_SYMBOL( EMBX_GetTransportInfo );

EXPORT_SYMBOL( EMBX_Alloc );
EXPORT_SYMBOL( EMBX_Free );
EXPORT_SYMBOL( EMBX_Release );
EXPORT_SYMBOL( EMBX_GetBufferSize );

EXPORT_SYMBOL( EMBX_CreatePort );
EXPORT_SYMBOL( EMBX_Connect );
EXPORT_SYMBOL( EMBX_ConnectBlock );
EXPORT_SYMBOL( EMBX_ConnectBlockTimeout );
EXPORT_SYMBOL( EMBX_ClosePort );
EXPORT_SYMBOL( EMBX_InvalidatePort );

EXPORT_SYMBOL( EMBX_Receive );
EXPORT_SYMBOL( EMBX_ReceiveBlock );
EXPORT_SYMBOL( EMBX_ReceiveBlockTimeout );
EXPORT_SYMBOL( EMBX_SendMessage );

EXPORT_SYMBOL( EMBX_Offset );
EXPORT_SYMBOL( EMBX_Address );

/* Transport module helper exports */
EXPORT_SYMBOL( EMBX_RegisterTransport );
EXPORT_SYMBOL( EMBX_UnregisterTransport );

/* EMBXSHM emulation */
EXPORT_SYMBOL( EMBXSHM_mailbox_factory );
EXPORT_SYMBOL( EMBXSHMC_mailbox_factory );

#if 0
/*
 * DELETED functions
 */
EXPORT_SYMBOL( EMBX_RegisterObject );
EXPORT_SYMBOL( EMBX_DeregisterObject );
EXPORT_SYMBOL( EMBX_GetObject );
EXPORT_SYMBOL( EMBX_SendObject );
EXPORT_SYMBOL( EMBX_UpdateObject );
#endif

static int embx_probe (struct platform_device *pdev)
{
	printk("EMBX init debug_flags 0x%x debug_chan 0x%x\n", debug_flags, debug_chan);
  
	ics_debug_flags(debug_flags);

	ics_debug_chan(debug_chan);

	return 0;
}

static int embx_remove (struct platform_device *pdev) 
{
	return 0;
}

static struct platform_driver embx_driver = {
	.driver.name  = "embx",
	.driver.owner = THIS_MODULE,
	.probe        = embx_probe,
	.remove       = embx_remove,
};

static void embx_release (struct device *dev) {}

struct platform_device embx_pdev = {
	.name           = "embx",
	.id             = -1,
	.dev.release    = embx_release,
};


int __init embx_mod_init( void )
{
	platform_driver_register(&embx_driver);

	platform_device_register(&embx_pdev);
	
	return 0;
}

void __exit embx_mod_exit( void )
{
	platform_device_unregister(&embx_pdev);
	
	platform_driver_unregister(&embx_driver);

	return;
}

module_init(embx_mod_init);
module_exit(embx_mod_exit);

#endif /* defined(__KERNEL__) && defined(MODULE) */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
