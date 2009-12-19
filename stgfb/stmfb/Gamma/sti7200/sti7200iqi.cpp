/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200iqi.cpp
 * Copyright (c) 2007/2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>


#include "sti7200iqi.h"


/* 7200cut1 same as piaget cut 3 */
/* 7200cut2 same as kelton cut 2 */


#define IQI_CONF                      0x00
/* for IQI config */
#  define IQI_BYPASS                   (1 << 0)
#  define IQI_ENABLESINXCOMPENSATION   (1 << 1)
#  define IQI_ENABLE_PEAKING           (1 << 2)
#  define IQI_ENABLE_LTI               (1 << 3)
#  define IQI_ENABLE_CTI               (1 << 4)
#  define IQI_ENABLE_LE                (1 << 5)
#  define IQI_ENABLE_CSC               (1 << 6)
#  define IQI_ENABLE_CE                (1 << 7) /* only available in rev1!  */
#  define IQI_ENABLE_ALL               (~IQI_BYPASS                  \
                                        | IQI_ENABLESINXCOMPENSATION \
                                        | IQI_ENABLE_PEAKING         \
                                        | IQI_ENABLE_LTI             \
                                        | IQI_ENABLE_CTI             \
                                        | IQI_ENABLE_LE              \
                                        | IQI_ENABLE_CSC             \
                                       )
#  define IQI_DEMO_LE                  (1 << 16)
#  define IQI_DEMO_PEAKING             (1 << 17)
#  define IQI_DEMO_LTI                 (1 << 18)
#  define IQI_DEMO_CE                  (1 << 19) /* only available in rev1! */
#  define IQI_DEMO_CTI                 (1 << 20)
#  define IQI_DEMO_ALL                 (IQI_DEMO_LE        \
                                        | IQI_DEMO_PEAKING \
                                        | IQI_DEMO_LTI     \
                                        | IQI_DEMO_CTI     \
                                       )

#define IQI_DEMO_WINDOW               0x04

#define IQI_PEAKING_CONF              0x08
#  define PEAKING_RANGEMAX_MASK        (0x0000f000)
#  define PEAKING_RANGEMAX_SHIFT       (12)
#  define PEAKING_UNDERSHOOT_MASK      (0x00000c00)
#  define PEAKING_UNDERSHOOT_SHIFT     (10)
#  define PEAKING_OVERSHOOT_MASK       (0x00000300)
#  define PEAKING_OVERSHOOT_SHIFT      (8)
#  define PEAKING_RANGE_GAIN_LUT_INIT  (1 << 4)
#  define PEAKING_V_PK_EN              (1 << 3)
#  define PEAKING_CORING_MODE          (1 << 2)
#  define PEAKING_ENV_DETECT           (1 << 1)
#  define PEAKING_EXTENDED_SIZE        (1 << 0)
#  define IQI_PEAKING_CONF_ALL_MASK    (PEAKING_RANGEMAX_MASK \
                                        | PEAKING_UNDERSHOOT_MASK \
                                        | PEAKING_OVERSHOOT_MASK \
                                        | PEAKING_RANGE_GAIN_LUT_INIT \
                                        | PEAKING_V_PK_EN \
                                        | PEAKING_CORING_MODE \
                                        | PEAKING_ENV_DETECT \
                                        | PEAKING_EXTENDED_SIZE)

#define IQI_PEAKING_COEFX0            0x0c
#define IQI_PEAKING_COEFX1            0x10
#define IQI_PEAKING_COEFX2            0x14
#define IQI_PEAKING_COEFX3            0x18
#define IQI_PEAKING_COEFX4            0x1c
#  define PEAKING_COEFFX_MASK          (0x000003ff)

#define IQI_PEAKING_RANGE_GAIN_LUT_BA 0x20

#define IQI_PEAKING_GAIN              0x24
#  define PEAKING_GAIN_SINC_MASK       (0x0000f000)
#  define PEAKING_GAIN_SINC_SHIFT      (12)
#  define PEAKING_GAIN_VERT_GAIN_MASK  (0x00000f00)
#  define PEAKING_GAIN_VERT_GAIN_SHIFT (8)
#  define PEAKING_GAIN_HOR_GAIN_MASK   (0x0000001f)
#  define PEAKING_GAIN_HOR_GAIN_SHIFT  (0)
#  define IQI_PEAKING_GAIN_ALL_MASK    (PEAKING_GAIN_SINC_MASK \
                                        | PEAKING_GAIN_VERT_GAIN_MASK \
                                        | PEAKING_GAIN_HOR_GAIN_MASK)

#define IQI_PEAKING_CORING_LEVEL      0x28
#  define PEAKING_CORING_LEVEL_MASK    (0x0000003f)
#  define PEAKING_CORING_LEVEL_SHIFT   (0)
#  define IQI_PEAKING_CORING_LEVEL_ALL_MASK (PEAKING_CORING_LEVEL_MASK)

#define IQI_LTI_CONF                  0x2c
#  define LTI_CONF_HMMS_PREFILTER_MASK  (0x00030000)
#  define LTI_CONF_HMMS_PREFILTER_SHIFT (16)
#  define LTI_CONF_V_LTI_STRENGTH_MASK  (0x0000f000)
#  define LTI_CONF_V_LTI_STRENGTH_SHIFT (12)
#  define LTI_CONF_HEC_HEIGHT_MASK      (0x00000800)
#  define LTI_CONF_HEC_HEIGHT_SHIFT     (11)
#  define LTI_CONF_HEC_GRADIENT_MASK    (0x00000600)
#  define LTI_CONF_HEC_GRADIENT_SHIFT   (9)
#  define LTI_CONF_H_LTI_STRENGTH_MASK  (0x000001f0)
#  define LTI_CONF_H_LTI_STRENGTH_SHIFT (4)
#  define LTI_CONF_VERTICAL_LTI         (1 << 3)
#  define LTI_CONF_ANTIALIASING         (1 << 1)
#  define LTI_CONF_SELECTIVE_EDGE       (1 << 0)

#define IQI_LTI_THRESHOLD             0x30
#  define LTI_CORNERTEST_MASK                (0xff000000)
#  define LTI_CORNERTEST_SHIFT               (24)
#  define LTI_VERTDIFF_MASK                  (0x00ff0000)
#  define LTI_VERTDIFF_SHIFT                 (16)
#  define LTI_MINCORRELATION_MASK            (0x0000ff00)
#  define LTI_MINCORRELATION_SHIFT           (8)
#  define LTI_DISTURBANCE_MASK               (0x000000ff)
#  define LTI_DISTURBANCE_SHIFT              (0)
#    define LTI_DEFAULT_CORNERTEST             (0x20)
#    define LTI_DEFAULT_VERTDIFF               (0x20)
#    define LTI_DEFAULT_MINCORRELATION         (0x10)
#    define LTI_DEFAULT_DISTURBANCE            (0x04)

#define IQI_LTI_DELTA_SLOPE           0x34
#  define LTI_DELTA_SLOPE_MASK         (0x000000ff)
#  define LTI_DELTA_SLOPE_SHIFT        (0)

/* revision 1 CTI */
#define IQIr1_CTI_CONF                0x38
#  define CTIr1_COR_MASK                     (0x000f0000)
#  define CTIr1_COR_SHIFT                    (16)
#  define CTIr1_STRENGTH_MASK                (0x00003000)
#  define CTIr1_STRENGTH_SHIFT               (12)
#  define CTIr1_MED_MASK                     (0x000003f0)
#  define CTIr1_MED_SHIFT                    (4)
#  define CTIr1_MON_MASK                     (0x0000000f)
#  define CTIr1_MON_SHIFT                    (0)
#    define CTIr1_DEFAULT_COR                  (0x01)
#    define CTIr1_DEFAULT_STRENGTH             (IQICS_NONE)
#    define CTIr1_DEFAULT_MED                  (0x1C)
#    define CTIr1_DEFAULT_MON                  (0x04)
#  define IQIr1_CTI_ALL_MASK                 (CTIr1_COR_MASK \
                                              | CTIr1_STRENGTH_MASK \
                                              | CTIr1_MED_MASK \
                                              | CTIr1_MON_MASK)
/* revision 2 CTI */
#define IQIr2_CTI_CONF                  0x38
#  define CTIr2_COR_MASK                       (0x000f0000)
#  define CTIr2_COR_SHIFT                      (16)
#  define CTIr2_EXTENDED_MASK                  (0x00004000)
#  define CTIr2_EXTENDED_SHIFT                 (14)
#  define CTIr2_STRENGTH2_MASK                 (0x00003000)
#  define CTIr2_STRENGTH2_SHIFT                (12)
#  define CTIr2_STRENGTH1_MASK                 (0x00000c00)
#  define CTIr2_STRENGTH1_SHIFT                (10)
#  define CTIr2_MED_MASK                       (0x000003f0)
#  define CTIr2_MED_SHIFT                      (4)
#  define CTIr2_MON_MASK                       (0x0000000f)
#  define CTIr2_MON_SHIFT                      (0)
#    define CTIr2_DEFAULT_COR                  (0x06)
#    define CTIr2_DEFAULT_STRENGTH2            (IQICS_NONE)
#    define CTIr2_DEFAULT_STRENGTH1            (IQICS_NONE)
#    define CTIr2_DEFAULT_MED                  (0x0C)
#    define CTIr2_DEFAULT_MON                  (0x04)
#  define IQIr2_CTI_ALL_MASK                   (CTIr2_COR_MASK \
                                                | CTIr2_STRENGTH2_MASK \
                                                | CTIr2_STRENGTH1_MASK \
                                                | CTIr2_MED_MASK \
                                                | CTIr2_MON_MASK)

