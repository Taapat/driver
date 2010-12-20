/***********************************************************************
 * File: stgfb/Linux/video/stmvout_driver.h
 * Copyright (c) 2005 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 * 
 * Buffer queue management has been derived from the 2.2/2.4 version of V4L2,
 * see comments below.
 * 
 ***********************************************************************/
 
/*
 *	Video for Linux Two
 *
 *	Header file for v4l or V4L2 drivers and applications, for
 *	Linux kernels 2.2.x or 2.4.x.
 *
 *	Author: Bill Dirks <bdirks@pacbell.net>
 *		Justin Schoeman
 *		et al.
 * 
 * 
 */

#ifndef __STMVOUT_DRIVER_H
#define __STMVOUT_DRIVER_H

#include <linux/version.h>

//#define DEBUG

#if defined DEBUG
#define PKMOD "stmvout: "
#define debug_msg(fmt,arg...) printk(PKMOD fmt,##arg)
#define err_msg(fmt,arg...) printk(PKMOD fmt,##arg)
#define info_msg(fmt,arg...) printk(PKMOD fmt,##arg)
#else
#define PKMOD "stmvout: "
#define debug_msg(fmt,arg...)
#define err_msg(fmt,arg...) printk(PKMOD fmt,##arg)
#define info_msg(fmt,arg...)
#endif

#define MAX_OPENS	     3
#define MAX_STREAM_BUFFERS  15

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <media/v4l2-dev.h>
#endif

struct _stvout_device;


struct v4l2_q_node
{
	struct v4l2_q_node 	*forw, *back;
};


struct v4l2_queue
{
	struct v4l2_q_node	*forw, *back;
	rwlock_t		qlock;
};


typedef struct _streaming_buffer
{
  struct v4l2_q_node     qnode;
  struct v4l2_buffer     vidBuf;
  unsigned long          physicalAddr;
  char                  *cpuAddr;
  int                    isAllocated;
  int			 mapCount;
  struct _stvout_device *pDev;
  stm_blitter_surface_t  drawingSurface;
  stm_display_buffer_t   bufferHeader;
  unsigned long          clutTable;
  unsigned long          clutTable_phys;
} streaming_buffer_t;


//typedef struct _open_data
//{
//  int                    isOpen;
//  int                    isOpenForIO;
//  struct _stvout_device *pDev;
//} open_data_t;

typedef enum { STM_V4L2_DYNAMIC_ALLOCATION, STM_V4L2_FIXED_POOL } stvout_memory_type;

typedef struct _stvout_device
{
  //  struct video_device     v;		/* Must be first */
  struct v4l2_format      bufferFormat;
  struct v4l2_crop        srcCrop;
  struct v4l2_crop        outputCrop;
  
  struct v4l2_queue       streamQpending;
  struct v4l2_queue       streamQcomplete;

  struct stmcore_display_pipeline_data displayPipe;
  
//  int                     numOutputs;
  int                     currentOutputNum;
  int                     isStreaming;
  //  int                     isRegistered;
  int                     inUse;

  struct stmcore_plane_data *planeConfig;
  stm_display_plane_t*    pPlane;
  stm_display_output_t*   pOutput;
  
  int                     bufferQueuesInit;

  wait_queue_head_t       wqBufDQueue;
  wait_queue_head_t       wqStreamingState;
  struct semaphore        devLock;
  
	//open_data_t             openData[MAX_OPENS];
  streaming_buffer_t      streamBuffers[MAX_STREAM_BUFFERS];
  streaming_buffer_t      userBuffers[MAX_STREAM_BUFFERS];
  
  stvout_memory_type      deviceMemoryType;
  unsigned long           deviceMemoryBase;
  unsigned long           deviceMemorySize;
  
  SURF_FMT                planeFormat;
  unsigned int            globalAlpha;
  unsigned long           clutBuffer;
  unsigned long           clutBuffer_phys;
} stvout_device_t;


void  initBuffers        (stvout_device_t *pDev);
void  deinitBuffers      (stvout_device_t *pDev);
int   queueBuffer        (stvout_device_t *pDev, struct v4l2_buffer* pVidBuf);
int   dequeueBuffer      (stvout_device_t *pDev, struct v4l2_buffer* pVidBuf);
int   hasQueuedBuffers   (stvout_device_t *pDev);
int   hasCompletedBuffers(stvout_device_t *pDev);
void  writeNextFrame     (stvout_device_t *pDev);
int   streamoff          (stvout_device_t *pDev);

int   setVideoOutput (stvout_device_t *pDev, int index);

int   setBufferFormat(stvout_device_t *pDev, struct v4l2_format *fmt, int updateConfig);

int   setOutputCrop  (stvout_device_t *pDev, struct v4l2_crop *pCrop);
int   setBufferCrop  (stvout_device_t *pDev, struct v4l2_crop *pCrop);


int   mmapAllocateBuffers(stvout_device_t *pDev, struct v4l2_requestbuffers* req);
int   mmapDeleteBuffers  (stvout_device_t *pDev);

streaming_buffer_t *mmapGetBufferFromOffset(stvout_device_t *pDev, unsigned long offset);

int   getDisplayStandard   (stvout_device_t *pDev, struct v4l2_standard *std);
int   getDisplayDefaults   (stvout_device_t *pDev, struct v4l2_cropcap *cap);

int   enumerateBufferFormat(stvout_device_t *pDev, struct v4l2_fmtdesc *f);

unsigned long getCtrlName(int id);

int   doBlitterCommand(stvout_device_t *pDev, STMFBIO_BLT_DATA *bltData);

#endif /* __STMVOUT_DRIVER_H */
