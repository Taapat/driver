/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111auxoutput.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7111AUXOUTPUT_H
#define _STi7111AUXOUTPUT_H

#include <Gamma/GenericGammaOutput.h>

class CGenericGammaDevice;
class CSTmFSynth;
class CSTmSDVTG;
class CSTi7111AuxMixer;
class CSTi7111DENC;
class CSTmHDFormatterAWG;
class CSTi7111HDFormatterOutput;

class CSTi7111AuxOutput: public CGenericGammaOutput
{
public:
  CSTi7111AuxOutput(CGenericGammaDevice *pDev,
                    CSTmSDVTG           *pVTG,
                    CSTi7111DENC        *pDENC,
                    CSTi7111AuxMixer    *pMixer,
                    CSTmFSynth          *pFSynth,
                    CSTmHDFormatterAWG  *pAWG,
                    CSTi7111HDFormatterOutput *pHDFOutput);

  virtual ~CSTi7111AuxOutput();

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

  void WriteMainTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7111_MAIN_TVOUT_BASE + reg)>>2), val); }
  ULONG ReadMainTVOutReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7111_MAIN_TVOUT_BASE + reg)>>2)); }

  void WriteAuxTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7111_AUX_TVOUT_BASE + reg)>>2), val); }
  ULONG ReadAuxTVOutReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7111_AUX_TVOUT_BASE + reg)>>2)); }

  void WriteTVFmtReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7111_HD_FORMATTER_BASE + reg)>>2), val); }
  ULONG ReadTVFmtReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7111_HD_FORMATTER_BASE + reg)>>2)); }

  void WriteSysCfgReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7111_SYSCFG_BASE + reg)>>2), val); }
  ULONG ReadSysCfgReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7111_SYSCFG_BASE +reg)>>2)); }

  void WriteClkReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((STi7111_CLKGEN_BASE + reg)>>2), val); }
  ULONG ReadClkReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((STi7111_CLKGEN_BASE +reg)>>2)); }

  CSTi7111AuxOutput(const CSTi7111AuxOutput&);
  CSTi7111AuxOutput& operator=(const CSTi7111AuxOutput&);
};


#endif //_STi7111AUXOUTPUT_H