#define IQI_LE_CONF                   0x3c
#  define LE_WEIGHT_GAIN_MASK          (0x0000007c)
#  define LE_WEIGHT_GAIN_SHIFT         (2)
#  define LE_601                       (1 << 1)
#  define LE_LUT_INIT                  (1 << 0)
#  define IQI_LE_ALL_MASK              (LE_WEIGHT_GAIN_MASK \
                                        | LE_601 \
                                        | LE_LUT_INIT)

#define IQI_LE_LUT_BA                 0x40

/* CE removed in cut 2! */
#define IQI_CE_CONF                   0x44
#  define CE_BLUE_STRETCH_THRESH_MASK  (0x01f00000)
#  define CE_BLUE_STRETCH_THRESH_SHIFT (20)
#  define CE_BLUE_STRETCH_MASK         (0x000f0000)
#  define CE_BLUE_STRETCH_SHIFT        (16)
#  define CE_TINT_MASK                 (0x0000fc00)
#  define CE_TINT_SHIFT                (10)
#  define CE_BLUE_BOOST_MASK           (0x000003e0)
#  define CE_BLUE_BOOST_SHIFT          (5)
#  define CE_GREEN_BOOST_MASK          (0x0000001f)
#  define CE_GREEN_BOOST_SHIFT         (0)
#  define IQI_CE_ALL_MASK              (CE_BLUE_STRETCH_THRESH_MASK \
                                        | CE_BLUE_STRETCH_MASK \
                                        | CE_TINT_MASK \
                                        | CE_BLUE_BOOST_MASK \
                                        | CE_GREEN_BOOST_MASK)

#define IQI_CE_FLESH                  0x48
#  define CE_FLESH_AREA_MASK           (0x00000600)
#  define CE_FLESH_AREA_SHIFT          (9)
#  define CE_FLESH_AXIS_MASK           (0x00000180)
#  define CE_FLESH_AXIS_SHIFT          (7)
#  define CE_FLESH_VALUE_MASK          (0x0000007f)
#  define CE_FLESH_VALUE_SHIFT         (0)
#  define IQI_CE_FLESH_ALL_MASK        (CE_FLESH_AREA_MASK \
                                        | CE_FLESH_AXIS_MASK \
                                        | CE_FLESH_VALUE_MASK)

#define IQI_CONBRI                    0x4c
#  define CONBRI_CONBRI_OFFSET_MASK    (0x0fff0000)
#  define CONBRI_CONBRI_OFFSET_SHIFT   (16)
#  define CONBRI_CONTRAST_GAIN_MASK    (0x000001ff)
#  define CONBRI_CONTRAST_GAIN_SHIFT   (0)
#    define CONBRI_MIN_CONTRAST_GAIN   (  0 << CONBRI_CONTRAST_GAIN_SHIFT)
#    define CONBRI_MAX_CONTRAST_GAIN   (256 << CONBRI_CONTRAST_GAIN_SHIFT)
#  define IQI_CONBRI_ALL_MASK          (CONBRI_CONBRI_OFFSET_MASK \
                                        | CONBRI_CONTRAST_GAIN_MASK)

#define IQI_SAT                       0x50
#  define SAT_SAT_GAIN_MASK            (0x000001ff)
#  define SAT_SAT_GAIN_SHIFT           (0)
#    define SAT_DEFAULT_SAT_GAIN         (256 << SAT_SAT_GAIN_SHIFT)
#  define IQI_SAT_ALL_MASK             (SAT_SAT_GAIN_MASK)





enum IQIPeakingVertGain {
  IQIPVG_N6_0DB = 0,
  IQIPVG_N5_5DB = 0,
  IQIPVG_N5_0DB = 0,
  IQIPVG_N4_5DB = 0,
  IQIPVG_N4_0DB = 0,
  IQIPVG_N3_5DB = 0,
  IQIPVG_N3_0DB = 0,
  IQIPVG_N2_5DB = 0,
  IQIPVG_N2_0DB,
  IQIPVG_N1_5DB,
  IQIPVG_N1_0DB,
  IQIPVG_N0_5DB,
  IQIPVG_0DB = IQIPVG_N0_5DB,
  IQIPVG_P0_5DB = IQIPVG_0DB,
  IQIPVG_P1_0DB,
  IQIPVG_P1_5DB = IQIPVG_P1_0DB,
  IQIPVG_P2_0DB,
  IQIPVG_P2_5DB = IQIPVG_P2_0DB,
  IQIPVG_P3_0DB,
  IQIPVG_P3_5DB = IQIPVG_P3_0DB,
  IQIPVG_P4_0DB,
  IQIPVG_P4_5DB,
  IQIPVG_P5_0DB,
  IQIPVG_P5_5DB,
  IQIPVG_P6_0DB,
  IQIPVG_P6_5DB,
  IQIPVG_P7_0DB,
  IQIPVG_P7_5DB,
  IQIPVG_P8_0DB = IQIPVG_P7_5DB,
  IQIPVG_P8_5DB = IQIPVG_P8_0DB,
  IQIPVG_P9_0DB = IQIPVG_P8_5DB,
  IQIPVG_P9_5DB = IQIPVG_P9_0DB,
  IQIPVG_P10_0DB = IQIPVG_P9_5DB,
  IQIPVG_P10_5DB = IQIPVG_P10_0DB,
  IQIPVG_P11_0DB = IQIPVG_P10_5DB,
  IQIPVG_P11_5DB = IQIPVG_P11_0DB,
  IQIPVG_P12_0DB = IQIPVG_P11_5DB,

  IQIPVG_LAST = IQIPVG_P12_0DB
};

/* highpass filter coefficient for 9 taps peaking filter */
static const short hor_filter_coefficients_HP[IQIPFF_COUNT][5]=
{
  { 358,-120, -55,  -8,   4  },  /* Correspond to 1.0MHZ   for 1H signal   (1.0/6.75  = 0.15) */
  { 322,-133, -35,   5,   2  },  /* Correspond to 1.25MHZ  for 1H signal   (1.25/6.75 = 0.18) */
  { 398, -99, -65, -29,  -6  },  /* Correspond to 1.5MHZ   for 1H signal   (1.5/6.75  = 0.22) */
  { 378,-109, -62, -19,   1  },  /* Correspond to 1.75MHZ  for 1H signal   (1.75/6.75 = 0.26) */
  { 358,-120, -55,  -8,   4  },  /* Correspond to 2.0MHZ   for 1H signal   (2.0/6.75  = 0.30) */
  { 342,-128, -48,   0,   5  },  /* Correspond to 2.25MHZ  for 1H signal   (2.25/6.75 = 0.33) */
  { 326,-134, -40,   6,   5  },  /* Correspond to 2.5MHZ   for 1H signal   (2.5/6.75  = 0.37) */
  { 304,-144, -31,  16,   7  },  /* Correspond to 2.75MHZ  for 1H signal   (2.75/6.75 = 0.40) */
  { 286,-147, -19,  19,   4  },  /* Correspond to 3.0MHZ   for 1H signal   (3.0/6.75  = 0.44) */
  { 266,-149,  -6,  21,   1  },  /* Correspond to 3.25MHZ  for 1H signal   (3.25/6.75 = 0.48) */
  { 246,-148,   6,  20,  -1  },  /* Correspond to 3.5MHZ   for 1H signal   (3.5/6.75  = 0.51) */
  { 226,-145,  18,  17,  -3  },  /* Correspond to 3.75MHZ  for 1H signal   (3.75/6.75 = 0.55} */
  { 206,-141,  30,  13,  -5  },  /* Correspond to 4.0MHZ   for 1H signal   (4.0/6.75  = 0.58) */
  { 188,-135,  40,   7,  -6  },  /* Correspond to 4.25MHZ  for 1H signal   (4.25/6.75 = 0.63) */
};
/* bandpass filter coefficient for 9 taps peaking filter */
static const short hor_filter_coefficients_BP[IQIPFF_COUNT][5]=
{
  { 154,  87, -35, -85, -44  },  /* Correspond to 1.0MHZ   for 1H signal   (1.0/6.75  = 0.15) */
  { 192,  70, -92, -73,  -1  },  /* Correspond to 1.25MHZ  for 1H signal   (1.25/6.75 = 0.18) */
  { 240,  37,-127, -26,  -4  },  /* Correspond to 1.5MHZ   for 1H signal   (1.5/6.75  = 0.22) */
  { 132,  88,  -7, -74, -73  },  /* Correspond to 1.75MHZ  for 1H signal   (1.75/6.75 = 0.26) */
  { 154,  87, -35, -85, -44  },  /* Correspond to 2.0MHZ   for 1H signal   (2.0/6.75  = 0.30) */
  { 176,  82, -65, -85, -20  },  /* Correspond to 2.25MHZ  for 1H signal   (2.25/6.75 = 0.33) */
  { 192,  70, -92, -73,  -1  },  /* Correspond to 2.5MHZ   for 1H signal   (2.5/6.75  = 0.37) */
  { 208,  54,-112, -53,   7  },  /* Correspond to 2.75MHZ  for 1H signal   (2.75/6.75 = 0.40) */
  { 222,  34,-124, -31,  10  },  /* Correspond to 3.0MHZ   for 1H signal   (3.0/6.75  = 0.44) */
  { 234,  12,-129,  -9,   9  },  /* Correspond to 3.25MHZ  for 1H signal   (3.25/6.75 = 0.48) */
  { 246, -12,-125,   8,   6  },  /* Correspond to 3.5MHZ   for 1H signal   (3.5/6.75  = 0.51) */
  { 246, -36,-117,  24,   6  },  /* Correspond to 3.75MHZ  for 1H signal   (3.75/6.75 = 0.55) */
  { 220, -56,-109,  48,   7  },  /* Correspond to 4.0MHZ   for 1H signal   (4.0/6.75  = 0.58) */
  { 212, -75, -92,  61,   0  },  /* Correspond to 4.25MHZ  for 1H signal   (4.25/6.75 = 0.63) */
};

