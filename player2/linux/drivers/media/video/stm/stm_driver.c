/***********************************************************************\
 *
 * File: stgfb/Linux/media/video/stm_driver.c
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * V4L2 gerneic video driver
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
#include <linux/interrupt.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#include <media/v4l2-ioctl.h>
#endif
#include <asm/page.h>
#include <asm/io.h>
#include <asm/page.h>
#include <stmdisplay.h>
#include <Linux/video/stmfb.h>
#include <linux/stm/stmcoredisplay.h>

#include <asm/uaccess.h>
#include <asm/irq.h>

#include <linux/stm/stm_v4l2.h>
#include "stmvout.h"
#include "stmvout_driver.h"

#define MAX_DRIVERS 8   ///<! The maximum number of drivers that can be registered
#define MAX_DEVICES 3   ///<! The maximum number of devices (for us that's independent displays)
#define MAX_HANDLES 32  ///<! Maximum number of allowed file handles

struct stm_v4l2_driver  drivers[MAX_DRIVERS];
struct stm_v4l2_handles handles[MAX_HANDLES];
static int              number_index[STM_V4L2_MAX_TYPES][STM_V4L2_MAX_DEVICES];
struct video_device     devices[STM_V4L2_MAX_DEVICES];

static int              number_controls[STM_V4L2_MAX_DEVICES];
static int              number_drivers;

int __init st_v4l2out_init(void);

int stm_v4l2_register_driver(struct stm_v4l2_driver *driver)
{
  int i;

  for (i=0;i<STM_V4L2_MAX_DEVICES;i++) {
    driver->start_index[i]         = number_index[driver->type][i];
    driver->control_start_index[i] = number_controls[i];

    number_index[driver->type][i] += driver->number_indexs[i];
    number_controls[i]            += driver->number_controls[i];
  }

  drivers[number_drivers]     = *driver;
  number_drivers++;

  return 0;
}

int stm_v4l2_unregister_driver(struct stm_v4l2_driver *driver)
{
  printk("to be implemented\n");

  return 0;
}

// Not used yet...., we certainly need to take the device look for allocating a handle,
// but that might be it
struct semaphore        devLock;

EXPORT_SYMBOL(stm_v4l2_register_driver);
EXPORT_SYMBOL(stm_v4l2_unregister_driver);

/************************************************
 *  v4l2_open
 ************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static int v4l2_open(struct inode *inode, struct file *file)
{
  unsigned int     minor            = iminor(inode);

  debug_msg("v4l2_open in. minor = %d flags = 0x%08X\n", minor, file->f_flags);

  // Only /dev/video0 supported at the mo
  if (minor < 0 || minor >= MAX_DEVICES)
    return -ENODEV;

  file->private_data = NULL;

  debug_msg("v4l2_open out.\n");

  return 0;
}
#else /* >= 2.6.30 */
static int v4l2_open(struct file *file)
{
  file->private_data = NULL;
  return 0;
}
#endif /* >= 2.6.30 */


