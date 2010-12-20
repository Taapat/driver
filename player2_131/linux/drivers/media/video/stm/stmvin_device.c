/***********************************************************************
 *
 * File: stgfb/Linux/media/video/stmvin_device.c
 * Copyright (c) 2007 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 * 
 * V4L2 video input device driver for ST SoC display subsystems.
 *
 * 
 ***********************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/videodev.h>

#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/page.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include <stmdisplay.h>

#include <Linux/video/stmfb.h>
#include <linux/stm/stmcoredisplay.h>

#include "stmvout_driver.h"
#include "stmvout.h"
#include "stmvin_device.h"
#include "stmvin.h"

int enumerateInputFormat(stvout_device_t *pDev, struct v4l2_fmtdesc *f)
{
	return 0;
}
