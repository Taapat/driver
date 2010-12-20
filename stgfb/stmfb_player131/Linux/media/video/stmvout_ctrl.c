/***********************************************************************
 *
 * File: stgfb/Linux/media/video/stmvout_ctrl.c
 * Copyright (c) 2009 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * V4L2 video output device driver for ST SoC display subsystems.
 *
 ***********************************************************************/

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-common.h>

#include <stmdisplay.h>

#include <linux/stm/stmcoredisplay.h>

#include "stmvout_driver.h"
#include "stmvout.h"
#include "stm_v4l2.h"

static unsigned long
stmvout_get_ctrl_name (int id)
{
  switch (id) {
  case V4L2_CID_BRIGHTNESS:
    return STM_CTRL_BRIGHTNESS;
  case V4L2_CID_CONTRAST:
    return STM_CTRL_CONTRAST;
  case V4L2_CID_SATURATION:
    return STM_CTRL_SATURATION;
  case V4L2_CID_HUE:
    return STM_CTRL_HUE;

  case V4L2_CID_STM_IQI_SET_CONFIG:
  case V4L2_CID_STM_IQI_SET_CONFIG_VID0:
    return PLANE_CTRL_IQI_CONFIG;
  case V4L2_CID_STM_IQI_DEMO:
  case V4L2_CID_STM_IQI_DEMO_VID0:
    return PLANE_CTRL_IQI_DEMO;
  case V4L2_CID_STM_XVP_SET_CONFIG:
  case V4L2_CID_STM_XVP_SET_CONFIG_VID0:
    return PLANE_CTRL_XVP_CONFIG;
  case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE:
  case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0:
    return PLANE_CTRL_XVP_TNRNLE_OVERRIDE;
  case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP:
  case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0:
    return PLANE_CTRL_XVP_TNR_TOPBOT;
  case V4L2_CID_STM_DEI_SET_FMD_ENABLE:
  case V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0:
    return PLANE_CTRL_DEI_FMD_ENABLE;
  case V4L2_CID_STM_DEI_SET_MODE:
  case V4L2_CID_STM_DEI_SET_MODE_VID0:
    return PLANE_CTRL_DEI_MODE;
  case V4L2_CID_STM_DEI_SET_CTRLREG:
  case V4L2_CID_STM_DEI_SET_CTRLREG_VID0:
    return PLANE_CTRL_DEI_CTRLREG;

  case V4L2_CID_STM_PLANE_BRIGTHNESS:
    return PLANE_CTRL_PSI_BRIGHTNESS;
  case V4L2_CID_STM_PLANE_CONTRAST:
    return PLANE_CTRL_PSI_CONTRAST;
  case V4L2_CID_STM_PLANE_SATURATION:
    return PLANE_CTRL_PSI_SATURATION;
  case V4L2_CID_STM_PLANE_HUE:
    return PLANE_CTRL_PSI_TINT;

  case V4L2_CID_STM_MIXER_BACKGROUND_ARGB:
    return STM_CTRL_BACKGROUND_ARGB;

  case V4L2_CID_STM_CKEY_ENABLE:
  case V4L2_CID_STM_CKEY_FORMAT:
  case V4L2_CID_STM_CKEY_R_CR_MODE:
  case V4L2_CID_STM_CKEY_G_Y_MODE:
  case V4L2_CID_STM_CKEY_B_CB_MODE:
  case V4L2_CID_STM_CKEY_MINVAL:
  case V4L2_CID_STM_CKEY_MAXVAL:
  case V4L2_CID_STM_Z_ORDER_FIRST ... V4L2_CID_STM_Z_ORDER_LAST:
    return 1000;

  default:
    return 0;
  }
}


static const u32 stmvout_user_ctrls[] = {
  V4L2_CID_USER_CLASS,
  V4L2_CID_BRIGHTNESS,
  V4L2_CID_CONTRAST,
  V4L2_CID_SATURATION,
  V4L2_CID_HUE,
  0
};

static const u32 stmvout_stmvout_ctrls[] = {
  V4L2_CID_STMVOUT_CLASS,

  V4L2_CID_STM_IQI_SET_CONFIG,
  V4L2_CID_STM_IQI_DEMO,
  V4L2_CID_STM_XVP_SET_CONFIG,
  V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE,
  V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP,
  V4L2_CID_STM_DEI_SET_FMD_ENABLE,
  V4L2_CID_STM_DEI_SET_MODE,
  V4L2_CID_STM_DEI_SET_CTRLREG,

  V4L2_CID_STM_Z_ORDER_VID0,
  V4L2_CID_STM_Z_ORDER_VID1,
  V4L2_CID_STM_Z_ORDER_RGB0,
  V4L2_CID_STM_Z_ORDER_RGB1,
  V4L2_CID_STM_Z_ORDER_RGB2,

  V4L2_CID_STM_Z_ORDER_RGB1_0,
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

  0
};

static const u32 stmvout_stmvout_extctrls[] = {
  V4L2_CID_STMVOUTEXT_CLASS,

  V4L2_CID_STM_CKEY_ENABLE,
  V4L2_CID_STM_CKEY_FORMAT,
  V4L2_CID_STM_CKEY_R_CR_MODE,
  V4L2_CID_STM_CKEY_G_Y_MODE,
  V4L2_CID_STM_CKEY_B_CB_MODE,
  V4L2_CID_STM_CKEY_MINVAL,
  V4L2_CID_STM_CKEY_MAXVAL,

  V4L2_CID_STM_MIXER_BACKGROUND_ARGB,

  0
};


