/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200iqi.h
 * Copyright (c) 2007/2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/
#ifndef _STI7200_IQI_H
#define _STI7200_IQI_H

#include <stmdisplayplane.h>
#include <Generic/IOS.h>



struct IQISetup
{
  bool is_709_colorspace;
};


enum IQICellRevision
{
  IQI_CELL_REV1,
  IQI_CELL_REV2
};


enum IQIPeakingClippingMode {
  IQIPCM_NONE,
  IQIPCM_WEAK,
  IQIPCM_STRONG,

  IQIPCM_LAST = IQIPCM_STRONG
};

/* coeff used by peaking */
enum IQIPeakingHorGain {
  IQIPHG_N6_0DB,
  IQIPHG_N5_5DB,
  IQIPHG_N5_0DB,
  IQIPHG_N4_5DB,
  IQIPHG_N4_0DB = IQIPHG_N4_5DB,
  IQIPHG_N3_5DB,
  IQIPHG_N3_0DB = IQIPHG_N3_5DB,
  IQIPHG_N2_5DB,
  IQIPHG_N2_0DB = IQIPHG_N2_5DB,
  IQIPHG_N1_5DB,
  IQIPHG_N1_0DB = IQIPHG_N1_5DB,
  IQIPHG_N0_5DB,
  IQIPHG_0DB,
  IQIPHG_P0_5DB,
  IQIPHG_P1_0DB,
  IQIPHG_P1_5DB,
  IQIPHG_P2_0DB,
  IQIPHG_P2_5DB,
  IQIPHG_P3_0DB,
  IQIPHG_P3_5DB,
  IQIPHG_P4_0DB,
  IQIPHG_P4_5DB,
  IQIPHG_P5_0DB,
  IQIPHG_P5_5DB,
  IQIPHG_P6_0DB,
  IQIPHG_P6_5DB,
  IQIPHG_P7_0DB,
  IQIPHG_P7_5DB,
  IQIPHG_P8_0DB,
  IQIPHG_P8_5DB,
  IQIPHG_P9_0DB,
  IQIPHG_P9_5DB,
  IQIPHG_P10_0DB,
  IQIPHG_P10_5DB,
  IQIPHG_P11_0DB,
  IQIPHG_P11_5DB,
  IQIPHG_P12_0DB = IQIPHG_P11_5DB,

  IQIPHG_LAST = IQIPHG_P12_0DB
};

enum IQIPeakingFilterFrequency {
  IQIPFF_0_15_FsDiv2,
  IQIPFF_0_18_FsDiv2,
  IQIPFF_0_22_FsDiv2,
  IQIPFF_0_26_FsDiv2,
  IQIPFF_0_30_FsDiv2,
  IQIPFF_0_33_FsDiv2,
  IQIPFF_0_37_FsDiv2,
  IQIPFF_0_40_FsDiv2,
  IQIPFF_0_44_FsDiv2,
  IQIPFF_0_48_FsDiv2,
  IQIPFF_0_51_FsDiv2,
  IQIPFF_0_55_FsDiv2,
  IQIPFF_0_58_FsDiv2,
  IQIPFF_0_63_FsDiv2,

  IQIPFF_COUNT
};


class CSTi7200IQI
{
public:
  CSTi7200IQI (enum IQICellRevision revision,
               ULONG                baseAddr);
  virtual ~CSTi7200IQI (void);

  bool Create (void);

  bool SetControl (stm_plane_ctrl_t control,
                   ULONG            value);
  bool GetControl (stm_plane_ctrl_t  control,
                   ULONG            *value) const;

  virtual ULONG GetCapabilities (void) const; /* PLANE_CTRL_CAPS_IQIxxx */

  void CalculateSetup (const stm_display_buffer_t * const pFrame,
                       struct IQISetup            * const setup);
  void UpdateHW    (const struct IQISetup      * const setup,
                    ULONG                       width);


private:
  enum IQICellRevision m_Revision;

  ULONG    m_baseAddress;
  DMA_Area m_PeakingLUT;
  DMA_Area m_LumaLUT;

  bool  m_Was709Colorspace;

  bool  m_bIqiDemo;
  bool  m_bIqiDemoParamsChanged;
  short m_IqiDemoLastStart;
  short m_IqiDemoMoveDirection; /* 1: ltr, -1: rtl */
  UCHAR m_IqiPeakingLutLoadCycles;
  UCHAR m_IqiLumaLutLoadCycles;

  void SetDefaults (void);

  bool SetConfiguration (enum PlaneCtrlIQIConfiguration config);
  void SetContrast (USHORT contrast_gain,
                    USHORT black_level);
  void SetSaturation (USHORT saturation_gain);

  void LoadPeakingLut (enum IQIPeakingClippingMode mode);
  void PeakingGainSet (enum IQIPeakingHorGain         gain_highpass,
                       enum IQIPeakingHorGain         gain_bandpass,
                       enum IQIPeakingFilterFrequency cutoff_freq,
                       enum IQIPeakingFilterFrequency center_freq);
  void DoneLoadPeakingLut (void);
  void DoneLoadLumaLut    (void);

  void Set709Colorspace (bool is709); /* either 709 or 601 */

  void Demo (USHORT start_pixel,
             USHORT end_pixel); /* if start == end -> demo off*/

  void  WriteIQIReg (ULONG reg,
                     ULONG value) { DEBUGF2 (9, ("w %p <- %#.8lx\n",
                                                 (volatile ULONG *) m_baseAddress + (reg>>2),
                                                 value));
                                    g_pIOS->WriteRegister ((volatile ULONG *) m_baseAddress + (reg>>2),
                                                               value); }
  ULONG ReadIQIReg (ULONG reg)    { const ULONG value = g_pIOS->ReadRegister ((volatile ULONG *) m_baseAddress + (reg>>2));
                                    DEBUGF2 (9, ("r %p -> %#.8lx\n",
                                                 (volatile ULONG *) m_baseAddress + (reg>>2),
                                                 value));
                                    return value; }
};

#endif /* _STI7200_IQI_H */
