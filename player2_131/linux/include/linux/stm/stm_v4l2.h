/***********************************************************************
 *
 * File: stgfb/Linux/media/video/stm_driver.h
 * Copyright (c) 2007 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 * 
 * V4L2 gerneic video driver
 * 
\***********************************************************************/

#ifndef __STM_DRIVER_H
#define __STM_DRIVER_H

#include <linux/videodev.h>

// A single file handle can have more than one active type
// however audio and video can not be done together :-( limitation
// of API
#define STM_V4L2_VIDEO_OUTPUT  0
#define STM_V4L2_VIDEO_INPUT   1
#define STM_V4L2_AUDIO_OUTPUT  2
#define STM_V4L2_AUDIO_INPUT   3

#define STM_V4L2_MAX_TYPES     4

#define STM_V4L2_MAX_DEVICES   3

#define STM_V4L2_MAX_CONTROLS  255

struct stm_v4l2_driver;

struct stm_v4l2_handles {
  int                     used;
  int                     mask;

  // is this needed, sanyway its to identify /dev/video[0|1|2]
  int                     device;

  struct {
	  struct stm_v4l2_driver *driver;
	  void                   *handle;
  } v4l2type[STM_V4L2_MAX_TYPES];
};

struct stm_v4l2_driver {
  // Driver should fill in these before registering the driver
  int                type;                //!< Type of buffer, a driver can register as many as they like but only one type at a time.
  unsigned int       capabilities;        //!< Capabilities this provides (they get ored together to produce the final capailities
  int                number_controls[STM_V4L2_MAX_DEVICES];     //!< Number of controls this device exposes
  int                number_indexs[STM_V4L2_MAX_DEVICES];       //!< Number of output|overlay|capture devices
  void              *priv;                //!< Private Data for driver

  // Operations the driver should register with.
  // Only ioctl is mandatory, as the minimum you can implement
  // is a single control.

  // Once handle has been filled in via ioctl then the driver has claimed it for it's use.
  int          (*ioctl)       (struct stm_v4l2_handles *handle, struct stm_v4l2_driver *driver, int device, int type, struct file *file,unsigned int  cmd, void *arg);
  int          (*close)       (struct stm_v4l2_handles *handle, int type, struct file *file);
  unsigned int (*poll)        (struct stm_v4l2_handles *handle, int type, struct file *file, poll_table  *table);
  int          (*mmap)        (struct stm_v4l2_handles *handle, int type, struct file *file,struct vm_area_struct*  vma);
  //  int          (*munmap)      (struct stm_v4l2_driver *driver, void *handle, void *start, size_t length);

  // Private data for this driver, any index values will be subtracted by this amount
  int          start_index[STM_V4L2_MAX_DEVICES];         //!< Start index for this device, drivers need to handle this
  int          control_start_index[STM_V4L2_MAX_DEVICES]; //!< Index from which controls start, drivers don't need to handle this
};


// Registration functions
extern int stm_v4l2_register_driver(struct stm_v4l2_driver *driver);
extern int stm_v4l2_unregister_driver(struct stm_v4l2_driver *driver);

#endif
