/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : sysfs_module.h - streamer device access interface definitions
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
24-Mar-03   Created                                         Julian

************************************************************************/

#ifndef H_SYSFS_MODULE
#define H_SYSFS_MODULE

#include "player_interface.h"

#ifndef false
#define false   0
#define true    1
#endif

/*      Debug printing macros   */
#ifndef ENABLE_SYSFS_DEBUG
#define ENABLE_SYSFS_DEBUG              0
#endif

#define SYSFS_DEBUG(fmt, args...)       ((void) (ENABLE_SYSFS_DEBUG && \
                                            (printk("%s: " fmt, __FUNCTION__, ##args), 0)))

/* Output trace information off the critical path */
#define SYSFS_TRACE(fmt, args...)       (printk("%s: " fmt, __FUNCTION__, ##args))
/* Output errors, should never be output in 'normal' operation */
#define SYSFS_ERROR(fmt, args...)       (printk("ERROR:%s: " fmt, __FUNCTION__, ##args))


struct SysfsContext_s
{
    unsigned int        Something;
};

#endif
