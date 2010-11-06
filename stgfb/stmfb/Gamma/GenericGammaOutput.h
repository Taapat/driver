/***********************************************************************
 *
 * File: stgfb/Gamma/GenericGammaOutput.h
 * Copyright (c) 2000, 2004, 2005 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _GENERICGAMMAOUTPUT_H
#define _GENERICGAMMAOUTPUT_H

#include <Generic/Output.h>

class CGammaMixer;
class CSTmDENC;
class CSTmVTG;
class CSTmFSynth;

class CGenericGammaOutput : public COutput 
{
public:
  CGenericGammaOutput(CDisplayDevice*, CSTmDENC*, CSTmVTG*, CGammaMixer*, CSTmFSynth*);
  virtual ~CGenericGammaOutput();

  bool  Start(const stm_mode_line_t*, ULONG tvStandard);
  bool  Stop(void);
  void  Suspend(void);
  void  Resume(void);

  void  UpdateHW();

  bool  CanShowPlane(stm_plane_id_t planeID);
  bool  ShowPlane(stm_plane_id_t planeID);
  void  HidePlane(stm_plane_id_t planeID);

  bool  SetPlaneDepth(stm_plane_id_t planeID, int depth, bool activate);
  bool  GetPlaneDepth(stm_plane_id_t planeID, int *depth) const;

  ULONG SupportedControls(void) const;
  void  SetControl(stm_output_control_t, ULONG ulNewVal);
  ULONG GetControl(stm_output_control_t) const;

  stm_meta_data_result_t QueueMetadata(stm_meta_data_t *);
  void FlushMetadata(stm_meta_data_type_t);

  bool  SetFilterCoefficients(const stm_display_filter_setup_t *);

  void  SetClockReference(stm_clock_ref_frequency_t, int error_ppm);

  void  SoftReset(void);

  bool  HandleInterrupts(void);

protected:
  CSTmDENC             *m_pDENC;
  CSTmVTG              *m_pVTG; 
  CGammaMixer          *m_pMixer;
  CSTmFSynth           *m_pFSynth;
  ULONG                *m_pGammaReg;

  const stm_mode_line_t * volatile m_pPendingMode;

  ULONG                 m_ulBrightness;
  ULONG                 m_ulContrast;
  ULONG                 m_ulSaturation;
  ULONG                 m_ulHue;

  bool                  m_bUsingDENC;

  /*
   * Current DAC rescaling parameters for HD/ED analogue outputs, which can
   * be board dependent due to variation in output driver configurations.
   */
  ULONG m_maxDACVoltage;
  ULONG m_DACSaturation;
  ULONG m_DAC_43IRE;
  ULONG m_DAC_321mV;
  ULONG m_DAC_700mV;
  ULONG m_DAC_RGB_Scale;
  ULONG m_DAC_Y_Scale;
  ULONG m_DAC_C_Scale;
  ULONG m_DAC_Y_Offset;
  ULONG m_DAC_C_Offset;
  ULONG m_AWG_Y_C_Offset;

  virtual void ProgramPSIControls(void);

  virtual bool SetOutputFormat(ULONG) = 0;

  virtual void EnableDACs(void)       = 0;
  virtual void DisableDACs(void)      = 0;

  bool TryModeChange(const stm_mode_line_t*, ULONG tvStandard);
  void RecalculateDACSetup(void);

  void WriteDevReg(ULONG reg, ULONG val) { 
        DEBUGF2(1,("### CGenericGammaOutput::WriteDevReg = %08X to %08x\n", m_pGammaReg + (reg>>2), val));
        g_pIOS->WriteRegister(m_pGammaReg + (reg>>2), val); }
  ULONG ReadDevReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + (reg>>2)); }
};

#endif //_GENERICGAMMAOUTPUT_H
