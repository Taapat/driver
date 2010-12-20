/***********************************************************************
 *
 * File: stgfb/Linux/media/video/stmvout_device.c
 * Copyright (c) 2000, 2004, 2005 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * V4L2 video output device driver for ST SoC display subsystems.
 *
 * Buffer queue management has been derived from the 2.2/2.4 out-of-kernel
 * version of V4L2 (videodev.c), see comments below.
 *
 ***********************************************************************/

/*
 *      Video for Linux Two
 *
 *      A generic video device interface for the LINUX operating system
 *      using a set of device structures/vectors for low level operations.
 *
 *      This file replaces the videodev.c file that comes with the
 *      regular kernel distribution.
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 * Author:      Bill Dirks <bdirks@pacbell.net>
 *              based on code by Alan Cox, <alan@cymru.net>
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/videodev.h>
#include <linux/ioport.h>

#if defined(CONFIG_BIGPHYS_AREA)
#include <linux/bigphysarea.h>
#elif defined(CONFIG_BPA2)
#include <linux/bpa2.h>
#else
#error Kernel must have one of Bigphysarea or BPA2 memory allocators configured
#endif


#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/page.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include <stmdisplay.h>

#include <Linux/video/stmfb.h>
#include <linux/stm/stmcoredisplay.h>

#include "stmvout_driver.h"
#include "stmvout.h"

static void bufferFinishedCallback(void *pDevId, const stm_buffer_presentation_stats_t *stats);
static void timestampCallback     (void *pDevId, TIME64 vsyncTime);

/*
 * Imports form the framebuffer module to get access to the blitter.
 */
extern int  stmfbio_draw_rectangle(stm_display_blitter_t* blitter,
				   stm_blitter_surface_t* fbsurf,
				   STMFBIO_BLT_DATA* pData);

extern int  stmfbio_do_blit       (stm_display_blitter_t* blitter,
				   stm_blitter_surface_t* srcsurf,
				   stm_blitter_surface_t* dstsurf,
				   unsigned long          clutaddress,
				   STMFBIO_BLT_DATA*      pData);

/* Map from SURF_FORMAT_... to V4L2 format descriptions */
typedef struct {
  struct v4l2_fmtdesc fmt;
  int    depth;
} format_info;

static format_info g_capfmt[] =
{
  /*
   * This isn't strictly correct as the V4L RGB565 format has red
   * starting at the least significant bit. Unfortunately there is no V4L2 16bit
   * format with blue starting at the LSB. It is recognised in the V4L2 API
   * documentation that this is strange and that drivers may well lie for
   * pragmatic reasons.
   */
  [SURF_RGB565]     = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "RGB-16 (5-6-5)", V4L2_PIX_FMT_RGB565 },    16},

  [SURF_ARGB1555]     = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "RGBA-16 (5-5-5-1)", V4L2_PIX_FMT_BGRA5551 }, 16},

  [SURF_ARGB4444]     = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "RGBA-16 (4-4-4-4)", V4L2_PIX_FMT_BGRA4444 }, 16},

  [SURF_RGB888]     = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "RGB-24 (B-G-R)", V4L2_PIX_FMT_BGR24 },     24},

  [SURF_ARGB8888]   = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "ARGB-32 (8-8-8-8)", V4L2_PIX_FMT_BGR32 },  32}, /* Note that V4L2 doesn't define
									   * the alpha channel
									   */

  [SURF_BGRA8888]   = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "BGRA-32 (8-8-8-8)", V4L2_PIX_FMT_RGB32 },  32}, /* Bigendian BGR as BTTV driver,
									   * not as V4L2 spec
									   */

  [SURF_YCBCR422R]  = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "YUV 4:2:2 (U-Y-V-Y)", V4L2_PIX_FMT_UYVY }, 16},

  [SURF_YUYV]       = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "YUV 4:2:2 (Y-U-Y-V)", V4L2_PIX_FMT_YUYV }, 16},

  [SURF_YCBCR422MB] = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "YUV 4:2:2MB", V4L2_PIX_FMT_STM422MB },     8}, /* Bits per pixel for Luma only */

  [SURF_YCBCR420MB] = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "YUV 4:2:0MB", V4L2_PIX_FMT_STM420MB },     8}, /* Bits per pixel for Luma only */

  [SURF_YUV420]     = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "YUV 4:2:0 (YUV)", V4L2_PIX_FMT_YUV420 },   8}, /* Bits per pixel for Luma only */

  [SURF_YVU420]     = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "YUV 4:2:0 (YVU)", V4L2_PIX_FMT_YVU420 },   8}, /* Bits per pixel for Luma only */

  [SURF_YUV422P]    = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "YUV 4:2:2 (YUV)", V4L2_PIX_FMT_YUV422P },  8}, /* Bits per pixel for Luma only */

  [SURF_CLUT2]      = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "CLUT2 (RGB)", V4L2_PIX_FMT_CLUT2 },  2},

  [SURF_CLUT4]      = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "CLUT4 (RGB)", V4L2_PIX_FMT_CLUT4 },  4},

  [SURF_CLUT8]      = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "CLUT8 (RGB)", V4L2_PIX_FMT_CLUT8 },  8},

  [SURF_ACLUT88]     = {{ 0, V4L2_BUF_TYPE_VIDEO_OUTPUT, 0,
			 "CLUT8 with 8 for Alpha (RGB)", V4L2_PIX_FMT_CLUTA8 },  16},

  /* Make sure the array covers all the SURF_FMT enumerations  */
  [SURF_END]         = {{ 0, 0, 0, "", 0 }, 0 }
};



/*
 * Simple queue management, taken from the original V4L2, this is no longer
 * in the latest kernel version, but the new V4L2 buffer management is too
 * tied to capture devices and SG DMA to be used at this time.
 */
static rwlock_t rw_lock_unlocked = RW_LOCK_UNLOCKED;
void
v4l2_q_init(struct v4l2_queue *q)
{
	if (q == NULL)
		return;
	q->qlock = rw_lock_unlocked;
	q->forw = (struct v4l2_q_node *)q;
	q->back = (struct v4l2_q_node *)q;
}
void
v4l2_q_add_head(struct v4l2_queue *q, struct v4l2_q_node *node)
{
	unsigned long flags;
	if (q == NULL || node == NULL)
		return;
	if (q->forw == NULL || q->back == NULL)
		v4l2_q_init(q);
	write_lock_irqsave(&(q->qlock), flags);
	node->forw = q->forw;
	node->back = (struct v4l2_q_node *)q;
	q->forw->back = node;
	q->forw = node;
	write_unlock_irqrestore(&(q->qlock), flags);
}
void
v4l2_q_add_tail(struct v4l2_queue *q, struct v4l2_q_node *node)
{
	unsigned long flags;
	if (q == NULL || node == NULL)
		return;
	if (q->forw == NULL || q->back == NULL)
		v4l2_q_init(q);
	write_lock_irqsave(&(q->qlock), flags);
	node->forw = (struct v4l2_q_node *)q;
	node->back = q->back;
	q->back->forw = node;
	q->back = node;
	write_unlock_irqrestore(&(q->qlock), flags);
}
void *
v4l2_q_del_head(struct v4l2_queue *q)
{
	unsigned long flags;
	struct v4l2_q_node *node;
	if (q == NULL)
		return NULL;
	write_lock_irqsave(&(q->qlock), flags);
	if (q->forw == NULL || q->back == NULL ||
	    q->forw == (struct v4l2_q_node *)q ||
	    q->back == (struct v4l2_q_node *)q)
	{
		write_unlock_irqrestore(&(q->qlock), flags);
		return NULL;
	}
	node = q->forw;
	node->forw->back = (struct v4l2_q_node *)q;
	q->forw = node->forw;
	node->forw = NULL;
	node->back = NULL;
	write_unlock_irqrestore(&(q->qlock), flags);
	return node;
}
void *
v4l2_q_del_tail(struct v4l2_queue *q)
{
	unsigned long flags;
	struct v4l2_q_node *node;
	if (q == NULL)
		return NULL;
	write_lock_irqsave(&(q->qlock), flags);
	if (q->forw == NULL || q->back == NULL ||
	    q->forw == (struct v4l2_q_node *)q ||
	    q->back == (struct v4l2_q_node *)q)
	{
		write_unlock_irqrestore(&(q->qlock), flags);
		return NULL;
	}
	node = q->back;
	node->back->forw = (struct v4l2_q_node *)q;
	q->back = node->back;
	node->forw = NULL;
	node->back = NULL;
	write_unlock_irqrestore(&(q->qlock), flags);
	return node;
}
void *
v4l2_q_peek_head(struct v4l2_queue *q)
{
	unsigned long flags;
	struct v4l2_q_node *node;
	read_lock_irqsave(&(q->qlock), flags);
	if (q == NULL || q->forw == NULL || q->forw == (struct v4l2_q_node *)q)
	{
		read_unlock_irqrestore(&(q->qlock), flags);
		return NULL;
	}
	node = q->forw;
	read_unlock_irqrestore(&(q->qlock), flags);
	return node;
}
void *
v4l2_q_peek_tail(struct v4l2_queue *q)
{
	unsigned long flags;
	struct v4l2_q_node *node;
	read_lock_irqsave(&(q->qlock), flags);
	if (q == NULL || q->back == NULL || q->back == (struct v4l2_q_node *)q)
	{
		read_unlock_irqrestore(&(q->qlock), flags);
		return NULL;
	}
	node = q->back;
	read_unlock_irqrestore(&(q->qlock), flags);
	return node;
}
void *
v4l2_q_yank_node(struct v4l2_queue *q, struct v4l2_q_node *node)
{
	unsigned long flags;
	struct v4l2_q_node *t;
	if (v4l2_q_peek_head(q) == NULL || node == NULL)
		return NULL;
	write_lock_irqsave(&(q->qlock), flags);
	for (t = q->forw; t != (struct v4l2_q_node *)q; t = t->forw)
		if (t == node)
		{
			node->back->forw = node->forw;
			node->forw->back = node->back;
			node->forw = NULL;
			node->back = NULL;
			write_unlock_irqrestore(&(q->qlock), flags);
			return node;
		}
	write_unlock_irqrestore(&(q->qlock), flags);
	return NULL;
}
int
v4l2_q_last(struct v4l2_queue *q)
{
/*  This function by Olivier Carmona  */

	unsigned long flags;
	read_lock_irqsave(&(q->qlock), flags);
	if (q == NULL)
	{
		read_unlock_irqrestore(&(q->qlock), flags);
		return -1;
	}
	if (q->forw == NULL || q->back == NULL ||
	    q->forw == (struct v4l2_q_node *)q ||
	    q->back == (struct v4l2_q_node *)q)
	{
		read_unlock_irqrestore(&(q->qlock), flags);
		return -1;
	}
	if (q->forw == q->back)
	{
		read_unlock_irqrestore(&(q->qlock), flags);
		return 1;
	}
	read_unlock_irqrestore(&(q->qlock), flags);
	return 0;
}