/************************************************
 *  v4l2_close
 ************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static int v4l2_close(struct inode *inode, struct file *file)
#else
static int v4l2_close(struct file *file)
#endif
{
  struct stm_v4l2_handles *handle = file->private_data;
//  unsigned int     minor          = iminor(inode);
  int n;

  if (handle) {
	  for (n=0;n<STM_V4L2_MAX_TYPES;n++)
		  if (handle->v4l2type[n].driver &&
		      handle->v4l2type[n].driver->close)
			  handle->v4l2type[n].driver->close(handle,
							    n,
							    file);

	  memset(handle,0,sizeof(struct stm_v4l2_handles));
  }

  return 0;
}

static struct stm_v4l2_handles *alloc_handle(void)
{
  int m;

  // mutex handle lock

  for (m=0;m<MAX_HANDLES;m++)
    if (!handles[m].used) {
	  memset(&handles[m],0,sizeof(struct stm_v4l2_handles));
      handles[m].used = 1;
      return &handles[m];
    }

  printk("Probably going to crash fix me...\n");
  return NULL;
}

/************************************************
 *  stm_v4l2_do_ioctl
 ************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static int stm_v4l2_do_ioctl(struct inode *inode, struct file  *file, unsigned int  cmd, void *arg)
#else
static long stm_v4l2_do_ioctl(struct file  *file, unsigned int  cmd, void *arg)
#endif
{
  struct stm_v4l2_handles *handle = file->private_data;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
  unsigned int     minor          = iminor(inode);
#else
  unsigned minor = iminor(file->f_dentry->d_inode);
#endif
  int ret = 0;
  int type = -1,index  = -1;
  int n;

    switch (cmd)
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
    case VIDIOC_RESERVED:         //_IO   ('V',  1)
    case VIDIOC_G_MPEGCOMP:       //_IOR  ('V',  6, struct v4l2_mpeg_compression)
    case VIDIOC_S_MPEGCOMP:       //_IOW  ('V',  7, struct v4l2_mpeg_compression)
    case VIDIOC_G_PRIORITY:       //_IOR  ('V', 67, enum v4l2_priority)
    case VIDIOC_S_PRIORITY:       //_IOW  ('V', 68, enum v4l2_priority)
    case VIDIOC_G_SLICED_VBI_CAP: //_IOR  ('V', 69, struct v4l2_sliced_vbi_cap)
    case VIDIOC_G_MODULATOR:      //_IOWR ('V', 54, struct v4l2_modulator)
    case VIDIOC_S_MODULATOR:      //_IOW  ('V', 55, struct v4l2_modulator)
    case VIDIOC_G_FREQUENCY:      //_IOWR ('V', 56, struct v4l2_frequency)
    case VIDIOC_S_FREQUENCY:      //_IOW  ('V', 57, struct v4l2_frequency)
    case VIDIOC_G_TUNER:          //_IOWR ('V', 29, struct v4l2_tuner)
    case VIDIOC_S_TUNER:          //_IOW  ('V', 30, struct v4l2_tuner)
    case VIDIOC_G_JPEGCOMP:       //_IOR  ('V', 61, struct v4l2_jpegcompression)
    case VIDIOC_S_JPEGCOMP:       //_IOW  ('V', 62, struct v4l2_jpegcompression)
    default:
      debug_msg("IOCTL is not implemented\n");
      ret = -ENODEV;// TIMPLEMENTED;
      break;
#endif
      // video ioctls
    case VIDIOC_QUERYCAP:         //_IOR  ('V',  0, struct v4l2_capability)
    {
      struct v4l2_capability *b = arg;

      //debug_msg("v4l2_ioctl - VIDIOC_QUERYCAP\n");

      strcpy(b->driver, "stm_v4l2");
      strcpy(b->card, "STMicroelectronics CE Device");
      strcpy(b->bus_info,"");

      b->version      = LINUX_VERSION_CODE;
      b->capabilities = V4L2_CAP_VIDEO_OVERLAY | V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
      return 0;
    }

      // audio input ioctls
    case VIDIOC_G_AUDIO:          //_IOR  ('V', 33, struct v4l2_audio)
    case VIDIOC_S_AUDIO:          //_IOW  ('V', 34, struct v4l2_audio)
    case VIDIOC_ENUMAUDIO:        //_IOWR ('V', 65, struct v4l2_audio)
      {
	struct v4l2_audio *audio = arg;
	type = STM_V4L2_AUDIO_INPUT;
	if (audio) index = audio->index;
	else ret = -EINVAL;
      }
      break;

      // audio output ioctls
    case VIDIOC_ENUMAUDOUT:       //_IOWR ('V', 66, struct v4l2_audioout)
    case VIDIOC_G_AUDOUT:         //_IOR  ('V', 49, struct v4l2_audioout)
    case VIDIOC_S_AUDOUT:         //_IOW  ('V', 50, struct v4l2_audioout)
      {
	struct v4l2_audio *audio = arg;
	type = STM_V4L2_AUDIO_OUTPUT;
	if (audio) index = audio->index;
	else ret = -EINVAL;
      }
      break;


      // video input ioctls
    case VIDIOC_ENUMINPUT:        //_IOWR ('V', 26, struct v4l2_input)
      {
	struct v4l2_input *input = arg;
	type = STM_V4L2_VIDEO_INPUT;
	if (input) index = input->index;
	else ret = -EINVAL;
      }
      break;

    case VIDIOC_G_INPUT:          //_IOR  ('V', 38, int)
    case VIDIOC_S_INPUT:          //_IOWR ('V', 39, int)
      {
	int *input = arg;
	type = STM_V4L2_VIDEO_INPUT;
	if (arg) index = *input;
	else ret = -EINVAL;
      }
      break;

    case VIDIOC_G_FBUF:           //_IOR  ('V', 10, struct v4l2_framebuffer)
    case VIDIOC_S_FBUF:           //_IOW  ('V', 11, struct v4l2_framebuffer)
    case VIDIOC_OVERLAY:          //_IOW  ('V', 14, int)
	type = STM_V4L2_VIDEO_INPUT;
	break;

	// these are for inputs only, read the spec...
    case VIDIOC_G_STD:            //_IOR  ('V', 23, v4l2_std_id)
    case VIDIOC_S_STD:            //_IOW  ('V', 24, v4l2_std_id)
      	type = STM_V4L2_VIDEO_INPUT;
	break;


      // video output ioctls
    case VIDIOC_G_OUTPUT:         //_IOR  ('V', 46, int)
    case VIDIOC_S_OUTPUT:         //_IOWR ('V', 47, int)
      {
	int *input = arg;
	type = STM_V4L2_VIDEO_OUTPUT;
	if (arg) index = *input;
	else ret = -EINVAL;
      }
      break;

    case VIDIOC_ENUMOUTPUT:       //_IOWR ('V', 48, struct v4l2_output)
      {
	struct v4l2_output *output = arg;
	type = STM_V4L2_VIDEO_OUTPUT;
	if (output) index = output->index;
	else ret = -EINVAL;
      }
      break;

    case VIDIOC_G_FMT:            //_IOWR ('V',  4, struct v4l2_format)
    case VIDIOC_S_FMT:            //_IOWR ('V',  5, struct v4l2_format)
    case VIDIOC_TRY_FMT:          //_IOWR ('V', 64, struct v4l2_format)
      {
	struct v4l2_format *format = arg;
	if (format) {
	  if (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
	    type = STM_V4L2_VIDEO_OUTPUT;
	  else
	    type = STM_V4L2_VIDEO_INPUT;
	} else ret = -EINVAL;
      }
      break;

      // this needs to be generic, and probably a way to allocate from video or system memory
      // remember buffers can be allocated for input or output, for the moment we will pass to the driver
    case VIDIOC_REQBUFS:          //_IOWR ('V',  8, struct v4l2_requestbuffers)
      {
	struct v4l2_requestbuffers *rb = arg;
	if (rb) {
	  if (rb->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
	    type = STM_V4L2_VIDEO_OUTPUT;
	  else
	    type = STM_V4L2_VIDEO_INPUT;
	} else ret = -EINVAL;
      }
      break;

    case VIDIOC_QUERYBUF:         //_IOWR ('V',  9, struct v4l2_buffer)
    case VIDIOC_QBUF:             //_IOWR ('V', 15, struct v4l2_buffer)
    case VIDIOC_DQBUF:            //_IOWR ('V', 17, struct v4l2_buffer)
      {
	struct v4l2_buffer *buffer = arg;
	if (buffer) {
	  if (buffer->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
	    type = STM_V4L2_VIDEO_OUTPUT;
	  else
	    type = STM_V4L2_VIDEO_INPUT;

	} else ret = -EINVAL;
      }
      break;

    case VIDIOC_G_PARM:           //_IOWR ('V', 21, struct v4l2_streamparm)
    case VIDIOC_S_PARM:           //_IOWR ('V', 22, struct v4l2_streamparm)
      {
	struct v4l2_streamparm *param = arg;
	if (param) {
	  if (param->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
	    type = STM_V4L2_VIDEO_OUTPUT;
	  else
	    type = STM_V4L2_VIDEO_INPUT;
	} else ret = -EINVAL;
      }
      break;

    case VIDIOC_CROPCAP:          //_IOWR ('V', 58, struct v4l2_cropcap)
      {
	struct v4l2_cropcap *cropcap = arg;
	if (cropcap) {
	  if (cropcap->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
	    type = STM_V4L2_VIDEO_OUTPUT;
	  else
	    type = STM_V4L2_VIDEO_INPUT;
	} else ret = -EINVAL;
      }
      break;

    case VIDIOC_G_CROP:           //_IOWR ('V', 59, struct v4l2_crop)
    case VIDIOC_S_CROP:           //_IOW  ('V', 60, struct v4l2_crop)
      {
	struct v4l2_crop *crop = arg;
	if (crop) {
		// what should we do about BUF_TYPE_PRIVATE
	  if (crop->type == V4L2_BUF_TYPE_VIDEO_OUTPUT || crop->type == V4L2_BUF_TYPE_PRIVATE)
	    type = STM_V4L2_VIDEO_OUTPUT;
	  else
	    type = STM_V4L2_VIDEO_INPUT;
	} else ret = -EINVAL;
      }
      break;

      // can be input or output but why not audio?????
    case VIDIOC_STREAMON:         //_IOW  ('V', 18, int)
    case VIDIOC_STREAMOFF:        //_IOW  ('V', 19, int)
      {
	int *v4ltype = arg;
	if (v4ltype) {
	  if (*v4ltype == V4L2_BUF_TYPE_VIDEO_OUTPUT)
	    type = STM_V4L2_VIDEO_OUTPUT;
	  else
	    type = STM_V4L2_VIDEO_INPUT;
	} else ret = -EINVAL;
      }
      break;

      // Driver independent
      // however we need to get the correct ones
    case VIDIOC_ENUM_FMT:         //_IOWR ('V',  2, struct v4l2_fmtdesc)
      {
	struct v4l2_fmtdesc *crop = arg;
	if (crop) {
	  if (crop->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
	    type = STM_V4L2_VIDEO_OUTPUT;
	  else
	    type = STM_V4L2_VIDEO_INPUT;
	} else ret = -EINVAL;
      }
      break;

      // These should be driver independent the stmvout modules does not
      // comply with the spec...
    case VIDIOC_ENUMSTD:          //_IOWR ('V', 25, struct v4l2_standard)
    case VIDIOC_QUERYSTD:         //_IOR  ('V', 63, v4l2_std_id)
      type = STM_V4L2_VIDEO_OUTPUT;
      break;

      // control ioctls
    case VIDIOC_G_CTRL:           //_IOWR ('V', 27, struct v4l2_control)
    case VIDIOC_S_CTRL:           //_IOWR ('V', 28, struct v4l2_control)
    case VIDIOC_QUERYCTRL:        //_IOWR ('V', 36, struct v4l2_queryctrl)
    case VIDIOC_QUERYMENU:        //_IOWR ('V', 37, struct v4l2_querymenu)
      { // cheat a little we know id is always first one
	struct v4l2_control *control = arg;
	if (arg) index = control->id;
	else ret = -EINVAL;

	type = STM_V4L2_VIDEO_INPUT;
      }
      break;
    };

    // Cool now we have all the information we need.

    // first if we got an error lets just return that
    if (ret) return ret;

    // If it is a control let's deal with that
    if (type==STM_V4L2_MAX_TYPES) {
      for (n=0;n<number_drivers;n++)
	if (index >= drivers[n].control_start_index[minor] && index < (drivers[n].control_start_index[minor] + drivers[n].number_controls[minor]))
	  {
	    struct v4l2_control *control = arg;

	    control->id -= drivers[n].control_start_index[minor];
	    ret = drivers[n].ioctl(handle,&drivers[n],minor,drivers[n].type,file,cmd,arg);
	    control->id += drivers[n].control_start_index[minor];
	    return ret;
	  }
      return -EINVAL;
    }


    // if no handle, then we need to get one
    // or there would be no handle for a specified type
    if (!handle || (handle && type != -1 && handle->v4l2type[type].driver == NULL)) {
		if (!handle) handle = alloc_handle();

      		for (n=0;n<number_drivers;n++) {
				if (type == -1 || type == drivers[n].type) {
				  ret = drivers[n].ioctl(handle,&drivers[n],minor,drivers[n].type,file,cmd,arg);
	  				if (handle->v4l2type[drivers[n].type].handle) {
	    				if (ret) printk("%s: BUG ON... can't set a handle and return an error\n",__FUNCTION__);
					    	handle->v4l2type[drivers[n].type].driver = &drivers[n];
						handle->device = minor;
						printk("%s: Assigning handle %x to device %d\n",__FUNCTION__,(unsigned int)handle,minor);
				    	file->private_data = handle;
				    	return ret;
	  				}

				  // Not sure if this is safe but if the driver didn't return a problem then assume
				  // it was an enum or something that didn't need to be associated (allocated is too strong a word)
				 if (!ret) {
				    if (!file->private_data) memset(handle,0,sizeof(struct stm_v4l2_handles));
				    return ret;
				  }
			}
		}

      // Nothing worked so clear the handle and go again
      if (!file->private_data) memset(handle,0,sizeof(struct stm_v4l2_handles));
      return -ENODEV;
    }

    // Ok if type != -1 and handle[type].driver->ioctl and handle[type].handle, just call it
    if (type != -1)
    {
      if (handle->v4l2type[type].driver)
      {
	return handle->v4l2type[type].driver->ioctl(handle,handle->v4l2type[type].driver,minor,type,file,cmd,arg);
      }
      else
      {
		return -ENODEV;
      }
    }

    for (n=0;n<STM_V4L2_MAX_TYPES;n++)
      if (handle->v4l2type[n].driver)
	if (!handle->v4l2type[n].driver->ioctl(handle,handle->v4l2type[n].driver,minor,n,file,cmd,arg))
      	  return 0;

    return -ENODEV;
}

/*********************************************************
 * v4l2_ioctl - ioctl via standard helper proxy to manage
 *              the argument copying to and from userspace
 *********************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static int stm_v4l2_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
  return video_usercopy(inode, file, cmd, arg, stm_v4l2_do_ioctl);
}
#else
static long stm_v4l2_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  return video_usercopy(file, cmd, arg, stm_v4l2_do_ioctl);
}
#endif


/************************************************
 * v4l2_poll
 ************************************************/
