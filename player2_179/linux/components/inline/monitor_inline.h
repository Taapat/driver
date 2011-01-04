/************************************************************************
COPYRIGHT (C) STMicroelectronics 2008

Source file name : monitorinline.h (kernel version)
Author :           Julian

Provides access to the monitor functions and types

Date        Modification                                    Name
----        ------------                                    --------
30-Jul-08   Created                                         Julian

************************************************************************/

#ifndef H_MONITOR_INLINE
#define H_MONITOR_INLINE


#include "monitor_types.h"

#if defined (CONFIG_MONITOR)

#ifdef __cplusplus
extern "C" {
#endif
void                    MonitorSignalEvent     (monitor_event_code_t            EventCode,
                                                unsigned int                    Parameters[MONITOR_PARAMETER_COUNT],
                                                const char*                     Description);
#ifdef __cplusplus
}
#endif

#else

static inline void      MonitorSignalEvent     (monitor_event_code_t            EventCode,
                                                unsigned int                    Parameters[MONITOR_PARAMETER_COUNT],
                                                const char*                     Description) {}

#endif

#endif