/*
 * This needs to be exported for the V4L2 driver structure.
 */
const u32 *stmvout_ctrl_classes[] = {
  stmvout_user_ctrls,
  stmvout_stmvout_ctrls,
  stmvout_stmvout_extctrls,
  NULL
};


static struct _stvout_device *
_get_device_by_id (struct _stvout_device pipeline_devs[],
                   stm_plane_id_t        id)
{
  int plane;

  if (pipeline_devs)
    for (plane = 0; plane < MAX_PLANES; ++plane)
      if (pipeline_devs[plane].planeConfig->id == id)
        return &pipeline_devs[plane];

  return NULL;
}


int
stmvout_queryctrl (struct _stvout_device * const pDev,
                   struct _stvout_device  pipeline_devs[],
                   struct v4l2_queryctrl * const pqc)
{
  unsigned long          output_caps;
  struct _stvout_device *this_dev = pDev;
  int                    ctrl_supported = 1;

  pqc->id = v4l2_ctrl_next (stmvout_ctrl_classes, pqc->id);
  if (!pqc->id)
    return -EINVAL;

  BUG_ON (pDev == NULL);

  if (stm_display_output_get_control_capabilities (pDev->pOutput,
                                                   &output_caps) < 0)
    return -ERESTARTSYS;

  switch (pqc->id) {
  case V4L2_CID_USER_CLASS:
    stm_v4l2_ctrl_query_fill (pqc, "User controls", 0, 0, 0, 0,
                              V4L2_CTRL_TYPE_CTRL_CLASS,
                              V4L2_CTRL_FLAG_READ_ONLY);
    break;
  case V4L2_CID_STMVOUT_CLASS:
    stm_v4l2_ctrl_query_fill (pqc, "STMvout controls", 0, 0, 0, 0,
                              V4L2_CTRL_TYPE_CTRL_CLASS,
                              V4L2_CTRL_FLAG_READ_ONLY);
    break;
  case V4L2_CID_STMVOUTEXT_CLASS:
    stm_v4l2_ctrl_query_fill (pqc, "STMvout ext controls", 0, 0, 0, 0,
                              V4L2_CTRL_TYPE_CTRL_CLASS,
                              V4L2_CTRL_FLAG_READ_ONLY);
    break;

  case V4L2_CID_BRIGHTNESS:
    stm_v4l2_ctrl_query_fill (pqc, "Brightness",
                              0, 255, 1, 128, V4L2_CTRL_TYPE_INTEGER,
                              V4L2_CTRL_FLAG_SLIDER);
    ctrl_supported = output_caps & STM_CTRL_CAPS_BRIGHTNESS;
    break;

  case V4L2_CID_CONTRAST:
    stm_v4l2_ctrl_query_fill (pqc, "Contrast",
                              0, 255, 1, 0, V4L2_CTRL_TYPE_INTEGER,
                              V4L2_CTRL_FLAG_SLIDER);
    ctrl_supported = output_caps & STM_CTRL_CAPS_CONTRAST;
    break;

  case V4L2_CID_SATURATION:
    stm_v4l2_ctrl_query_fill (pqc, "Saturation",
                              0, 255, 1, 128, V4L2_CTRL_TYPE_INTEGER,
                              V4L2_CTRL_FLAG_SLIDER);
    ctrl_supported = output_caps & STM_CTRL_CAPS_SATURATION;
    break;

  case V4L2_CID_HUE:
    stm_v4l2_ctrl_query_fill (pqc, "Hue",
                              0, 255, 1, 128, V4L2_CTRL_TYPE_INTEGER,
                              V4L2_CTRL_FLAG_SLIDER);
    ctrl_supported = output_caps & STM_CTRL_CAPS_HUE;
    break;

  case V4L2_CID_STM_Z_ORDER_VID0 ... V4L2_CID_STM_Z_ORDER_RGB2:
    {
      static const char * const name_map[] = { "Z order for plane YUV0",
                                               "Z order for plane YUV1",
                                               "Z order for plane RGB0",
                                               "Z order for plane RGB1",
                                               "Z order for plane RGB2"
      };
      int depth;
      stm_v4l2_ctrl_query_fill (pqc,
                                name_map[pqc->id - V4L2_CID_STM_Z_ORDER_VID0],
                                0, 4, 1, pqc->id - V4L2_CID_STM_Z_ORDER_VID0,
                                V4L2_CTRL_TYPE_INTEGER, 0);
      ctrl_supported = stm_display_plane_get_depth (this_dev->pPlane,
                                                    this_dev->pOutput,
                                                    &depth);
      if (ctrl_supported < 0)
        return signal_pending (current) ? -ERESTARTSYS : -EBUSY;
    }
    break;

  /* the v plane controls are unsupported atm */
  case V4L2_CID_STM_Z_ORDER_RGB1_0 ... V4L2_CID_STM_Z_ORDER_RGB1_15:
    {
      static const char * const name_map[] = { "Z order for v plane RGB1.0",
                                               "Z order for v plane RGB1.1",
                                               "Z order for v plane RGB1.2",
                                               "Z order for v plane RGB1.3",
                                               "Z order for v plane RGB1.4",
                                               "Z order for v plane RGB1.5",
                                               "Z order for v plane RGB1.6",
                                               "Z order for v plane RGB1.7",
                                               "Z order for v plane RGB1.8",
                                               "Z order for v plane RGB1.9",
                                               "Z order for v plane RGB1.10",
                                               "Z order for v plane RGB1.11",
                                               "Z order for v plane RGB1.12",
                                               "Z order for v plane RGB1.13",
                                               "Z order for v plane RGB1.14",
                                               "Z order for v plane RGB1.15"
      };
      stm_v4l2_ctrl_query_fill (pqc,
                                name_map[pqc->id - V4L2_CID_STM_Z_ORDER_RGB1_0],
                                0, 15, 1, pqc->id - V4L2_CID_STM_Z_ORDER_RGB1_0,
                                V4L2_CTRL_TYPE_INTEGER, V4L2_CTRL_FLAG_DISABLED);
      ctrl_supported = 0;
    }
    break;

  case V4L2_CID_STM_IQI_SET_CONFIG_VID0:
  case V4L2_CID_STM_IQI_DEMO_VID0:
  case V4L2_CID_STM_XVP_SET_CONFIG_VID0:
  case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0:
  case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0:
  case V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0:
  case V4L2_CID_STM_DEI_SET_MODE_VID0:
  case V4L2_CID_STM_DEI_SET_CTRLREG_VID0:
    this_dev = _get_device_by_id (pipeline_devs, OUTPUT_VID1);
    if (!this_dev)
      return -ENODEV;
    if (this_dev != pDev && down_interruptible (&this_dev->devLock))
      return -ERESTARTSYS;
    /* fall through */

  case V4L2_CID_STM_IQI_SET_CONFIG:
  case V4L2_CID_STM_IQI_DEMO:
  case V4L2_CID_STM_XVP_SET_CONFIG:
  case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE:
  case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP:
  case V4L2_CID_STM_DEI_SET_FMD_ENABLE:
  case V4L2_CID_STM_DEI_SET_MODE:
  case V4L2_CID_STM_DEI_SET_CTRLREG:

  case V4L2_CID_STM_PLANE_BRIGTHNESS:
  case V4L2_CID_STM_PLANE_CONTRAST:
  case V4L2_CID_STM_PLANE_SATURATION:
  case V4L2_CID_STM_PLANE_HUE:
    {
      stm_plane_caps_t planeCaps;
      int              ret;

      if (!this_dev->pPlane || !this_dev->pOutput) {
        if (this_dev != pDev)
          up (&this_dev->devLock);
        return -EINVAL;
      }

      ret = stm_display_plane_get_capabilities (this_dev->pPlane, &planeCaps);

      if (this_dev != pDev)
        up (&this_dev->devLock);

      if (ret < 0)
        return signal_pending (current) ? -ERESTARTSYS : -EBUSY;

      switch (pqc->id) {
      case V4L2_CID_STM_IQI_SET_CONFIG:
      case V4L2_CID_STM_IQI_SET_CONFIG_VID0:
        stm_v4l2_ctrl_query_fill (pqc, "IQI config",
                                  PCIQIC_FIRST, PCIQIC_COUNT - 1, 1,
                                  PCIQIC_BYPASS,
                                  V4L2_CTRL_TYPE_MENU, 0);
        ctrl_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_IQI;
        if (pqc->id == V4L2_CID_STM_IQI_SET_CONFIG_VID0)
          snprintf (pqc->name, sizeof (pqc->name), "IQI config on VID0");
        break;

      case V4L2_CID_STM_IQI_DEMO:
      case V4L2_CID_STM_IQI_DEMO_VID0:
        stm_v4l2_ctrl_query_fill (pqc, "IQI demo",
                                  VCSIDM_OFF, VCSIDM_ON, 1, VCSIDM_OFF,
				  V4L2_CTRL_TYPE_BOOLEAN, 0);
        ctrl_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_IQI;
        if (pqc->id == V4L2_CID_STM_IQI_DEMO_VID0)
          snprintf (pqc->name, sizeof (pqc->name), "IQI demo on VID0");
        break;

      case V4L2_CID_STM_XVP_SET_CONFIG:
      case V4L2_CID_STM_XVP_SET_CONFIG_VID0:
        stm_v4l2_ctrl_query_fill (pqc, "xVP config",
                                  PCxVPC_FIRST, PCxVPC_COUNT - 1, 1,
                                  PCxVPC_BYPASS, V4L2_CTRL_TYPE_MENU, 0);
        ctrl_supported = planeCaps.ulControls & (PLANE_CTRL_CAPS_FILMGRAIN
                                                 | PLANE_CTRL_CAPS_TNR);
        if (pqc->id == V4L2_CID_STM_XVP_SET_CONFIG_VID0)
          snprintf (pqc->name, sizeof (pqc->name), "xVP config on VID0");
        break;

      case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE:
      case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0:
        stm_v4l2_ctrl_query_fill (pqc, "NLE override",
                                  0, 255, 1, 0, V4L2_CTRL_TYPE_INTEGER, 0);
        ctrl_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_TNR;
        if (pqc->id == V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0)
          snprintf (pqc->name, sizeof (pqc->name), "NLE override on VID0");
        break;

      case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP:
      case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0:
        stm_v4l2_ctrl_query_fill (pqc, "TNR Top/Bottom swap",
                                  0, 1, 1, 0, V4L2_CTRL_TYPE_BOOLEAN, 0);
        ctrl_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_TNR;
        if (pqc->id == V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0)
          snprintf (pqc->name, sizeof (pqc->name), "TNR Top/Bottom swap on VID0");
        break;

      case V4L2_CID_STM_DEI_SET_FMD_ENABLE:
      case V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0:
        stm_v4l2_ctrl_query_fill (pqc, "FMD enable",
                                  VCSDSFEM_OFF, VCSDSFEM_ON, 1, VCSDSFEM_ON,
				  V4L2_CTRL_TYPE_BOOLEAN, 0);
        ctrl_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_DEINTERLACE;
        if (pqc->id == V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0)
          snprintf (pqc->name, sizeof (pqc->name), "FMD enable on VID0");
        break;

      case V4L2_CID_STM_DEI_SET_MODE:
      case V4L2_CID_STM_DEI_SET_MODE_VID0:
        stm_v4l2_ctrl_query_fill (pqc, "DEI enable",
                                  VCSDSMM_FIRST, VCSDSMM_COUNT - 1, 1,
                                  VCSDSMM_3DMOTION, V4L2_CTRL_TYPE_MENU, 0);
        ctrl_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_DEINTERLACE;
        if (pqc->id == V4L2_CID_STM_DEI_SET_MODE_VID0)
          snprintf (pqc->name, sizeof (pqc->name), "DEI enable on VID0");
        break;

      case V4L2_CID_STM_DEI_SET_CTRLREG:
      case V4L2_CID_STM_DEI_SET_CTRLREG_VID0:
        stm_v4l2_ctrl_query_fill (pqc, "DEI ctrl register",
                                  INT_MIN, INT_MAX, 1,
                                  0, V4L2_CTRL_TYPE_INTEGER, 0);
        ctrl_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_DEINTERLACE;
        if (pqc->id == V4L2_CID_STM_DEI_SET_CTRLREG_VID0)
          snprintf (pqc->name, sizeof (pqc->name), "DEI ctrl register on VID0");
        break;

      case V4L2_CID_STM_PLANE_BRIGTHNESS:
      case V4L2_CID_STM_PLANE_CONTRAST:
      case V4L2_CID_STM_PLANE_SATURATION:
      case V4L2_CID_STM_PLANE_HUE:
        {
          static const char * const name_map[] = { "Plane Brightness",
                                                   "Plane Contrast",
                                                   "Plane Saturation",
                                                   "Plane Hue"
          };
          stm_v4l2_ctrl_query_fill (pqc,
                                    name_map[pqc->id - V4L2_CID_STM_PLANE_BRIGTHNESS],
                                    0, 255, 1,
                                    128, V4L2_CTRL_TYPE_INTEGER,
                                    V4L2_CTRL_FLAG_SLIDER);
          ctrl_supported = planeCaps.ulControls & PLANE_CTRL_CAPS_PSI_CONTROLS;
        }
        break;

      default:
        /* shouldn't be reached */
        BUG ();
        return -EINVAL;
      }
    }
    break;

  /* we assume color key is always supported */
  case V4L2_CID_STM_CKEY_ENABLE:
    stm_v4l2_ctrl_query_fill (pqc,
                              "ColorKey", 0, 3, 1, 0, V4L2_CTRL_TYPE_MENU, 0);
    break;
  case V4L2_CID_STM_CKEY_FORMAT:
    stm_v4l2_ctrl_query_fill (pqc,
                              "ColorKey format", 0, 1, 1, 0, V4L2_CTRL_TYPE_MENU,
                              0);
    break;
  case V4L2_CID_STM_CKEY_R_CR_MODE:
  case V4L2_CID_STM_CKEY_G_Y_MODE:
  case V4L2_CID_STM_CKEY_B_CB_MODE:
    {
      static const char * const name_map[] = { "ColorKey R/Cr mode",
                                               "ColorKey G/Y mode",
                                               "ColorKey B/Cb mode"
      };
      stm_v4l2_ctrl_query_fill (pqc,
                                name_map[pqc->id - V4L2_CID_STM_CKEY_R_CR_MODE],
                                0, 3, 1, 0, V4L2_CTRL_TYPE_MENU, 0);
    }
    break;
  case V4L2_CID_STM_CKEY_MINVAL:
  case V4L2_CID_STM_CKEY_MAXVAL:
    {
      static const char * const name_map[] = { "ColorKey minval",
                                               "ColorKey maxval"
      };
      stm_v4l2_ctrl_query_fill (pqc,
                                name_map[pqc->id - V4L2_CID_STM_CKEY_MINVAL],
                                INT_MIN, INT_MAX, 1, 0, V4L2_CTRL_TYPE_INTEGER,
                                0);
    }
    break;

  case V4L2_CID_STM_MIXER_BACKGROUND_ARGB:
    stm_v4l2_ctrl_query_fill (pqc, "Mixer Background",
                              INT_MIN, INT_MAX, 1,
                              0x101060, /* CGammaMixer.cpp: DEFAULT_BKG_COLOR */
                              V4L2_CTRL_TYPE_INTEGER,
                              0);
    ctrl_supported = output_caps & STM_CTRL_CAPS_BACKGROUND;
    break;

  default:
    return -EINVAL;
  }