static unsigned int v4l2_poll(struct file *file, poll_table  *table)
{
  struct stm_v4l2_handles *handle = file->private_data;
  int ret = 0;
  int n;

  if (handle)
    for (n=0;n<STM_V4L2_MAX_TYPES;n++)
      if (handle->v4l2type[n].driver)
	if (handle->v4l2type[n].driver->poll)
	  ret = handle->v4l2type[n].driver->poll(handle,n,file,table);

  return ret;
}

static int  v4l2_mmap(struct file  *file, struct vm_area_struct*  vma)
{
  struct stm_v4l2_handles *handle = file->private_data;
  int ret = -ENODEV;
  int n;

  if (handle)
    for (n=0;n<STM_V4L2_MAX_TYPES;n++)
      if (handle->v4l2type[n].driver && handle->v4l2type[n].handle)
	if (handle->v4l2type[n].driver->mmap)
	  if (!(ret = handle->v4l2type[n].driver->mmap(handle,n,file,vma)))
	    return 0;

  return ret;
}

void v4l2_vdev_release (struct video_device *vdev)
{
  /*
   * Nothing to do as the video_device is not dynamically allocated,
   * but the V4L framework complains if this method is not present.
   */
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static struct file_operations v4l2_fops = 
#else
static struct v4l2_file_operations v4l2_fops = 
#endif
{
	.owner   = THIS_MODULE,
	.open    = v4l2_open,
	.release = v4l2_close,
	.ioctl   = stm_v4l2_ioctl,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	.llseek  = no_llseek,
#endif
	.read    = NULL,
	.write   = NULL,
	.mmap    = v4l2_mmap,
	.poll    = v4l2_poll,
};

struct video_device v4l2_template __devinitdata = {
	.name     = "",
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	.type     = (VID_TYPE_OVERLAY   | VID_TYPE_SCALES),
	.type2    = (V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT),
	.hardware = VID_HARDWARE_STMVOUT,
#endif
	.fops     = &v4l2_fops,
	.release  = &v4l2_vdev_release,
	.minor    = -1
};

static int video_nr = -1;
module_param(video_nr, int, 0444);

/************************************************
 *  cleanup_module
 ************************************************/
void stm_v4l2_exit(void)
{

}

/************************************************
 *  init_module
 ************************************************/
int __init stm_v4l2_init(void)
{
  int n;

  for (n=0;n<MAX_DEVICES;n++) {
    memcpy(&devices[n], &v4l2_template, sizeof(struct video_device));
    if (video_register_device(&devices[n], VFL_TYPE_GRABBER, n) < 0)
      return -ENODEV;
  }

  st_v4l2out_init();
  //avr_init();

  return 0;
}

module_init(stm_v4l2_init);
module_exit(stm_v4l2_exit);

MODULE_LICENSE("GPL");