static const UCHAR hor_lti_strength_to_delta_slope[32] =
{
  0, 0,  0,  0,  0,  0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,
  0, 8, 17, 26, 37, 47, 59, 72, 85, 100, 116, 134, 154, 175, 199, 226
};

#define IQIPCM_COUNT   (IQIPCM_LAST+1)
static const UCHAR IQI_ClippingCurves[IQIPCM_COUNT][256] =  {
  {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
  },
  {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    254,253,252,251,250,249,248,247,247,246,245,244,243,242,241,240,
    239,238,237,236,235,234,233,232,231,230,229,228,227,226,225,224,
    223,222,221,220,219,218,217,216,215,214,213,212,211,210,209,208,
    207,206,205,204,203,202,201,200,199,198,197,196,195,194,193,192,
    191,190,189,188,187,186,185,184,183,182,181,180,179,178,177,176,
    175,174,173,172,171,170,169,168,167,166,165,164,163,162,161,160,
    159,158,157,156,155,154,153,152,151,150,149,148,147,146,145,144,
    143,142,141,140,139,138,137,136,135,134,133,132,131,130,129,128
  },
  {
    255,247,240,233,227,220,215,209,204,199,194,190,185,181,177,173,
    170,166,163,160,157,154,151,148,145,143,140,138,136,133,131,129,
    127,125,123,121,119,117,116,114,113,111,110,108,107,105,104,103,
    101,100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86,
     85, 84, 83, 82, 82, 81, 80, 79, 78, 77, 76, 76, 75, 74, 73, 72,
     72, 71, 71, 70, 71, 70, 69, 68, 68, 67, 66, 65, 65, 64, 64, 63,
     63, 62, 62, 61, 61, 60, 60, 60, 59, 59, 58, 58, 58, 57, 57, 56,
     56, 56, 55, 55, 55, 54, 54, 54, 53, 53, 53, 52, 52, 52, 51, 51,
     51, 50, 50, 50, 49, 49, 49, 48, 48, 48, 47, 47, 47, 46, 46, 46,
     46, 45, 45, 45, 45, 44, 44, 44, 44, 43, 43, 43, 43, 42, 42, 42,
     42, 41, 41, 41, 41, 40, 40, 40, 40, 40, 39, 39, 39, 39, 39, 39,
     38, 38, 38, 38, 38, 38, 38, 37, 37, 37, 37, 36, 36, 36, 36, 36,
     36, 35, 35, 35, 35, 35, 35, 34, 34, 34, 34, 34, 34, 34, 33, 33,
     33, 33, 33, 33, 33, 32, 32, 32, 32, 32, 32, 32, 32, 31, 31, 31,
     31, 31, 31, 31, 30, 30, 30, 30, 30, 30, 30, 30, 30, 29, 29, 29,
     29, 29, 29, 29, 29, 29, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28
  }
};


/* LUMA LUT */
#define LUMA_LUT_SIZE         128  /* maximum size of IQI Luma LUT enhancer */

static const USHORT IQI_LumaLUT[LUMA_LUT_SIZE] = {
#if 0
  /*- LUMA ENHANCER COEFFICIENTS -*/
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0008, 0x0010, 0x0020, 0x0028, 0x0030, 0x0040, 0x0048, 0x0050,
  0x0058, 0x0068, 0x0070, 0x0078, 0x0088, 0x0090, 0x0098, 0x00A8,
  0x00B0, 0x00B8, 0x00C8, 0x00D0, 0x00D8, 0x00E0, 0x00F0, 0x00F8,
  0x0100, 0x0108, 0x0110, 0x0118, 0x0120, 0x0128, 0x0130, 0x0138,
  0x0140, 0x0148, 0x0150, 0x0158, 0x0160, 0x0168, 0x0170, 0x0178,
  0x0180, 0x0188, 0x0190, 0x0198, 0x01A0, 0x01A8, 0x01B0, 0x01B8,
  0x01C0, 0x01C8, 0x01D0, 0x01D8, 0x01E0, 0x01E8, 0x01F0, 0x01F8,
  0x0200, 0x0208, 0x0210, 0x0218, 0x0220, 0x0228, 0x0230, 0x0238,
  0x0240, 0x0248, 0x0250, 0x0258, 0x0260, 0x0268, 0x0270, 0x0278,
  0x0280, 0x0288, 0x0290, 0x0298, 0x02A0, 0x02A8, 0x02B0, 0x02B8,
  0x02C0, 0x02C8, 0x02D0, 0x02D8, 0x02E0, 0x02E8, 0x02F0, 0x02F8,
  0x0300, 0x0310, 0x0318, 0x0320, 0x0330, 0x0338, 0x0340, 0x0350,
  0x0358, 0x0368, 0x0378, 0x0388, 0x0390, 0x03A0, 0x03A8, 0x03B8,
  0x03C0, 0x03D0, 0x03D8, 0x03E8, 0x03F0, 0x03F8, 0x03F8, 0x03F8,
  0x03F8, 0x03F8, 0x03F8, 0x03F8, 0x03F8, 0x03F8, 0x03F8, 0x03F8
#else
     0,    8,   16,   24,   32,   40,   48,   56,
    66,   70,   74,   79,   84,   90,   95,  100,
   106,  109,  118,  125,  133,  141,  149,  157,
   165,  173,  181,  189,  197,  206,  213,  222,
   231,  238,  247,  255,  263,  272,  280,  289,
   298,  305,  313,  321,  329,  337,  346,  355,
   363,  372,  381,  390,  398,  407,  416,  425,
   433,  442,  451,  460,  468,  478,  487,  496,
   504,  513,  523,  532,  540,  549,  558,  568,
   576,  585,  595,  604,  612,  621,  631,  640,
   649,  658,  667,  677,  685,  694,  704,  713,
   722,  731,  741,  749,  756,  764,  770,  778,
   785,  792,  799,  807,  814,  821,  828,  836,
   842,  850,  858,  864,  872,  879,  887,  893,
   901,  909,  915,  923,  931,  936,  944,  952,
   960,  968,  976,  984,  992, 1000, 1008, 1016
#endif
};



enum IQIPeakingUndershootFactor {
  IQIPUF_100,
  IQIPUF_075,
  IQIPUF_050,
  IQIPUF_025,

  IQIPUF_LAST = IQIPUF_025
};
enum IQIPeakingOvershootFactor {
  IQIPOF_100,
  IQIPOF_075,
  IQIPOF_050,
  IQIPOF_025,

  IQIPOF_LAST = IQIPOF_025
};

struct _IQIInitParameters {
  /* LE */
  struct _LE {
    UCHAR weight_gain;
  } le;
  struct _LTI {
    /* LTI */
    UCHAR threshold_corner_test;     /* threshold corner test*/
    UCHAR threshold_vert_diff;       /* threshold vertical difference */
    UCHAR threshold_min_correlation; /* threshold minimum correlation */
    UCHAR threshold_disturbance;     /* threshold disturbance */
  } lti;
  struct _CTIr1 {
    /* CTI */
    UCHAR tmon;   /* value of cti used in monotonicity checker */
    UCHAR tmed;   /* value of the cti threshold used in 3 points median filter */
    UCHAR coring; /* value of cti coring used in the peaking filter */
  } ctiR1;
  struct _CTIr2 {
    /* CTI */
    UCHAR tmon;   /* value of cti used in monotonicity checker */
    UCHAR tmed;   /* value of the cti threshold used in 3 points median filter */
    UCHAR coring; /* value of cti coring used in the peaking filter */
  } ctiR2;
  struct _Peaking {
    /* Peaking */
    UCHAR range_max;                            /* start Value for Range Gain non linear curve */
    enum IQIPeakingUndershootFactor undershoot; /* Undershoot factor: 00: 1.00, 01: 0.75, 10: 0.50, 11: 0.25 */
    enum IQIPeakingOvershootFactor overshoot;   /* Overshoot factor: 00: 1.00, 01: 0.75, 10: 0.50, 11: 0.25 */
    bool vertical_peaking_enabled;              /* Vertical peaking enabled(1)/disabled(0) */
    bool coring_mode;                           /* if set the peaking is done in chroma adaptive coring mode, if reset it is done in manual coring mode */
    bool envelope_detect;                       /* if set the envelope detection is done 3 lines, if reset it is only done on 1 line */
    bool extended_size;                         /* Extended size enabled(1)/disabled(0) */
    LONG coeffs[5];                             /* Horizontal peaking coefficients -512...511 */
    enum IQIPeakingVertGain ver_gain;           /* vertical gain 0...15  (-2.5dB...7.5dB) */
    enum IQIPeakingHorGain hor_gain;            /* horizontal gain 0...31 */
    UCHAR coring_level;                         /* peaking coring value */
    enum IQIPeakingClippingMode clipping_mode;  /* clipping mode */
  } peaking;
  struct _CE {
    /* CE */
    UCHAR threshold_blue_stretch; /* threshold blue stretch */
  } ce;
};



