/***********************************************************************
 * File: stgfb/Linux/video/stmvout.h
 * Copyright (c) 2005 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 * 
 ***********************************************************************/
 
#ifndef __STMVOUT_H
#define __STMVOUT_H

#define VID_HARDWARE_STMVOUT 100

#define V4L2_PIX_FMT_STM422MB v4l2_fourcc('4','2','2','B') /* STMicroelectronics 422 Macro Block */
#define V4L2_PIX_FMT_STM420MB v4l2_fourcc('4','2','0','B') /* STMicroelectronics 420 Macro Block */

/*
 * Add some defines for 16bit RGB formats with alpha channel
 */
#define V4L2_PIX_FMT_BGRA5551 v4l2_fourcc('B','G','R','T')
#define V4L2_PIX_FMT_BGRA4444 v4l2_fourcc('B','G','R','S')

#define VIDIOC_S_OVERLAY_ALPHA _IOW ('V',  BASE_VIDIOC_PRIVATE, unsigned int)

/*
 * Add some CLUT surfaces
 */
#define V4L2_PIX_FMT_CLUT2  v4l2_fourcc('C','L','T','2')
#define V4L2_PIX_FMT_CLUT4  v4l2_fourcc('C','L','T','4')
#define V4L2_PIX_FMT_CLUT8  v4l2_fourcc('C','L','T','8')
#define V4L2_PIX_FMT_CLUTA8 v4l2_fourcc('C','L','T','A')

/*
 * Repeats the first field (see V4L2_BUF_FLAG_BOTTOM_FIELD_FIRST).
 */
#define V4L2_BUF_FLAG_REPEAT_FIRST_FIELD  0x1000

/*
 * V4L2 does not allow us to specify if an interlaced buffer is to be
 * presented top field first or bottom field first. For video capture
 * this can be determined by the input video standard; for output
 * devices this is a property of the content to be presented, not the
 * video display standard. So we define an additional buffer flag
 * to indicate the field order of this buffer. When combined with the
 * V4L2_BUF_FLAG_REPEAT_FIRST_FIELD it can be used to implement 3/2 pulldown
 * as specified in MPEG2 picture extension headers.
 */
#define V4L2_BUF_FLAG_BOTTOM_FIELD_FIRST  0x4000

/*
 * When doing pause or slow motion with interlaced content the fields
 * will get displayed (alternately) for several "frames". As there may
 * be motion between the fields, this results in the image "shaking" usually
 * from side to side. If you know this is going to be the case, then setting
 * the following buffer flag will cause the driver to produce both display
 * fields from the same field data in the buffer (using interpolation for the
 * wrong field) while the buffer continues to be on display. This produces a
 * stable image, but with reduced image quality due to the interpolation.
 */
#define V4L2_BUF_FLAG_INTERPOLATE_FIELDS  0x8000

/*
 * When displaying ARGB buffers the driver will by default blend with the 
 * layer below assuming that pixel RGB values are already pre-multiplied by the
 * pixel alpha value. Setting this flag changes the blend maths so that
 * each pixel's RGB values are multiplied by the pixel's alpha value before the
 * blend takes place.
 */ 
#define V4L2_BUF_FLAG_NON_PREMULTIPLIED_ALPHA 0x10000

/*
 * By the default the full colour range of the buffer contents is output to
 * the compositor. This is generally correct for video but not for RGB graphics.
 * Buffers being queued on graphics planes can optionally rescale the colour
 * components to the nominal 8bit range 16-235; although internally this is
 * all done after pixel values have been upscaled to a 10bit range so there is
 * no loss of colour information with this operation.
 */
#define V4L2_BUF_FLAG_RESCALE_COLOUR_TO_VIDEO_RANGE 0x20000

/*
 * Normal Controls
 */
#define V4L2_CID_STM_IQI_SET_CONFIG     (V4L2_CID_PRIVATE_BASE+0)
#define V4L2_CID_STM_IQI_DEMO           (V4L2_CID_PRIVATE_BASE+1)
#define V4L2_CID_STM_XVP_SET_CONFIG     (V4L2_CID_PRIVATE_BASE+2)
#define V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE (V4L2_CID_PRIVATE_BASE+3)

/*
 * Z-Order Controls 
 */
#define V4L2_CID_STM_Z_ORDER_VID0       (V4L2_CID_PRIVATE_BASE+20)
#define V4L2_CID_STM_Z_ORDER_VID1       (V4L2_CID_PRIVATE_BASE+21)
#define V4L2_CID_STM_Z_ORDER_RGB0       (V4L2_CID_PRIVATE_BASE+22)
#define V4L2_CID_STM_Z_ORDER_RGB1       (V4L2_CID_PRIVATE_BASE+23)
#define V4L2_CID_STM_Z_ORDER_RGB1_0     (V4L2_CID_PRIVATE_BASE+24)
#define V4L2_CID_STM_Z_ORDER_RGB1_1     (V4L2_CID_PRIVATE_BASE+25)
#define V4L2_CID_STM_Z_ORDER_RGB1_2     (V4L2_CID_PRIVATE_BASE+26)
#define V4L2_CID_STM_Z_ORDER_RGB1_3     (V4L2_CID_PRIVATE_BASE+27)
#define V4L2_CID_STM_Z_ORDER_RGB1_4     (V4L2_CID_PRIVATE_BASE+28)
#define V4L2_CID_STM_Z_ORDER_RGB1_5     (V4L2_CID_PRIVATE_BASE+29)
#define V4L2_CID_STM_Z_ORDER_RGB1_6     (V4L2_CID_PRIVATE_BASE+30)
#define V4L2_CID_STM_Z_ORDER_RGB1_7     (V4L2_CID_PRIVATE_BASE+31)
#define V4L2_CID_STM_Z_ORDER_RGB1_8     (V4L2_CID_PRIVATE_BASE+32)
#define V4L2_CID_STM_Z_ORDER_RGB1_9     (V4L2_CID_PRIVATE_BASE+33)
#define V4L2_CID_STM_Z_ORDER_RGB1_10    (V4L2_CID_PRIVATE_BASE+34)
#define V4L2_CID_STM_Z_ORDER_RGB1_11    (V4L2_CID_PRIVATE_BASE+35)
#define V4L2_CID_STM_Z_ORDER_RGB1_12    (V4L2_CID_PRIVATE_BASE+36)
#define V4L2_CID_STM_Z_ORDER_RGB1_13    (V4L2_CID_PRIVATE_BASE+37)
#define V4L2_CID_STM_Z_ORDER_RGB1_14    (V4L2_CID_PRIVATE_BASE+38)
#define V4L2_CID_STM_Z_ORDER_RGB1_15    (V4L2_CID_PRIVATE_BASE+39)
#define V4L2_CID_STM_Z_ORDER_RGB2       (V4L2_CID_PRIVATE_BASE+99)

#endif /* __STMVOUT_H */
