/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : player_module.h - backend streamer device definitions
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
24-Mar-03   Created                                         Julian

************************************************************************/

#ifndef H_PLAYER_MODULE
#define H_PLAYER_MODULE

#include "report.h"

#define MODULE_NAME             "ST Havana Player1"

#ifndef false
#define false   0
#define true    1
#endif

#define TRANSPORT_PACKET_SIZE           188
#define DEMUX_BUFFER_SIZE               (200*TRANSPORT_PACKET_SIZE)

/*      Debug printing macros   */
#ifndef ENABLE_PLAYER_DEBUG
#define ENABLE_PLAYER_DEBUG             0
#endif

#define PLAYER_DEBUG(fmt, args...)      ((void) (ENABLE_PLAYER_DEBUG && \
                                            (report(severity_note, "Player:%s: " fmt, __FUNCTION__, ##args), 0)))

/* Output trace information off the critical path */
#define PLAYER_TRACE(fmt, args...)      (report(severity_note, "Player:%s: " fmt, __FUNCTION__, ##args))
/* Output errors, should never be output in 'normal' operation */
#define PLAYER_ERROR(fmt, args...)      (report(severity_error, "Player:%s: " fmt, __FUNCTION__, ##args))

#endif
