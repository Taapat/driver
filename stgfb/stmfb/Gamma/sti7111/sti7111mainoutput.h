/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111mainoutput.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7111MAINOUTPUT_H
#define _STi7111MAINOUTPUT_H

#include "sti7111hdfoutput.h"

class CDisplayDevice;
class CSTmFSynth;
class CSTmSDVTG;
class CSTi7111MainMixer;
class CSTi7111DENC;
class CSTmHDFormatterAWG;

class CSTi7111MainOutput: public CSTi7111HDFormatterOutput
{
public:
  CSTi7111MainOutput(CDisplayDevice *,
                     CSTmSDVTG *,
                     CSTmSDVTG *,
                     CSTi7111DENC *,
                     CGammaMixer *,
                     CSTmFSynth *,
                     CSTmFSynth *,
                     CSTmHDFormatterAWG *);

  virtual ~CSTi7111MainOutput();

  bool ShowPlane(stm_plane_id_t planeID);

protected:
  void StartHDClocks(const stm_mode_line_t*);
  void StartSDInterlacedClocks(const stm_mode_line_t*);
  void StartSDProgressiveClocks(const stm_mode_line_t*);
  void SetMainClockToHDFormatter(void);

  void EnableDACs(void);
  void DisableDACs(void);

private:
  CSTmFSynth *m_pFSynthAux;

  void WriteSysCfgReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7111_SYSCFG_BASE + reg)>>2), val); }
  ULONG ReadSysCfgReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7111_SYSCFG_BASE +reg)>>2)); }

  void WriteClkReg(ULONG reg, ULONG val);
  ULONG ReadClkReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7111_CLKGEN_BASE +reg)>>2)); }

  CSTi7111MainOutput(const CSTi7111MainOutput&);
  CSTi7111MainOutput& operator=(const CSTi7111MainOutput&);
};


#endif //_STi7111MAINOUTPUT_H