static const struct _IQIInitParameters IQIinitDefaults = {
  le      : { weight_gain : 8 },
  lti     : { threshold_corner_test : LTI_DEFAULT_CORNERTEST,
              threshold_vert_diff : LTI_DEFAULT_VERTDIFF,
              threshold_min_correlation : LTI_DEFAULT_MINCORRELATION,
              threshold_disturbance : LTI_DEFAULT_DISTURBANCE
            },
  ctiR1   : { tmon : CTIr1_DEFAULT_MON,
              tmed : CTIr1_DEFAULT_MED,
              coring : CTIr1_DEFAULT_COR
            },
  ctiR2   : { tmon : CTIr2_DEFAULT_MON,
              tmed : CTIr2_DEFAULT_MED,
              coring : CTIr2_DEFAULT_COR
            },
  peaking : { range_max : 4,
              undershoot : IQIPUF_100,
              overshoot : IQIPOF_100,
              vertical_peaking_enabled : false,
              coring_mode : false,
              envelope_detect : false,
              extended_size : false,
              coeffs : { 252, 12, -387, -498, 0 },
              ver_gain : IQIPVG_0DB,
              hor_gain : IQIPHG_P3_0DB,
              coring_level : 8,
              clipping_mode : IQIPCM_NONE,
            },
  ce      : { threshold_blue_stretch : 28 }
};


enum IQILtiHmmsPrefilter {
  IQILHPF_NONE,
  IQILHPF_WEAK,
  IQILHPF_STRONG,

  IQILHPF_COUNT
};

enum IQILtiHecGradientOffset {
  IQILHGO_0,
  IQILHGO_4,
  IQILHGO_8,

  IQILHGO_COUNT
};

enum IQICtiStrength {
  IQICS_NONE,
  IQICS_MIN,
  IQICS_MEDIUM,
  IQICS_STRONG,

  IQICS_COUNT
};

struct _IQIConfigurationParameters {
  struct _Conf {
    UCHAR enable;
  } conf;
  struct _Peaking {
    enum IQIPeakingUndershootFactor undershoot;
    enum IQIPeakingOvershootFactor overshoot;
    bool coring_mode;
    UCHAR coring_level; /* 0 ... 63 */
    bool vertical_peaking;
    enum IQIPeakingVertGain ver_gain;
    enum IQIPeakingClippingMode clipping_mode;

    enum IQIPeakingFilterFrequency bandpassfreq;
    enum IQIPeakingFilterFrequency highpassfreq;
    enum IQIPeakingHorGain bandpassgain;
    enum IQIPeakingHorGain highpassgain;
  } peaking;
  struct _Lti {
    enum IQILtiHecGradientOffset selective_edge;
    enum IQILtiHmmsPrefilter hmms_prefilter;
    bool anti_aliasing;
    bool vertical;
    UCHAR ver_strength; /* 0 ... 15 */
    UCHAR hor_strength; /* 0 ... 31 */
  } lti;
  struct _CTIr1 {
    enum IQICtiStrength strength;
  } ctiR1;
  struct _CTIr2 {
    enum IQICtiStrength strength1;
    enum IQICtiStrength strength2;
  } ctiR2;
  struct _LE {
    UCHAR csc_gain; /* 0 ... 31 */
  } le;
  struct _CE {
    UCHAR blue_boost;  /* 0 ...  31 */
    UCHAR green_boost; /* 0 ...  31 */
    UCHAR auto_flesh;  /* 0 ... 127 */
  } ce;
};

/* FIXME: get some defaults from validation - Samir BEN MESSAOUD! */
static const struct _IQIConfigurationParameters IQIConfigParams[PCIQIC_COUNT] = {
  { conf : { enable : IQI_BYPASS },
  },

  { conf : { enable : IQI_ENABLE_ALL },
    peaking : { undershoot   : IQIPUF_100,
                overshoot    : IQIPOF_050,
                coring_mode  : false,
                coring_level : 10,
                vertical_peaking : true,
                ver_gain     : IQIPVG_P6_0DB,
                clipping_mode : IQIPCM_STRONG,
                bandpassfreq : IQIPFF_0_30_FsDiv2,
                highpassfreq : IQIPFF_0_48_FsDiv2,
                bandpassgain : IQIPHG_P7_0DB,
                highpassgain : IQIPHG_P4_0DB },
    lti: { selective_edge : IQILHGO_8,
           hmms_prefilter : IQILHPF_STRONG,
           anti_aliasing  : true,
           vertical       : true,
           ver_strength   : 15,
           hor_strength   : 20 },
    ctiR1: { strength : IQICS_MEDIUM },
    ctiR2: { strength1 : IQICS_MEDIUM,
             strength2 : IQICS_MEDIUM },
    le: { csc_gain : 31 },
    ce: { blue_boost  :  2,
          green_boost : 20,
          auto_flesh  : 60 },
  },
};