/************************************************
 *  SetVideoOutput
 ************************************************/
int setVideoOutput(stvout_device_t *pDev, int index)
{
  void *newoutput;

  debug_msg("output %d\n",index);

  if(pDev->currentOutputNum == index)
  {
    /*
     * Already connected to the required output, do nothing
     */
    return 0;
  }

  newoutput = stm_display_get_output(pDev->displayPipe.device, index);
  if(!newoutput)
  {
    if(signal_pending(current))
      return -ERESTARTSYS;
    else
      return -EINVAL;
  }

  debug_msg("succeeded\n");

  pDev->currentOutputNum = index;
  pDev->pOutput = newoutput;

  return 0;
}


/************************************************
 *  writeNextFrame
 ************************************************/
void writeNextFrame(stvout_device_t *pDev)
{
  streaming_buffer_t *pStreamBuffer = NULL;

  while((pStreamBuffer = v4l2_q_peek_head(&(pDev->streamQpending))) != NULL)
  {
      if(stm_display_plane_queue_buffer(pDev->pPlane, &(pStreamBuffer->bufferHeader)) < 0)
      {
	/* There is a signal pending or the hardware queue is full.
	 * In either case do not take the streaming buffer off the
	 * pending queue, because it hasn't been written yet.
	 */
	debug_msg("writeNextFrame: buffer queueing failed\n");
	return;
      }

      v4l2_q_del_head(&(pDev->streamQpending));
  }

}


/************************************************
 * Callback routines for buffer displayed/buffer
 * complete events from the device driver.
 ************************************************/
static void timestampCallback(void * pUser, TIME64 vsyncTime)
{
  streaming_buffer_t *pStreamBuffer = (streaming_buffer_t *)pUser;
  UTIME64 oneSecond = 1000000LL;
  UTIME64 tmp       = 0;
  TIME64  seconds   = 0;

  /*
   * For the moment we basically just have to know that vsyncTime is
   * specified in microseconds on Linux.
   *
   * Unfortunately we dont have a signed 64 divide in the Linux kernel yet
   * so we have to do a slight of hand for negative times.
   */
  if(vsyncTime < 0)
    tmp = (UTIME64)(-vsyncTime);
  else
    tmp = (UTIME64)vsyncTime;

  tmp = tmp/oneSecond;

  if(vsyncTime < 0)
    seconds = -((TIME64)tmp) - 1;
  else
    seconds = (TIME64)tmp;

  pStreamBuffer->vidBuf.timestamp.tv_sec  = seconds;
  pStreamBuffer->vidBuf.timestamp.tv_usec = vsyncTime - (seconds*(TIME64)oneSecond);
}


static void bufferFinishedCallback(void* pUser, const stm_buffer_presentation_stats_t *stats)
{
  streaming_buffer_t *pStreamBuffer = (streaming_buffer_t *)pUser;
  stvout_device_t    *pDev          = pStreamBuffer->pDev;

  if(pDev->isStreaming)
  {
    pStreamBuffer->vidBuf.flags |= V4L2_BUF_FLAG_DONE;

    v4l2_q_add_tail(&(pDev->streamQcomplete), &(pStreamBuffer->qnode));
    wake_up_interruptible(&(pDev->wqBufDQueue));
  }
}

static void allocateBuffer(unsigned long size,streaming_buffer_t *buf)
{
  unsigned long nPages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
#if defined(CONFIG_BPA2)
  char *partname;
  struct bpa2_part *part;
  if((buf->pDev->planeConfig->flags & STMCORE_PLANE_MEM_ANY) == STMCORE_PLANE_MEM_VIDEO)
    partname = "v4l2-video-buffers";
  else
    partname = "bigphysarea";

  buf->physicalAddr = 0;
  if (size == 0) return;

  part = bpa2_find_part(partname);
  if(!part)
    return;

  printk("BPA2 Alloc of %lu pages (total 0x%08lu bytes)\n",nPages, nPages * PAGE_SIZE);
  buf->physicalAddr = bpa2_alloc_pages(part, nPages, 1, GFP_KERNEL);
  if(buf->physicalAddr)
    buf->cpuAddr = ioremap_cache(buf->physicalAddr, nPages*PAGE_SIZE);

#else
  buf->physicalAddr = 0;
  buf->cpuAddr = bigphysarea_alloc_pages(nPages, 1, GFP_KERNEL);
  if(buf->cpuAddr)
    buf->physicalAddr = virt_to_phys(buf->cpuAddr);
#endif
}


static void freeBuffer(streaming_buffer_t *buf)
{
#if defined(CONFIG_BPA2)
  char *partname;
  struct bpa2_part *part;
  if((buf->pDev->planeConfig->flags & STMCORE_PLANE_MEM_ANY) == STMCORE_PLANE_MEM_VIDEO)
    partname = "v4l2-video-buffers";
  else
    partname = "bigphysarea";

  part = bpa2_find_part(partname);

  iounmap(buf->cpuAddr);
  bpa2_free_pages(part, buf->physicalAddr);
#else
  bigphysarea_free_pages(buf->cpuAddr);
#endif
  buf->physicalAddr = 0;
  buf->cpuAddr = 0;
}

#define CLUT_SIZE (256*4)

/************************************************
 *  mmapAllocateBuffers
 ************************************************/
