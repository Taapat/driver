/***********************************************************************
 *
 * File: stgfb/Gamma/GammaMixer.h
 * Copyright (c) 2000, 2004, 2005 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef GAMMAMIXER_H
#define GAMMAMIXER_H

class CDisplayDevice;

typedef enum
{
  /*
   * To be interpreted as an or'd bit field, hence the explicit values
   */
  STM_MA_NONE     = 0,
  STM_MA_GRAPHICS = 1,
  STM_MA_VIDEO    = 2,
  STM_MA_ALL      = 3
} stm_mixer_activation_t;

class CGammaMixer
{
public:
  CGammaMixer(CDisplayDevice* pDev, ULONG ulRegOffset);
  virtual ~CGammaMixer();

  virtual bool Start(const stm_mode_line_t *);
  virtual void Stop(void);

  bool PlaneValid(stm_plane_id_t planeID) const { return (m_validPlanes & planeID) != 0; }

  ULONG SupportedControls(void) const;
  void  SetControl(stm_output_control_t, ULONG ulNewVal);
  ULONG GetControl(stm_output_control_t) const;

  virtual bool EnablePlane(stm_plane_id_t planeID, stm_mixer_activation_t act = STM_MA_NONE);
  virtual bool DisablePlane(stm_plane_id_t planeID);

  virtual bool SetPlaneDepth(stm_plane_id_t planeID, int depth, bool activate);
  virtual bool GetPlaneDepth(stm_plane_id_t planeID, int *depth) const;

  ULONG GetActivePlanes(void) const { return m_planeMasks; }

protected:
  CDisplayDevice* m_pDev;         // Display Device pointer
  ULONG*          m_pGammaReg;    // Memory mapped registers
  ULONG           m_validPlanes;  // OR of planes in specific mixer
  int             m_crossbarSize; // The number of plane entries in the crossbar
  ULONG           m_planeMasks;   // Current enabled plane
  ULONG           m_planeActive;  // Active content, graphics or video
  ULONG           m_ulRegOffset;  // Mixer address register offset
  ULONG           m_ulBackground; // Background colour

  ULONG           m_ulAVO;
  ULONG           m_ulAVS;
  ULONG           m_ulBCO;
  ULONG           m_ulBCS;

  bool                   m_bHasYCbCrMatrix;
  stm_ycbcr_colorspace_t m_colorspaceMode;

  const stm_mode_line_t *m_pCurrentMode;

  virtual bool CanEnablePlane(stm_plane_id_t planeID);
  virtual void SetMixerForPlanes(stm_plane_id_t planeID, bool isEnabled, stm_mixer_activation_t act);

  ULONG GetColorspaceCTL(const stm_mode_line_t *pModeLine) const;

  ULONG ReadMixReg(ULONG reg) const { return g_pIOS->ReadRegister(m_pGammaReg + ((m_ulRegOffset+reg)>>2)); }
  void  WriteMixReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((m_ulRegOffset+reg)>>2), val); }

};

#endif /* GAMMAMIXER_H */
