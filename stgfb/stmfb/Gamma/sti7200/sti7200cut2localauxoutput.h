/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cut2localauxoutput.h
 * Copyright (c) 2007,2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7200CUT2LOCALAUXOUTPUT_H
#define _STi7200CUT2LOCALAUXOUTPUT_H

#include <Gamma/GenericGammaOutput.h>

class CSTi7200Cut2LocalDevice;
class CSTmFSynthType2;
class CSTmSDVTG;
class CSTi7200LocalAuxMixer;
class CSTi7200Cut2LocalDENC;
class CSTmHDFormatterAWG;
class CSTi7111HDFormatterOutput;

class CSTi7200Cut2LocalAuxOutput: public CGenericGammaOutput
{
public:
  CSTi7200Cut2LocalAuxOutput(CSTi7200Cut2LocalDevice *,
                             CSTmSDVTG *,
                             CSTi7200Cut2LocalDENC *,
                             CSTi7200LocalAuxMixer *,
                             CSTmFSynthType2 *,
                             CSTmHDFormatterAWG *,
                             CSTi7111HDFormatterOutput *);

  virtual ~CSTi7200Cut2LocalAuxOutput();

  bool Start(const stm_mode_line_t*, ULONG tvStandard);

  ULONG GetCapabilities(void) const;

  const stm_mode_line_t* SupportedMode(const stm_mode_line_t *) const;

  bool ShowPlane(stm_plane_id_t planeID);

private:
  CSTmHDFormatterAWG *m_pAWG;
  CSTi7111HDFormatterOutput *m_pHDFOutput;

  bool SetOutputFormat(ULONG);
  void EnableDACs(void);
  void DisableDACs(void);

  void WriteMainTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C2_LOCAL_DISPLAY_BASE + STi7200_MAIN_TVOUT_OFFSET + reg)>>2), val); }
  ULONG ReadMainTVOutReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C2_LOCAL_DISPLAY_BASE + STi7200_MAIN_TVOUT_OFFSET + reg)>>2)); }

  void WriteAuxTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C2_LOCAL_AUX_TVOUT_BASE + reg)>>2), val); }
  ULONG ReadAuxTVOutReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C2_LOCAL_AUX_TVOUT_BASE + reg)>>2)); }

  void WriteTVFmtReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200C2_LOCAL_FORMATTER_BASE + reg)>>2), val); }
  ULONG ReadTVFmtReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200C2_LOCAL_FORMATTER_BASE + reg)>>2)); }

  void WriteSysCfgReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200_SYSCFG_BASE + reg)>>2), val); }
  ULONG ReadSysCfgReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200_SYSCFG_BASE +reg)>>2)); }

  void WriteClkReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200_CLKGEN_BASE + reg)>>2), val); }
  ULONG ReadClkReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200_CLKGEN_BASE +reg)>>2)); }

  CSTi7200Cut2LocalAuxOutput(const CSTi7200Cut2LocalAuxOutput&);
  CSTi7200Cut2LocalAuxOutput& operator=(const CSTi7200Cut2LocalAuxOutput&);
};


#endif //_STI7200CUT1REMOTEOUTPUT_H
