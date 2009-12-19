/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cut2mainvideopipe.cpp
 * Copyright (c) 2008 by STMicroelectronics. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>

#include <Gamma/stb7100/stb7109deireg.h>

#include "sti7200mainvideopipe.h"


/* DEI_CTL register */
#if 0
/* our setup */
#define COMMON_CTRLS    (DEI_CTL_BYPASS\
                         | DEI_CTL_DD_MODE_DIAG9                       \
                         | DEI_CTL_CLAMP_MODE_DIRECTIONAL_NOT_VERTICAL \
                         | DEI_CTL_DIR_RECURSIVE_ENABLE                \
                         | (0x6 << DEI_CTL_KCORRECTION_SHIFT)          \
                         | (0xa << DEI_CTL_T_DETAIL_SHIFT)             \
                         | C2_DEI_CTL_KMOV_FACTOR_14                   \
                         | C2_DEI_CTL_MD_MODE_OFF                      \
                         | C2_DEI_CTL_FMD_ENABLE                       \
                        )
#endif
#if 1
/* from ST1000 */
#define COMMON_CTRLS    (DEI_CTL_BYPASS\
                         | DEI_CTL_DD_MODE_DIAG9                       \
                         | DEI_CTL_CLAMP_MODE_DIRECTIONAL_NOT_VERTICAL \
                         | DEI_CTL_DIR_RECURSIVE_ENABLE                \
                         | (0x7 << DEI_CTL_KCORRECTION_SHIFT)          \
                         | (0x8 << DEI_CTL_T_DETAIL_SHIFT)             \
                         | C2_DEI_CTL_KMOV_FACTOR_14                   \
                         | C2_DEI_CTL_MD_MODE_OFF                      \
                         | C2_DEI_CTL_FMD_ENABLE                       \
                        )
#endif
#if 0
/* from validation */
#define COMMON_CTRLS    (DEI_CTL_BYPASS\
                         | DEI_CTL_DD_MODE_DIAG9                       \
                         | DEI_CTL_CLAMP_MODE_DIRECTIONAL_NOT_VERTICAL \
                         | DEI_CTL_DIR_RECURSIVE_ENABLE                \
                         | DEI_CTL_DIR3_ENABLE                         \
                         | (0x4 << DEI_CTL_KCORRECTION_SHIFT)          \
                         | (0xc << DEI_CTL_T_DETAIL_SHIFT)             \
                         | C2_DEI_CTL_KMOV_FACTOR_14                   \
                         | C2_DEI_CTL_MD_MODE_OFF                      \
                         | C2_DEI_CTL_FMD_ENABLE                       \
                         | C2_DEI_CTL_REDUCE_MOTION                    \
                        )
#endif



static const FMDSW_FmdInitParams_t g_fmd_init_params = {
  ulH_block_sz  : 40, /*!< Horizontal block size for BBD */
  ulV_block_sz  : 20, /*!< Vertical block size for BBD */

#ifdef USE_FMD
  count_miss    :  2, /*!< Delay for a film mode detection */
  count_still   : 30, /*!< Delay for a still mode detection */
  t_noise       : 10, /*!< Noise threshold */
  k_cfd1        : 21, /*!< Consecutive field difference factor 1 */
  k_cfd2        : 16, /*!< Consecutive field difference factor 2 */
  k_cfd3        :  6, /*!< Consecutive field difference factor 3 */
  t_mov         : 10, /*!< Moving pixel threshold */
  t_num_mov_pix :  9, /*!< Moving block threshold */
  t_repeat      : 70, /*!< Threshold on BBD for a field repetition */
  t_scene       : 15, /*!< Threshold on BBD for a scene change  */
  k_scene       : 25, /*!< Percentage of blocks with BBD > t_scene */
  d_scene       :  1, /*!< Scene change detection delay (1,2,3 or 4) */
#endif
};




