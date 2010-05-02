/***********************************************************************
 *
 * File: stgfb/Linux/media/video/stmvout_driver.c
 * Copyright (c) 2000, 2004, 2005 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * V4L2 video output device driver for ST SoC display subsystems.
 *
\***********************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/videodev.h>
#include <linux/interrupt.h>

#include <asm/page.h>
#include <asm/io.h>
#include <asm/page.h>
#include <stmdisplay.h>
#include <Linux/video/stmfb.h>
#include <linux/stm/stmcoredisplay.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include <stmdisplayplane.h>

#include <linux/stm/stm_v4l2.h>
#include "stmvout_driver.h"
#include "stmvout.h"

//static stm_display_device_t  *g_pDisplayDevice;
//static stm_display_blitter_t *g_pBlitter;
struct plane_map {
  stm_plane_id_t     plane_id;
  char              *plane_name;
};

static struct plane_map plane_mappings[] = {
  { OUTPUT_GDP1,   "RGB0"},
  { OUTPUT_GDP2,   "RGB1"},
  { OUTPUT_GDP3,   "RGB2"},
  { OUTPUT_GDP4,   "RGB3"},
  { OUTPUT_VID1,   "YUV0"},
  { OUTPUT_VID2,   "YUV1"},
//  { OUTPUT_CUR,   "CUR"}, // Should be enabled after testing
  { OUTPUT_VIRT1,  "RGB1.0" },
  { OUTPUT_VIRT2,  "RGB1.1" },
  { OUTPUT_VIRT3,  "RGB1.2" },
  { OUTPUT_VIRT4,  "RGB1.3" },
  { OUTPUT_VIRT5,  "RGB1.4" },
  { OUTPUT_VIRT6,  "RGB1.5" },
  { OUTPUT_VIRT7,  "RGB1.6" },
  { OUTPUT_VIRT8,  "RGB1.7" },
  { OUTPUT_VIRT9,  "RGB1.8" },
  { OUTPUT_VIRT10, "RGB1.9" },
  { OUTPUT_VIRT11, "RGB1.10" },
  { OUTPUT_VIRT12, "RGB1.11" },
  { OUTPUT_VIRT13, "RGB1.12" },
  { OUTPUT_VIRT14, "RGB1.13" },
  { OUTPUT_VIRT15, "RGB1.14" },
  { 0, NULL }
};

/*  OUTPUT Devices

    *  "YUV0" video, rear most video (YUV) plane
    * "MIXER0_PRIMARY" audio, primary audio mixer input
    * "YUV1" video, front most video (YUV) plane
    * "MIXER0_SECONDARY" audio, secondary audio mixer input
    * "RGB0", rear most RGB plane
    * "RGB1", middle RGB plane (this is exclusive to RGB1.?, ie. You can't open this device if any of RGB1.? is selected)
    * "RGB1.0", Virtual RGB Plane 0, exclusive with RGB1
    * "RGB1.1", Virtual RGB Plane 1, exclusive with RGB1
    * ...
    * "RGB1.15", Virtual RGB Plane 15, exclusive with RGB1
    * "RGB2", front most RGB plane (only displayed if the DirectFB primary surface is disabled)
    * "MIXER1_PRIMARY" audio, physically independent audio device used for second room audio output
    */
struct stvout_description {
  char name[32];

  //int audioId (when implemented the id assoicated with audio description)
  int  deviceId;
};

#define MAX_PLANES     8 // We will go for 8 planes per device

//< Describes the output surfaces to user
static struct stvout_description g_voutDevice[MAX_PLANES][STM_V4L2_MAX_DEVICES];
static int g_voutCount[STM_V4L2_MAX_DEVICES];

static stvout_device_t g_voutData[MAX_PLANES][STM_V4L2_MAX_DEVICES];

//struct semaphore        devLock;

/************************************************
 *  v4l2_close
 ************************************************/
static int v4l2_close(struct stm_v4l2_handles *handle, int type, struct file *file)
{
  stvout_device_t  *pDev      = handle->v4l2type[type].handle;
  struct semaphore *pLock     = &pDev->devLock;

  debug_msg("v4l2_close in.\n");

  if(!pDev) return 0;

  if(down_interruptible(pLock))
    return -ERESTARTSYS;

  if(pDev->isStreaming)
      if (streamoff(pDev) < 0)
      {
	      up(pLock);
	      return -ERESTARTSYS;
      }

  if(pDev->pPlane)
  {
    debug_msg("v4l2_close: releasing hardware resources\n");

    if(stm_display_plane_release(pDev->pPlane)<0)
    {
      up(pLock);
      return -ERESTARTSYS;
    }

    pDev->pPlane = NULL;
  }

  if (pDev->bufferQueuesInit)
  {
    deinitBuffers(pDev);
    pDev->bufferQueuesInit = 0;
  }

  pDev->inUse      = 0;

  up(pLock);

  debug_msg("v4l2_close out.\n");

  return 0;
}


/************************************************
 *  v4l2_ioctl
 ************************************************/