  if (!ctrl_supported)
    pqc->flags |= V4L2_CTRL_FLAG_DISABLED;

  return 0;
}


static const char * const *
stmvout_ctrl_get_menu (u32 id)
{
  static const char * const iqi_config[PCIQIC_COUNT] = {
    [PCIQIC_BYPASS]     = "bypass",
    [PCIQIC_ST_DEFAULT] = "default"
  };
  static const char * const xvp_config[PCxVPC_COUNT] = {
    [PCxVPC_BYPASS]      = "bypass",
    [PCxVPC_FILMGRAIN]   = "filmgrain",
    [PCxVPC_TNR]         = "tnr",
    [PCxVPC_TNR_BYPASS]  = "tnr (bypass)",
    [PCxVPC_TNR_NLEOVER] = "tnr (NLE override)"
  };
  static const char * const dei_mode[PCDEIC_COUNT] = {
    [PCDEIC_3DMOTION] = "enable (3D)",
    [PCDEIC_DISABLED] = "disable",
    [PCDEIC_MEDIAN]   = "enable (median)"
  };
  static const char * const colorkey_enable[] = {
    [VCSCEF_DISABLED]                         = "disable",
    [VCSCEF_ENABLED]                          = "enable on next VSync",
    [VCSCEF_ACTIVATE_BUFFER]                  = "disable",
    [VCSCEF_ACTIVATE_BUFFER | VCSCEF_ENABLED] = "enable on next buffer"
  };
  static const char * const colorkey_format[] = {
    [VCSCFM_RGB]   = "RGB",
    [VCSCFM_CrYCb] = "CrYCb"
  };
  static const char * const colorkey_mode[] = {
    [VCSCCCM_DISABLED]                   = "ignore",
    [VCSCCCM_ENABLED]                    = "enable",
    [VCSCCCM_INVERSE & ~VCSCCCM_ENABLED] = "ignore",
    [VCSCCCM_INVERSE]                    = "inverse"
  };

  switch (id) {
  case V4L2_CID_STM_IQI_SET_CONFIG:
  case V4L2_CID_STM_IQI_SET_CONFIG_VID0:
    return iqi_config;
  case V4L2_CID_STM_XVP_SET_CONFIG:
  case V4L2_CID_STM_XVP_SET_CONFIG_VID0:
    return xvp_config;
  case V4L2_CID_STM_DEI_SET_MODE:
  case V4L2_CID_STM_DEI_SET_MODE_VID0:
    return dei_mode;
  case V4L2_CID_STM_CKEY_ENABLE:
    return colorkey_enable;
  case V4L2_CID_STM_CKEY_FORMAT:
    return colorkey_format;
  case V4L2_CID_STM_CKEY_R_CR_MODE:
  case V4L2_CID_STM_CKEY_G_Y_MODE:
  case V4L2_CID_STM_CKEY_B_CB_MODE:
    return colorkey_mode;
  }

  return NULL;
}


