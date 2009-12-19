/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cut2localmainoutput.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7200CUT2LOCALMAINOUTPUT_H
#define _STi7200CUT2LOCALMAINOUTPUT_H

#include <Gamma/sti7111/sti7111hdfoutput.h>

class CDisplayDevice;
class CSTmFSynth;
class CSTmSDVTG;
class CSTi7200MainMixer;
class CSTi7200Cut2LocalDENC;
class CSTmHDFormatterAWG;

class CSTi7200Cut2LocalMainOutput: public CSTi7111HDFormatterOutput
{
public:
  CSTi7200Cut2LocalMainOutput(CDisplayDevice *,
                              CSTmSDVTG *,
                              CSTmSDVTG *,
                              CSTi7200Cut2LocalDENC *,
                              CGammaMixer *,
                              CSTmFSynth *,
                              CSTmHDFormatterAWG *);

  virtual ~CSTi7200Cut2LocalMainOutput();

  bool ShowPlane(stm_plane_id_t planeID);

protected:
  void StartHDClocks(const stm_mode_line_t*);
  void StartSDInterlacedClocks(const stm_mode_line_t*);
  void StartSDProgressiveClocks(const stm_mode_line_t*);
  void SetMainClockToHDFormatter(void);

  void EnableDACs(void);
  void DisableDACs(void);

private:
  void WriteSysCfgReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7200_SYSCFG_BASE + reg)>>2), val); }
  ULONG ReadSysCfgReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7200_SYSCFG_BASE +reg)>>2)); }

  CSTi7200Cut2LocalMainOutput(const CSTi7200Cut2LocalMainOutput&);
  CSTi7200Cut2LocalMainOutput& operator=(const CSTi7200Cut2LocalMainOutput&);
};


#endif //_STi7200CUT2LOCALMAINOUTPUT_H
