/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2003

Source file name : h264ppio.h
Author :           Nick

Contains the ioctl interface between the h264 pre-processor user level components
(h264ppinline.h) and the h264 pre-processor device level components.

Date        Modification                                    Name
----        ------------                                    --------
02-Dec-05   Created                                         Nick

************************************************************************/

#ifndef H_H264PPIO
#define H_H264PPIO

#define H264_PP_IOCTL_QUEUE_BUFFER              1
#define H264_PP_IOCTL_GET_PREPROCESSED_BUFFER   2

#define H264_PP_DEVICE                          "/dev/h264pp"

//

typedef struct h264pp_ioctl_queue_s
{
    unsigned int	  QueueIdentifier;
    void		 *InputBufferCachedAddress;
    void		 *InputBufferPhysicalAddress;
    void		 *OutputBufferCachedAddress;
    void		 *OutputBufferPhysicalAddress;
    unsigned int	  Field;
    unsigned int          InputSize;
    unsigned int          SliceCount;
    unsigned int          CodeLength;
    unsigned int          PicWidth;
    unsigned int          Cfg;
} h264pp_ioctl_queue_t;

//

typedef struct h264pp_ioctl_dequeue_s
{
    unsigned int	  QueueIdentifier;
    unsigned int	  OutputSize;
    unsigned int          ErrorMask;
} h264pp_ioctl_dequeue_t;

//

#endif
