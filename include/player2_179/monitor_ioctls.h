/************************************************************************
COPYRIGHT (C) STMicroelectronics 2008

Source file name : monitor_ioctls.h - external interface definitions
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
24-Jul-08   Created                                         Julian

************************************************************************/

#ifndef H_MONITOR_IOCTLS
#define H_MONITOR_IOCTLS

#include "monitor_types.h"

#define MONITOR_TIME_UNKNOWN                    0xfedcba9876543210ull

#define MONITOR_EVENT_RECORD_SIZE               128

typedef unsigned int                            monitor_subsystem_mask_t;

typedef enum monitor_status_code_e
{
    MONITOR_STATUS_IDLE,
    MONITOR_STATUS_RUNNING
} monitor_status_code_t;

struct monitor_status_s
{
    monitor_status_code_t       status_code;
    monitor_subsystem_mask_t    subsystem_mask;
    unsigned int                event_count;
    unsigned int                events_lost;
};

#define MONITOR_EVENT_INDEX_MASK                0x000000ff

typedef char                    monitor_event_t[MONITOR_EVENT_RECORD_SIZE];

struct monitor_event_request_s
{
    monitor_event_code_t        event_code;
    unsigned int                parameters[MONITOR_PARAMETER_COUNT];
    unsigned int                count;
    unsigned int                reset;
};


#define MONITOR_ENABLE          _IOW('o', 10, monitor_subsystem_mask_t)
#define MONITOR_DISABLE         _IOW('o', 11, monitor_subsystem_mask_t)
#define MONITOR_GET_STATUS      _IOR('o', 12, struct monitor_status_s)
#define MONITOR_REQUEST_EVENT   _IOW('o', 12, struct monitor_event_request_s)

#endif
