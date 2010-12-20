/*
 * Player2 Platform registration
 *
 * Copyright (C) 2006 STMicroelectronics Limited
 * Author: Peter Bennett <peter.bennett@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */

#include <linux/version.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/stm/slim.h>
#include <linux/autoconf.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm-sh/processor.h>
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm-sh/irq-ilc.h>
#include <asm-sh/irq.h>
#else
#include <linux/irq.h>
#endif
#else /* STLinux 2.2 kernel */
#define ILC_IRQ(x) (x + MUXED_IRQ_BASE)
#endif 

static struct plat_slim tkdma_core = {
	.name        = "tkdma",
	.version     = 2,
	.imem_offset = 0x6000,
	.dmem_offset = 0x4000,
	.pers_offset = 0x5000,
	.regs_offset = 0x2000,
	.imem_size   = (1536 * 4),
	.dmem_size   = (512 * 4) 
};

#ifdef __TDT__
int tkdma = 0;
#else
int tkdma = 1;
#endif
module_param(tkdma, bool, S_IRUGO|S_IWUSR);

#if defined(CONFIG_CPU_SUBTYPE_STB7100) || defined (CONFIG_CPU_SUBTYPE_STX7100)
#include "mb442.h"
#include "hms1.h"
#include "platform_710x.h"
#elif defined(CONFIG_CPU_SUBTYPE_STX7200)
#include "mb520.h"
#include "platform_7200.h"
#elif defined(CONFIG_CPU_SUBTYPE_STX7141)
#include "platform_7141.h"
#elif defined(CONFIG_CPU_SUBTYPE_STX7111)
#include "platform_7111.h"
#elif defined(CONFIG_CPU_SUBTYPE_STX7105)
#include "platform_7105.h"
#endif

MODULE_DESCRIPTION("Player2 Platform Driver");
MODULE_AUTHOR("STMicroelectronics Limited");
MODULE_VERSION("0.9");
MODULE_LICENSE("GPL");
