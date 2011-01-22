/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : dvb_dvr.h - dvr device definitions
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
01-Nov-06   Created                                         Julian

************************************************************************/

#ifndef H_DVB_DVR
#define H_DVB_DVR

#include "dvb_module.h"

struct dvb_device*      DvrInit        (struct file_operations*         KernelDvrFops);

#endif