int
stmvout_querymenu (struct _stvout_device * const device,
                   struct _stvout_device   pipeline_devs[],
                   struct v4l2_querymenu * const qmenu)
{
  struct v4l2_queryctrl qctrl;

  qctrl.id = qmenu->id;
  stmvout_queryctrl (device, pipeline_devs, &qctrl);
  return v4l2_ctrl_query_menu (qmenu, &qctrl,
                               (const char **) stmvout_ctrl_get_menu (qmenu->id));
}


static int
stmvout_write_state (struct _stvout_device_state   * const state,
                     const struct v4l2_ext_control * const ctrl)
{
  switch (ctrl->id) {
  case V4L2_CID_STM_CKEY_ENABLE:
    state->ckey.flags |= SCKCF_ENABLE;
    state->ckey.enable = ctrl->value;
    break;

  case V4L2_CID_STM_CKEY_FORMAT:
    state->ckey.flags |= SCKCF_FORMAT;
    state->ckey.format = ctrl->value;
    break;

  case V4L2_CID_STM_CKEY_R_CR_MODE:
    state->ckey.flags |= SCKCF_R_INFO;
    state->ckey.r_info = ctrl->value;
    break;

  case V4L2_CID_STM_CKEY_G_Y_MODE:
    state->ckey.flags |= SCKCF_G_INFO;
    state->ckey.g_info = ctrl->value;
    break;

  case V4L2_CID_STM_CKEY_B_CB_MODE:
    state->ckey.flags |= SCKCF_B_INFO;
    state->ckey.b_info = ctrl->value;
    break;

  case V4L2_CID_STM_CKEY_MINVAL:
    state->ckey.flags |= SCKCF_MINVAL;
    state->ckey.minval = ctrl->value;
    break;

  case V4L2_CID_STM_CKEY_MAXVAL:
    state->ckey.flags |= SCKCF_MAXVAL;
    state->ckey.maxval = ctrl->value;
    break;

  case V4L2_CID_STM_MIXER_BACKGROUND_ARGB:
    state->new_mixer_background_argb = (u32) ctrl->value;
    break;

  default:
    return -EINVAL;
  }

  return 0;
}