int mmapAllocateBuffers(stvout_device_t *pDev, struct v4l2_requestbuffers* req)
{
  int           i;
  unsigned long bufLen;

  /* Make sure the number of requested buffers is legal */
  if (req->count > MAX_STREAM_BUFFERS)
    req->count = MAX_STREAM_BUFFERS;

  /* set the buffer length to a multiple of the page size */

  bufLen = (pDev->bufferFormat.fmt.pix.sizeimage + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

  debug_msg("Attempting to allocate %d buffers of %x bytes\n",req->count,(unsigned int)bufLen);

  /* now allocate the buffers */
  for (i = 0; i < req->count; ++i)
  {
    pDev->streamBuffers[i].pDev = pDev;

    allocateBuffer(bufLen, &pDev->streamBuffers[i]);
    pDev->streamBuffers[i].clutTable = pDev->clutBuffer + (i * CLUT_SIZE);
    pDev->streamBuffers[i].clutTable_phys = pDev->clutBuffer_phys + (i * CLUT_SIZE);

    if (pDev->streamBuffers[i].physicalAddr != 0)//bufferHeader.src.pVideoBuffer != 0)
    {
      debug_msg("Allocated buffer %d at 0x%08lx.\n",i, (unsigned long)pDev->streamBuffers[i].physicalAddr);

      pDev->streamBuffers[i].bufferHeader.src.ulVideoBufferAddr = pDev->streamBuffers[i].physicalAddr;
      memset(&(pDev->streamBuffers[i].vidBuf), 0, sizeof(struct v4l2_buffer));

      pDev->streamBuffers[i].isAllocated      = 1;

      pDev->streamBuffers[i].vidBuf.index     = i;
      pDev->streamBuffers[i].vidBuf.type      = req->type;
      pDev->streamBuffers[i].vidBuf.field     = pDev->bufferFormat.fmt.pix.field;

      pDev->streamBuffers[i].vidBuf.memory    = V4L2_MEMORY_MMAP;
//virt_to_phys(pDev->streamBuffers[i].bufferHeader.src.pVideoBuffer);
      pDev->streamBuffers[i].vidBuf.m.offset  = pDev->streamBuffers[i].bufferHeader.src.ulVideoBufferAddr;
      pDev->streamBuffers[i].vidBuf.length    = bufLen;

      /* Clear the buffer to prevent data leaking leaking into userspace */
      memset(pDev->streamBuffers[i].cpuAddr, 0x0, bufLen);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30)
      dma_cache_wback(pDev->streamBuffers[i].cpuAddr, bufLen);
#else
      writeback_ioremap_region(0, pDev->streamBuffers[i].cpuAddr, 0, bufLen);
#endif

      pDev->streamBuffers[i].bufferHeader.src.ulVideoBufferSize = bufLen;

      /* This is only applicable for YUV buffers with separate luma and chroma */
      pDev->streamBuffers[i].bufferHeader.src.chromaBufferOffset = pDev->bufferFormat.fmt.pix.priv;

      pDev->streamBuffers[i].bufferHeader.src.ulColorFmt    = pDev->planeFormat;
      pDev->streamBuffers[i].bufferHeader.src.ulPixelDepth  = g_capfmt[pDev->planeFormat].depth;

      pDev->streamBuffers[i].bufferHeader.src.ulStride      = pDev->bufferFormat.fmt.pix.bytesperline;
      pDev->streamBuffers[i].bufferHeader.src.ulTotalLines  = pDev->bufferFormat.fmt.pix.height;
      pDev->streamBuffers[i].bufferHeader.src.Rect.x        = 0;
      pDev->streamBuffers[i].bufferHeader.src.Rect.y        = 0;
      pDev->streamBuffers[i].bufferHeader.src.Rect.width    = pDev->bufferFormat.fmt.pix.width;
      pDev->streamBuffers[i].bufferHeader.src.Rect.height   = pDev->bufferFormat.fmt.pix.height;
      pDev->streamBuffers[i].bufferHeader.src.ulFlags       = (pDev->bufferFormat.fmt.pix.colorspace == V4L2_COLORSPACE_REC709)?STM_PLANE_SRC_COLORSPACE_709:0;

      pDev->streamBuffers[i].bufferHeader.src.ulClutBusAddress = pDev->streamBuffers[i].clutTable_phys;
	//virt_to_phys((void*)(pDev->streamBuffers[i].clutTable));

      /*
       * Specify that the buffer is persistent, this allows us to pause video by
       * simply not queuing any more buffers and the last frame stays on screen.
       */
      pDev->streamBuffers[i].bufferHeader.info.ulFlags            = STM_PLANE_PRESENTATION_PERSISTENT;
      pDev->streamBuffers[i].bufferHeader.info.pDisplayCallback   = &timestampCallback;
      pDev->streamBuffers[i].bufferHeader.info.pCompletedCallback = &bufferFinishedCallback;
      pDev->streamBuffers[i].bufferHeader.info.pUserData          = &(pDev->streamBuffers[i]);

      /*
       * Allocate a drawing surface for blitter access, note that we do not
       * support operations to and from YUV surfaces except for 422R.
       */
      if(pDev->planeFormat != SURF_YCBCR422MB &&
	 pDev->planeFormat != SURF_YCBCR420MB &&
	 pDev->planeFormat != SURF_YUYV       &&
	 pDev->planeFormat != SURF_YUV420     &&
	 pDev->planeFormat != SURF_YVU420     &&
	 pDev->planeFormat != SURF_YUV422P)
      {
//virt_to_phys(pDev->streamBuffers[i].bufferHeader.src.pVideoBuffer);
	pDev->streamBuffers[i].drawingSurface.ulMemory = pDev->streamBuffers[i].bufferHeader.src.ulVideoBufferAddr;
	pDev->streamBuffers[i].drawingSurface.ulSize   = bufLen;
	pDev->streamBuffers[i].drawingSurface.ulWidth  = pDev->bufferFormat.fmt.pix.width;
	pDev->streamBuffers[i].drawingSurface.ulHeight = pDev->bufferFormat.fmt.pix.height;
	pDev->streamBuffers[i].drawingSurface.ulStride = pDev->bufferFormat.fmt.pix.bytesperline;
	pDev->streamBuffers[i].drawingSurface.format   = pDev->planeFormat;
      }
      else
      {
	pDev->streamBuffers[i].drawingSurface.ulMemory = 0;
      }
    }
    else
    {
      debug_msg("Maximum number of buffers allocated.\n");

      // We've allocated as many buffers as we can
      req->count = i;
      break;
    }
  }

  return req->count;
}


/************************************************
 *  mmapDeleteBuffers
 ************************************************/
int mmapDeleteBuffers(stvout_device_t  *pDev)
{
  int i;

  debug_msg("mmapDeleteBuffers in\n");;

  for(i = 0; i < MAX_STREAM_BUFFERS; i++)
  {
    if (pDev->streamBuffers[i].isAllocated && (pDev->streamBuffers[i].mapCount > 0))
      return 0;

    if (pDev->userBuffers[i].isAllocated && (pDev->userBuffers[i].mapCount > 0))
      return 0;

  }

  for(i = 0; i < MAX_STREAM_BUFFERS; i++)
  {
    if (pDev->streamBuffers[i].isAllocated)
    {
      debug_msg("de-allocating buffer %d.\n",i);

      freeBuffer(&pDev->streamBuffers[i]);

      pDev->streamBuffers[i].isAllocated                        = 0;
      pDev->streamBuffers[i].vidBuf.m.offset                    = 0xFFFFFFFF;
      pDev->streamBuffers[i].vidBuf.flags                       = 0;
      pDev->streamBuffers[i].bufferHeader.src.ulVideoBufferAddr = 0;
      pDev->streamBuffers[i].bufferHeader.src.ulVideoBufferSize = 0;
      pDev->streamBuffers[i].drawingSurface.ulMemory = 0;
    }

    if (pDev->userBuffers[i].isAllocated)
    {
	    pDev->userBuffers[i].isAllocated                        = 0;
	    pDev->userBuffers[i].vidBuf.m.offset                    = 0xFFFFFFFF;
	    pDev->userBuffers[i].vidBuf.flags                       = 0;
	    pDev->streamBuffers[i].bufferHeader.src.ulVideoBufferAddr = 0;
	    pDev->streamBuffers[i].bufferHeader.src.ulVideoBufferSize = 0;
	    pDev->userBuffers[i].drawingSurface.ulMemory           = 0;
    }

  }

  debug_msg("mmapDeleteBuffers out\n");
  return 1;
}

/************************************************
 *  initBuffers and Queus
 ************************************************/
void initBuffers(stvout_device_t *pDev)
{
#if defined(CONFIG_BPA2)
  const char       * const partname = "bigphysarea";
  struct bpa2_part * const part     = bpa2_find_part(partname);

  BUG_ON (pDev->clutBuffer_phys != 0);
  BUG_ON (pDev->clutBuffer      != 0);

  if(!part)
    return;

  pDev->clutBuffer_phys = bpa2_alloc_pages(part, ((CLUT_SIZE * MAX_STREAM_BUFFERS * 2) + PAGE_SIZE) / PAGE_SIZE, 1, GFP_KERNEL);
  if(pDev->clutBuffer_phys)
      pDev->clutBuffer = (unsigned long)ioremap(pDev->clutBuffer_phys, CLUT_SIZE * MAX_STREAM_BUFFERS * 2);

#else
  pDev->clutBuffer = bigphysarea_alloc_pages(((CLUT_SIZE * MAX_STREAM_BUFFERS * 2) + PAGE_SIZE) / PAGE_SIZE, 1, GFP_KERNEL);
#endif

  v4l2_q_init(&(pDev->streamQpending));
  v4l2_q_init(&(pDev->streamQcomplete));
}

/************************************************
 *  initBuffers and Queus
 ************************************************/
void deinitBuffers(stvout_device_t *pDev)
{
#if defined(CONFIG_BPA2)
  const char       * const partname = "bigphysarea";
  struct bpa2_part * const part     = bpa2_find_part(partname);

  iounmap((void *) pDev->clutBuffer);

  bpa2_free_pages(part, pDev->clutBuffer_phys);
  pDev->clutBuffer_phys = 0;
#else
  bigphysarea_free_pages(pDev->clutBuffer);
#endif

  pDev->clutBuffer = 0;
}


unsigned long getPhysicalContiguous(unsigned long ptr,size_t size)
{
	struct mm_struct *mm = current->mm;
	unsigned virt_base =  (ptr / PAGE_SIZE) * PAGE_SIZE;
	unsigned phys_base = 0;

	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *ptep, pte;

	debug_msg("User pointer = 0x%08x\n", ptr);

	spin_lock(&mm->page_table_lock);

	pgd = pgd_offset(mm, virt_base);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
	  goto out;

	pmd = pmd_offset((pud_t*)pgd, virt_base);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
	  goto out;

	ptep = pte_offset_map(pmd, virt_base);

	if (!ptep)
	  goto out;

	pte = *ptep;

	if (pte_present(pte)) {
	  phys_base = __pa(page_address(pte_page(pte)));
	}

	if (!phys_base)
		goto out;

	spin_unlock(&mm->page_table_lock);
	return phys_base + (ptr - virt_base);


out:
	spin_unlock(&mm->page_table_lock);
	return 0;
}


