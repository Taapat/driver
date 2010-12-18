/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : player_interface.h - player access points
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
31-Jan-07   Created                                         Julian

************************************************************************/

#ifndef H_PLAYER_INTERFACE
#define H_PLAYER_INTERFACE

#include <linux/kthread.h>

#include "player_interface_ops.h"
#include "sysfs_module.h"

/*      Debug printing macros   */

#ifndef ENABLE_INTERFACE_DEBUG
#define ENABLE_INTERFACE_DEBUG            1
#endif

#define INTERFACE_DEBUG(fmt, args...)  ((void) (ENABLE_INTERFACE_DEBUG && \
                                            (printk("Interface:%s: " fmt, __FUNCTION__, ##args), 0)))

/* Output trace information off the critical path */
#define INTERFACE_TRACE(fmt, args...)  (printk("Interface:%s: " fmt, __FUNCTION__, ##args))
/* Output errors, should never be output in 'normal' operation */
#define INTERFACE_ERROR(fmt, args...)  (printk("ERROR:Interface:%s: " fmt, __FUNCTION__, ##args))


/* Entry point list */

int PlayerInterfaceInit                (void);
int PlayerInterfaceDelete              (void);

int ComponentGetAttribute              (player_component_handle_t       Component,
                                        const char*                     Attribute,
                                        union attribute_descriptor_u*   Value);
int ComponentSetAttribute              (player_component_handle_t       Component,
                                        const char*                     Attribute,
                                        union attribute_descriptor_u*   Value);

player_event_signal_callback PlayerRegisterEventSignalCallback         (player_event_signal_callback  Callback);

#endif