static int
stmvout_read_state (struct v4l2_ext_control           * const ctrl,
                    const struct _stvout_device_state * const state)
{
  switch (ctrl->id) {
  case V4L2_CID_STM_CKEY_ENABLE:    ctrl->value = state->ckey.enable; break;
  case V4L2_CID_STM_CKEY_FORMAT:    ctrl->value = state->ckey.format; break;
  case V4L2_CID_STM_CKEY_R_CR_MODE: ctrl->value = state->ckey.r_info; break;
  case V4L2_CID_STM_CKEY_G_Y_MODE:  ctrl->value = state->ckey.g_info; break;
  case V4L2_CID_STM_CKEY_B_CB_MODE: ctrl->value = state->ckey.b_info; break;
  case V4L2_CID_STM_CKEY_MINVAL:    ctrl->value = state->ckey.minval; break;
  case V4L2_CID_STM_CKEY_MAXVAL:    ctrl->value = state->ckey.maxval; break;

  case V4L2_CID_STM_MIXER_BACKGROUND_ARGB:
    ctrl->value = state->old_mixer_background_argb;
    break;

  default:
    return -EINVAL;
  }

  return 0;
}


static int
stmvout_apply_state (struct _stvout_device       *device,
                     struct _stvout_device_state * const state)
{
  int ret = 0;

