/***********************************************************************
 * File: stmfb/Linux/media/video/stmvout.h
 * Copyright (c) 2005-2009 STMicroelectronics Limited.
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
 * Queuing progressive buffers containing animated graphics, on an interlaced
 * display, can result in artifacts due to vertical motion. If the animation
 * updates on each interlaced field, parts of the image may appear to be
 * invisible as the vertical motion keeps them out of sync with the output
 * field. This flag indicates that the buffer contains graphics and that the
 * system should update the display plane only on a top display field.
 *
 * Note: this used to be the default behaviour on graphics planes, but this is
 * no longer the case. This flag is implemented for both video and graphics
 * planes.
 */
#define V4L2_BUF_FLAG_GRAPHICS 0x40000

/*
 * The following define private controls available in the driver
 */
#define V4L2_CTRL_CLASS_STM         0x0ae00000  /* STMicro class controls */
#define V4L2_CID_STM_CLASS_BASE     (V4L2_CTRL_CLASS_STM | 0x900)
#define V4L2_CID_STM_CLASS          (V4L2_CTRL_CLASS_STM | 1)

/* note to readers: 0 << 17 should be STM_V4L2_VIDEO_OUTPUT << 17 but we can
   not do this at the moment, because of include problems...*/
#define V4L2_CTRL_CLASS_STMVOUT     (V4L2_CTRL_CLASS_STM | (0 << 17))  /* STMicro vout class controls */
#define V4L2_CID_STMVOUT_CLASS_BASE (V4L2_CTRL_CLASS_STMVOUT | 0x0900)
#define V4L2_CID_STMVOUT_CLASS      (V4L2_CTRL_CLASS_STMVOUT | 1)

#define V4L2_CTRL_CLASS_STMVOUTEXT     (V4L2_CTRL_CLASS_STM | (0 << 17) | (1 << 16))  /* STMicro vout extended class controls */
#define V4L2_CID_STMVOUTEXT_CLASS_BASE (V4L2_CTRL_CLASS_STMVOUTEXT | 0x900)
#define V4L2_CID_STMVOUTEXT_CLASS      (V4L2_CTRL_CLASS_STMVOUTEXT | 1)



enum _V4L2_CID_STM_IQI_SET_CONFIG_MODE {
  /* for V4L2_CID_STM_IQI_SET_CONFIG */
  VCSISCM_BYPASS,  /* bypass IQI */
  VCSISCM_DEFAULT, /* ST Micro IQI configuration */
};

enum _V4L2_CID_STM_IQI_DEMO_MODE {
  /* for V4L2_CID_STM_IQI_DEMO */
  VCSIDM_OFF, /* turn demo mode off */
  VCSIDM_ON   /* turn demo mode on */
};

enum _V4L2_CID_STM_XVP_SET_CONFIG_MODE {
  /* for V4L2_CID_STM_XVP_SET_CONFIG */
  VCSXSCM_BYPASS,     /* bypass */
  VCSXSCM_FILMGRAIN,  /* enable FilmGrain (not implemented) */
  VCSXSCM_TNR,        /* enable TNR - only for SD captured from
			 DVP, introduces an additional delay of
			 two fields in the output path */
  VCSXSCM_TNR_BYPASS, /* send data through TNR data path (delay
			 is still added, but don't actually apply
			 any TNR */
};

enum _V4L2_CID_STM_DEI_SET_FMD_ENABLE_MODE {
  /* for V4L2_CID_STM_DEI_SET_FMD_ENABLE */
  VCSDSFEM_OFF, /* turn Field Mode Detection algo off */
  VCSDSFEM_ON   /* turn Field Mode Detection algo on */
};

enum _V4L2_CID_STM_DEI_SET_MODE_MODE {
  /* for V4L2_CID_STM_DEI_SET_MODE */
  VCSDSMM_FIRST,

  VCSDSMM_3DMOTION = VCSDSMM_FIRST, /* put deinterlacer into 3D mode */
  VCSDSMM_DISABLED,                 /* completely disable deinterlacer */
  VCSDSMM_MEDIAN,                   /* put deinterlacer into median mode */

  VCSDSMM_COUNT
};

enum _V4L2_CID_STM_CKEY_ENABLE_FLAGS {
  /* for V4L2_CID_STM_CKEY_ENABLE */
  VCSCEF_DISABLED        = 0x00000000, /* disable */
  VCSCEF_ENABLED         = 0x00000001, /* enable */
  VCSCEF_ACTIVATE_BUFFER = 0x00000002  /* change takes effect immediately
					  (or better: on next VSync)
					  or on a new buffer that is queued
					  after making this change */
};