CSTi7200MainVideoPipe::CSTi7200MainVideoPipe (stm_plane_id_t   GDPid,
                                              CGammaVideoPlug *plug,
                                              ULONG            dei_base): CSTi7xxxMainVideoPipe (GDPid,
                                                                                                 plug,
                                                                                                 dei_base,
                                                                                                 IQI_CELL_REV2)
{
  DEBUGF2 (4, (FENTRY ", self: %p, planeID: %d, dei_base: %lx\n",
               __PRETTY_FUNCTION__, this, GDPid, dei_base));

  /*
   * Keep the motion buffers in LMI0 for the moment, so they will be
   * separate from the actual video buffers in LMI1 and so will balance
   * the memory bandwidth requirements for de-interlacing 1080i.
   */
  m_ulMotionDataFlag = SDAAF_NONE;

  /*
   * The following T3I plug configuration is completely against the
   * validation recommendations, but this setup works for more cases
   * particularly when working with de-interlaced 1080i or 1080p content.
   * Validation are aware of this issue and are expected to provide new
   * plug setup recommendations shortly. For the moment we are going with
   * what works in practice.
   *
   */
  m_nMBChunkSize          = 0x3;// Validation recommend 0xf
  /*
   * Validation recommendation is LD32. Note that when the video pipe is used
   * on an empty system LD8 appears slightly better, but once the DVP is
   * enabled you appear to get mutual interference between the two IPs unless
   * the video plane is using LD32.
   */
  m_nRasterOpcodeSize     = 32;
  m_nRasterChunkSize      = 0x3;// Validation recommend 0x7
  m_nPlanarChunkSize      = 0x3;// Validation do not test this mode so do not have a recommendation

  /*
   * Setup memory plug configuration for the motion buffers, which is
   * now separate from the video configuration
   */
  m_nMotionOpcodeSize     = 8; // Validation recommend LD32, but it seems less efficient (TO BE UNDERSTOOD)
  WriteVideoReg (DEI_T3I_MOTION_CTL, (DEI_T3I_CTL_OPCODE_SIZE_8 |
                                      (0x3 << DEI_T3I_CTL_CHUNK_SIZE_SHIFT)));


  /* supports 1080i de-interlacing. Note the use of 1088 lines as this is
     a multiple of the size of a macroblock. */
  m_nDEIPixelBufferLength = 1920;
  m_nDEIPixelBufferHeight = 1088;

  /*
   * Override the motion control setup for this version of the IP.
   */
  m_nDEICtlLumaInterpShift   = C2_DEI_CTL_LUMA_INTERP_SHIFT;
  m_nDEICtlChromaInterpShift = C2_DEI_CTL_CHROMA_INTERP_SHIFT;
  m_nDEICtlSetupMotionSD     = COMMON_CTRLS;
  m_nDEICtlSetupMotionHD     = COMMON_CTRLS | C2_DEI_CTL_REDUCE_MOTION;
  m_nDEICtlSetupMedian       = COMMON_CTRLS;
  m_nDEICtlMDModeShift       = C2_DEI_CTL_MD_MODE_SHIFT;
  m_nDEICtlMainModeDEI       = 0;

  /* has the P2I block */
  m_bHasProgressive2InterlacedHW = true;

  /* has a new FMD */
  m_bHaveFMDBlockOffset  = true;
  m_fmd_init_params      = &g_fmd_init_params;
  m_MaxFMDBlocksPerLine  = 48;
  m_MaxFMDBlocksPerField = 27;
  m_FMDStatusSceneCountMask   = FMD_STATUS_C2_SCENE_COUNT_MASK;
  m_FMDStatusMoveStatusMask   = FMD_STATUS_C2_MOVE_STATUS_MASK;
  m_FMDStatusRepeatStatusMask = FMD_STATUS_C2_REPEAT_STATUS_MASK;

  DEBUGF2 (4, (FEXIT ", self: %p\n", __PRETTY_FUNCTION__, this));
}


CSTi7200MainVideoPipe::~CSTi7200MainVideoPipe (void)
{
  DEBUGF2 (4, (FENTRY ", self: %p\n",
               __PRETTY_FUNCTION__, this));
  DEBUGF2 (4, (FEXIT ", self: %p\n", __PRETTY_FUNCTION__, this));
}
