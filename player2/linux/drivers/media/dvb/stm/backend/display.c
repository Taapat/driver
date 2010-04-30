/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : display.c
Author :           Julian

Access to all platform specific display information etc


Date        Modification                                    Name
----        ------------                                    --------
05-Apr-07   Created                                         Julian

************************************************************************/

#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <stmdisplay.h>
#include <linux/stm/stmcoredisplay.h>
#include "osdev_user.h"
#include "player_module.h"
#include "display.h"

#define MAX_PIPELINES                           4

static const stm_plane_id_t                     PlaneId[MAX_PIPELINES]  = {OUTPUT_VID1, OUTPUT_VID2, OUTPUT_VID1, OUTPUT_GDP2};
static struct stmcore_display_pipeline_data     PipelineData[STMCORE_MAX_PLANES];
static unsigned int                             Pipelines;

/*{{{  DisplayInit*/
int DisplayInit (void)
{
    int i;

    Pipelines   = 0;
    for (i = 0; i < MAX_PIPELINES; i++)
    {
        if (stmcore_get_display_pipeline (i, &PipelineData[i]) == 0)
            PLAYER_TRACE("Pipeline %d Device %p, Name = %s, Preferred video plane = %x\n", i, PipelineData[i].device, PipelineData[i].name, PipelineData[i].preferred_video_plane);
        else
            break;
    }
    Pipelines   = i;
    return 0;
}
/*}}}  */
#if 0
/*{{{  GetDisplayInfo*/
int GetDisplayInfo     (unsigned int            Id,
                        DeviceHandle_t*         Device,
                        unsigned int*           DisplayPlaneId,
                        unsigned int*           OutputId,
                        BufferLocation_t*       BufferLocation)
{
    int                 i, j;
    stm_plane_id_t      PreferredPlane;

    *BufferLocation     = BufferLocationVideoMemory;

    if (Id >= MAX_PIPELINES)
        return  0;

    *DisplayPlaneId     = PlaneId[Id];
#if defined (CONFIG_DUAL_DISPLAY)
    PreferredPlane      = PlaneId[Id];          /* Search for preferred video plane which matches chosen display plane */
#else
    PreferredPlane      = PlaneId[0];           /* Search for preferred video plane which matches main display plane */
#endif

    for (i = 0; i < Pipelines; i++)
    {
        if (PipelineData[i].preferred_video_plane == PreferredPlane)
        {
            for (j = 0; j < STMCORE_MAX_PLANES; j++)
            {
                if (PipelineData[i].planes[j].id == *DisplayPlaneId)
                {
                    PLAYER_DEBUG("Pipeline %d, Plane %x, Flags %x\n", i, *DisplayPlaneId, PipelineData[i].planes[j].flags);

                    if ((PipelineData[i].planes[j].flags & STMCORE_PLANE_MEM_ANY) == STMCORE_PLANE_MEM_ANY)
                        *BufferLocation = BufferLocationEither;
                    else if ((PipelineData[i].planes[j].flags & STMCORE_PLANE_MEM_SYS) == STMCORE_PLANE_MEM_SYS)
                        *BufferLocation = BufferLocationSystemMemory;
                    break;
                }
            }
            break;
        }
    }

    if (i == Pipelines)
    {
        PLAYER_ERROR("Unable to locate desired display plane %d\n", Id);
        return  -ENODEV;
    }

    *Device     = PipelineData[i].device;
    *OutputId   = PipelineData[i].main_output_id;
    PLAYER_TRACE ("Device %p, DisplayPlaneId %x, OutputId %d, Pipeline %d\n", *Device, *DisplayPlaneId, *OutputId, i);

    return  0;
}
/*}}}  */
#else
/*{{{  GetDisplayInfo*/
int GetDisplayInfo     (unsigned int            Id,
                        DeviceHandle_t*         Device,
                        unsigned int*           DisplayPlaneId,
                        unsigned int*           OutputId,
                        BufferLocation_t*       BufferLocation)
{
    int                                         i;
    struct stmcore_display_pipeline_data*       Pipeline;

    *DisplayPlaneId     = PlaneId[Id];
#if defined (CONFIG_DUAL_DISPLAY)
    Pipeline            = &PipelineData[Id];
#else
    if (Id == DISPLAY_ID_PIP)
        Pipeline        = &PipelineData[DISPLAY_ID_MAIN];
    else
        Pipeline        = &PipelineData[Id];
#endif

    for (i = 0; i < STMCORE_MAX_PLANES; i++)
    {
        if (Pipeline->planes[i].id == *DisplayPlaneId)
        {
            PLAYER_DEBUG("Pipeline %d, Plane %x, Flags %x\n", i, *DisplayPlaneId, Pipeline->planes[i].flags);

            if ((Pipeline->planes[i].flags & STMCORE_PLANE_MEM_ANY) == STMCORE_PLANE_MEM_ANY)
                *BufferLocation = BufferLocationEither;
            else if ((Pipeline->planes[i].flags & STMCORE_PLANE_MEM_SYS) == STMCORE_PLANE_MEM_SYS)
                *BufferLocation = BufferLocationSystemMemory;
            break;
        }
    }

    *Device     = Pipeline->device;
    *OutputId   = Pipeline->main_output_id;
    PLAYER_TRACE ("Device %p, DisplayPlaneId %x, OutputId %d, Pipeline %d\n", *Device, *DisplayPlaneId, *OutputId, i);

    return  0;
}
/*}}}  */
#endif
