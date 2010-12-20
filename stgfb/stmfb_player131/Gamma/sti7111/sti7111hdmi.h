/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111hdmi.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STI7111HDMI_H
#define _STI7111HDMI_H

#include <STMCommon/stmhdfhdmi.h>

class CSTmSDVTG;
class COutput;

class CSTi7111HDMI: public CSTmHDFormatterHDMI
{
public:
  CSTi7111HDMI(CDisplayDevice *pDev, COutput *pMainOutput, CSTmSDVTG *pVTG);
  virtual ~CSTi7111HDMI(void);

  bool Create(void);

  bool Stop(void);

protected:
  bool SetInputSource(ULONG  source);
  bool PreConfiguration(const stm_mode_line_t*, ULONG tvStandard);

  void WriteSysCfgReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pDevRegs + ((m_ulSysCfgOffset+reg)>>2), val); }
  ULONG ReadSysCfgReg(ULONG reg) { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulSysCfgOffset+reg)>>2)); }

  ULONG ReadTVOutReg(ULONG reg)   { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulTVOutOffset+reg)>>2)); }
  void WriteTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pDevRegs + ((m_ulTVOutOffset+reg)>>2), val); }

private:
  void GetAudioHWState(stm_hdmi_audio_state_t *);
  int  GetAudioFrequency(void);

  void SetupRejectionPLL(const stm_mode_line_t*, ULONG tvStandard);

  void StartAudioClocks(void);

  ULONG m_ulSysCfgOffset;
  ULONG m_ulAudioOffset;
  ULONG m_ulPCMPOffset;
  ULONG m_ulSPDIFOffset;
  ULONG m_ulTVOutOffset;
  ULONG m_ulClkOffset;

  void WriteClkReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pDevRegs + ((m_ulClkOffset + reg)>>2), val); }
  ULONG ReadClkReg(ULONG reg) { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulClkOffset +reg)>>2)); }

  ULONG ReadAUDReg(ULONG reg)   { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulAudioOffset+reg)>>2)); }
  void WriteAUDReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pDevRegs + ((m_ulAudioOffset+reg)>>2), val); }

  ULONG ReadPCMPReg(ULONG reg) { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulPCMPOffset+reg)>>2)); }
  ULONG ReadSPDIFReg(ULONG reg) { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulSPDIFOffset+reg)>>2)); }

  CSTi7111HDMI(const CSTi7111HDMI&);
  CSTi7111HDMI& operator=(const CSTi7111HDMI&);

};

#endif //_STI7111HDMI_H