  if (device->pPlane) {
    struct StmColorKeyConfig ckey;

    /* color key */
    /* if state is now disabled, or state changed from activate_plane to
       activate_buffer, clear the setting on the plane. */
    if (state->ckey.flags & SCKCF_ENABLE
        && (!(state->ckey.enable & VCSCEF_ENABLED)
            || ((state->ckey.enable & VCSCEF_ACTIVATE_BUFFER)
                && !(device->state.ckey.enable & VCSCEF_ACTIVATE_BUFFER)))) {
      memset (&ckey, 0, sizeof (struct StmColorKeyConfig));
      ckey.flags = SCKCF_ALL;
      ret = stm_display_plane_set_control (device->pPlane,
                                           PLANE_CTRL_COLOR_KEY,
                                           (unsigned long) &ckey);
    } else if (!(state->ckey.enable & VCSCEF_ACTIVATE_BUFFER)) {
      /* otherwise just set it (if not per buffer) */
      ckey = state->ckey;
      ckey.flags &= ~VCSCEF_ACTIVATE_BUFFER;
      ret = stm_display_plane_set_control (device->pPlane,
                                           PLANE_CTRL_COLOR_KEY,
                                           (unsigned long) &ckey);
    }
    if (ret)
      return signal_pending (current) ? -ERESTARTSYS : -EBUSY;

    if (state->old_mixer_background_argb != state->new_mixer_background_argb) {
      ret = stm_display_output_set_control (device->pOutput,
                                            STM_CTRL_BACKGROUND_ARGB,
                                            state->new_mixer_background_argb);
      if (ret < 0)
        return signal_pending (current) ? -ERESTARTSYS : -EBUSY;
      state->old_mixer_background_argb = state->new_mixer_background_argb;
    }
  }

  return ret;
}


int
stmvout_s_ctrl (struct _stvout_device * const pDev,
                struct _stvout_device   pipeline_devs[],
                struct v4l2_control    * const pctrl)
{
  ULONG                  ctrlname;
  struct _stvout_device *this_dev = pDev;
  int                    ret;

  BUG_ON (pDev == NULL);

  ctrlname = stmvout_get_ctrl_name (pctrl->id);
  if (ctrlname == 0)
    return -EINVAL;

  switch (pctrl->id) {
  case V4L2_CID_STM_IQI_SET_CONFIG_VID0:
  case V4L2_CID_STM_IQI_DEMO_VID0:
  case V4L2_CID_STM_XVP_SET_CONFIG_VID0:
  case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0:
  case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0:
  case V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0:
  case V4L2_CID_STM_DEI_SET_MODE_VID0:
  case V4L2_CID_STM_DEI_SET_CTRLREG_VID0:
    /* don't be confused by OUTPUT_VID1, the naming scheme is different... */
    this_dev = _get_device_by_id (pipeline_devs, OUTPUT_VID1);
    if (!this_dev)
      return -ENODEV;
    if (this_dev != pDev && down_interruptible (&this_dev->devLock))
      return -ERESTARTSYS;
    /* fall through */

  case V4L2_CID_STM_IQI_SET_CONFIG:
  case V4L2_CID_STM_IQI_DEMO:
  case V4L2_CID_STM_XVP_SET_CONFIG:
  case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE:
  case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP:
  case V4L2_CID_STM_DEI_SET_FMD_ENABLE:
  case V4L2_CID_STM_DEI_SET_MODE:
  case V4L2_CID_STM_DEI_SET_CTRLREG:

  case V4L2_CID_STM_PLANE_BRIGTHNESS:
  case V4L2_CID_STM_PLANE_CONTRAST:
  case V4L2_CID_STM_PLANE_SATURATION:
  case V4L2_CID_STM_PLANE_HUE:
    if (!this_dev->pPlane) {
      if (this_dev != pDev)
        up (&this_dev->devLock);
      return -EINVAL;
    }

    ret = stm_display_plane_set_control (this_dev->pPlane,
                                         ctrlname, pctrl->value);

    if (this_dev != pDev)
      up (&this_dev->devLock);

    if (ret < 0)
      return signal_pending (current) ? -ERESTARTSYS : -EBUSY;
    break;

  case V4L2_CID_STM_Z_ORDER_FIRST ... V4L2_CID_STM_Z_ORDER_LAST:
    {
      static const stm_plane_id_t id_map[] = {
        OUTPUT_VID1, OUTPUT_VID2,
        OUTPUT_GDP1, OUTPUT_GDP2, OUTPUT_GDP3
      };

      if (pctrl->id > V4L2_CID_STM_Z_ORDER_RGB2)
        return -EINVAL;

      this_dev = _get_device_by_id (pipeline_devs,
                                    id_map[pctrl->id - V4L2_CID_STM_Z_ORDER_VID0]);
      if (!this_dev)
        return -EINVAL;

      if (this_dev != pDev && down_interruptible (&this_dev->devLock))
        return -ERESTARTSYS;

      if (!this_dev->pPlane || !this_dev->pOutput) {
        if (this_dev != pDev)
          up (&this_dev->devLock);
        return -EINVAL;
      }

      ret = stm_display_plane_set_depth (this_dev->pPlane, this_dev->pOutput,
                                         pctrl->value, 1);

      if (this_dev != pDev)
        up (&this_dev->devLock);

      if (ret < 0)
        return signal_pending (current) ? -ERESTARTSYS : -EBUSY;
    }
    break;

  case V4L2_CID_STM_CKEY_ENABLE:
  case V4L2_CID_STM_CKEY_FORMAT:
  case V4L2_CID_STM_CKEY_R_CR_MODE:
  case V4L2_CID_STM_CKEY_G_Y_MODE:
  case V4L2_CID_STM_CKEY_B_CB_MODE:
  case V4L2_CID_STM_CKEY_MINVAL:
  case V4L2_CID_STM_CKEY_MAXVAL:
  case V4L2_CID_STM_MIXER_BACKGROUND_ARGB:
    {
      /* compat */
      struct v4l2_ext_control e = { .id = pctrl->id };
      e.value = pctrl->value;
      if (stmvout_write_state (&pDev->state, &e))
        return -EINVAL;
      return stmvout_apply_state (pDev, &pDev->state);
    }
    break;

  case V4L2_CID_BRIGHTNESS:
  case V4L2_CID_CONTRAST:
  case V4L2_CID_SATURATION:
  case V4L2_CID_HUE:
    if (stm_display_output_set_control (pDev->pOutput,
                                        ctrlname, pctrl->value) < 0) {
      return signal_pending (current) ? -ERESTARTSYS : -EBUSY;
    }
    break;

  default:
    return -EINVAL;
  }

