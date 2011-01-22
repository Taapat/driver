/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : dvb_audio.h - audio device definitions
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
24-Mar-03   Created                                         Julian

************************************************************************/

#ifndef H_DVB_AUDIO
#define H_DVB_AUDIO

#include "dvbdev.h"

struct dvb_device*  AudioInit                          (struct DeviceContext_s*        Context);
int                 AudioIoctlPlay                     (struct DeviceContext_s*        Context);
int                 AudioIoctlStop                     (struct DeviceContext_s*        Context);
int                 AudioIoctlSetId                    (struct DeviceContext_s*        Context,
                                                        int                            Id);
int                 AudioIoctlSetPlayInterval          (struct DeviceContext_s*        Context,
                                                        audio_play_interval_t*         PlayInterval);

#endif