/************************************************
 *  queueBuffer
 ************************************************/
int queueBuffer(stvout_device_t *pDev, struct v4l2_buffer* pVidBuf)
{
  unsigned int           repeatCount;
  enum v4l2_field        field;
  const stm_mode_line_t* pCurrentMode;
  stm_plane_caps_t       planeCaps;
  unsigned long          physicalPtr;
  int                    i         = pVidBuf->index;
  streaming_buffer_t    *pBuf      = NULL;
  TIME64                 oneSecond = 1000000LL;
  int                    clutSize  = 0;

  if (pVidBuf->memory == V4L2_MEMORY_USERPTR) {
	  for (i=0;i<MAX_STREAM_BUFFERS;i++)

		  if ((pDev->userBuffers[i].vidBuf.flags & V4L2_BUF_FLAG_QUEUED) == 0x00 ) {

			  physicalPtr = getPhysicalContiguous(pVidBuf->m.userptr, 1);

			  if (physicalPtr)
			  {
				  pBuf = &(pDev->userBuffers[i]);
				  pBuf->isAllocated = 1;

				  pBuf->pDev = pDev;

				  pBuf->vidBuf = *pVidBuf;
				  pBuf->vidBuf.m.offset = physicalPtr;

				  // Is this really a hack???
				  //pBuf->bufferHeader.src.pVideoBuffer = phys_to_virt(physicalPtr);
				  pBuf->bufferHeader.src.ulVideoBufferAddr = physicalPtr;

				  pBuf->bufferHeader.src.ulVideoBufferSize = pVidBuf->length;

				  /* This is only applicable for YUV buffers with separate luma and chroma */
				  pBuf->bufferHeader.src.chromaBufferOffset = pDev->bufferFormat.fmt.pix.priv;

				  pBuf->bufferHeader.src.ulColorFmt    = pDev->planeFormat;
				  pBuf->bufferHeader.src.ulPixelDepth  = g_capfmt[pDev->planeFormat].depth;

				  //printk("%x %d\n",pDev->planeFormat,g_capfmt[pDev->planeFormat].depth);

				  pBuf->bufferHeader.src.ulStride      = pDev->bufferFormat.fmt.pix.bytesperline;
				  pBuf->bufferHeader.src.ulTotalLines  = pDev->bufferFormat.fmt.pix.height;

				  //printk("%d bytes, %d lines\n",pDev->bufferFormat.fmt.pix.bytesperline, pDev->bufferFormat.fmt.pix.height);

				  pBuf->bufferHeader.src.Rect.x        = 0;
				  pBuf->bufferHeader.src.Rect.y        = 0;
				  pBuf->bufferHeader.src.Rect.width    = pDev->bufferFormat.fmt.pix.width;
				  pBuf->bufferHeader.src.Rect.height   = pDev->bufferFormat.fmt.pix.height;

				  //printk("%d x %d\n",pDev->bufferFormat.fmt.pix.width,pDev->bufferFormat.fmt.pix.height);

				  pBuf->bufferHeader.src.ulFlags       = (pDev->bufferFormat.fmt.pix.colorspace == V4L2_COLORSPACE_REC709)?STM_PLANE_SRC_COLORSPACE_709:0;

				  /*
				   * Specify that the buffer is persistent, this allows us to pause video by
				   * simply not queuing any more buffers and the last frame stays on screen.
				   */
				  pBuf->bufferHeader.info.ulFlags            = STM_PLANE_PRESENTATION_PERSISTENT;
				  pBuf->bufferHeader.info.pDisplayCallback   = &timestampCallback;
				  pBuf->bufferHeader.info.pCompletedCallback = &bufferFinishedCallback;
				  pBuf->bufferHeader.info.pUserData          = pBuf;

//                                printk("clut = %x + %x\n",pDev->clutBuffer , (CLUT_SIZE * (i + MAX_STREAM_BUFFERS)));
				  pBuf->clutTable = pDev->clutBuffer + (CLUT_SIZE * (i + MAX_STREAM_BUFFERS));

				  pBuf->bufferHeader.src.ulClutBusAddress = pDev->clutBuffer_phys + (CLUT_SIZE * (i + MAX_STREAM_BUFFERS));

				  debug_msg("Found the physical pointer full pointer 0x%08x\n",physicalPtr);
				  break;
			  } else {

				  err_msg("User space pointer is a bit wonky\n");
				  return -EINVAL;
			  }
		  }

	  if (!pBuf) {
		  err_msg("queueBuffer - too many queued user buffers, please dequeue one first\n");
		  return -EIO;
	  }
  } else {
	  if (i < 0 || i >= MAX_STREAM_BUFFERS)
	  {
		  err_msg("queueBuffer - buffer index %d is out of range\n", i);
		  return -EINVAL;
	  }

	  pBuf = &(pDev->streamBuffers[i]);
  }

  if (pBuf->vidBuf.flags & V4L2_BUF_FLAG_QUEUED)
  {
	  err_msg("queueBuffer - buffer %d is already queued\n", i);
	  return -EIO;
  }

  pCurrentMode = stm_display_output_get_current_display_mode(pDev->pOutput);
  if(!pCurrentMode)
	  return -EIO;

  if(stm_display_plane_get_capabilities(pDev->pPlane, &planeCaps)<0)
    return -ERESTARTSYS;

  if (!(planeCaps.ulCaps & PLANE_CAPS_RESIZE))
  {
    /*
     * We need to check this here because the buffer format and output
     * window size can change independently, only at the point of queuing
     * the buffer can we decide if the configuration is valid, which
     * isn't ideal.
     */
    if(pDev->srcCrop.c.width != pDev->outputCrop.c.width)
    {
      debug_msg("queueBuffer - failed, HW doesn't support resizing\n");
      return -EINVAL;
    }

    if(pDev->srcCrop.c.height != pDev->outputCrop.c.height)
    {
      debug_msg("queueBuffer - failed, HW doesn't support resizing\n");
      return -EINVAL;
    }
  }

  pBuf->bufferHeader.src.Rect.x      = pDev->srcCrop.c.left;
  pBuf->bufferHeader.src.Rect.y      = pDev->srcCrop.c.top;
  pBuf->bufferHeader.src.Rect.width  = pDev->srcCrop.c.width;
  pBuf->bufferHeader.src.Rect.height = pDev->srcCrop.c.height;

  pBuf->bufferHeader.dst.Rect.x      = pDev->outputCrop.c.left;
  pBuf->bufferHeader.dst.Rect.y      = pDev->outputCrop.c.top;
  pBuf->bufferHeader.dst.Rect.width  = pDev->outputCrop.c.width;
  pBuf->bufferHeader.dst.Rect.height = pDev->outputCrop.c.height;

  //

  clutSize = 0;

  if (pVidBuf->reserved)
  switch(pDev->bufferFormat.fmt.pix.pixelformat)
  {
    case V4L2_PIX_FMT_CLUTA8:
    case V4L2_PIX_FMT_CLUT8:
      clutSize += (256 - 16);
    case V4L2_PIX_FMT_CLUT4:
      clutSize += (16 - 4);
    case V4L2_PIX_FMT_CLUT2:
      clutSize += 4;      
      for (i=0;i<clutSize;i++)
	      ((int*)pBuf->clutTable)[i] = ((((int*)pVidBuf->reserved)[i]) & 0x00ffffff) | ((((int*)pVidBuf->reserved)[i] >> 1) & 0x7f000000);
    break;

  default:
      break;
  }

  //

  if(pDev->globalAlpha < 255)
  {
    pBuf->bufferHeader.src.ulConstAlpha = pDev->globalAlpha;
    pBuf->bufferHeader.src.ulFlags |= STM_PLANE_SRC_CONST_ALPHA;
  }
  else
  {
    pBuf->bufferHeader.src.ulFlags &= ~STM_PLANE_SRC_CONST_ALPHA;
  }
  /*
   * This is a bit of OLD V4L2 that we are keeping around until we properly
   * support presentation timestamps to support slow motion.
   */
  repeatCount = ((pVidBuf->flags & 0xC0000000) >> 30)+1;

  field = pVidBuf->field;

  if(pDev->bufferFormat.fmt.pix.field == V4L2_FIELD_ANY)
  {
    if(field != V4L2_FIELD_NONE && field != V4L2_FIELD_INTERLACED)
    {
      err_msg("queueBuffer - field option not supported\n");
      return -EINVAL;
    }

    if(field                                       == V4L2_FIELD_INTERLACED &&
       pCurrentMode->ModeParams.ScanType           == SCAN_P &&
       (planeCaps.ulCaps & PLANE_CAPS_DEINTERLACE) == 0)
    {
      err_msg("queueBuffer - de-interlaced content on a progressive display not supported\n");
      return -EINVAL;
    }

  }
  else
  {
    if(field != pDev->bufferFormat.fmt.pix.field)
    {
      err_msg("queueBuffer - field option does not match buffer format\n");
      return -EINVAL;
    }
  }

  pBuf->vidBuf.field = field;

  switch(pBuf->vidBuf.field)
  {
    default:
      debug_msg("queueBuffer - buffer field setting has been corrupted\n");
      /* Fall through */
    case V4L2_FIELD_NONE:
      /*
       * Not all hardware (e.g. GPDs) can be updated every frame on a
       * progressive display. The planeCaps tell us the minimum number
       * of fields/frames (1 or 2) that a buffer must be on display for
       * in the current display mode.
       */
      repeatCount *= planeCaps.ulMinFields;
      pBuf->bufferHeader.src.ulFlags &= ~STM_PLANE_SRC_INTERLACED;
      break;

    case V4L2_FIELD_INTERLACED:
      repeatCount *= 2;
      pBuf->bufferHeader.src.ulFlags |= STM_PLANE_SRC_INTERLACED;
      break;
  }

  pBuf->bufferHeader.info.nfields          = repeatCount;
  pBuf->bufferHeader.info.presentationTime = ((TIME64)pVidBuf->timestamp.tv_sec * oneSecond) +
					      (TIME64)pVidBuf->timestamp.tv_usec;

  if(pVidBuf->flags & V4L2_BUF_FLAG_REPEAT_FIRST_FIELD)
  {
    pBuf->vidBuf.flags             |= V4L2_BUF_FLAG_REPEAT_FIRST_FIELD;
    pBuf->bufferHeader.src.ulFlags |= STM_PLANE_SRC_REPEAT_FIRST_FIELD;
  }
  else
  {
    pBuf->vidBuf.flags             &= ~V4L2_BUF_FLAG_REPEAT_FIRST_FIELD;
    pBuf->bufferHeader.src.ulFlags &= ~STM_PLANE_SRC_REPEAT_FIRST_FIELD;
  }

  if(pVidBuf->flags & V4L2_BUF_FLAG_BOTTOM_FIELD_FIRST)
  {
    pBuf->vidBuf.flags             |= V4L2_BUF_FLAG_BOTTOM_FIELD_FIRST;
    pBuf->bufferHeader.src.ulFlags |= STM_PLANE_SRC_BOTTOM_FIELD_FIRST;
  }
  else
  {
    pBuf->vidBuf.flags             &= ~V4L2_BUF_FLAG_BOTTOM_FIELD_FIRST;
    pBuf->bufferHeader.src.ulFlags &= ~STM_PLANE_SRC_BOTTOM_FIELD_FIRST;
  }

  if(pVidBuf->flags & V4L2_BUF_FLAG_INTERPOLATE_FIELDS)
  {
    pBuf->vidBuf.flags             |= V4L2_BUF_FLAG_INTERPOLATE_FIELDS;
    pBuf->bufferHeader.src.ulFlags |= STM_PLANE_SRC_INTERPOLATE_FIELDS;
  }
  else
  {
    pBuf->vidBuf.flags             &= ~V4L2_BUF_FLAG_INTERPOLATE_FIELDS;
    pBuf->bufferHeader.src.ulFlags &= ~STM_PLANE_SRC_INTERPOLATE_FIELDS;
  }

  pBuf->vidBuf.flags &= ~V4L2_BUF_FLAG_DONE;
  pBuf->vidBuf.flags |= V4L2_BUF_FLAG_QUEUED;

  /*
   * The API spec states that the flags are reflected in the structure the
   * user passed in, but nothing else is.
   */
  pVidBuf->flags = pBuf->vidBuf.flags;

  v4l2_q_add_tail(&pDev->streamQpending, &pBuf->qnode);

/*  debug_msg("queueBuffer - queued buffer %d\n",i); */

  if (pDev->isStreaming)
  {
    writeNextFrame(pDev);
  }

  return 0;
}