enum _V4L2_CID_STM_CKEY_FORMAT_MODE {
  /* for V4L2_CID_STM_CKEY_FORMAT */
  VCSCFM_RGB   = 0x00000000, /* key in V4L2_CID_STM_CKEY_MINVAL is specified
				in RGB format */
  VCSCFM_CrYCb = 0x00000001  /* key in V4L2_CID_STM_CKEY_MINVAL is specified
				in CrYCb format */
};

enum _V4L2_CID_STM_CKEY_COLOR_COMPONENT_MODE {
  /* for V4L2_CID_STM_CKEY_R_CR_MODE etc. */
  VCSCCCM_DISABLED = 0x00000000,                  /* matching on this color
						     component disabled */
  VCSCCCM_ENABLED  = 0x00000001,                  /* match if
						     (min <= val <= max) */
  VCSCCCM_INVERSE  = 0x00000002 | VCSCCCM_ENABLED /* match if
						     ((val < min)
						      || (val > max)) */
};

enum {
  /*  STM-class control IDs specific to the VOUT driver */
  V4L2_CID_STM_IQI_SET_CONFIG = V4L2_CID_STMVOUT_CLASS_BASE,
				/* enum _V4L2_CID_STM_IQI_SET_CONFIG_MODE */
  V4L2_CID_STM_IQI_DEMO,	/* enum _V4L2_CID_STM_IQI_DEMO_MODE */
  V4L2_CID_STM_XVP_SET_CONFIG,	/* enum _V4L2_CID_STM_XVP_SET_CONFIG_MODE */
  V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE,
  V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP,
  V4L2_CID_STM_DEI_SET_FMD_ENABLE, /* enum _V4L2_CID_STM_DEI_SET_FMD_ENABLE_MODE */
  V4L2_CID_STM_DEI_SET_MODE,	/* enum _V4L2_CID_STM_DEI_SET_MODE_MODE */
  V4L2_CID_STM_DEI_SET_CTRLREG,

  /* Z-Order Controls */
  V4L2_CID_STM_Z_ORDER_FIRST,
  V4L2_CID_STM_Z_ORDER_VID0 = V4L2_CID_STM_Z_ORDER_FIRST,
  V4L2_CID_STM_Z_ORDER_VID1,
  V4L2_CID_STM_Z_ORDER_RGB0,
  V4L2_CID_STM_Z_ORDER_RGB1,
  V4L2_CID_STM_Z_ORDER_RGB2,

  V4L2_CID_STM_Z_ORDER_VIRTUAL_FIRST,
  V4L2_CID_STM_Z_ORDER_RGB1_0 = V4L2_CID_STM_Z_ORDER_VIRTUAL_FIRST,
  V4L2_CID_STM_Z_ORDER_RGB1_1,
  V4L2_CID_STM_Z_ORDER_RGB1_2,
  V4L2_CID_STM_Z_ORDER_RGB1_3,
  V4L2_CID_STM_Z_ORDER_RGB1_4,
  V4L2_CID_STM_Z_ORDER_RGB1_5,
  V4L2_CID_STM_Z_ORDER_RGB1_6,
  V4L2_CID_STM_Z_ORDER_RGB1_7,
  V4L2_CID_STM_Z_ORDER_RGB1_8,
  V4L2_CID_STM_Z_ORDER_RGB1_9,
  V4L2_CID_STM_Z_ORDER_RGB1_10,
  V4L2_CID_STM_Z_ORDER_RGB1_11,
  V4L2_CID_STM_Z_ORDER_RGB1_12,
  V4L2_CID_STM_Z_ORDER_RGB1_13,
  V4L2_CID_STM_Z_ORDER_RGB1_14,
  V4L2_CID_STM_Z_ORDER_RGB1_15,
  V4L2_CID_STM_Z_ORDER_VIRTUAL_LAST = V4L2_CID_STM_Z_ORDER_RGB1_15,
  V4L2_CID_STM_Z_ORDER_LAST = V4L2_CID_STM_Z_ORDER_VIRTUAL_LAST,

  _deprecated_V4L2_CID_STM_IQI_SET_CONFIG_VID0,
  _deprecated_V4L2_CID_STM_IQI_DEMO_VID0,
  _deprecated_V4L2_CID_STM_XVP_SET_CONFIG_VID0,
  _deprecated_V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0,
  _deprecated_V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0,
  _deprecated_V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0,
  _deprecated_V4L2_CID_STM_DEI_SET_MODE_VID0,
  _deprecated_V4L2_CID_STM_DEI_SET_CTRLREG_VID0,

  V4L2_CID_STM_PLANE_BRIGTHNESS,
  V4L2_CID_STM_PLANE_CONTRAST,
  V4L2_CID_STM_PLANE_SATURATION,
  V4L2_CID_STM_PLANE_HUE,

