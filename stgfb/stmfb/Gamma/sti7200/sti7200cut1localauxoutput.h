/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cut1localauxoutput.h
 * Copyright (c) 2007,2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7200CUT1LOCALAUXOUTPUT_H
#define _STi7200CUT1LOCALAUXOUTPUT_H

#include <Gamma/GenericGammaOutput.h>

class CSTi7200Cut1LocalDevice;
class CSTmFSynthType2;
class CSTmSDVTG;
class CSTi7200LocalAuxMixer;
class CSTi7200DENC;


class CSTi7200Cut1LocalAuxOutput: public CGenericGammaOutput
{
public:
  CSTi7200Cut1LocalAuxOutput(CSTi7200Cut1LocalDevice *,
                             CSTmSDVTG *,
                             CSTi7200DENC *,
                             CSTi7200LocalAuxMixer *,
                             CSTmFSynthType2 *);

  virtual ~CSTi7200Cut1LocalAuxOutput();

  bool Start(const stm_mode_line_t*, ULONG tvStandard);

  ULONG GetCapabilities(void) const;

  const stm_mode_line_t* SupportedMode(const stm_mode_line_t *) const;

  bool ShowPlane(stm_plane_id_t planeID);

private:
  bool SetOutputFormat(ULONG);
  void EnableDACs(void);
  void DisableDACs(void);

  void WriteTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C1_LOCAL_AUX_TVOUT_BASE + reg)>>2), val); }
  ULONG ReadTVOutReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C1_LOCAL_AUX_TVOUT_BASE + reg)>>2)); }

  void WriteTVFmtReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C1_LOCAL_FORMATTER_BASE + reg)>>2), val); }
  ULONG ReadTVFmtReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C1_LOCAL_FORMATTER_BASE + reg)>>2)); }

  void WriteSysCfgReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200_SYSCFG_BASE + reg)>>2), val); }
  ULONG ReadSysCfgReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200_SYSCFG_BASE +reg)>>2)); }

  void WriteHDMIReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C1_HDMI_BASE + reg)>>2), val); }
  ULONG ReadHDMIReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C1_HDMI_BASE + reg)>>2)); }

  CSTi7200Cut1LocalAuxOutput(const CSTi7200Cut1LocalAuxOutput&);
  CSTi7200Cut1LocalAuxOutput& operator=(const CSTi7200Cut1LocalAuxOutput&);
};


#endif //_STI7200CUT1REMOTEOUTPUT_H