/************************************************
 *  hasQueuedBuffers
 ************************************************/
int hasQueuedBuffers(stvout_device_t *pDev)
{
  return (v4l2_q_peek_head(&(pDev->streamQpending)) != NULL);
}


/************************************************
 *  hasCompletedBuffers
 ************************************************/
int hasCompletedBuffers(stvout_device_t *pDev)
{
  return (v4l2_q_peek_head(&(pDev->streamQcomplete)) != NULL);
}


/************************************************
 *  dequeueBuffer : 1 = success; 0 = failed
 ************************************************/
int  dequeueBuffer(stvout_device_t *pDev, struct v4l2_buffer* pVidBuf)
{
  streaming_buffer_t *pBuf;

  pBuf = v4l2_q_del_head(&(pDev->streamQcomplete));
  if (pBuf == NULL)
  {
    return 0;
  }

  pBuf->vidBuf.flags &= ~(V4L2_BUF_FLAG_QUEUED | V4L2_BUF_FLAG_DONE);

/*  debug_msg("dequeueBuffer - dequeueing buffer %d\n",newBuf->vidBuf.index); */

  *pVidBuf = pBuf->vidBuf;

  return 1;
}


/************************************************
 *  setBufferFormat - ioctl hepler function for S_FMT
 *  returns 0 success or a standard error code.
 ************************************************/