  /* extended controls are following here */
  /* colorkey controls */
  V4L2_CID_STM_CKEY_ENABLE = V4L2_CID_STMVOUTEXT_CLASS_BASE,
			       /* enum _V4L2_CID_STM_CKEY_ENABLE_FLAGS
				  (enable or disable) (immediateley or on
				  next queued & displayed buffer) */
  V4L2_CID_STM_CKEY_FORMAT,    /* enum _V4L2_CID_STM_CKEY_FORMAT_MODE
				  format of V4L2_CID_STM_CKEY_[MIN|MAX]VAL */
  V4L2_CID_STM_CKEY_R_CR_MODE, /* enum _V4L2_CID_STM_CKEY_COLOR_COMPONENT_MODE */
  V4L2_CID_STM_CKEY_G_Y_MODE,  /* enum _V4L2_CID_STM_CKEY_COLOR_COMPONENT_MODE */
  V4L2_CID_STM_CKEY_B_CB_MODE, /* enum _V4L2_CID_STM_CKEY_COLOR_COMPONENT_MODE */
  V4L2_CID_STM_CKEY_MINVAL,    /* RGB or CrYCb format according to config */
  V4L2_CID_STM_CKEY_MAXVAL,    /* RGB or CrYCb format according to config */

  V4L2_CID_STM_MIXER_BACKGROUND_ARGB, /* mixer background in ARGB */
};

/*
 * Display standard IDs for all output display modes not covered by the
 * vanilla API.
 */
#define V4L2_STD_VGA_60           ((v4l2_std_id)0x0000000100000000ULL)
#define V4L2_STD_VGA_59_94        ((v4l2_std_id)0x0000000200000000ULL)
#define V4L2_STD_480P_60          ((v4l2_std_id)0x0000000400000000ULL)
#define V4L2_STD_480P_59_94       ((v4l2_std_id)0x0000000800000000ULL)
#define V4L2_STD_576P_50          ((v4l2_std_id)0x0000001000000000ULL)
#define V4L2_STD_1080P_60         ((v4l2_std_id)0x0000002000000000ULL)
#define V4L2_STD_1080P_59_94      ((v4l2_std_id)0x0000004000000000ULL)
#define V4L2_STD_1080P_50         ((v4l2_std_id)0x0000008000000000ULL)
#define V4L2_STD_1080P_23_98      ((v4l2_std_id)0x0000010000000000ULL)
#define V4L2_STD_1080P_24         ((v4l2_std_id)0x0000020000000000ULL)
#define V4L2_STD_1080P_25         ((v4l2_std_id)0x0000040000000000ULL)
#define V4L2_STD_1080P_29_97      ((v4l2_std_id)0x0000080000000000ULL)
#define V4L2_STD_1080P_30         ((v4l2_std_id)0x0000100000000000ULL)
#define V4L2_STD_1080I_60         ((v4l2_std_id)0x0000200000000000ULL)
#define V4L2_STD_1080I_59_94      ((v4l2_std_id)0x0000400000000000ULL)
#define V4L2_STD_1080I_50         ((v4l2_std_id)0x0000800000000000ULL)
#define V4L2_STD_720P_60          ((v4l2_std_id)0x0001000000000000ULL)
#define V4L2_STD_720P_59_94       ((v4l2_std_id)0x0002000000000000ULL)
#define V4L2_STD_720P_50          ((v4l2_std_id)0x0004000000000000ULL)

#define V4L2_STD_SMPTE293M (V4L2_STD_480P_60    | \
                            V4L2_STD_480P_59_94 | \
                            V4L2_STD_576P_50)

#define V4L2_STD_SMPTE274M (V4L2_STD_1080P_60    | \
                            V4L2_STD_1080P_59_94 | \
                            V4L2_STD_1080P_50    | \
                            V4L2_STD_1080P_23_98 | \
                            V4L2_STD_1080P_24    | \
                            V4L2_STD_1080P_25    | \
                            V4L2_STD_1080P_29_97 | \
                            V4L2_STD_1080P_30    | \
                            V4L2_STD_1080I_60    | \
                            V4L2_STD_1080I_59_94 | \
                            V4L2_STD_1080I_50)

#define V4L2_STD_SMPTE296M (V4L2_STD_720P_60    | \
                            V4L2_STD_720P_59_94 | \
                            V4L2_STD_720P_50)

#define V4L2_STD_HD        (V4L2_STD_SMPTE274M | \
                            V4L2_STD_SMPTE296M)

#define V4L2_STD_VESA      (V4L2_STD_VGA_60    | \
                            V4L2_STD_VGA_59_94)

