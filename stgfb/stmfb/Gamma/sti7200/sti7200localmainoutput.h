/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200localmainoutput.h
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STI7200LOCALMAINOUTPUT_H
#define _STI7200LOCALMAINOUTPUT_H

#include <Gamma/GenericGammaOutput.h>

class CDisplayDevice;
class CSTmFSynthType2;
class CSTmSDVTG;
class CSTi7200LocalMainMixer;
class CSTi7200DENC;
class CSTmHDFormatterAWG;

class CSTi7200LocalMainOutput: public CGenericGammaOutput
{
public:
  CSTi7200LocalMainOutput(CDisplayDevice *,
                          CSTmSDVTG *,
                          CSTmSDVTG *,
                          CSTi7200DENC *,
                          CSTi7200LocalMainMixer *,
                          CSTmFSynthType2 *,
                          CSTmHDFormatterAWG *);

  virtual ~CSTi7200LocalMainOutput();

  bool Start(const stm_mode_line_t*, ULONG tvStandard);
  bool Stop(void);
  void Suspend(void);
  void Resume(void);

  bool ShowPlane(stm_plane_id_t planeID);

  ULONG GetCapabilities(void) const;

  void  SetControl(stm_output_control_t, ULONG ulNewVal);
  ULONG GetControl(stm_output_control_t) const;

  const stm_mode_line_t* SupportedMode(const stm_mode_line_t *) const;

  void SetSlaveOutputs(COutput *hdmi, COutput *dvo);

private:
  CSTmSDVTG          *m_pVTG2; // Slave VTG for SD modes on HD display outputs
  COutput            *m_pHDMI;
  COutput            *m_pDVO;
  CSTmHDFormatterAWG *m_pAWGAnalog;

  bool m_bDacHdPowerDisabled;

  bool StartHDDisplay(const stm_mode_line_t*);
  bool StartSDInterlacedDisplay(const stm_mode_line_t*, ULONG tvStandard);
  bool StartSDProgressiveDisplay(const stm_mode_line_t*, ULONG tvStandard);
  
  bool SetOutputFormat(ULONG);
  void EnableDACs(void);
  void DisableDACs(void);

  void SetFlexVPDivider(ULONG);
  void SetUpsampler(int);

  void WriteTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C1_LOCAL_DISPLAY_BASE + STi7200_MAIN_TVOUT_OFFSET + reg)>>2), val); }
  ULONG ReadTVOutReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C1_LOCAL_DISPLAY_BASE + STi7200_MAIN_TVOUT_OFFSET +reg)>>2)); }

  void WriteAuxTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C1_LOCAL_AUX_TVOUT_BASE + reg)>>2), val); }
  ULONG ReadAuxTVOutReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C1_LOCAL_AUX_TVOUT_BASE +reg)>>2)); }

  void WriteTVFmtReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C1_LOCAL_FORMATTER_BASE + reg)>>2), val); }
  ULONG ReadTVFmtReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C1_LOCAL_FORMATTER_BASE +reg)>>2)); }

  void WriteClkReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200_CLKGEN_BASE + reg)>>2), val); }
  ULONG ReadClkReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200_CLKGEN_BASE +reg)>>2)); }

  void WriteSysCfgReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200_SYSCFG_BASE + reg)>>2), val); }
  ULONG ReadSysCfgReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200_SYSCFG_BASE +reg)>>2)); }

  void WriteHDMIReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C1_HDMI_BASE + reg)>>2), val); }
  ULONG ReadHDMIReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C1_HDMI_BASE + reg)>>2)); }

  CSTi7200LocalMainOutput(const CSTi7200LocalMainOutput&);
  CSTi7200LocalMainOutput& operator=(const CSTi7200LocalMainOutput&);

  /* on CUT1, there is a bug and we need a lock in certain operations */
  ULONG m_ulAWGLock;
};

#endif //_STI7200LOCALMAINOUTPUT_H