int setBufferFormat(stvout_device_t *pDev, struct v4l2_format *fmt, int updateConfig)
{
  const SURF_FMT *formats;
  int num_formats;
  int index;
  int minbytesperline;

  stm_plane_caps_t       planeCaps;
  const stm_mode_line_t* pCurrentMode;

  if(stm_display_plane_get_capabilities(pDev->pPlane, &planeCaps)<0)
    return -ERESTARTSYS;


  pCurrentMode = stm_display_output_get_current_display_mode(pDev->pOutput);
  if(!pCurrentMode)
  {
    if(signal_pending(current))
      return -ERESTARTSYS;
    else
      return -EIO;
  }

  num_formats = stm_display_plane_get_image_formats(pDev->pPlane, &formats);
  if(signal_pending(current))
    return -ERESTARTSYS;

  /*
   * Setting buffer format should not fail when a requested format isn't
   * supported, but fall back to the closest the hardware can provide. The fmt
   * structure is both input and output (unlike in S_CROP) so must reflect the
   * actual format selected once we have finished.
   */
  if(fmt->fmt.pix.field != V4L2_FIELD_ANY  &&
     fmt->fmt.pix.field != V4L2_FIELD_NONE &&
     fmt->fmt.pix.field != V4L2_FIELD_INTERLACED )
  {
    fmt->fmt.pix.field = V4L2_FIELD_ANY;
  }

  if(fmt->fmt.pix.field                     == V4L2_FIELD_INTERLACED &&
     pCurrentMode->ModeParams.ScanType      == SCAN_P &&
     (planeCaps.ulCaps & PLANE_CAPS_DEINTERLACE) == 0)
  {
    err_msg("setBufferFormat - de-interlaced content on a progressive display not supported on this plane");
    fmt->fmt.pix.field = V4L2_FIELD_NONE;
  }

  switch(fmt->fmt.pix.pixelformat)
  {
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_STM420MB:
    case V4L2_PIX_FMT_STM422MB:
      /*
       * The plane converts YUV to 10bit/component RGB internally for
       * mixing with the rest of the display, the mixed RGB output gets converted
       * back to the correct YUV colourspace for the display mode automatically.
       * A source YUV buffer to be displayed on a plane can be specified as being
       * in either 601 (SMPTE170M) or 709; this specifies the maths used for the
       * internal conversion to RGB.
       */
      if(fmt->fmt.pix.colorspace != V4L2_COLORSPACE_SMPTE170M &&
	 fmt->fmt.pix.colorspace != V4L2_COLORSPACE_REC709)
      {
	/*
	 * Pick a default based on the display standard. SD = 601, HD = 709
	 */
	if((pCurrentMode->ModeParams.OutputStandards & STM_OUTPUT_STD_HD_MASK) != 0)
	  fmt->fmt.pix.colorspace  = V4L2_COLORSPACE_REC709;
	else
	  fmt->fmt.pix.colorspace  = V4L2_COLORSPACE_SMPTE170M;
      }
      break;
    default:
      /*
       * For RGB surface formats sRGB content is assumed.
       */
      fmt->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
      break;
  }

  for (index = 0; index < num_formats; index++)
  {
    if (g_capfmt[formats[index]].fmt.pixelformat == fmt->fmt.pix.pixelformat)
      break;
  }

  if (index == num_formats)
  {
      /*
       * If the plane doesn't support the requested format
       * pick the first format it can support.
       */
      index = 0;
      fmt->fmt.pix.pixelformat = g_capfmt[formats[index]].fmt.pixelformat;
  }

  /*
   * WARNING: Do not change "index" after this point as it is used at the end of
   *          the function - which by definition is way too long if it needs
   *          this comment.
   */

  /*
   * For interlaced buffers we need an even number of lines.
   * We will round up and try and provide more than the number of
   * lines requested, assuming this doesn't go over the maximum allowed.
   */
  if (fmt->fmt.pix.field == V4L2_FIELD_INTERLACED || fmt->fmt.pix.field == V4L2_FIELD_ANY)
  {
    fmt->fmt.pix.height += fmt->fmt.pix.height%2;
  }

  switch(fmt->fmt.pix.pixelformat)
  {
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_YUYV:
      /*
       * 422 Raster (interleaved luma and chroma), we need an even number
       * of luma samples (pixels) per line
       */
      fmt->fmt.pix.width  += fmt->fmt.pix.width % 2;
      break;
    case V4L2_PIX_FMT_STM420MB:
    case V4L2_PIX_FMT_STM422MB:
      /*
       * For planar macroblock formats we round to 1 macroblock (16pixels/lines
       * per macroblock). The actual physical size of the memory buffer has to
       * be larger as there is wastage due to the macroblock memory
       * oprganization,but that is dealt with below.
       */
      fmt->fmt.pix.width  = (fmt->fmt.pix.width  + 15) & ~15;
      fmt->fmt.pix.height = (fmt->fmt.pix.height + 15) & ~15;
      break;
  }

  /*
   * If the plane caps came back with 0 for the maximum width
   * or height then fallback to using the display mode values.
   */
  if(planeCaps.ulMaxWidth == 0)
    planeCaps.ulMaxWidth = pCurrentMode->TimingParams.PixelsPerLine;

  if(planeCaps.ulMaxHeight == 0)
    planeCaps.ulMaxHeight = pCurrentMode->ModeParams.ActiveAreaHeight;

  if (fmt->fmt.pix.width > planeCaps.ulMaxWidth)
    fmt->fmt.pix.width  = planeCaps.ulMaxWidth;

  if (fmt->fmt.pix.width < planeCaps.ulMinWidth)
    fmt->fmt.pix.width  = planeCaps.ulMinWidth;

  if (fmt->fmt.pix.height > planeCaps.ulMaxHeight)
    fmt->fmt.pix.height = planeCaps.ulMaxHeight;

  if (fmt->fmt.pix.height < planeCaps.ulMinHeight)
    fmt->fmt.pix.height = planeCaps.ulMinHeight;

  /*
   * We only honour the bytesperline set by the application for raster formats.
   * If the user specified value is less than the minimum required for the
   * requested width/height/format then we override the user value. Additionally
   * we ensure this is rounded to 4bytes in 16 and 8bit formats; although
   * this is probably not required by the hardware, it has always been done
   * this way.
   *
   * For the ST planar macroblock formats we _always_ set this to the "virtual"
   * stride of the luma buffer, which is used to present the buffer correctly.
   */
  minbytesperline  = fmt->fmt.pix.width * (g_capfmt[formats[index]].depth >> 3);
  minbytesperline += minbytesperline % 4;

  if (fmt->fmt.pix.bytesperline < minbytesperline        ||
      fmt->fmt.pix.pixelformat  == V4L2_PIX_FMT_STM420MB ||
      fmt->fmt.pix.pixelformat  == V4L2_PIX_FMT_STM422MB)
  {
    fmt->fmt.pix.bytesperline = minbytesperline;
  }

  /*
   * Work out the physical size of the buffer, for normal formats it is simply
   * the stride * height. However for the two ST planar macroblock formats
   * we need to add the size of the chroma buffer to this.
   */
  switch(fmt->fmt.pix.pixelformat)
  {
    case V4L2_PIX_FMT_STM422MB:
    case V4L2_PIX_FMT_STM420MB:
    {
      int lumasize;
      int chromasize;
      int mbwidth    = fmt->fmt.pix.width  / 16;
      int mbheight   = fmt->fmt.pix.height / 16;
      /*
       * These buffer size calculations are taken directly from the datasheet.
       * However if we use them with an odd mbwidth (e.g. 720pixels->45MBs)
       * then we get video corruption when using the hardware MPEG decoder to
       * fill the buffers. So, although we don't understand why, we up the
       * size of the luma buffer by ensuring mbwidth is even. Sigh...
       */
      mbwidth += mbwidth % 2;

      lumasize   = mbwidth * ((mbheight+1)/2) * 512;
      chromasize = ((mbwidth+1)/2) * ((mbheight+1)/2) * 512;

      if(fmt->fmt.pix.pixelformat == V4L2_PIX_FMT_STM422MB)
	chromasize *= 2;

      fmt->fmt.pix.sizeimage = lumasize+chromasize;

      /*
       * This specifies the start offset in bytes of the Chroma from the
       * beginning of the buffer in planar macroblock YUV formats.
       */
      fmt->fmt.pix.priv = lumasize;

      break;
    }
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420:
    {
      /*
       * 420 with separate luma and chroma buffers, the format is defined such
       * that the size of 2 chroma lines is the same as one luma line including
       * padding.
       */
      int lumasize = fmt->fmt.pix.bytesperline * fmt->fmt.pix.height;

      /*
       * Ensure the chroma buffers start on an 8 byte boundary
       */
      lumasize += (lumasize % 8);

      fmt->fmt.pix.priv = lumasize;

      fmt->fmt.pix.sizeimage = lumasize + (lumasize/2);
      break;

    }
    case V4L2_PIX_FMT_YUV422P:
    {
      /*
       * 422 with separate luma and chroma buffers, the format is defined such
       * that the size of 2 chroma lines is the same as one luma line including
       * padding.
       */
      int lumasize = fmt->fmt.pix.bytesperline * fmt->fmt.pix.height;

      /*
       * Ensure the chroma buffers start on an 8 byte boundary
       */
      lumasize += (lumasize % 8);

      fmt->fmt.pix.priv = lumasize;
      /*
       * The chroma buffers combined are the same size as the luma buffer in 422
       */
      fmt->fmt.pix.sizeimage = lumasize * 2;
      break;
    }
    default:
    {
      fmt->fmt.pix.sizeimage = fmt->fmt.pix.bytesperline * fmt->fmt.pix.height;
      break;
    }
  }


  /*
   * Change the driver state if requested by the caller.
   */
  if(updateConfig)
  {
    pDev->planeFormat  = formats[index];
    pDev->bufferFormat = *fmt;

    /*
     * Set the source crop to default to the whole buffer
     */
    pDev->srcCrop.type     = V4L2_BUF_TYPE_PRIVATE;
    pDev->srcCrop.c.top    = 0;
    pDev->srcCrop.c.left   = 0;
    pDev->srcCrop.c.width  = fmt->fmt.pix.width;
    pDev->srcCrop.c.height = fmt->fmt.pix.height;
    setBufferCrop(pDev, &pDev->srcCrop);
  }

  return 0;
}


/************************************************
 *  setOutputCrop - ioctl hepler function for S_CROP
 ************************************************/
int setOutputCrop(stvout_device_t *pDev, struct v4l2_crop *pCrop)
{
  const stm_mode_line_t* pCurrentMode;
  stm_plane_caps_t       planeCaps;
  struct v4l2_crop       origCrop;

  debug_msg("setOutputCrop - (%ld,%ld,%ld,%ld)\n",(long)pCrop->c.left,(long)pCrop->c.top,(long)pCrop->c.width,(long)pCrop->c.height);

  pCurrentMode = stm_display_output_get_current_display_mode(pDev->pOutput);
  if(!pCurrentMode)
  {
    if(signal_pending(current))
      return -ERESTARTSYS;
    else
      return -EIO;
  }

  if(stm_display_plane_get_capabilities(pDev->pPlane, &planeCaps)<0)
    return -ERESTARTSYS;

  origCrop = *pCrop;

  /*
   * Make sure the width and height can be contained in the video active area
   * of the current display mode, adjusting them down while maintaining the
   * original requested aspect ratio.
   *
   * Note: that this does not produce an error if the application request
   * is outside the bounds of the display. The user code should query the actual
   * crop selected by doing a G_CROP IOCTL; it can use this to continue to
   * negotiate a mutually suitable output window with the driver.
   */
  if(pCrop->c.width > pCurrentMode->ModeParams.ActiveAreaWidth)
  {
    pCrop->c.width  = pCurrentMode->ModeParams.ActiveAreaWidth;
    pCrop->c.height = (pCrop->c.width * origCrop.c.height) / origCrop.c.width;
  }

  if(pCrop->c.height > pCurrentMode->ModeParams.ActiveAreaHeight)
  {
    pCrop->c.height = pCurrentMode->ModeParams.ActiveAreaHeight;
    pCrop->c.width = (pCrop->c.height * origCrop.c.width) / origCrop.c.height;
  }

  if(pCurrentMode->ModeParams.ScanType == SCAN_I)
  {
    /*
     * Round the height up to a multiple of two lines if needed, don't bother
     * to readjust the width, the aspect ratio will be close enough.
     */
    pCrop->c.height += pCrop->c.height%2;

    /*
     * Make sure top of the output is on the top field on an interlaced display,
     * we do this by shifting one line up if necessary.
     *
     * Note that the display driver translates (0,0) to the correct first active
     * top field line and pixel based on the current display mode; this code
     * doesn't need to deal with blanking lines etc..
     */
    pCrop->c.top -= pCrop->c.top%2;
  }

  /*
   * Now constrain the crop to within the screen bounds.
   */
  if (pCrop->c.left < 0)
    pCrop->c.left = 0; /* v4l2_rect structure contains signed values */

  if (pCrop->c.top < 0)
    pCrop->c.top = 0;

  if ((pCrop->c.left + pCrop->c.width) > pCurrentMode->ModeParams.ActiveAreaWidth)
  {
    pCrop->c.left = pCurrentMode->ModeParams.ActiveAreaWidth - pCrop->c.width;
  }

  if ((pCrop->c.top + pCrop->c.height) > pCurrentMode->ModeParams.ActiveAreaHeight)
  {
    pCrop->c.top = pCurrentMode->ModeParams.ActiveAreaHeight - pCrop->c.height;
  }

  pDev->outputCrop = *pCrop;

  return 0;
}


