/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200hdmi.h
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STI7200HDMI_H
#define _STI7200HDMI_H

#include <STMCommon/stmhdfhdmi.h>

class CSTmSDVTG;
class CSTi7200LocalMainOutput;
class CSTi7200Cut2LocalMainOutput;
class COutput;

class CSTi7200HDMI: public CSTmHDFormatterHDMI
{
public:
  CSTi7200HDMI(CDisplayDevice *,
               ULONG ulHDMIBase,
               ULONG ulHDFormatterBase,
               stm_hdf_hdmi_hardware_version_t,
               COutput *,
               CSTmSDVTG *);

  virtual ~CSTi7200HDMI(void);

  bool Create(void);

  bool Stop(void);

protected:
  bool PreConfiguration(const stm_mode_line_t*, ULONG tvStandard);
  bool PostConfiguration(const stm_mode_line_t*, ULONG tvStandard);

  void GetAudioHWState(stm_hdmi_audio_state_t *);
  int  GetAudioFrequency(void);

  void SetupRejectionPLL(const stm_mode_line_t*, ULONG tvStandard);

  ULONG m_ulSysCfgOffset;
  ULONG m_ulAudioOffset;
  ULONG m_ulPCMPOffset;
  ULONG m_ulSPDIFOffset;

  ULONG ReadAUDReg(ULONG reg)   { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulAudioOffset+reg)>>2)); }
  void WriteAUDReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pDevRegs + ((m_ulAudioOffset+reg)>>2), val); }

  ULONG ReadPCMPReg(ULONG reg) { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulPCMPOffset+reg)>>2)); }
  ULONG ReadSPDIFReg(ULONG reg) { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulSPDIFOffset+reg)>>2)); }

  void WriteSysCfgReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pDevRegs + ((m_ulSysCfgOffset+reg)>>2), val); }
  ULONG ReadSysCfgReg(ULONG reg) { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulSysCfgOffset+reg)>>2)); }

private:
  CSTi7200HDMI(const CSTi7200HDMI&);
  CSTi7200HDMI& operator=(const CSTi7200HDMI&);
};


class CSTi7200HDMICut1: public CSTi7200HDMI
{
public:
  CSTi7200HDMICut1(CDisplayDevice *,
                   CSTi7200LocalMainOutput *,
                   CSTmSDVTG *);

  virtual ~CSTi7200HDMICut1(void);

protected:
  bool SetInputSource(ULONG  source);

private:
  CSTi7200HDMICut1(const CSTi7200HDMICut1&);
  CSTi7200HDMICut1& operator=(const CSTi7200HDMICut1&);
};


class CSTi7200HDMICut2: public CSTi7200HDMI
{
public:
  CSTi7200HDMICut2(CDisplayDevice *,
                   CSTi7200Cut2LocalMainOutput *,
                   CSTmSDVTG *);

  virtual ~CSTi7200HDMICut2(void);

  bool PreConfiguration(const stm_mode_line_t*, ULONG tvStandard);

private:
  ULONG m_ulTVOutOffset;

  ULONG ReadTVOutReg(ULONG reg)   { return g_pIOS->ReadRegister(m_pDevRegs + ((m_ulTVOutOffset+reg)>>2)); }
  void WriteTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pDevRegs + ((m_ulTVOutOffset+reg)>>2), val); }

  CSTi7200HDMICut2(const CSTi7200HDMICut2&);
  CSTi7200HDMICut2& operator=(const CSTi7200HDMICut2&);
};

#endif //_STI7200HDMI_H
