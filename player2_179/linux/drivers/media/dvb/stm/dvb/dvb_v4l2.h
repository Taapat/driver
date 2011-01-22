/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : dvb_v4l2.h
Author :           Pete

Describes internal v4l interfaces

Date        Modification                                    Name
----        ------------                                    --------
09-Dec-07   Created                                         Pete

************************************************************************/

#ifndef __DVB_V4L2
#define __DVB_V4L2

void* stm_v4l2_findbuffer(unsigned long userptr, unsigned int size,int device);

#endif