/************************************************
 *  setBufferCrop - ioctl hepler function for S_CROP
 ************************************************/
int setBufferCrop(stvout_device_t *pDev, struct v4l2_crop *pCrop)
{
  struct v4l2_crop       origCrop = *pCrop;

  debug_msg("setBufferCrop - (%ld,%ld,%ld,%ld)\n",(long)pCrop->c.left,(long)pCrop->c.top,(long)pCrop->c.width,(long)pCrop->c.height);

  /*
   * Make sure the width and height can be contained in the buffer
   * them down while maintaining the original requested aspect ratio.
   *
   * As with setting output crop the application should use G_CROP to
   * find out the result and try S_CROP again if it wants to adjust the
   * selction futher.
   */
  if(pCrop->c.width > pDev->bufferFormat.fmt.pix.width)
  {
    pCrop->c.width  = pDev->bufferFormat.fmt.pix.width;
    pCrop->c.height = (pCrop->c.width * origCrop.c.height) / origCrop.c.width;
  }

  if(pCrop->c.height > pDev->bufferFormat.fmt.pix.height)
  {
    pCrop->c.height = pDev->bufferFormat.fmt.pix.height;
    pCrop->c.width  = (pCrop->c.height * origCrop.c.width) / origCrop.c.height;
  }

  if(pDev->bufferFormat.fmt.pix.field == V4L2_FIELD_INTERLACED)
  {
    /*
     * Round the height up to a multiple of two lines for interlaced buffers
     */
    pCrop->c.height += pCrop->c.height%2;

    /*
     * Make sure the first line is the top field otherwise the fields will
     * be reversed on both interlaced and progressive displays.
     *
     */
    pCrop->c.top -= pCrop->c.top%2;
  }

  /*
   * Now constrain the crop to within the buffer extent.
   */
  if (pCrop->c.left < 0)
    pCrop->c.left = 0; /* v4l2_rect structure contains signed values */

  if (pCrop->c.top < 0)
    pCrop->c.top = 0;

  if ((pCrop->c.left + pCrop->c.width) > pDev->bufferFormat.fmt.pix.width)
  {
    pCrop->c.left = pDev->bufferFormat.fmt.pix.width - pCrop->c.width;
  }

  if ((pCrop->c.top + pCrop->c.height) > pDev->bufferFormat.fmt.pix.height)
  {
    pCrop->c.top = pDev->bufferFormat.fmt.pix.height - pCrop->c.height;
  }

  pDev->srcCrop = *pCrop;

  return 0;
}


/************************************************
 *  streamoff - ioctl helper function for
 ************************************************/
int streamoff(stvout_device_t  *pDev)
{
  streaming_buffer_t *newBuf;

  debug_msg("streamoff in - pDev = 0x%p\n",pDev);

  if (pDev->isStreaming)
  {
    /* Discard the pending queue */
    while((newBuf = v4l2_q_del_head(&(pDev->streamQpending))) != NULL)
    {
      newBuf->vidBuf.flags &= ~V4L2_BUF_FLAG_QUEUED;
      debug_msg("streamoff - discarding buffer %d\n",newBuf->vidBuf.index);
    }

    debug_msg("streamoff - About to flush plane = 0x%p\n",pDev->pPlane);

    /*
     * Unlock (and Flush) the display plane, this will cause all the completed
     * callbacks for each queued buffer to be called.
     */
    if(stm_display_plane_unlock(pDev->pPlane)<0)
    {
      if(signal_pending(current))
	return -ERESTARTSYS;
      else
	return -EIO;
    }

    /*
     * Disconnect the plane from the current output, this allows us to
     * change output once we are no longer streaming.
     */
    if(stm_display_plane_disconnect_from_output(pDev->pPlane, pDev->pOutput)<0)
    {
      if(signal_pending(current))
	return -ERESTARTSYS;
      else
	return -EIO;
    }

    debug_msg("streamoff - About to dequeue completed queue \n");

    /* dqueue all buffers from the complete queue */
    while((newBuf = v4l2_q_del_head(&(pDev->streamQcomplete))) != NULL)
    {
      newBuf->vidBuf.flags &= ~V4L2_BUF_FLAG_QUEUED;
      debug_msg("streamoff - dequeuing buffer %d\n",newBuf->vidBuf.index);
    }

    pDev->isStreaming=0;
    wake_up_interruptible(&(pDev->wqStreamingState));
  }

  debug_msg("streamoff out\n");
  return 0;
}


/************************************************
 *  mmapGetBufferFromOffset
 ************************************************/
streaming_buffer_t *mmapGetBufferFromOffset(stvout_device_t  *pDev, unsigned long offset)
{
  int   i;

  debug_msg("mmapGetBufferFromOffset in. offset = 0x%08x\n", (int)offset);

  for (i = 0; i < MAX_STREAM_BUFFERS; ++i)
  {
    if (offset >= pDev->streamBuffers[i].vidBuf.m.offset &&
	offset <  (pDev->streamBuffers[i].vidBuf.m.offset + pDev->streamBuffers[i].vidBuf.length))
    {
      debug_msg("mmapGetBufferFromOffset out. buffer found. index = %d\n",i);
      return &(pDev->streamBuffers[i]);
    }
  }

  debug_msg("mmapGetBufferFromOffset out. buffer not found\n");

  return NULL;
}


/************************************************
 *  getCtrlName - ioctl hepler function for S_CTRL and G_CTRL
 *  to translate V4L2 control IDs to STG driver control IDs.
 ************************************************/
unsigned long getCtrlName(int id)
{
  switch(id)
  {
    case V4L2_CID_BRIGHTNESS:
      return STM_CTRL_BRIGHTNESS;

    case V4L2_CID_CONTRAST:
      return STM_CTRL_CONTRAST;

    case V4L2_CID_SATURATION:
      return STM_CTRL_SATURATION;

    case V4L2_CID_HUE:
      return STM_CTRL_HUE;

    case V4L2_CID_STM_IQI_SET_CONFIG:
      return PLANE_CTRL_IQI_CONFIG;
    case V4L2_CID_STM_IQI_DEMO:
      return PLANE_CTRL_IQI_DEMO;
    case V4L2_CID_STM_XVP_SET_CONFIG:
      return PLANE_CTRL_XVP_CONFIG;
    case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE:
      return PLANE_CTRL_XVP_TNRNLE_OVERRIDE;

    default:
      if (id >= V4L2_CID_STM_Z_ORDER_VID0 && id <= V4L2_CID_STM_Z_ORDER_RGB2)
	return 1000;

      return 0;
  }
}


/************************************************
 *  getDisplayStandard - ioctl hepler for G_STD and ENUM_STD
 *  returns 0 on success or a standard error code.
 ************************************************/