  return 0;
}


int
stmvout_g_ctrl (struct _stvout_device * const pDev,
                struct _stvout_device  pipeline_devs[],
                struct v4l2_control   * const pctrl)
{
  ULONG                  ctrlname;
  struct _stvout_device *this_dev = pDev;
  ULONG                  val;
  int                    ret;

  BUG_ON (pDev == NULL);

  ctrlname = stmvout_get_ctrl_name (pctrl->id);
  if (ctrlname == 0)
    return -EINVAL;

  switch (pctrl->id) {
  case V4L2_CID_STM_IQI_SET_CONFIG_VID0:
  case V4L2_CID_STM_IQI_DEMO_VID0:
  case V4L2_CID_STM_XVP_SET_CONFIG_VID0:
  case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE_VID0:
  case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP_VID0:
  case V4L2_CID_STM_DEI_SET_FMD_ENABLE_VID0:
  case V4L2_CID_STM_DEI_SET_MODE_VID0:
  case V4L2_CID_STM_DEI_SET_CTRLREG_VID0:
    /* don't be confused by OUTPUT_VID1, the naming scheme is different... */
    this_dev = _get_device_by_id (pipeline_devs, OUTPUT_VID1);
    if (!this_dev)
      return -ENODEV;
    if (this_dev != pDev && down_interruptible (&this_dev->devLock))
      return -ERESTARTSYS;
    /* fall through */

  case V4L2_CID_STM_IQI_SET_CONFIG:
  case V4L2_CID_STM_IQI_DEMO:
  case V4L2_CID_STM_XVP_SET_CONFIG:
  case V4L2_CID_STM_XVP_SET_TNRNLE_OVERRIDE:
  case V4L2_CID_STM_XVP_SET_TNR_TOPBOTSWAP:
  case V4L2_CID_STM_DEI_SET_FMD_ENABLE:
  case V4L2_CID_STM_DEI_SET_MODE:
  case V4L2_CID_STM_DEI_SET_CTRLREG:

  case V4L2_CID_STM_PLANE_BRIGTHNESS:
  case V4L2_CID_STM_PLANE_CONTRAST:
  case V4L2_CID_STM_PLANE_SATURATION:
  case V4L2_CID_STM_PLANE_HUE:
    if (!this_dev->pPlane) {
      if (this_dev != pDev)
        up (&this_dev->devLock);
      return -EINVAL;
    }

    ret = stm_display_plane_get_control (this_dev->pPlane, ctrlname, &val);

    if (this_dev != pDev)
      up (&this_dev->devLock);

    if (ret < 0)
      return signal_pending (current) ? -ERESTARTSYS : -EBUSY;
    break;

  case V4L2_CID_STM_Z_ORDER_FIRST ... V4L2_CID_STM_Z_ORDER_LAST:
    {
      static const stm_plane_id_t id_map[] = {
        OUTPUT_VID1, OUTPUT_VID2,
        OUTPUT_GDP1, OUTPUT_GDP2, OUTPUT_GDP3
      };

      if (pctrl->id > V4L2_CID_STM_Z_ORDER_RGB2)
        return -EINVAL;

      this_dev = _get_device_by_id (pipeline_devs,
                                    id_map[pctrl->id - V4L2_CID_STM_Z_ORDER_VID0]);

      if (!this_dev)
        return -EINVAL;

      if (this_dev != pDev && down_interruptible (&this_dev->devLock))
          return -ERESTARTSYS;

      if (!this_dev->pPlane || !this_dev->pOutput) {
        if (this_dev != pDev)
          up (&this_dev->devLock);
        return -EINVAL;
      }

      ret = stm_display_plane_get_depth (this_dev->pPlane,
                                         this_dev->pOutput, (int *) &val);

      if (this_dev != pDev)
        up (&this_dev->devLock);

      if (ret < 0)
        return signal_pending (current) ? -ERESTARTSYS : -EBUSY;
    }
    break;

  case V4L2_CID_STM_CKEY_ENABLE:
  case V4L2_CID_STM_CKEY_FORMAT:
  case V4L2_CID_STM_CKEY_R_CR_MODE:
  case V4L2_CID_STM_CKEY_G_Y_MODE:
  case V4L2_CID_STM_CKEY_B_CB_MODE:
  case V4L2_CID_STM_CKEY_MINVAL:
  case V4L2_CID_STM_CKEY_MAXVAL:
  case V4L2_CID_STM_MIXER_BACKGROUND_ARGB:
    {
      /* compat */
      struct v4l2_ext_control e = { .id = pctrl->id };
      if (stmvout_read_state (&e, &pDev->state))
        return -EINVAL;
      pctrl->value = e.value;
    }
    break;

  case V4L2_CID_BRIGHTNESS:
  case V4L2_CID_CONTRAST:
  case V4L2_CID_SATURATION:
  case V4L2_CID_HUE:
    if (stm_display_output_get_control (pDev->pOutput,
                                        ctrlname, &val) < 0)
      return signal_pending (current) ? -ERESTARTSYS : -EBUSY;
    break;

  default:
    return -EINVAL;
  }

