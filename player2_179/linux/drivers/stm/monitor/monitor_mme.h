/************************************************************************
COPYRIGHT (C) STMicroelectronics 2008

Source file name : monitor_mme.h - monitor mme definitions
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
08_Aug-08   Created                                         Julian

************************************************************************/

#ifndef H_MONITOR_MME
#define H_MONITOR_MME

#include <linux/sched.h>
#ifdef __TDT__
#include <linux/version.h>
#endif
#if defined(__TDT__) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30))
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include "mme.h"
#include "EVENT_Log_TransformerTypes.h"

#define MONITOR_MAX_MME_DEVICES         4

struct MMEContext_s
{
    unsigned int                        Id;
    char                                TransformerName[MME_MAX_TRANSFORMER_NAME];

    unsigned int                        ClockAddress;
    unsigned long long                  ClockMaxValue;
    unsigned long long                  TicksPerSecond;

    struct task_struct*                 MonitorMMEThread;
    struct semaphore                    EventReceived;
    unsigned int                        Monitoring;
    struct semaphore                    ThreadTerminated;

    MME_TransformerHandle_t             MMEHandle;
    EVENT_LOG_CommandStatus_t           MMECommandStatus;

    struct DeviceContext_s*             DeviceContext;

    unsigned int                        TransformerInitialized;
};


int MonitorMMEInit                     (struct DeviceContext_s*         DeviceContext,
                                        struct MMEContext_s*            Context,
                                        unsigned int                    Id);
int MonitorMMETerminate                (struct MMEContext_s*            Context);


#endif