#define V4L2_STD_CEA861    (V4L2_STD_525_60    | \
                            V4L2_STD_625_50    | \
                            V4L2_STD_VGA_60    | \
                            V4L2_STD_VGA_59_94 | \
                            V4L2_STD_SMPTE293M | \
                            V4L2_STD_SMPTE274M | \
                            V4L2_STD_SMPTE296M)

#define V4L2_STD_INTERLACED (V4L2_STD_525_60      | \
                             V4L2_STD_625_50      | \
                             V4L2_STD_1080I_60    | \
                             V4L2_STD_1080I_59_94 | \
                             V4L2_STD_1080I_50)

#define V4L2_STD_PROGRESSIVE (V4L2_STD_VGA_60      | \
                              V4L2_STD_VGA_59_94   | \
                              V4L2_STD_SMPTE293M   | \
                              V4L2_STD_1080P_60    | \
                              V4L2_STD_1080P_59_94 | \
                              V4L2_STD_1080P_50    | \
                              V4L2_STD_1080P_23_98 | \
                              V4L2_STD_1080P_24    | \
                              V4L2_STD_1080P_25    | \
                              V4L2_STD_1080P_29_97 | \
                              V4L2_STD_1080P_30    | \
                              V4L2_STD_SMPTE296M)

/*
 * The standard V4L2 IOCTLs for standards (no pun intended) are only defined
 * for input. So we have to define a parallel set, for controlling the display
 * pipeline output, which have exactly the same behaviour.
 */
#define VIDIOC_G_OUTPUT_STD    _IOR  ('V', BASE_VIDIOC_PRIVATE,   v4l2_std_id)
#define VIDIOC_S_OUTPUT_STD    _IOW  ('V', BASE_VIDIOC_PRIVATE+1, v4l2_std_id)
#define VIDIOC_ENUM_OUTPUT_STD _IOWR ('V', BASE_VIDIOC_PRIVATE+2, struct v4l2_standard)

#define VIDIOC_S_OUTPUT_ALPHA  _IOW  ('V', BASE_VIDIOC_PRIVATE+3, unsigned int)


/* migration helpers. Scheduled for removal soon. Do not use
   for new code + migrate existing code! */
#if !defined(__KERNEL__)
static inline int
__attribute__((deprecated,const))
deprecated_V4L2_CID_STM_VOUT_CONTROL (__u32 control)
{
  return control;
}
#define V4L2_CID_STM_IQI_SET_CONFIG_VID0          deprecated_V4L2_CID_STM_VOUT_CONTROL (_deprecated_V4L2_CID_STM_IQI_SET_CONFIG_VID0)
#define V4L2_CID_STM_IQI_DEMO_VID0                deprecated_V4L2_CID_STM_VOUT_CONTROL (_deprecated_V4L2_CID_STM_IQI_DEMO_VID0)
#define V4L2_CID_STM_XVP_SET_CONFIG_VID0          deprecated_V4L2_CID_STM_VOUT_CONTROL (_deprecated_V4L2_CID_STM_XVP_SET_CONFIG_VID0)
#define V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0 deprecated_V4L2_CID_STM_VOUT_CONTROL (_deprecated_V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0)
#define V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0  deprecated_V4L2_CID_STM_VOUT_CONTROL (_deprecated_V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0)
#define V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0      deprecated_V4L2_CID_STM_VOUT_CONTROL (_deprecated_V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0)
#define V4L2_CID_STM_DEI_SET_MODE_VID0            deprecated_V4L2_CID_STM_VOUT_CONTROL (_deprecated_V4L2_CID_STM_DEI_SET_MODE_VID0)
#define V4L2_CID_STM_DEI_SET_CTRLREG_VID0         deprecated_V4L2_CID_STM_VOUT_CONTROL (_deprecated_V4L2_CID_STM_DEI_SET_CTRLREG_VID0)
#else /* __KERNEL__ */
#define V4L2_CID_STM_IQI_SET_CONFIG_VID0          _deprecated_V4L2_CID_STM_IQI_SET_CONFIG_VID0
#define V4L2_CID_STM_IQI_DEMO_VID0                _deprecated_V4L2_CID_STM_IQI_DEMO_VID0
#define V4L2_CID_STM_XVP_SET_CONFIG_VID0          _deprecated_V4L2_CID_STM_XVP_SET_CONFIG_VID0
#define V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0 _deprecated_V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0
#define V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0  _deprecated_V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0
#define V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0      _deprecated_V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0
#define V4L2_CID_STM_DEI_SET_MODE_VID0            _deprecated_V4L2_CID_STM_DEI_SET_MODE_VID0
#define V4L2_CID_STM_DEI_SET_CTRLREG_VID0         _deprecated_V4L2_CID_STM_DEI_SET_CTRLREG_VID0
#endif /* __KERNEL__ */



#endif /* __STMVOUT_H */