static int v4l2_do_ioctl(struct stm_v4l2_handles *handle, struct stm_v4l2_driver *driver, int device,int type, struct file  *file,unsigned int  cmd, void *arg)
{
  int ret;
  stvout_device_t  *pDev      = NULL;
  struct semaphore *pLock     = NULL;

  if (handle) pDev = handle->v4l2type[type].handle;

  //debug_msg("%x %x(%x) %x %x %x\n",type,pDev,file,cmd,arg);

  if (pDev) pLock = &pDev->devLock;
  if (pLock)
	  if(down_interruptible(pLock))
		  return -ERESTARTSYS;

  if(!pDev)
    switch (cmd)
    {
    case VIDIOC_ENUMOUTPUT:
    case VIDIOC_S_OUTPUT:
	    break;

    default:
      if (pLock) up(pLock);
      return -ENODEV;
    };


  switch(cmd)
  {
    case VIDIOC_ENUM_FMT:
    {
      struct v4l2_fmtdesc *f = arg;

      switch(f->type)
	{
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	  if ((ret = enumerateBufferFormat(pDev, f)) < 0)
	    goto err_ret;

	  break;

	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	  if ((ret = enumerateBufferFormat(pDev, f)) < 0)
	    goto err_ret;

	  break;

	default:
	  goto err_inval;
	}


      break;
    }

    case VIDIOC_G_FMT:
    {
      struct v4l2_format* fmt = arg;

      debug_msg("v4l2_ioctl - VIDIOC_G_FMT\n");

      switch (fmt->type)
      {
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	{
	  fmt->fmt.pix = pDev->bufferFormat.fmt.pix;
	  break;
	}
	default:
	{
	  err_msg("G_FMT invalid buffer type %d\n",fmt->type);
	  goto err_inval;
	}
      }

      break;
    }

    case VIDIOC_S_FMT:
    {
      struct v4l2_format* fmt = arg;

      debug_msg("v4l2_ioctl - VIDIOC_S_FMT\n");

      switch (fmt->type)
      {
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	{
	  /* Check ioctl is legal */
	  //.PB. not sure what open for io means?? if (!pOpenData->isOpenForIO)
	  //{
	  //  err_msg("S_FMT: changing IO format not available on this file descriptor\n");
	  //  goto err_busy;
	  //}

	  if (pDev->isStreaming)
	  {
	    err_msg("S_FMT: device is streaming data, cannot change format\n");
	    goto err_busy;
	  }

	  if ((ret = setBufferFormat(pDev, fmt, 1)) < 0)
	    goto err_ret;

	  break;
	}
	default:
	{
	  err_msg("S_FMT: invalid buffer type %d\n",fmt->type);
	  goto err_inval;
	}
      }

      break;
    }

    case VIDIOC_TRY_FMT:
    {
      struct v4l2_format* fmt = arg;

      debug_msg("v4l2_ioctl - VIDIOC_TRY_FMT\n");

      switch (fmt->type)
      {
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	{
	  if ((ret = setBufferFormat(pDev, fmt, 0)) < 0)
	    goto err_ret;

	  break;
	}
	default:
	{
	  err_msg("TRY_FMT: invalid buffer type %d\n",fmt->type);
	  goto err_inval;
	}
      }

      break;
    }

    case VIDIOC_CROPCAP:
    {
      struct v4l2_cropcap* cropcap = arg;

      debug_msg("v4l2_ioctl - VIDIOC_CROPCAP\n");

      switch(cropcap->type)
      {
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	  if(getDisplayDefaults(pDev,cropcap) < 0)
	    goto err_inval;

	  break;

	default:
	  debug_msg("v4l2_ioctl - VIDIOC_CROPCAP: invalid type\n");
	  goto err_inval;
      };

      break;
    }

    case VIDIOC_G_CROP:
    {
      struct v4l2_crop* crop = arg;
      debug_msg("v4l2_ioctl - VIDIOC_G_CROP\n");

      switch (crop->type)
      {
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	{
	  crop->c = pDev->outputCrop.c;
	  break;
	}
	case V4L2_BUF_TYPE_PRIVATE:
	{
	  crop->c = pDev->srcCrop.c;
	  break;
	}
	default:
	{
	  err_msg("G_CROP: invalid buffer type %d\n",crop->type);
	  goto err_inval;
	}
      }

      break;
    }

    case VIDIOC_S_CROP:
    {
      struct v4l2_crop* crop = arg;

      debug_msg("v4l2_ioctl - VIDIOC_S_CROP\n");

      switch (crop->type)
      {
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	{
	  if((ret = setOutputCrop(pDev,crop)) < 0)
	    goto err_ret;

	  break;
	}
	case V4L2_BUF_TYPE_PRIVATE:
	{
	  /*
	   * This is a private interface for specifying the crop of the
	   * source buffer to be displayed, as opposed to the crop on the
	   * screen. V4L2 has no way of doing this but we need to be able to
	   * to accomodate MPEG decode where:
	   *
	   * 1. the actual image size is only known once you start decoding
	   *    (generally too late to set the buffer format and allocate display buffers)
	   * 2. the buffer dimensions may be larger than the image size due to
	   *    HW restrictions (e.g. Macroblock formats, 720 pixel wide video
	   *    must be in a 736 pixel wide buffer to have an even number of macroblocks)
	   * 3. the image size can change dynamically in the stream.
	   * 4. we need to support pan/scan and center cutout with widescreen->
	   *    4:3 scaling
	   * 5. to support DVD zoom modes
	   *
	   */
	  if((ret = setBufferCrop(pDev,crop)) < 0)
	    goto err_ret;

	  break;
	}
	default:
	{
	  err_msg("S_CROP: invalid buffer type %d\n",crop->type);
	  goto err_inval;
	}
      }

      break;
    }

    case VIDIOC_G_PARM:
    {
      struct v4l2_streamparm* sp = arg;

      debug_msg("v4l2_ioctl - VIDIOC_G_PARM\n");

      /*
       * The output streaming params are only relevent when using write() not
       * when using real streaming buffers. As we do not support the write()
       * interface this is always zeroed.
       */

      switch (sp->type)
      {
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	  memset(&sp->parm.output,0,sizeof(struct v4l2_outputparm));
	  break;

	default:
	  if (sp->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
	    goto err_inval;
      };

      break;
    }

    case VIDIOC_S_PARM:
    {
      debug_msg("v4l2_ioctl - VIDIOC_S_PARM\n");
      goto err_inval;
    }

    case VIDIOC_REQBUFS:
    {
      struct v4l2_requestbuffers *req = arg;

      debug_msg("v4l2_ioctl - VIDIOC_REQBUFS\n");

      //if (!pOpenData->isOpenForIO)
      //{
      //  err_msg("REQBUFS: illegal in non-io open\n");
      //        goto err_busy;
      //}

      //.PB. if (req->type != V4L2_BUF_TYPE_VIDEO_OUTPUT || req->memory != V4L2_MEMORY_MMAP)
      if (req->type != V4L2_BUF_TYPE_VIDEO_OUTPUT || !(req->memory == V4L2_MEMORY_MMAP || req->memory == V4L2_MEMORY_USERPTR))
      {
	err_msg("REQBUFS: unsupported type or memory parameters\n");
	goto err_inval;
      }

      if(pDev->bufferFormat.type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
      {
	err_msg("REQBUFS: pixel format not set, cannot allocate buffers\n");
	goto err_inval;
      }

      if (!mmapDeleteBuffers(pDev))
      {
	err_msg("REQBUFS: video buffer(s) still mapped, cannot change\n");
	goto err_busy;
      }

      if (req->memory == V4L2_MEMORY_MMAP)
      if (!mmapAllocateBuffers(pDev, req))
      {
	err_msg("REQBUFS: unable to allocate video buffers\n");
	goto err_inval;
      }

      break;
    }

    case VIDIOC_QUERYBUF:
    {
      struct v4l2_buffer* pBuf = arg;
      int i;

      debug_msg("v4l2_ioctl - VIDIOC_QUERYBUF index = %d\n", pBuf->index);

      switch(pBuf->type)
      {
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	  break;

	default:
	  err_msg("QUERYBUF: illegal stream type\n");
	  goto err_inval;
      };

      //      if (!pOpenData->isOpenForIO)
      //      {
      //        err_msg("QUERYBUF: IO not available on this file descriptor\n");
      //        goto err_busy;
      //      }


      i = pBuf->index;

      if (i < 0 || i >= MAX_STREAM_BUFFERS || !pDev->streamBuffers[i].isAllocated)
      {
	err_msg("QUERYBUF: bad parameter i = %d isAllocated = %d\n",i,pDev->streamBuffers[i].isAllocated);
	goto err_inval;
      }

      *pBuf = pDev->streamBuffers[i].vidBuf;

      break;
    }

    case VIDIOC_QBUF:
    {
      struct v4l2_buffer* pBuf = arg;

      if (pBuf->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
      {
	err_msg("QBUF: illegal stream type\n");
	goto err_inval;
      }

      //.PB. if (pBuf->memory != V4L2_MEMORY_MMAP)
      if (pBuf->memory != V4L2_MEMORY_MMAP && pBuf->memory != V4L2_MEMORY_USERPTR )
      {
	err_msg("QBUF: illegal memory type, must be mmap\n");
	goto err_inval;
      }

      if(pDev->outputCrop.type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
      {
	err_msg("QBUF: no valid output crop set, cannot queue buffer\n");
	goto err_inval;
      }

      //      if (!pOpenData->isOpenForIO)
      //      {
      //        err_msg("QBUF: IO not available on this file descriptor\n");
      //        goto err_busy;
      //      }

      if ((ret = queueBuffer(pDev, pBuf)) < 0)
	goto err_ret;

      break;
    }

    case VIDIOC_DQBUF:
    {
      struct v4l2_buffer* pBuf = arg;

      if (pBuf->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
      {
	err_msg("DQBUF: illegal stream type\n");
	goto err_inval;
      }

      /*
       * The API documentation states that the memory field must be set correctly
       * by the application for the call to succeed. It isn't clear under what
       * circumstances this would ever be needed and the requirement
       * seems a bit over zealous, but we honour it anyway. I bet users will
       * miss the small print and log lots of bugs that this call doesn't work.
       */
      //.PB. if (pBuf->memory != V4L2_MEMORY_MMAP)
      if (pBuf->memory != V4L2_MEMORY_MMAP && pBuf->memory != V4L2_MEMORY_USERPTR)
      {
	err_msg("DQBUF: illegal memory type, must be mmap\n");
	goto err_inval;
      }

      //      if (!pOpenData->isOpenForIO)
      //      {
      //        err_msg("DQBUF: IO not available on this file descriptor\n");
      //        goto err_busy;
      //      }

      /*
       * We can release the driver lock now as dequeueBuffer is safe
       * (the buffer queues have their own access locks), this makes
       * the logic a bit simpler particularly in the blocking case.
       */
      up(pLock);

      if((file->f_flags & O_NONBLOCK) == O_NONBLOCK)
      {
	debug_msg("DQBUF: Non-Blocking dequeue\n");
	if (!dequeueBuffer(pDev, pBuf))
	  return -EAGAIN;

	return 0;
      }
      else
      {
	debug_msg("DQBUF: Blocking dequeue\n");
	return wait_event_interruptible(pDev->wqBufDQueue, dequeueBuffer(pDev, pBuf));
      }

      break;
    }

    case VIDIOC_STREAMON:
    {
      __u32*  type = arg;

      debug_msg("v4l2_ioctl - VIDIOC_STREAMON\n");

      if(*type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
      {
	err_msg("STREAMON: illegal stream type\n");
	goto err_inval;
      }

      if (pDev->isStreaming)
      {
	err_msg("STREAMON: device already streaming\n");
	goto err_busy;
      }

#if defined(_STRICT_V4L2_API)
      if(!hasQueuedBuffers(pDev))
      {
	/*
	 * The API states that at least one buffer must be queued before
	 * STREAMON succeeds.
	 */
	err_msg("STREAMON: no buffers queued on stream\n");
	goto err_inval;
      }
#endif

      debug_msg("STREAMON: Attaching plane to output\n");
      if(stm_display_plane_lock(pDev->pPlane)<0)
      {
	if(signal_pending(current))
	  return -ERESTARTSYS;
	else
	  return -EBUSY;
      }

      if(stm_display_plane_connect_to_output(pDev->pPlane, pDev->pOutput)<0)
      {
	stm_display_plane_unlock(pDev->pPlane);

	if(signal_pending(current))
	  return -ERESTARTSYS;
	else
	  return -EIO;
      }

      debug_msg("STREAMON: Starting the stream\n");

      pDev->isStreaming = 1;
      wake_up_interruptible(&(pDev->wqStreamingState));

      /*
       * Make sure any frames already on the pending queue are
       * written to the hardware
       */
      writeNextFrame(pDev);

      break;
    }

    case VIDIOC_STREAMOFF:
    {
      __u32*  type = arg;

      debug_msg("v4l2_ioctl - VIDIOC_STREAMOFF\n");

      if(*type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
      {
	err_msg("STREAMOFF: illegal stream type\n");
	goto err_inval;
      }

      if((ret = streamoff(pDev))<0)
	goto err_ret;

      break;
    }

    case VIDIOC_G_OUTPUT:
    {
      int *output = (int *)arg;

      debug_msg("v4l2_ioctl - VIDIOC_G_OUTPUT\n");
      *output = pDev->currentOutputNum;

      break;
    }

    case VIDIOC_S_OUTPUT:
    {
      int output = *(int *)arg;

      if (output < 0 || output >= g_voutCount[device])
      {
	err_msg("S_OUTPUT: Output number out of range %d\n", output);

	goto err_inval;
      }

      if (pDev)
      {
	err_msg("S_OUTPUT: Handle already associated to an ouput\n");
	goto err_busy;
      }

      pDev = &g_voutData[output][device];//g_voutDevice[output][device].deviceId];

      if (pDev->inUse)
      {
	 err_msg("S_OUTPUT: Output already in use %d\n", output);

	goto err_busy;
      }

#if 0
      if (pDev->displayPipe.device != device) {
	err_msg("S_OUTPUT: BUG ON the output device's do not match\n");

	goto err_busy;
      }
#endif


      if (!pDev->bufferQueuesInit)
      {
	initBuffers(pDev);
	pDev->bufferQueuesInit = 1;
      }

      if (!pDev->pPlane)
      {
	 pDev->pPlane = stm_display_get_plane(pDev->displayPipe.device, pDev->planeConfig->id);
	 if (!pDev->pPlane)
	 {
	    if(signal_pending(current))
	      goto err_restartsys;
	    else
	      goto err_busy;
	 }
      }

      pDev->inUse = 1;
      handle->v4l2type[type].handle = pDev;

      debug_msg("Setting handle %x ....\n",pDev);

      break;
    }

    case VIDIOC_ENUMOUTPUT:
    {
      struct v4l2_output* output = arg;
      struct v4l2_standard std;

      debug_msg("v4l2_ioctl - VIDIOC_ENUMOUTPUT\n");

      if (output->index < (0 + driver->start_index[device]) || output->index >= (g_voutCount[device] + driver->start_index[device]))
	goto err_inval;

      sprintf(output->name, "%s",g_voutDevice[output->index - driver->start_index[device]][device].name);
      output->type      = V4L2_OUTPUT_TYPE_ANALOG;
      output->audioset  = 0; // output->audioId
      output->modulator = 0;

      // what to do about standards??????????
      if(!pDev)
	output->std = 0;
      else
	if((ret = getDisplayStandard(pDev,&std)) < 0)
	  goto err_ret;
	else output->std = std.id;

      break;
    }

    case VIDIOC_G_STD:
    {
      v4l2_std_id* std_id = arg;
      struct v4l2_standard std;

      debug_msg("v4l2_ioctl - VIDIOC_G_STD\n");

      if((ret = getDisplayStandard(pDev,&std)) < 0)
	goto err_ret;

      *std_id = std.id;

      break;
    }

    case VIDIOC_S_STD:
    {
      v4l2_std_id* std_id = arg;
      struct v4l2_standard std;

      debug_msg("v4l2_ioctl - VIDIOC_S_STD\n");

      if((ret = getDisplayStandard(pDev,&std)) < 0)
	goto err_ret;

      /*
       * At the moment we do not support changing the current standard,
       * so only succeed if the user is trying to set the standard that
       * is already active. As we have the situation where the hardware covers
       * several variants of PAL with one mode, also allow the individual
       * setting of a substandard if that is covered by the current mode.
       */
      if((std.id & *std_id) != *std_id)
	goto err_inval;

      break;
    }

    case VIDIOC_ENUMSTD:
    {
      struct v4l2_standard*  std = arg;

      debug_msg("v4l2_ioctl - VIDIOC_ENUMSTD\n");

      /*
       * We only enumerate the standard currently selected by the framebuffer
       * setup.
       */
      if (std->index != 0)
	goto err_inval;

      if((ret = getDisplayStandard(pDev,std)) < 0)
	goto err_ret;

      break;
    }

    case VIDIOC_QUERYCTRL:
    {
      struct v4l2_queryctrl* pqc = arg;
      unsigned long ctrlCaps;

      debug_msg("v4l2_ioctl - VIDIOC_QUERYCTRL\n");

      if(stm_display_output_get_control_capabilities(pDev->pOutput, &ctrlCaps)<0)
	goto err_restartsys;

      switch(pqc->id)
      {
	case V4L2_CID_BRIGHTNESS:
	  strcpy(pqc->name,"Brightness");
	  pqc->minimum = 0;
	  pqc->maximum = 255;
	  pqc->step = 1;
	  pqc->default_value = 128;
	  pqc->type = V4L2_CTRL_TYPE_INTEGER;
	  pqc->flags = (ctrlCaps & STM_CTRL_CAPS_BRIGHTNESS)?0:V4L2_CTRL_FLAG_DISABLED;
	  break;

	case V4L2_CID_CONTRAST:
	  strcpy(pqc->name,"Contrast");
	  pqc->minimum = 0;
	  pqc->maximum = 255;
	  pqc->step = 1;
	  pqc->default_value = 0;
	  pqc->type = V4L2_CTRL_TYPE_INTEGER;
	  pqc->flags = (ctrlCaps & STM_CTRL_CAPS_CONTRAST)?0:V4L2_CTRL_FLAG_DISABLED;
	  break;

	case V4L2_CID_SATURATION:
	  strcpy(pqc->name,"Saturation");
	  pqc->minimum = 0;
	  pqc->maximum = 255;
	  pqc->step = 1;
	  pqc->default_value = 128;
	  pqc->type = V4L2_CTRL_TYPE_INTEGER;
	  pqc->flags = (ctrlCaps & STM_CTRL_CAPS_SATURATION)?0:V4L2_CTRL_FLAG_DISABLED;
	  break;

	case V4L2_CID_HUE:
	  strcpy(pqc->name,"Hue");
	  pqc->minimum = 0;
	  pqc->maximum = 255;
	  pqc->step = 1;
	  pqc->default_value = 128;
	  pqc->type = V4L2_CTRL_TYPE_INTEGER;
	  pqc->flags = (ctrlCaps & STM_CTRL_CAPS_HUE)?0:V4L2_CTRL_FLAG_DISABLED;
	  break;

      case V4L2_CID_STM_Z_ORDER_VID0:
      case V4L2_CID_STM_Z_ORDER_VID1:
      case V4L2_CID_STM_Z_ORDER_RGB0:
      case V4L2_CID_STM_Z_ORDER_RGB1:
      case V4L2_CID_STM_Z_ORDER_RGB2:
	  sprintf(pqc->name,"Z Order for plane %s\n",g_voutDevice[pqc->id - V4L2_CID_STM_Z_ORDER_VID0][device].name);
	  pqc->minimum       = 0;
	  pqc->maximum       = 4;
	  pqc->step          = 1;
	  pqc->default_value = ((pqc->id >= V4L2_CID_STM_Z_ORDER_VID0) && (pqc->id <= V4L2_CID_STM_Z_ORDER_RGB1)) ? (pqc->id -V4L2_CID_STM_Z_ORDER_VID0) : 4;
	  pqc->type          = V4L2_CTRL_TYPE_INTEGER;
	  pqc->flags         = 0;
	  break;

	case V4L2_CID_STM_IQI_SET_CONFIG:
	case V4L2_CID_STM_IQI_DEMO:
	  {
	  stm_plane_caps_t planeCaps;
	  int              iqi_supported;
	  if (stm_display_plane_get_capabilities (pDev->pPlane,
						  &planeCaps) < 0)

	    goto err_restartsys;
	  iqi_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_IQI;

	  switch (pqc->id)
	    {
	    case V4L2_CID_STM_IQI_SET_CONFIG:
	      strcpy(pqc->name,"IQI CONFIG");
	      pqc->minimum = PCIQIC_FIRST;
	      pqc->maximum = PCIQIC_COUNT - 1;
	      pqc->step    = 1;
	      pqc->default_value = PCIQIC_BYPASS;
	      pqc->type = V4L2_CTRL_TYPE_MENU;
	      pqc->flags = iqi_supported ? 0 : V4L2_CTRL_FLAG_DISABLED;
	      break;

	    case V4L2_CID_STM_IQI_DEMO:
	      strcpy(pqc->name,"IQI Demo");
	      pqc->minimum = 0; /* off */
	      pqc->maximum = 1; /* on */
	      pqc->step    = 1;
	      pqc->default_value = 0;
	      pqc->type = V4L2_CTRL_TYPE_BOOLEAN;
	      pqc->flags = iqi_supported ? 0 : V4L2_CTRL_FLAG_DISABLED;
	      break;

	    default:
	      /* shouldn't be reached */
	      BUG ();
	      goto err_inval;
	    }
	  }
	  break;

	case V4L2_CID_STM_XVP_SET_CONFIG:
	  {
	  stm_plane_caps_t planeCaps;
	  int              xvp_supported;
	  if (stm_display_plane_get_capabilities (pDev->pPlane,
						  &planeCaps) < 0)

	    goto err_restartsys;
	  xvp_supported = planeCaps.ulControls & (PLANE_CTRL_CAPS_FILMGRAIN
						  | PLANE_CTRL_CAPS_TNR);

	  strcpy(pqc->name,"xVP CONFIG");
	  pqc->minimum = PCxVPC_FIRST;
	  pqc->maximum = PCxVPC_COUNT - 1;
	  pqc->step    = 1;
	  pqc->default_value = PCxVPC_BYPASS;
	  pqc->type = V4L2_CTRL_TYPE_MENU;
	  pqc->flags = xvp_supported ? 0 : V4L2_CTRL_FLAG_DISABLED;
	  }
	  break;

	case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE:
	  {
	  stm_plane_caps_t planeCaps;
	  int              tnr_supported;
	  if (stm_display_plane_get_capabilities (pDev->pPlane,
						  &planeCaps) < 0)

	    goto err_restartsys;
	  tnr_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_TNR;

	  strcpy(pqc->name,"NLE override for TNR");
	  pqc->minimum = 0;
	  pqc->maximum = 255;
	  pqc->step    = 1;
	  pqc->default_value = 0;
	  pqc->type = V4L2_CTRL_TYPE_INTEGER;
	  pqc->flags = tnr_supported ? 0 : V4L2_CTRL_FLAG_DISABLED;
	  }
	  break;


	default:
	  goto err_inval;
      }

      break;
    }

    case VIDIOC_QUERYMENU:
    {
      struct v4l2_querymenu* pqm = arg;
      switch(pqm->id)
      {
	case V4L2_CID_STM_IQI_SET_CONFIG:
	  if (pqm->index > PCIQIC_COUNT)
	    goto err_inval;

	  switch ((enum PlaneCtrlIQIConfiguration) pqm->index)
	    {
	    case PCIQIC_BYPASS:
	      strcpy (pqm->name,"bypass");
	      break;

	    case PCIQIC_ST_DEFAULT:
	      strcpy (pqm->name,"default");
	      break;

	    case PCIQIC_COUNT:
	      goto err_inval;
	    }
	  break;

	case V4L2_CID_STM_XVP_SET_CONFIG:
	  if (pqm->index > PCxVPC_COUNT)
	    goto err_inval;

	  switch ((enum PlaneCtrlxVPConfiguration) pqm->index)
	    {
	    case PCxVPC_BYPASS:
	      strcpy (pqm->name,"bypass");
	      break;

	    case PCxVPC_FILMGRAIN:
	      strcpy (pqm->name,"filmgrain");
	      break;

	    case PCxVPC_TNR:
	      strcpy (pqm->name,"tnr");
	      break;

	    case PCxVPC_TNR_BYPASS:
	      strcpy (pqm->name,"tnr (bypass)");
	      break;

	    case PCxVPC_TNR_NLEOVER:
	      strcpy (pqm->name,"tnr (NLE override)");
	      break;

	     case PCxVPC_COUNT:
	      goto err_inval;
	    }
	  break;

	}

      break;
    }

    case VIDIOC_S_CTRL:
    {
      struct v4l2_control* pctrl        = arg;
      ULONG ctrlname;

      debug_msg("v4l2_ioctl - VIDIOC_S_CTRL\n");

      /*
       * This is the defined API behaviour for value out of range, instead of
       * the more tradition -EINVAL
       */
      if ((pctrl->value < 0) || (pctrl->value > STM_V4L2_MAX_CONTROLS))
	goto err_range;

      ctrlname = getCtrlName(pctrl->id);
      if(ctrlname == 0)
	goto err_inval;

      switch (pctrl->id)
	{
	case V4L2_CID_STM_IQI_SET_CONFIG:
	case V4L2_CID_STM_IQI_DEMO:
	case V4L2_CID_STM_XVP_SET_CONFIG:
	case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE:
	  if (stm_display_plane_set_control(pDev->pPlane,
					    ctrlname, pctrl->value) < 0)
	    goto err_restartsys;
	  break;

	default:
	  if (pctrl->id < V4L2_CID_STM_Z_ORDER_VID0) {
	    if(stm_display_output_set_control(pDev->pOutput, ctrlname, pctrl->value) < 0)
	      goto err_restartsys;
	    } else {
	      stvout_device_t *pdev = &g_voutData[pctrl->id - V4L2_CID_STM_Z_ORDER_VID0][device];

	      if (!pdev || !pdev->inUse)
		goto err_inval;

	      if(stm_display_plane_set_depth(pdev->pPlane,
					     pdev->pOutput,
					     pctrl->value,1) < 0)
		goto err_restartsys;
	    }
	    break;
	}

    break;
    }

    case VIDIOC_G_CTRL:
    {
      struct v4l2_control* pctrl = arg;
      ULONG ctrlname;
      ULONG val;

      debug_msg("v4l2_ioctl - VIDIOC_G_CTRL\n");

      ctrlname = getCtrlName(pctrl->id);
      if(ctrlname == 0)
	goto err_inval;

      switch (pctrl->id)
	{
	case V4L2_CID_STM_IQI_SET_CONFIG:
	case V4L2_CID_STM_IQI_DEMO:
	case V4L2_CID_STM_XVP_SET_CONFIG:
	case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE:
	  if (stm_display_plane_get_control(pDev->pPlane,
					    ctrlname, &val) < 0)
	    goto err_restartsys;
	  break;

	default:
	  if (pctrl->id < V4L2_CID_STM_Z_ORDER_VID0) {
	    if(stm_display_output_get_control(pDev->pOutput, ctrlname, &val) < 0)
	      goto err_restartsys;
	    } else {
	      stvout_device_t *pdev = &g_voutData[pctrl->id - V4L2_CID_STM_Z_ORDER_VID0][device];

	      if(stm_display_plane_get_depth(pdev->pPlane,pdev->pOutput, (int *)&val) < 0)
		goto err_restartsys;
	    }
	    break;
	}

      pctrl->value = val;
      break;
    }

    case VIDIOC_S_OVERLAY_ALPHA:
    {
      unsigned int alpha = *(unsigned int*)arg;

      if (alpha > 255)
	goto err_inval;

      pDev->globalAlpha = alpha;
      break;
    }

    /*
     * All of the following are not supported and there need to return -EINVAL
     * as per the V4L2 API spec.
     */
    case VIDIOC_ENUMAUDIO:
    case VIDIOC_ENUMAUDOUT:
    case VIDIOC_G_AUDIO:
    case VIDIOC_S_AUDIO:
    case VIDIOC_G_AUDOUT:
    case VIDIOC_S_AUDOUT:
#ifdef VIDIOC_G_COMP
    /*
     * These are currently ifdef'd out in the kernel headers, being deprecated?
     */
    case VIDIOC_G_COMP:
    case VIDIOC_S_COMP:
#endif
    case VIDIOC_G_FBUF:
    case VIDIOC_G_FREQUENCY:
    case VIDIOC_S_FREQUENCY:
    case VIDIOC_G_INPUT:
//    case VIDIOC_S_INPUT:
    case VIDIOC_G_JPEGCOMP:
    case VIDIOC_S_JPEGCOMP:
    case VIDIOC_G_MODULATOR:
    case VIDIOC_S_MODULATOR:
    case VIDIOC_G_PRIORITY:
    case VIDIOC_S_PRIORITY:
    case VIDIOC_G_TUNER:
    case VIDIOC_S_TUNER:
	  goto err_inval;

    case STMFBIO_BLT:
    {

      debug_msg("v4l2_ioctl - STMFBIO_BLT\n");

      ret = doBlitterCommand(pDev, (STMFBIO_BLT_DATA *)arg);

      if(ret < 0)
      {
	if(signal_pending(current))
	  goto err_restartsys;
	else
	  goto err_inval;
      }

      break;
    }

    case STMFBIO_SYNC_BLITTER:
    {

      debug_msg("v4l2_ioctl - STMFBIO_STNC_BLITTER\n");


      if(!pDev->displayPipe.blitter)
	      goto err_nodev;

      if((ret = stm_display_blitter_sync(pDev->displayPipe.blitter)) != 0)
      {
	if(signal_pending(current))
	  goto err_restartsys;
	else
	  goto err_ret;
      }

      break;
    }

    default:
	    if (pLock) up(pLock);
      return -ENOIOCTLCMD;
  }

  if (pLock) up(pLock);
  return 0;

 err_restartsys:
  if (pLock) up(pLock);
  return -ERESTARTSYS;

 err_nodev:
  if (pLock) up(pLock);
  return -ENODEV;

 err_busy:
  if (pLock) up(pLock);
  return -ENODEV;

 err_inval:
  if (pLock) up(pLock);
  return -EINVAL;

 err_range:
  if (pLock) up(pLock);
  return -ERANGE;

 err_ret:
  if (pLock) up(pLock);
  return ret;
}


/**********************************************************
 * mmap helper functions
 **********************************************************/
static void
stmvout_vm_open(struct vm_area_struct *vma)
{
  streaming_buffer_t *pBuf = vma->vm_private_data;

  debug_msg("vm_open %p [count=%d,vma=%08lx-%08lx]\n",pBuf,
	     pBuf->mapCount,vma->vm_start,vma->vm_end);

  pBuf->mapCount++;
}

static void
stmvout_vm_close(struct vm_area_struct *vma)
{
  streaming_buffer_t *pBuf = vma->vm_private_data;

  debug_msg("vm_close %p [count=%d,vma=%08lx-%08lx]\n",pBuf,
	     pBuf->mapCount,vma->vm_start,vma->vm_end);

  pBuf->mapCount--;
  if(pBuf->mapCount == 0)
  {
    debug_msg("vm_close %p clearing mapped flag\n",pBuf);
    pBuf->vidBuf.flags &= ~V4L2_BUF_FLAG_MAPPED;
  }

  return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
static struct page*
stmvout_vm_nopage(struct vm_area_struct *vma, unsigned long vaddr, int *type)
{
  struct page *page;
  void *page_addr;
  unsigned long page_frame;

  debug_msg("nopage: fault @ %08lx [vma %08lx-%08lx]\n",
	    vaddr,vma->vm_start,vma->vm_end);

  if (vaddr > vma->vm_end)
    return NOPAGE_SIGBUS;

  /*
   * Note that this assumes an identity mapping between the page offset and
   * the pfn of the physical address to be mapped. This will get more complex
   * when the 32bit SH4 address space becomes available.
   */
  page_addr = (void*)((vaddr - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT));

  page_frame = ((unsigned long)page_addr >> PAGE_SHIFT);

  if(!pfn_valid(page_frame))
    return NOPAGE_SIGBUS;

  page = virt_to_page(__va(page_addr));

  get_page(page);

  if (type)
    *type = VM_FAULT_MINOR;
  return page;
}
#else /* >= 2.6.24 */
static int stmvout_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
  struct page *page;
  void *page_addr;
  unsigned long page_frame;

  debug_msg("nopage: fault @ %08lx [vma %08lx-%08lx]\n",
	    (unsigned long) vmf->virtual_address,vma->vm_start,vma->vm_end);

  if ((unsigned long) vmf->virtual_address > vma->vm_end)
    return VM_FAULT_SIGBUS;

  /*
   * Note that this assumes an identity mapping between the page offset and
   * the pfn of the physical address to be mapped. This will get more complex
   * when the 32bit SH4 address space becomes available.
   */
  page_addr = (void*)(((unsigned long) vmf->virtual_address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT));

  page_frame  = ((unsigned long)page_addr >> PAGE_SHIFT);

  if(!pfn_valid(page_frame))
    return VM_FAULT_SIGBUS;

  page = virt_to_page(__va(page_addr));
  get_page(page);
  vmf->page = page;
  
  return 0;
}
#endif /* >= 2.6.24 */

static struct vm_operations_struct stmvout_vm_ops_memory =
{
  .open     = stmvout_vm_open,
  .close    = stmvout_vm_close,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
  .nopage   = stmvout_vm_nopage,
#else
  .fault    = stmvout_vm_fault,
#endif
};

static struct vm_operations_struct stmvout_vm_ops_ioaddr =
{
  .open     = stmvout_vm_open,
  .close    = stmvout_vm_close,
};

/************************************************
 *  v4l2_mmap
 ************************************************/
static int  v4l2_mmap(struct stm_v4l2_handles *handle, int type, struct file *file, struct vm_area_struct*  vma)
{
  //  open_data_t        *pOpenData = (open_data_t *)file->private_data;
  stvout_device_t    *pDev      = (stvout_device_t *)handle->v4l2type[type].handle;
  struct semaphore *pLock;
  streaming_buffer_t *pBuf;

  if (!pDev)
	  return -EINVAL;

  pLock = &pDev->devLock;

  debug_msg("%s in.\n", __FUNCTION__);

  if (!(vma->vm_flags & VM_WRITE))
  {
    debug_msg("%s bug: PROT_WRITE please\n", __FUNCTION__);
    return -EINVAL;
  }

  if (!(vma->vm_flags & VM_SHARED))
  {
    debug_msg("%s bug: MAP_SHARED please\n", __FUNCTION__);
    return -EINVAL;
  }

  if(down_interruptible(pLock))
    return -ERESTARTSYS;

  //  if (!pOpenData->isOpenForIO)
  //  {
  //    err_msg("%s another open file descriptor exists and is doing IO\n", __FUNCTION__);
  //    up(pLock);
  //    return -EBUSY;
  //  }

  debug_msg("%s offset = 0x%08X\n", __FUNCTION__, (int)vma->vm_pgoff);
  pBuf = mmapGetBufferFromOffset(pDev, vma->vm_pgoff*PAGE_SIZE);

  if (pBuf == NULL)
  {
    /* no such buffer */
    err_msg("%s Invalid offset parameter\n", __FUNCTION__);
    up(pLock);
    return -EINVAL;
  }

  debug_msg("%s pBuf = 0x%08lx\n", __FUNCTION__, (unsigned long)pBuf);

  if (pBuf->vidBuf.length != (vma->vm_end - vma->vm_start))
  {
    /* wrong length */
    err_msg("%s Wrong length parameter\n", __FUNCTION__);
    up(pLock);
    return -EINVAL;
  }

  if (!pBuf->isAllocated)
  {
    /* not requested */
    err_msg("%s Buffer is not available for mapping\n", __FUNCTION__);
    up(pLock);
    return -EINVAL;
  }

  if (pBuf->mapCount > 0)
  {
    err_msg("%s Buffer is already mapped\n", __FUNCTION__);
    up(pLock);
    return -EBUSY;
  }

  debug_msg("%s offset = 0x%08X\n", __FUNCTION__, (int)vma->vm_pgoff);
  pBuf = mmapGetBufferFromOffset(pDev, vma->vm_pgoff*PAGE_SIZE);

  if (pBuf == NULL)
  {
    /* no such buffer */
    err_msg("%s Invalid offset parameter\n", __FUNCTION__);
    up(pLock);
    return -EINVAL;
  }

  debug_msg("v4l2_mmap pBuf = 0x%08lx\n", (unsigned long)pBuf);

  if (pBuf->vidBuf.length != (vma->vm_end - vma->vm_start))
  {
    /* wrong length */
    err_msg("mmap() Wrong length parameter\n");
    up(pLock);
    return -EINVAL;
  }

  if (!pBuf->cpuAddr)
  {
    /* not requested */
    err_msg("mmap() Buffer is not available for mapping\n");
    up(pLock);
    return -EINVAL;
  }

  if (pBuf->mapCount > 0)
  {
    err_msg("mmap() Buffer is already mapped\n");
    up(pLock);
    return -EBUSY;
  }


  vma->vm_flags       |= VM_RESERVED | VM_IO | VM_DONTEXPAND | VM_LOCKED;
  vma->vm_page_prot    = pgprot_noncached(vma->vm_page_prot);
  vma->vm_private_data = pBuf;

  if(virt_addr_valid(pBuf->cpuAddr))
  {
    debug_msg("v4l2_mmap remapping memory.\n");
    vma->vm_ops = &stmvout_vm_ops_memory;
  }
  else
  {
    debug_msg("v4l2_mmap remapping IO space.\n");
    /*
     * Note that this assumes an identity mapping between the page offset and
     * the pfn of the physical address to be mapped. This will get more complex
     * when the 32bit SH4 address space becomes available.
     */
    if(remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, (vma->vm_end - vma->vm_start), vma->vm_page_prot))
      return -EAGAIN;

    vma->vm_ops = &stmvout_vm_ops_ioaddr;
  }

  pBuf->mapCount = 1;
  pBuf->vidBuf.flags |= V4L2_BUF_FLAG_MAPPED;

  up(pLock);

  debug_msg("v4l2_mmap out.\n");

  return 0;
}


/************************************************
 * v4l2_poll
 ************************************************/
static unsigned int v4l2_poll(struct stm_v4l2_handles *handle, int type, struct file*file,poll_table  *table)
{
  stvout_device_t    *pDev      = (stvout_device_t *)handle->v4l2type[type].handle;
  struct semaphore   *pLock     = &pDev->devLock;
  unsigned int        mask      = 0;

  if(down_interruptible(pLock))
    return -ERESTARTSYS;

  /* Add DQueue wait queue to the poll wait state */
  poll_wait(file, &(pDev->wqBufDQueue),      table);
  poll_wait(file, &(pDev->wqStreamingState), table);

  //if (!pOpenData->isOpenForIO)
  if (0)
  {
    mask = POLLERR;
  }
  else if (!pDev->isStreaming)
  {
    mask = POLLOUT; /* Write is available when not streaming */
  }
  else
  {
    if(hasCompletedBuffers(pDev))
	mask = POLLIN; /* Indicate we can do a DQUEUE ioctl */
  }

  up(pLock);

  return mask;
}

extern struct video_device v4l2_template;

/************************************************
 *  ConfigureVOUTDevice
 ************************************************/

static int ConfigureVOUTDevice(stvout_device_t *pDev,int pipelinenr,int planenr)
{
  int ret;
  // int num = pDev - g_voutData;
  //int i;
  debug_msg("ConfigureVOUTDevice in. pDev = 0x%08X\n", (unsigned int)pDev);

  init_waitqueue_head(&(pDev->wqBufDQueue));
  init_waitqueue_head(&(pDev->wqStreamingState));

  sema_init(&(pDev->devLock),1);

  stmcore_get_display_pipeline(pipelinenr,&pDev->displayPipe);

  //  memcpy(&pDev->v, &v4l2_template, sizeof(struct video_device));

  pDev->planeConfig = &pDev->displayPipe.planes[planenr];

  //  i = 0;
  //  while((plane_mappings[i].plane_name != NULL) && (plane_mappings[i].plane_id != pDev->planeConfig->id))
  //    i++;

//  strcpy(pDev->v.name, device_config[num].plane_name);
//  sprintf(pDev->v.name,"%s-%d", plane_mappings[i].plane_name, pipelinenr);

  debug_msg("ConfigureVOUTDevice name = %s\n", pDev->v.name);

  pDev->globalAlpha  = 255;  /* Initially set to opqaue */

  pDev->currentOutputNum = -1;

  if((ret = setVideoOutput(pDev, 0)) < 0)
    return ret;

  debug_msg("ConfigureVOUTDevice out.\n");

  return 0;
}


/************************************************
 *      ClearVOUTDeviceConfig
 ************************************************/
/*static void ClearVOUTDeviceConfig(stvout_device_t *pDev)
{
  debug_msg("ClearVOUTDeviceConfig in. pDev = 0x%08X\n", (unsigned int)pDev);

  //  down(&pDev->devLock);

  memset(pDev, 0, sizeof(stvout_device_t));

  debug_msg("ClearVOUTDeviceConfig out.\n");
}*/

struct stm_v4l2_driver stgfb_output = {
  .type       = STM_V4L2_VIDEO_OUTPUT,

  .ioctl      = v4l2_do_ioctl,
  .close      = v4l2_close,
  .poll       = v4l2_poll,
  .mmap       = v4l2_mmap,
};

/************************************************
 *  cleanup_module
 ************************************************/
/*static void st_v4l2out_exit(void)
{
  int m;
  int i;

  debug_msg("cleanup_module in.\n");

  for (m = 0; m < STM_V4L2_MAX_DEVICES; m++)
  for (i = 0; i < MAX_PLANES; ++i)
    ClearVOUTDeviceConfig(&g_voutData[i][m]);

  debug_msg("cleanup_module out.\n");
}*/

/************************************************
 *  init_module
 ************************************************/
int __init st_v4l2out_init(void)
{
  int pipelinenr = 0;
  int planenr = 0;
  struct stmcore_display_pipeline_data tmpdisplaypipe;
  int i;

  debug_msg("init_module in.\n");

  memset(g_voutData, 0, sizeof(g_voutData));

  while(!stmcore_get_display_pipeline(pipelinenr,&tmpdisplaypipe))
  {
    planenr = 0;

    while((tmpdisplaypipe.planes[planenr].id != 0))
    {
    char *name = "Unknown";
      if (ConfigureVOUTDevice(&g_voutData[planenr][pipelinenr],pipelinenr,planenr))
      {
	printk("ERROR: Tried to configure a device we know exists, why should we fail %d:%d:%d???\n",planenr,pipelinenr,tmpdisplaypipe.planes[planenr].id);
      }

      for (i=0;i<(sizeof(plane_mappings)/sizeof(struct plane_map));i++)
	if (plane_mappings[i].plane_id == tmpdisplaypipe.planes[planenr].id)
	  break;

      if (i < sizeof(plane_mappings)/sizeof(struct plane_map))
	name = plane_mappings[i].plane_name;

      strcpy(g_voutDevice[planenr][pipelinenr].name,name);

      // *** CHECK THIS NOT SURE IF THIS IS CORRECT ***
      g_voutDevice[planenr][pipelinenr].deviceId = planenr;

      planenr++;
    }

    stgfb_output.number_indexs[pipelinenr]   = planenr;
    stgfb_output.number_controls[pipelinenr] = V4L2_CID_STM_Z_ORDER_RGB2+1;
    g_voutCount[pipelinenr]                  = planenr;

    pipelinenr++;
  }

  // Register the driver
  stm_v4l2_register_driver(&stgfb_output);

  return 0;
}


