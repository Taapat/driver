/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : sysfs_module.c
Author :           Julian

Implementation of the sysfs access to the ST streaming engine

Date        Modification                                    Name
----        ------------                                    --------
28-Apr-08   Created                                         Julian

************************************************************************/

#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/ioport.h>
#include <linux/bpa2.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/autoconf.h>
#include <asm/uaccess.h>

#include "sysfs_module.h"
#include "player_interface.h"

static int  __init      SysfsLoadModule (void);
static void __exit      SysfsUnloadModule (void);

module_init             (SysfsLoadModule);
module_exit             (SysfsUnloadModule);

MODULE_DESCRIPTION      ("Sysfs driver for accessing STM streaming architecture.");
MODULE_AUTHOR           ("Julian Wilson");
MODULE_LICENSE          ("GPL");

#define MODULE_NAME     "Player sysfs"


struct SysfsContext_s*   SysfsContext;

static int __init SysfsLoadModule (void)
{
    SysfsContext        = kmalloc (sizeof (struct SysfsContext_s),  GFP_KERNEL);
    if (SysfsContext == NULL)
    {
        SYSFS_ERROR("Unable to allocate device memory\n");
        return -ENOMEM;
    }

    PlayerInterfaceInit ();

    SYSFS_DEBUG("sysfs interface to stream device loaded\n");

    return 0;
}

static void __exit SysfsUnloadModule (void)
{
    PlayerInterfaceDelete ();

    if (SysfsContext != NULL)
        kfree (SysfsContext);

    SysfsContext        = NULL;

    SYSFS_DEBUG("STM sysfs interface to stream device unloaded\n");

    return;
}

