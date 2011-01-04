/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2002-2005

Source file name : demux_asf.h
Author :           Daniel

Definition of the asf demux implementation of the demux pure virtual class
module for havana.


Date        Modification                                    Name
----        ------------                                    --------
17-Nov-05   Cloned from demux_avi.h                         Daniel

************************************************************************/

#ifndef H_ASF
#define H_ASF

#include "asf_guids.h"

// generic
#define ASF_OBJECT_ID_OFFSET 0
#define ASF_OBJECT_SIZE_OFFSET 16

// file properties object
#define ASF_FILE_PROPERTIES_FLAGS_OFFSET 88
#define ASF_FILE_PROPERTIES_MINIMUM_DATA_PACKET_SIZE_OFFSET 92
#define ASF_FILE_PROPERTIES_MAXIMUM_DATA_PACKET_SIZE_OFFSET 96

// stream properties object
#define ASF_STREAM_PROPERTIES_STREAM_TYPE_OFFSET 24
#define ASF_STREAM_PROPERTIES_FLAGS_OFFSET 72

// clock object
#define ASF_CLOCK_CLOCK_TYPE_OFFSET 24
#define ASF_CLOCK_CLOCK_SIZE_OFFSET 40

// data object
#define ASF_DATA_TOTAL_DATA_PACKETS_OFFSET 40

#endif // H_ASF