  pctrl->value = val;

  return 0;
}


static int
stmvout_ext_ctrls (struct _stvout_device_state * const state,
                   struct _stvout_device       * const device,
                   struct v4l2_ext_controls    *ctrls,
                   unsigned int                 cmd)
{
  int err = 0;
  int i;

  if (cmd == VIDIOC_G_EXT_CTRLS) {
    for (i = 0; i < ctrls->count; i++) {
      err = stmvout_read_state (&ctrls->controls[i], state);
      if (err) {
        ctrls->error_idx = i;
        break;
      }
    }

    return err;
  }

  for (i = 0; i < ctrls->count; i++) {
    struct v4l2_ext_control * const e = &ctrls->controls[i];
    struct v4l2_queryctrl    qctrl;
    const char              * const *menu_items = NULL;

    qctrl.id = e->id;
    err = stmvout_queryctrl (device, NULL, &qctrl);
    if (err)
      break;
    if (qctrl.type == V4L2_CTRL_TYPE_MENU)
      menu_items = stmvout_ctrl_get_menu (qctrl.id);
    err = v4l2_ctrl_check (e, &qctrl, (const char **) menu_items);
    if (err)
      break;
    err = stmvout_write_state (state, e);
    if (err)
      break;
  }
  if (err)
    ctrls->error_idx = i;

  return err;
}


int
stmvout_g_ext_ctrls (struct _stvout_device    * const device,
                     struct v4l2_ext_controls *ctrls)
{
  if (ctrls->ctrl_class == V4L2_CTRL_CLASS_USER
      || ctrls->ctrl_class == V4L2_CTRL_CLASS_STMVOUT) {
    int                 i;
    struct v4l2_control ctrl;
    int                 err = 0;

    for (i = 0; i < ctrls->count; i++) {
      ctrl.id = ctrls->controls[i].id;
      ctrl.value = ctrls->controls[i].value;
      err = stmvout_g_ctrl (device, NULL, &ctrl);
      ctrls->controls[i].value = ctrl.value;
      if (err) {
        ctrls->error_idx = i;
        break;
      }
    }
    return err;
  }

  return stmvout_ext_ctrls (&device->state, device, ctrls, VIDIOC_G_EXT_CTRLS);
}


int
stmvout_s_ext_ctrls (struct _stvout_device    * const device,
                     struct v4l2_ext_controls *ctrls)
{
  struct _stvout_device_state state;
  int                         ret;

  if (ctrls->ctrl_class == V4L2_CTRL_CLASS_USER
      || ctrls->ctrl_class == V4L2_CTRL_CLASS_STMVOUT) {
    int                 i;
    struct v4l2_control ctrl;
    int                 err = 0;

    for (i = 0; i < ctrls->count; i++) {
      ctrl.id = ctrls->controls[i].id;
      ctrl.value = ctrls->controls[i].value;
      err = stmvout_s_ctrl (device, NULL, &ctrl);
      ctrls->controls[i].value = ctrl.value;
      if (err) {
        ctrls->error_idx = i;
        break;
      }
    }
    return err;
  }

  state = device->state;
  ret = stmvout_ext_ctrls (&state, device, ctrls, VIDIOC_S_EXT_CTRLS);
  if (!ret) {
    ret = stmvout_apply_state (device, &state);
    device->state = state;
  }

  return ret;
}


int
stmvout_try_ext_ctrls (struct _stvout_device    * const device,
                       struct v4l2_ext_controls *ctrls)
{
  struct _stvout_device_state state;

  if (ctrls->ctrl_class != V4L2_CID_STMVOUTEXT_CLASS)
    return -EINVAL;

  state = device->state;
  return stmvout_ext_ctrls (&state, device, ctrls, VIDIOC_TRY_EXT_CTRLS);
}