#ifdef DEBUG
static void
CheckInitParams (ULONG                caps,
                 enum IQICellRevision iqi_rev)
{
  const struct _IQIInitParameters * const init = &IQIinitDefaults;

#  define CHECK(val,shift,mask)                                          \
    ASSERTF((((val) << (shift)) & (mask)) == ((val) << (shift)),         \
            ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__))
  /* le */
  CHECK ((ULONG) init->le.weight_gain, LE_WEIGHT_GAIN_SHIFT, LE_WEIGHT_GAIN_MASK);

  /* lti */
  CHECK ((ULONG) init->lti.threshold_corner_test, LTI_CORNERTEST_SHIFT, LTI_CORNERTEST_MASK);
  CHECK ((ULONG) init->lti.threshold_vert_diff, LTI_VERTDIFF_SHIFT, LTI_VERTDIFF_MASK);
  CHECK ((ULONG) init->lti.threshold_min_correlation, LTI_MINCORRELATION_SHIFT, LTI_MINCORRELATION_MASK);
  CHECK ((ULONG) init->lti.threshold_disturbance, LTI_DISTURBANCE_SHIFT, LTI_DISTURBANCE_MASK);
  /* cti */
  switch (iqi_rev)
    {
    case IQI_CELL_REV1:
      CHECK ((ULONG) init->ctiR1.tmon, CTIr1_MON_SHIFT, CTIr1_MON_MASK);
      CHECK ((ULONG) init->ctiR1.tmed, CTIr1_MED_SHIFT, CTIr1_MED_MASK);
      CHECK ((ULONG) init->ctiR1.coring, CTIr1_COR_SHIFT, CTIr1_COR_MASK);
      break;

    case IQI_CELL_REV2:
      CHECK ((ULONG) init->ctiR2.tmon, CTIr2_MON_SHIFT, CTIr2_MON_MASK);
      CHECK ((ULONG) init->ctiR2.tmed, CTIr2_MED_SHIFT, CTIr2_MED_MASK);
      CHECK ((ULONG) init->ctiR2.coring, CTIr2_COR_SHIFT, CTIr2_COR_MASK);
      break;

    default:
      ASSERTF (1 == 2, ("%s: should not be reached\n", __PRETTY_FUNCTION__));
      break;
    }
  /* peaking */
  CHECK ((ULONG) init->peaking.range_max, PEAKING_RANGEMAX_SHIFT, PEAKING_RANGEMAX_MASK);
  ASSERTF (init->peaking.undershoot <= IQIPUF_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (init->peaking.overshoot <= IQIPOF_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  /* we deliberately don't check init->peaking.coeffs[] */
  ASSERTF (init->peaking.ver_gain <= IQIPVG_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (init->peaking.hor_gain <= IQIPHG_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  CHECK ((ULONG) init->peaking.coring_level, PEAKING_CORING_LEVEL_SHIFT, PEAKING_CORING_LEVEL_MASK);
  ASSERTF (init->peaking.clipping_mode <= IQIPCM_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  /* ce */
  if (caps & PLANE_CTRL_CAPS_IQI_CE)
    CHECK ((ULONG) init->ce.threshold_blue_stretch, CE_BLUE_STRETCH_THRESH_SHIFT, CE_BLUE_STRETCH_THRESH_MASK);
#  undef CHECK
}

static void
CheckConfigParams (ULONG                                     caps,
                   enum IQICellRevision                      iqi_rev,
                   const struct _IQIConfigurationParameters * const param)
{
  /* peaking */
  ASSERTF (param->peaking.coring_level <= 63,
           ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->peaking.undershoot <= IQIPUF_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->peaking.overshoot <= IQIPOF_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->peaking.ver_gain <= IQIPVG_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->peaking.clipping_mode <= IQIPCM_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->peaking.bandpassfreq < IQIPFF_COUNT,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->peaking.highpassfreq < IQIPFF_COUNT,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->peaking.bandpassgain <= IQIPHG_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->peaking.highpassgain <= IQIPHG_LAST,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  /* lti */
  ASSERTF (param->lti.selective_edge < IQILHGO_COUNT,
           ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->lti.hmms_prefilter < IQILHPF_COUNT,
           ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->lti.hor_strength < N_ELEMENTS (hor_lti_strength_to_delta_slope),
           ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  ASSERTF (param->lti.ver_strength <= 31,
           ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  /* cti */
  switch (iqi_rev)
    {
    case IQI_CELL_REV1:
      ASSERTF (param->ctiR1.strength < IQICS_COUNT,
               ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
      break;

    case IQI_CELL_REV2:
      ASSERTF (param->ctiR2.strength1 < IQICS_COUNT,
               ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
      ASSERTF (param->ctiR2.strength2 < IQICS_COUNT,
               ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
      break;

    default:
      ASSERTF (1 == 2, ("%s: should not be reached!\n", __PRETTY_FUNCTION__));
      break;
    }
  /* le */
  ASSERTF (param->le.csc_gain <= 31,
           ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  /* ce */
  if (caps & PLANE_CTRL_CAPS_IQI_CE)
    {
      ASSERTF (param->ce.blue_boost <= 31,
               ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
      ASSERTF (param->ce.green_boost <= 31,
               ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
      ASSERTF (param->ce.auto_flesh <= 127,
               ("assertion failed: %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
    }
}
#else
#  define CheckInitParams(caps,rev)            do { } while(0)
#  define CheckConfigParams(caps,rev,param)    do { } while(0)
#endif


CSTi7200IQI::CSTi7200IQI (enum IQICellRevision revision,
                          ULONG                baseAddr)
{
  DEBUGF2 (4, (FENTRY ", rev: %d, baseAddr: %lx\n", __PRETTY_FUNCTION__, revision, baseAddr));

  m_Revision    = revision;
  m_baseAddress = baseAddr;

  m_PeakingLUT.pMemory = 0;
  m_LumaLUT.pMemory    = 0;

  CheckInitParams (GetCapabilities (), m_Revision);
  for (ULONG config = (ULONG) PCIQIC_FIRST; config < PCIQIC_COUNT; ++config)
    CheckConfigParams (GetCapabilities (), m_Revision,
                       &IQIConfigParams[config]);

  DEBUGF2 (4, (FEXIT "\n", __PRETTY_FUNCTION__));
}


CSTi7200IQI::~CSTi7200IQI (void)
{
  DEBUGF2 (4, (FENTRY "\n", __PRETTY_FUNCTION__));

  g_pIOS->FreeDMAArea (&m_PeakingLUT);
  g_pIOS->FreeDMAArea (&m_LumaLUT);

  DEBUGF2 (4, (FEXIT "\n", __PRETTY_FUNCTION__));
}


bool
CSTi7200IQI::Create (void)
{
  DEBUGF2 (4, (FENTRY "\n", __PRETTY_FUNCTION__));

  /* since we ignore the restrictions of the LUT (must be contained within
     one 64MB bank), we add an assertion to notice any change causing this
     to potentially happen. I.e. if sizeof (LUT) is < 4k, it will never cross
     a 64MB boundary because  */
  ASSERTF (sizeof (IQI_ClippingCurves) < 4096 - 8,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));
  /* same for Luma LUT */
  ASSERTF (sizeof (IQI_LumaLUT) < 4096 - 8,
           ("assertion failed %s:%d\n", __PRETTY_FUNCTION__, __LINE__));

  g_pIOS->AllocateDMAArea (&m_PeakingLUT, sizeof (IQI_ClippingCurves), 8, SDAAF_NONE);
  g_pIOS->AllocateDMAArea (&m_LumaLUT, sizeof (IQI_LumaLUT), 8, SDAAF_NONE);
  if (UNLIKELY (!m_PeakingLUT.pMemory
                || !m_LumaLUT.pMemory))
    {
      DEBUGF2 (1, ("%s 'out of memory'\n", __PRETTY_FUNCTION__));
      g_pIOS->FreeDMAArea (&m_PeakingLUT);
      g_pIOS->FreeDMAArea (&m_LumaLUT);
      return false;
    }
  g_pIOS->MemcpyToDMAArea (&m_PeakingLUT, 0,
                           IQI_ClippingCurves, sizeof (IQI_ClippingCurves));

  g_pIOS->MemcpyToDMAArea (&m_LumaLUT, 0,
                           IQI_LumaLUT, sizeof (IQI_LumaLUT));

  DEBUGF2 (2, ("%s: m_PeakingLUT: pMemory/pData/ulPhysical/size = %p/%p/%.8x/%lu\n",
               __PRETTY_FUNCTION__, m_PeakingLUT.pMemory, m_PeakingLUT.pData,
               m_PeakingLUT.ulPhysical, m_PeakingLUT.ulDataSize));
  DEBUGF2 (2, ("%s: m_LumaLUT: pMemory/pData/ulPhysical/size = %p/%p/%.8x/%lu\n",
               __PRETTY_FUNCTION__, m_LumaLUT.pMemory, m_LumaLUT.pData,
               m_LumaLUT.ulPhysical, m_LumaLUT.ulDataSize));

  /* initialize */
  SetDefaults ();
  SetConfiguration (PCIQIC_FIRST);

  m_IqiDemoMoveDirection  = 1;
  m_bIqiDemo              = false;
  m_bIqiDemoParamsChanged = true;

  m_Was709Colorspace = false;
  Set709Colorspace (m_Was709Colorspace);

  DEBUGF2 (4, (FEXIT "\n", __PRETTY_FUNCTION__));

  return true;
}


ULONG
CSTi7200IQI::GetCapabilities (void) const
{
  DEBUGF2 (4, ("%s\n", __PRETTY_FUNCTION__));
  return (PLANE_CTRL_CAPS_IQI
          | ((m_Revision == IQI_CELL_REV1) ? PLANE_CTRL_CAPS_IQI_CE
                                           : 0));
}


bool
CSTi7200IQI::SetControl (stm_plane_ctrl_t control,
                         ULONG            value)
{
  bool retval = true;

  DEBUGF2 (8, (FENTRY "\n", __PRETTY_FUNCTION__));

  switch (control)
    {
    case PLANE_CTRL_IQI_CONFIG:
      retval = SetConfiguration ((enum PlaneCtrlIQIConfiguration) value);
      if ((enum PlaneCtrlIQIConfiguration) value != PCIQIC_BYPASS)
        break;
      /* else fall through so demo is disabled, too */
      value = false;

    case PLANE_CTRL_IQI_DEMO:
      /* order is important so we can avoid locking */
      m_bIqiDemo              = value;
      m_bIqiDemoParamsChanged = true;
      break;

    case PLANE_CTRL_IQI_CE:
      if ((GetCapabilities () & PLANE_CTRL_CAPS_IQI_CE) == 0)
        {
          retval = false;
          break;
        }
      /* fall through */
    case PLANE_CTRL_IQI_PEAKING:
    case PLANE_CTRL_IQI_LTI:
    case PLANE_CTRL_IQI_CTI:
    case PLANE_CTRL_IQI_LE:
      retval = false;
      break;

    case PLANE_CTRL_IQI_CONTRAST:
      SetContrast (value >> 16, value & 0xffff);
      break;

    case PLANE_CTRL_IQI_SATURATION:
      SetSaturation (value);
      break;

    default:
      retval = false;
      break;
    }

  return retval;
}


bool
CSTi7200IQI::GetControl (stm_plane_ctrl_t  control,
                         ULONG            *value) const
{
  bool retval = true;

  DEBUGF2 (4, (FENTRY "\n", __PRETTY_FUNCTION__));

  switch (control)
    {
    case PLANE_CTRL_IQI_CONFIG:
      /* doesn't make sense */
      retval = false;
      break;

    case PLANE_CTRL_IQI_DEMO:
      *value = m_bIqiDemo;
      break;

    case PLANE_CTRL_IQI_CE:
      if ((GetCapabilities () & PLANE_CTRL_CAPS_IQI_CE) == 0)
        {
          retval = false;
          break;
        }
      /* fall through */
    case PLANE_CTRL_IQI_PEAKING:
    case PLANE_CTRL_IQI_LTI:
    case PLANE_CTRL_IQI_CTI:
    case PLANE_CTRL_IQI_LE:
    case PLANE_CTRL_IQI_CONTRAST:
    case PLANE_CTRL_IQI_SATURATION:
    default:
      retval = false;
      break;
    }

  return retval;
}


void
CSTi7200IQI::CalculateSetup (const stm_display_buffer_t * const pFrame,
                             struct IQISetup            * const setup)
{
  DEBUGF2 (8, (FENTRY "\n", __PRETTY_FUNCTION__));

  setup->is_709_colorspace
    = (pFrame->src.ulFlags & STM_PLANE_SRC_COLORSPACE_709) ? true : false;

  DEBUGF2 (8, (FEXIT "\n", __PRETTY_FUNCTION__));
}


void
CSTi7200IQI::UpdateHW (const struct IQISetup * const setup,
                       ULONG                  width)
{
  DEBUGF2 (9, (FENTRY "\n", __PRETTY_FUNCTION__));

  /* check demo mode */
  if (UNLIKELY (m_bIqiDemoParamsChanged))
    {
      if (LIKELY (m_bIqiDemo))
        {
          if (LIKELY (width))
            {
              USHORT start_pixel, end_pixel;

              ASSERTF (m_IqiDemoMoveDirection != 0,
                       ("%s\n", __PRETTY_FUNCTION__));

#if 1
              if ((m_IqiDemoLastStart + m_IqiDemoMoveDirection + width/3) > width)
                m_IqiDemoMoveDirection = -1;
              else if ((m_IqiDemoLastStart + m_IqiDemoMoveDirection) < 0)
                m_IqiDemoMoveDirection = 1;

              m_IqiDemoLastStart += m_IqiDemoMoveDirection;

              start_pixel = m_IqiDemoLastStart;
              end_pixel   = start_pixel + width / 3;
#else
              start_pixel = width / 2;
              end_pixel = width;
#endif
              Demo (start_pixel, end_pixel);
            }
        }
      else
        {
          Demo (0, 0);
          m_bIqiDemoParamsChanged = false;

          m_IqiDemoLastStart     = 0;
          m_IqiDemoMoveDirection = 1;
        }
    }

  /* check colorspace */
  if (LIKELY (setup)
      && UNLIKELY (setup->is_709_colorspace != m_Was709Colorspace))
    {
      DEBUGF (("%s: colorspace changed to %s\n",
               __PRETTY_FUNCTION__, 
               setup->is_709_colorspace ? "709" : "601"));
      Set709Colorspace (setup->is_709_colorspace);
      m_Was709Colorspace = setup->is_709_colorspace;
    }

  /* we don't continuously reload the peaking & luma LUTs to save some
     bandwidth.
     LUTs are loaded _after_ VSYNC only, so have to wait one vsync for
     operation to complete */
  if (UNLIKELY (m_IqiPeakingLutLoadCycles))
    if (--m_IqiPeakingLutLoadCycles == 0)
      DoneLoadPeakingLut ();
  if (UNLIKELY (m_IqiLumaLutLoadCycles))
    if (--m_IqiLumaLutLoadCycles == 0)
      DoneLoadLumaLut ();

  DEBUGF2 (9, (FEXIT "\n", __PRETTY_FUNCTION__));
}


void
CSTi7200IQI::Demo (USHORT start_pixel,
                   USHORT end_pixel)
{
  DEBUGF2 (4, ("%s start/end: %hu/%hu\n",
               __PRETTY_FUNCTION__, start_pixel, end_pixel));

  if (start_pixel != end_pixel)
    {
      WriteIQIReg (IQI_DEMO_WINDOW, end_pixel << 16 | start_pixel);
      WriteIQIReg (IQI_CONF, ReadIQIReg (IQI_CONF) | IQI_DEMO_ALL);
    }
  else
    {
      /* disable demo */
      WriteIQIReg (IQI_DEMO_WINDOW, 0x00000000);
      WriteIQIReg (IQI_CONF, ReadIQIReg (IQI_CONF) & ~IQI_DEMO_ALL);
    }
}


void
CSTi7200IQI::PeakingGainSet (enum IQIPeakingHorGain         gain_highpass,
                             enum IQIPeakingHorGain         gain_bandpass,
                             enum IQIPeakingFilterFrequency cutoff_freq,
                             enum IQIPeakingFilterFrequency center_freq)
{
  ULONG reg;
  ULONG gain, gain_divider;
  LONG  coeff0, coeff1, coeff2, coeff3, coeff4;
  ULONG coeff0_, coeff1_, coeff2_, coeff3_, coeff4_;

  /* find max gain from HP & BP */
  if (gain_highpass >= IQIPHG_0DB
      && gain_bandpass >= IQIPHG_0DB)
    {
      /* both are positive */
      if (gain_highpass >= gain_bandpass)
        {
          /* calculate relative gain */
          gain = 16 * (gain_bandpass - IQIPHG_0DB);
          gain_divider = gain_highpass - IQIPHG_0DB;

          if (gain_divider != 0)
            gain /= gain_divider;
          else
            gain = 0;

          reg = ReadIQIReg (IQI_PEAKING_GAIN) & ~PEAKING_GAIN_HOR_GAIN_MASK;
          WriteIQIReg (IQI_PEAKING_GAIN,
                       reg | (gain_highpass << PEAKING_GAIN_HOR_GAIN_SHIFT));
          coeff0 = hor_filter_coefficients_HP[cutoff_freq][0]
                   + ((hor_filter_coefficients_BP[center_freq][0]
                       * ((short) gain))
                      / 16);
          coeff1 = hor_filter_coefficients_HP[cutoff_freq][1]
                   + ((hor_filter_coefficients_BP[center_freq][1]
                       * ((short) gain))
                      / 16);
          coeff2 = hor_filter_coefficients_HP[cutoff_freq][2]
                   + ((hor_filter_coefficients_BP[center_freq][2]
                       * ((short) gain))
                      / 16);
          coeff3 = hor_filter_coefficients_HP[cutoff_freq][3]
                   + ((hor_filter_coefficients_BP[center_freq][3]
                       * ((short) gain))
                      / 16);
          coeff4 = hor_filter_coefficients_HP[cutoff_freq][4]
                   + ((hor_filter_coefficients_BP[center_freq][4]
                       * ((short) gain))
                      / 16);
        }
      else
        {
          /* calculate relative gain */
          gain = 16 * (gain_highpass - IQIPHG_0DB);
          gain_divider = gain_bandpass - IQIPHG_0DB;

          if (gain_divider != 0)
            gain /= gain_divider;
          else
            gain = 0;

          reg = ReadIQIReg (IQI_PEAKING_GAIN) & ~PEAKING_GAIN_HOR_GAIN_MASK;
          WriteIQIReg (IQI_PEAKING_GAIN,
                       reg | (gain_bandpass << PEAKING_GAIN_HOR_GAIN_SHIFT));
          coeff0 = hor_filter_coefficients_BP[center_freq][0]
                   + ((hor_filter_coefficients_HP[cutoff_freq][0]
                       * ((short) gain))
                      / 16);
          coeff1 = hor_filter_coefficients_BP[center_freq][1]
                   + ((hor_filter_coefficients_HP[cutoff_freq][1]
                       * ((short) gain))
                      / 16);
          coeff2 = hor_filter_coefficients_BP[center_freq][2]
                   + ((hor_filter_coefficients_HP[cutoff_freq][2]
                       * ((short) gain))
                      / 16);
          coeff3 = hor_filter_coefficients_BP[center_freq][3]
                   + ((hor_filter_coefficients_HP[cutoff_freq][3]
                       * ((short) gain))
                      / 16);
          coeff4 = hor_filter_coefficients_BP[center_freq][4]
                   + ((hor_filter_coefficients_HP[cutoff_freq][4]
                       * ((short) gain))
                      / 16);
        }
    }
  else
    {
      /* at least one of them is negative */
      if (gain_highpass < gain_bandpass)
        {
          reg = ReadIQIReg (IQI_PEAKING_GAIN) & ~PEAKING_GAIN_HOR_GAIN_MASK;
          WriteIQIReg (IQI_PEAKING_GAIN,
                       reg | (gain_highpass << PEAKING_GAIN_HOR_GAIN_SHIFT));
          coeff0 = hor_filter_coefficients_HP[cutoff_freq][0];
          coeff1 = hor_filter_coefficients_HP[cutoff_freq][1];
          coeff2 = hor_filter_coefficients_HP[cutoff_freq][2];
          coeff3 = hor_filter_coefficients_HP[cutoff_freq][3];
          coeff4 = hor_filter_coefficients_HP[cutoff_freq][4];
        }
      else
        {
          reg = ReadIQIReg (IQI_PEAKING_GAIN) & ~PEAKING_GAIN_HOR_GAIN_MASK;
          WriteIQIReg (IQI_PEAKING_GAIN,
                       reg | (gain_bandpass << PEAKING_GAIN_HOR_GAIN_SHIFT));
          coeff0 = hor_filter_coefficients_BP[center_freq][0];
          coeff1 = hor_filter_coefficients_BP[center_freq][1];
          coeff2 = hor_filter_coefficients_BP[center_freq][2];
          coeff3 = hor_filter_coefficients_BP[center_freq][3];
          coeff4 = hor_filter_coefficients_BP[center_freq][4];
        }
    }

  coeff0_ = (USHORT) coeff0 & (USHORT) PEAKING_COEFFX_MASK;
  coeff1_ = (USHORT) coeff1 & (USHORT) PEAKING_COEFFX_MASK;
  coeff2_ = (USHORT) coeff2 & (USHORT) PEAKING_COEFFX_MASK;
  coeff3_ = (USHORT) coeff3 & (USHORT) PEAKING_COEFFX_MASK;
  coeff4_ = (USHORT) coeff4 & (USHORT) PEAKING_COEFFX_MASK;
  WriteIQIReg (IQI_PEAKING_COEFX0, coeff0_);
  WriteIQIReg (IQI_PEAKING_COEFX1, coeff1_);
  WriteIQIReg (IQI_PEAKING_COEFX2, coeff2_);
  WriteIQIReg (IQI_PEAKING_COEFX3, coeff3_);
  WriteIQIReg (IQI_PEAKING_COEFX4, coeff4_);
}

bool
CSTi7200IQI::SetConfiguration (enum PlaneCtrlIQIConfiguration config)
{
  const struct _IQIConfigurationParameters * const param = &IQIConfigParams[config];
  ULONG                                     reg;

  if (config >= PCIQIC_COUNT)
    return false;

  /* peaking */
  if (param->conf.enable & IQI_ENABLE_PEAKING)
    {
      reg = ReadIQIReg (IQI_PEAKING_CONF) & ~(PEAKING_OVERSHOOT_MASK
                                              | PEAKING_UNDERSHOOT_MASK
                                              | PEAKING_CORING_MODE
                                              | PEAKING_V_PK_EN);
      WriteIQIReg (IQI_PEAKING_CONF,
                   reg
                   | (param->peaking.undershoot << PEAKING_UNDERSHOOT_SHIFT)
                   | (param->peaking.overshoot << PEAKING_OVERSHOOT_SHIFT)
                   | (param->peaking.coring_mode ? PEAKING_CORING_MODE : 0)
                   | (param->peaking.vertical_peaking ? PEAKING_V_PK_EN : 0)
                   );

      reg = ReadIQIReg (IQI_PEAKING_CORING_LEVEL) & ~PEAKING_CORING_LEVEL_MASK;
      WriteIQIReg (IQI_PEAKING_CORING_LEVEL,
                   reg
                   | (param->peaking.coring_level << PEAKING_CORING_LEVEL_SHIFT));

      /* vertical gain */
      reg = ReadIQIReg (IQI_PEAKING_GAIN) & ~PEAKING_GAIN_VERT_GAIN_MASK;
      WriteIQIReg (IQI_PEAKING_GAIN,
                   reg
                   | (param->peaking.ver_gain << PEAKING_GAIN_VERT_GAIN_SHIFT));
      /* clipping */
      LoadPeakingLut (param->peaking.clipping_mode);

      /* peaking cut off */
      enum IQIPeakingFilterFrequency cutoff_freq = param->peaking.highpassfreq;
      enum IQIPeakingFilterFrequency center_freq = param->peaking.bandpassfreq;
      if (cutoff_freq <= IQIPFF_0_26_FsDiv2
          && center_freq <= IQIPFF_0_26_FsDiv2)
        {
          /* in case of extended mode, multiply freq by 2 */
          WriteIQIReg (IQI_PEAKING_CONF,
                       ReadIQIReg (IQI_PEAKING_CONF) | PEAKING_EXTENDED_SIZE);
          switch (cutoff_freq)
            {
            case IQIPFF_0_15_FsDiv2: cutoff_freq = IQIPFF_0_30_FsDiv2; break;
            case IQIPFF_0_18_FsDiv2: cutoff_freq = IQIPFF_0_37_FsDiv2; break;
            case IQIPFF_0_22_FsDiv2: cutoff_freq = IQIPFF_0_44_FsDiv2; break;
            case IQIPFF_0_26_FsDiv2: cutoff_freq = IQIPFF_0_51_FsDiv2; break;
            case IQIPFF_0_30_FsDiv2: cutoff_freq = IQIPFF_0_58_FsDiv2; break;
            default:
              break;
            }
          switch (center_freq)
            {
            case IQIPFF_0_15_FsDiv2: center_freq = IQIPFF_0_30_FsDiv2; break;
            case IQIPFF_0_18_FsDiv2: center_freq = IQIPFF_0_37_FsDiv2; break;
            case IQIPFF_0_22_FsDiv2: center_freq = IQIPFF_0_44_FsDiv2; break;
            case IQIPFF_0_26_FsDiv2: center_freq = IQIPFF_0_51_FsDiv2; break;
            case IQIPFF_0_30_FsDiv2: center_freq = IQIPFF_0_58_FsDiv2; break;
            default:
              break;
            }
        }
      else if (cutoff_freq <= IQIPFF_0_26_FsDiv2
               && center_freq > IQIPFF_0_26_FsDiv2)
        {
          WriteIQIReg (IQI_PEAKING_CONF,
                       ReadIQIReg (IQI_PEAKING_CONF) | PEAKING_EXTENDED_SIZE);
          switch (cutoff_freq)
            {
            case IQIPFF_0_15_FsDiv2: cutoff_freq = IQIPFF_0_30_FsDiv2; break;
            case IQIPFF_0_18_FsDiv2: cutoff_freq = IQIPFF_0_37_FsDiv2; break;
            case IQIPFF_0_22_FsDiv2: cutoff_freq = IQIPFF_0_44_FsDiv2; break;
            case IQIPFF_0_26_FsDiv2: cutoff_freq = IQIPFF_0_51_FsDiv2; break;
            case IQIPFF_0_30_FsDiv2: cutoff_freq = IQIPFF_0_58_FsDiv2; break;
            default:
              break;
            }

          /* can't be greater than 0.63 */
          center_freq = IQIPFF_0_63_FsDiv2;
        }
      else if (cutoff_freq > IQIPFF_0_26_FsDiv2
               && center_freq <= IQIPFF_0_26_FsDiv2)
        {
          WriteIQIReg (IQI_PEAKING_CONF,
                       ReadIQIReg (IQI_PEAKING_CONF) | PEAKING_EXTENDED_SIZE);
          /* can't be greater than 0.63 */
          cutoff_freq = IQIPFF_0_63_FsDiv2;
          switch (center_freq)
            {
            case IQIPFF_0_15_FsDiv2: center_freq = IQIPFF_0_30_FsDiv2; break;
            case IQIPFF_0_18_FsDiv2: center_freq = IQIPFF_0_37_FsDiv2; break;
            case IQIPFF_0_22_FsDiv2: center_freq = IQIPFF_0_44_FsDiv2; break;
            case IQIPFF_0_26_FsDiv2: center_freq = IQIPFF_0_51_FsDiv2; break;
            case IQIPFF_0_30_FsDiv2: center_freq = IQIPFF_0_58_FsDiv2; break;
            default:
              break;
            }
        }
      else
        WriteIQIReg (IQI_PEAKING_CONF,
                     ReadIQIReg (IQI_PEAKING_CONF) & ~PEAKING_EXTENDED_SIZE);

      PeakingGainSet (param->peaking.highpassgain,
                      param->peaking.bandpassgain,
                      cutoff_freq,
                      center_freq);
    }

  /* LTI */
  if (param->conf.enable & IQI_ENABLE_LTI)
    {
      reg = ReadIQIReg (IQI_LTI_CONF) & ~(LTI_CONF_V_LTI_STRENGTH_MASK
                                          | LTI_CONF_H_LTI_STRENGTH_MASK
                                          | LTI_CONF_VERTICAL_LTI
                                          | LTI_CONF_ANTIALIASING
                                          | LTI_CONF_SELECTIVE_EDGE);
      WriteIQIReg (IQI_LTI_CONF,
                   reg
                   | (param->lti.ver_strength << LTI_CONF_V_LTI_STRENGTH_SHIFT)
                   | (param->lti.hor_strength << LTI_CONF_H_LTI_STRENGTH_SHIFT)
                   | (param->lti.vertical ? LTI_CONF_VERTICAL_LTI : 0)
                   | (param->lti.anti_aliasing ? LTI_CONF_ANTIALIASING : 0)
                   | (param->lti.selective_edge ? LTI_CONF_SELECTIVE_EDGE : 0));
      WriteIQIReg (IQI_LTI_DELTA_SLOPE,
                   hor_lti_strength_to_delta_slope[param->lti.hor_strength]);

      /* always zero hec height */
      reg = ReadIQIReg (IQI_LTI_CONF) & ~(LTI_CONF_HMMS_PREFILTER_MASK
                                          | LTI_CONF_HEC_HEIGHT_MASK
                                          | LTI_CONF_HEC_GRADIENT_MASK);
      WriteIQIReg (IQI_LTI_CONF,
                   reg
                   | (param->lti.selective_edge << LTI_CONF_HEC_GRADIENT_SHIFT)
                   | (param->lti.hmms_prefilter << LTI_CONF_HMMS_PREFILTER_SHIFT));
    }

  /* CTI */
  if (param->conf.enable & IQI_ENABLE_CTI)
    {
      switch (m_Revision)
        {
        case IQI_CELL_REV1:
          reg = ReadIQIReg (IQIr1_CTI_CONF) & ~CTIr1_STRENGTH_MASK;
          WriteIQIReg (IQIr1_CTI_CONF,
                       reg
                       | (param->ctiR1.strength << CTIr1_STRENGTH_SHIFT));
          break;

        case IQI_CELL_REV2:
          reg = ReadIQIReg (IQIr2_CTI_CONF) & ~(CTIr2_STRENGTH1_MASK
                                                | CTIr2_STRENGTH2_MASK);
          WriteIQIReg (IQIr2_CTI_CONF,
                       reg
                       | (param->ctiR2.strength1 << CTIr2_STRENGTH1_SHIFT)
                       | (param->ctiR2.strength2 << CTIr2_STRENGTH2_SHIFT));
          break;

        default:
          /* nothing, should not be reached! */
          break;
        }
    }

  /* LE */
  if (param->conf.enable & IQI_ENABLE_LE)
    {
      reg = ReadIQIReg (IQI_LE_CONF) & ~LE_WEIGHT_GAIN_MASK;
      WriteIQIReg (IQI_LE_CONF,
                   reg
                   | (param->le.csc_gain << LE_WEIGHT_GAIN_SHIFT));
    }

  /* CE */
  if ((GetCapabilities () & PLANE_CTRL_CAPS_IQI_CE)
      && param->conf.enable & IQI_ENABLE_CE)
    {
      reg = ReadIQIReg (IQI_CE_CONF) & ~(CE_BLUE_BOOST_MASK
                                         | CE_GREEN_BOOST_MASK);
      WriteIQIReg (IQI_CE_CONF,
                   reg
                   | (param->ce.blue_boost << CE_BLUE_BOOST_SHIFT)
                   | (param->ce.green_boost << CE_GREEN_BOOST_SHIFT));

      reg = ReadIQIReg (IQI_CE_FLESH) & ~CE_FLESH_VALUE_MASK;
      WriteIQIReg (IQI_CE_FLESH,
                   reg
                   | (param->ce.auto_flesh << CE_FLESH_VALUE_SHIFT));
    }

  /* write enable conf */
  reg = ReadIQIReg (IQI_CONF) & ~(IQI_ENABLE_ALL
                                  | IQI_DEMO_ALL);
  reg &= ~IQI_BYPASS;
  WriteIQIReg (IQI_CONF, reg | param->conf.enable);

  return true;
}



void
CSTi7200IQI::LoadPeakingLut (enum IQIPeakingClippingMode mode)
{
  DEBUGF2 (4, ("%s\n", __PRETTY_FUNCTION__));

  ULONG reg = ReadIQIReg (IQI_PEAKING_CONF);
  reg &= ~PEAKING_RANGE_GAIN_LUT_INIT;
  WriteIQIReg (IQI_PEAKING_CONF, reg);

  switch (mode)
    {
    case IQIPCM_NONE:
    case IQIPCM_WEAK:
    case IQIPCM_STRONG:
      WriteIQIReg (IQI_PEAKING_RANGE_GAIN_LUT_BA,
                   m_PeakingLUT.ulPhysical + mode*sizeof (IQI_ClippingCurves[0]));
      break;
    }

  reg |= PEAKING_RANGE_GAIN_LUT_INIT;
  WriteIQIReg (IQI_PEAKING_CONF, reg);

  m_IqiPeakingLutLoadCycles = 2;
}


void
CSTi7200IQI::DoneLoadPeakingLut (void)
{
  DEBUGF2 (4, ("%s\n", __PRETTY_FUNCTION__));

  ULONG reg = ReadIQIReg (IQI_PEAKING_CONF);
  reg &= ~PEAKING_RANGE_GAIN_LUT_INIT;
  WriteIQIReg (IQI_PEAKING_CONF, reg);
}


void
CSTi7200IQI::DoneLoadLumaLut (void)
{
  DEBUGF2 (4, ("%s\n", __PRETTY_FUNCTION__));

  ULONG reg = ReadIQIReg (IQI_LE_CONF);
  reg &= ~LE_LUT_INIT;
  WriteIQIReg (IQI_LE_CONF, reg);
}


void
CSTi7200IQI::Set709Colorspace (bool is709)
{
  ULONG reg = ReadIQIReg (IQI_LE_CONF) & ~LE_601;
  reg |= (is709 ? 0 : LE_601);
  WriteIQIReg (IQI_LE_CONF, reg);
}


void
CSTi7200IQI::SetContrast (USHORT contrast_gain,
                          USHORT black_level)
{
  ULONG reg = ReadIQIReg (IQI_CONBRI) & ~IQI_CONBRI_ALL_MASK;

  if (!contrast_gain)
    WriteIQIReg (IQI_CONBRI,
                 reg | black_level << 12 | CONBRI_MIN_CONTRAST_GAIN);
  else
    {
      ULONG offset = (4 * black_level) - contrast_gain;
      offset <<= 16;
      offset &= CONBRI_CONBRI_OFFSET_MASK;

      if (contrast_gain > CONBRI_MAX_CONTRAST_GAIN)
        contrast_gain = CONBRI_MAX_CONTRAST_GAIN;
      WriteIQIReg (IQI_CONBRI,
                   reg | offset << 12 | contrast_gain);
    }
}


void
CSTi7200IQI::SetSaturation (USHORT saturation_gain)
{
  ULONG reg = ReadIQIReg (IQI_SAT) & ~IQI_SAT_ALL_MASK;
  saturation_gain &= SAT_SAT_GAIN_MASK;
  WriteIQIReg (IQI_SAT, reg | saturation_gain);
}




void
CSTi7200IQI::SetDefaults (void)
{
  const struct _IQIInitParameters * const init = &IQIinitDefaults;

  /* init LE */
  ULONG reg = ReadIQIReg (IQI_LE_CONF) & ~IQI_LE_ALL_MASK;
  reg |= (init->le.weight_gain << LE_WEIGHT_GAIN_SHIFT);
  WriteIQIReg (IQI_LE_CONF, reg);

  /* load LCE LUT */
  WriteIQIReg (IQI_LE_LUT_BA, m_LumaLUT.ulPhysical);
  reg |= LE_LUT_INIT;
  WriteIQIReg (IQI_LE_CONF, reg);
  m_IqiLumaLutLoadCycles = 2;

  /* init CE */
  if (GetCapabilities () & PLANE_CTRL_CAPS_IQI_CE)
    {
      reg = ReadIQIReg (IQI_CE_CONF) & ~IQI_CE_ALL_MASK;
      WriteIQIReg (IQI_CE_CONF,
                   reg | (init->ce.threshold_blue_stretch << CE_BLUE_STRETCH_THRESH_SHIFT));
      WriteIQIReg (IQI_CE_FLESH,
                   ReadIQIReg (IQI_CE_FLESH) & ~IQI_CE_FLESH_ALL_MASK);
    }

  /* conbri */
  reg = ReadIQIReg (IQI_CONBRI) & ~IQI_CONBRI_ALL_MASK;
  WriteIQIReg (IQI_CONBRI,
               reg | CONBRI_MAX_CONTRAST_GAIN);

  /* iqi sat */
  reg = ReadIQIReg (IQI_SAT) & ~IQI_SAT_ALL_MASK;
  WriteIQIReg (IQI_SAT,
               reg | SAT_DEFAULT_SAT_GAIN);

  /* init LTI */
  WriteIQIReg (IQI_LTI_DELTA_SLOPE, ReadIQIReg (IQI_LTI_DELTA_SLOPE) & ~LTI_DELTA_SLOPE_MASK);
  WriteIQIReg (IQI_LTI_THRESHOLD,
               (init->lti.threshold_corner_test << LTI_CORNERTEST_SHIFT)
               | (init->lti.threshold_vert_diff << LTI_VERTDIFF_SHIFT)
               | (init->lti.threshold_min_correlation << LTI_MINCORRELATION_SHIFT)
               | (init->lti.threshold_disturbance << LTI_DISTURBANCE_SHIFT)
              );

  /* init CTI */
  switch (m_Revision)
    {
    case IQI_CELL_REV1:
      reg = ReadIQIReg (IQIr1_CTI_CONF) & ~IQIr1_CTI_ALL_MASK;
      WriteIQIReg (IQIr1_CTI_CONF,
                   reg
                   | (init->ctiR1.coring << CTIr1_COR_SHIFT)
                   | (CTIr1_DEFAULT_STRENGTH << CTIr1_STRENGTH_SHIFT)
                   | (init->ctiR1.tmed << CTIr1_MED_SHIFT)
                   | (init->ctiR1.tmon << CTIr1_MON_SHIFT)
                  );
      break;

    case IQI_CELL_REV2:
      reg = ReadIQIReg (IQIr2_CTI_CONF) & ~IQIr2_CTI_ALL_MASK;
      WriteIQIReg (IQIr2_CTI_CONF,
                   reg
                   | (init->ctiR2.coring << CTIr2_COR_SHIFT)
                   | (CTIr2_DEFAULT_STRENGTH2 << CTIr2_STRENGTH2_SHIFT)
                   | (CTIr2_DEFAULT_STRENGTH1 << CTIr2_STRENGTH1_SHIFT)
                   | (init->ctiR2.tmed << CTIr2_MED_SHIFT)
                   | (init->ctiR2.tmon << CTIr2_MON_SHIFT)
                  );
      break;

    default:
      ASSERTF (1 == 2, ("%s: should not be reached!\n", __PRETTY_FUNCTION__));
      break;
    }

  /* init peaking */
  for (int i = 0; i < 5; ++i)
    {
      reg = ReadIQIReg (IQI_PEAKING_COEFX0 + i*4) & ~PEAKING_COEFFX_MASK;
      reg |= init->peaking.coeffs[i] & PEAKING_COEFFX_MASK;
      WriteIQIReg (IQI_PEAKING_COEFX0 + i*4,
                   reg);
    }

  reg = ReadIQIReg (IQI_PEAKING_GAIN) & ~IQI_PEAKING_GAIN_ALL_MASK;
  WriteIQIReg (IQI_PEAKING_GAIN,
               reg
               | (init->peaking.ver_gain << PEAKING_GAIN_VERT_GAIN_SHIFT)
               | (init->peaking.hor_gain << PEAKING_GAIN_HOR_GAIN_SHIFT));

  reg = ReadIQIReg (IQI_PEAKING_CORING_LEVEL) & ~IQI_PEAKING_CORING_LEVEL_ALL_MASK;
  WriteIQIReg (IQI_PEAKING_CORING_LEVEL,
               reg | (init->peaking.coring_level << PEAKING_CORING_LEVEL_SHIFT));

  reg = ReadIQIReg (IQI_PEAKING_CONF) & ~IQI_PEAKING_CONF_ALL_MASK;
  WriteIQIReg (IQI_PEAKING_CONF,
               reg
               | (init->peaking.range_max << PEAKING_RANGEMAX_SHIFT)
               | (init->peaking.undershoot << PEAKING_UNDERSHOOT_SHIFT)
               | (init->peaking.overshoot << PEAKING_OVERSHOOT_SHIFT)
               | (init->peaking.vertical_peaking_enabled ? PEAKING_V_PK_EN : 0)
               | (init->peaking.coring_mode ? PEAKING_CORING_MODE : 0)
               | (init->peaking.envelope_detect ? PEAKING_ENV_DETECT : 0)
               | (init->peaking.extended_size ? PEAKING_EXTENDED_SIZE : 0)
              );
  LoadPeakingLut (init->peaking.clipping_mode);
}
