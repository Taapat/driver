/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : display.h - access to platform specific display info
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
05-Apr-07   Created                                         Julian

************************************************************************/

#ifndef H_DISPLAY
#define H_DISPLAY

#include "osdev_user.h"

#define DISPLAY_ID_MAIN                 0
#define DISPLAY_ID_PIP                  1
#define DISPLAY_ID_REMOTE               2

typedef enum
{
    BufferLocationSystemMemory  = 0x01,
    BufferLocationVideoMemory   = 0x02,
    BufferLocationEither        = 0x03,
} BufferLocation_t;

#ifdef __cplusplus
extern "C" {
#endif

int             DisplayInit             (void);
int             GetDisplayInfo          (unsigned int           Id,
                                         DeviceHandle_t*        DisplayDevice,
                                         unsigned int*          PlaneId,
                                         unsigned int*          OutputId,
                                         BufferLocation_t*      BufferLocation);

#ifdef __cplusplus
}
#endif

#endif