int getDisplayStandard(stvout_device_t *pDev, struct v4l2_standard *std)
{
  const stm_mode_line_t *pCurrentMode;
  ULONG tvStandard;

  pCurrentMode = stm_display_output_get_current_display_mode(pDev->pOutput);
  if(!pCurrentMode)
  {
    if(signal_pending(current))
      return -ERESTARTSYS;
    else
      return -EIO;
  }

  if(stm_display_output_get_current_tv_standard(pDev->pOutput, &tvStandard)<0)
    return -ERESTARTSYS;


  std->framelines              = pCurrentMode->TimingParams.LinesByFrame;
  std->frameperiod.numerator   = 1000;
  std->frameperiod.denominator = pCurrentMode->ModeParams.FrameRate;

  if(pCurrentMode->ModeParams.ScanType == SCAN_I)
    std->frameperiod.denominator = std->frameperiod.denominator/2;

  if(tvStandard & STM_OUTPUT_STD_SMPTE293M)
  {
    std->id = V4L2_STD_UNKNOWN;
    strcpy(std->name,"SMPTE293M");
    return 0;
  }

  if(tvStandard & STM_OUTPUT_STD_VESA)
  {
    std->id = V4L2_STD_UNKNOWN;
    strcpy(std->name,"VESA");
    return 0;
  }

  switch(tvStandard & (STM_OUTPUT_STD_SD_MASK | STM_OUTPUT_STD_HD_MASK))
  {
     case STM_OUTPUT_STD_PAL_BDGHI:
       std->id = (V4L2_STD_PAL & ~V4L2_STD_PAL_K);
       strcpy(std->name,"PAL_BDGHI");
       break;
     case STM_OUTPUT_STD_PAL_M:
       std->id = V4L2_STD_PAL_M;
       strcpy(std->name,"PAL_M");
       break;
     case STM_OUTPUT_STD_PAL_N:
       std->id = V4L2_STD_PAL_N;
       strcpy(std->name,"PAL_N");
       break;
     case STM_OUTPUT_STD_PAL_Nc:
       std->id = V4L2_STD_PAL_Nc;
       strcpy(std->name,"PAL_Nc");
       break;
     case STM_OUTPUT_STD_SECAM:
       std->id = V4L2_STD_SECAM;
       strcpy(std->name,"SECAM");
       break;
     case STM_OUTPUT_STD_NTSC_M:
     case STM_OUTPUT_STD_NTSC_443:
       std->id = V4L2_STD_NTSC_M;
       strcpy(std->name,"NTSC_M");
       break;
     case STM_OUTPUT_STD_NTSC_J:
       std->id = V4L2_STD_NTSC_M_JP;
       strcpy(std->name,"NTSC_J");
       break;
     case STM_OUTPUT_STD_SMPTE240M:
       std->id = V4L2_STD_UNKNOWN;
       strcpy(std->name,"SMPTE240M");
       break;
     case STM_OUTPUT_STD_SMPTE274M:
       std->id = V4L2_STD_UNKNOWN;
       strcpy(std->name,"SMPTE274M");
       break;
     case STM_OUTPUT_STD_SMPTE295M:
       std->id = V4L2_STD_UNKNOWN;
       strcpy(std->name,"SMPTE295M");
       break;
     case STM_OUTPUT_STD_SMPTE296M:
       std->id = V4L2_STD_UNKNOWN;
       strcpy(std->name,"SMPTE296M");
       break;
     case STM_OUTPUT_STD_AS4933:
       std->id = V4L2_STD_UNKNOWN;
       strcpy(std->name,"AS4933");
       break;
     default:
       return -EIO;
  }

  return 0;
}

/************************************************
 *  getDisplayDefaults - ioctl hepler for CROPCAP
 *  returns 0 on success or a standard error code.
 ************************************************/
int getDisplayDefaults(stvout_device_t *pDev, struct v4l2_cropcap *cap)
{
  const stm_mode_line_t *pCurrentMode;
  ULONG tvStandard;

  pCurrentMode = stm_display_output_get_current_display_mode(pDev->pOutput);
  if(!pCurrentMode)
  {
    if(signal_pending(current))
      return -ERESTARTSYS;
    else
      return -EIO;
  }

  if(stm_display_output_get_current_tv_standard(pDev->pOutput, &tvStandard)<0)
    return -ERESTARTSYS;

  /*
   * The output bounds and default output size are the full active video area
   * for the display mode with an origin at (0,0). The display driver will
   * position plane horizontally and vertically on the display, based on the
   * current display mode.
   */
  cap->bounds.left   = 0;
  cap->bounds.top    = 0;
  cap->bounds.width  = pCurrentMode->ModeParams.ActiveAreaWidth;
  cap->bounds.height = pCurrentMode->ModeParams.ActiveAreaHeight;

  cap->defrect = cap->bounds;

#ifndef __TDT__
  if(pCurrentMode->ModeParams.SquarePixel)
  {
    cap->pixelaspect.numerator   = 1;
    cap->pixelaspect.denominator = 1;
  }
  else
  {
    /*
     * We assume that the SMPTE293M SD progressive modes have the
     * same aspect ratio as the SD interlaced modes. We just have to
     * be careful because we may get a combined standard with both
     * SMPTE293M and SD_XXXX set (devices with progressive+automatic
     * re-interlaced VCR output via the DENC).
     */
    if(tvStandard == STM_OUTPUT_STD_SMPTE293M)
    {
      if(pCurrentMode->ModeParams.ActiveAreaHeight < 576)
      {
	cap->pixelaspect.numerator   = 11; /* From V4L2 API Spec for NTSC(-J) */
	cap->pixelaspect.denominator = 10;
      }
      else
      {
	cap->pixelaspect.numerator   = 54; /* From V4L2 API Spec for PAL/SECAM */
	cap->pixelaspect.denominator = 59;
      }
    }
    else
    {
      switch(tvStandard & (STM_OUTPUT_STD_SD_MASK | STM_OUTPUT_STD_HD_MASK | STM_OUTPUT_STD_VESA))
      {
	/*
	 * 625line systems, assuming 4:3 picture aspect
	 */
	case STM_OUTPUT_STD_PAL_BDGHI:
	case STM_OUTPUT_STD_PAL_N:
	case STM_OUTPUT_STD_PAL_Nc:
	case STM_OUTPUT_STD_SECAM:
	  cap->pixelaspect.numerator   = 54; /* From V4L2 API Spec */
	  cap->pixelaspect.denominator = 59;
	  break;
	/*
	 * 525line systems, assuming 4:3 picture aspect
	 */
	case STM_OUTPUT_STD_NTSC_M:
	case STM_OUTPUT_STD_NTSC_443:
	case STM_OUTPUT_STD_NTSC_J:
	case STM_OUTPUT_STD_PAL_M:
	  cap->pixelaspect.numerator   = 11; /* From V4L2 API Spec */
	  cap->pixelaspect.denominator = 10;
	  break;
	default:
	  return -EIO;
      }
    }
  }

#else
  cap->pixelaspect.numerator   = pCurrentMode->ModeParams.PixelAspectRatios[0].numerator;
  cap->pixelaspect.denominator = pCurrentMode->ModeParams.PixelAspectRatios[0].denominator;
#endif

  return 0;
}


int enumerateBufferFormat(stvout_device_t *pDev, struct v4l2_fmtdesc *f)
{
  int num_formats;
  const SURF_FMT *formats;
  __u32 index,count;

  num_formats = stm_display_plane_get_image_formats(pDev->pPlane, &formats);
  if(signal_pending(current))
    //  {
    //    up(&pDev->devLock);
    return -ERESTARTSYS;
  //  }

  /*
   * Run through the surface formats available on the plane, enumerating
   * _only_ those that are supported by V4L2.
   */
  count = 0;
  for(index=0; index<num_formats; index++)
  {
    /* Is this a V4L2 supported format? */
    if(g_capfmt[formats[index]].fmt.pixelformat != 0)
      count++;

    /* Have we found the Nth format specified in the application request? */
    if(count == (f->index+1))
      break;
  }

  if (index >= num_formats)
    //  {
    //    up(&pDev->devLock);
    return -EINVAL;
  //  }

  /* Copy the generic format descriptor */
  *f = g_capfmt[formats[index]].fmt;
  /* Fix up the index so it is the same as the input */
  f->index = count-1;

  return 0;
}

/************************************************
 *  doBlitterCommand
 ************************************************/
int doBlitterCommand(stvout_device_t *pDev,STMFBIO_BLT_DATA *bltData)
{
  stm_blitter_surface_t *pSrcSurface = 0;
  stm_blitter_surface_t *pDstSurface = 0;

  if(!pDev->displayPipe.blitter)
    return -ENODEV;

  if(bltData->srcOffset != 0)
  {
    streaming_buffer_t *buf = mmapGetBufferFromOffset(pDev, bltData->srcOffset);
    if(buf != 0)
    {
      pSrcSurface = &buf->drawingSurface;
      bltData->srcOffset -= buf->vidBuf.m.offset;
      debug_msg("doBlitterCommand: pSrcSurface = %p size = %ld\n",pSrcSurface,pSrcSurface->ulSize);
    }
  }

  if(bltData->dstOffset != 0)
  {
    streaming_buffer_t *buf = mmapGetBufferFromOffset(pDev, bltData->dstOffset);
    if(buf != 0)
    {
      pDstSurface = &buf->drawingSurface;
      bltData->dstOffset -= buf->vidBuf.m.offset;
      debug_msg("doBlitterCommand: pDstSurface = %p size = %ld\n",pDstSurface,pDstSurface->ulSize);
    }
  }


  switch(bltData->operation)
  {
    case BLT_OP_DRAW_RECTANGLE:
    case BLT_OP_FILL:
    {
      if(!pDstSurface || !pDstSurface->ulMemory)
	return -EFAULT;

      if(stmfbio_draw_rectangle(pDev->displayPipe.blitter, pDstSurface, bltData) < 0)
	return -EINVAL;

      break;
    }
    case BLT_OP_COPY:
    {
      if(!pSrcSurface || !pSrcSurface->ulMemory || !pDstSurface || !pDstSurface->ulMemory)
	return -EFAULT;

      /*
       * Note that the CLUT is NULL as we do not support LUT8 surfaces in this
       * special interface.
       */
      if(stmfbio_do_blit(pDev->displayPipe.blitter, pSrcSurface, pDstSurface, 0, bltData) < 0)
	return -EINVAL;

      break;
    }
    default:
    {
      return -ENOIOCTLCMD;
    }
  }

  return 0;
}
