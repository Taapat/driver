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

#include <embxmailbox.h>	/* EMBX API definitions */

#include "_embx_sys.h"

#if defined(__KERNEL__) && defined(MODULE)

MODULE_DESCRIPTION("EMBX: Mailbox SHIM module");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("GPL");

/*
 * configurable parameters
 *
 * Ignored, supported for backward compatibility only
 */

static char     * mailbox0;
module_param     (mailbox0, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox0, "Configuration string of the form `addr:irq:flags...'");

static char     * mailbox1;
module_param     (mailbox1, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox1, "Configuration string of the form `addr:irq:flags...'");

static char     * mailbox2;
module_param     (mailbox2, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox2, "Configuration string of the form `addr:irq:flags...'");

static char     * mailbox3;
module_param     (mailbox3, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox3, "Configuration string of the form `addr:irq:flags...'");


EXPORT_SYMBOL( EMBX_Mailbox_Init );
EXPORT_SYMBOL( EMBX_Mailbox_Deinit );
EXPORT_SYMBOL( EMBX_Mailbox_Register );
EXPORT_SYMBOL( EMBX_Mailbox_Deregister );
EXPORT_SYMBOL( EMBX_Mailbox_Alloc );
EXPORT_SYMBOL( EMBX_Mailbox_Synchronize );
EXPORT_SYMBOL( EMBX_Mailbox_Free );
EXPORT_SYMBOL( EMBX_Mailbox_UpdateInterruptHandler );
EXPORT_SYMBOL( EMBX_Mailbox_InterruptEnable );
EXPORT_SYMBOL( EMBX_Mailbox_InterruptDisable );
EXPORT_SYMBOL( EMBX_Mailbox_InterruptClear );
EXPORT_SYMBOL( EMBX_Mailbox_InterruptRaise );
EXPORT_SYMBOL( EMBX_Mailbox_StatusGet );
EXPORT_SYMBOL( EMBX_Mailbox_StatusSet );
EXPORT_SYMBOL( EMBX_Mailbox_StatusMask );

#if 0
/* XXXX Not yet supported */
EXPORT_SYMBOL( EMBX_Mailbox_AllocLock );
EXPORT_SYMBOL( EMBX_Mailbox_FreeLock );
#endif

#endif /* (__KERNEL__) && (MODULE) */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
