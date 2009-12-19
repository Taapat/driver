/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111hdfoutput.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7111HDFOUTPUT_H
#define _STi7111HDFOUTPUT_H

#include <Gamma/GenericGammaOutput.h>

class CDisplayDevice;
class CSTmFSynth;
class CSTmSDVTG;
class CSTb7100DENC;
class CSTmHDFormatterAWG;


/*
 * This is the HDFormatter and TVOut portion of the STi7111 main output,
 * separated from the chip's specific clock and system configuration.
 * This allows us to reuse it on at least the STi7200Cut2 where it is known
 * to be identical. We may push this up the tree at some point, but there are
 * already differences appearing in later chips so it isn't clear if it is
 * worth making it more generic at this point in time.
 */
class CSTi7111HDFormatterOutput: public CGenericGammaOutput
{
public:
  CSTi7111HDFormatterOutput(CDisplayDevice *,
                            ULONG ulMainTVOut,
                            ULONG ulAuxTVOut,
                            ULONG ulTVFmt,
                            CSTmSDVTG *,
                            CSTmSDVTG *,
                            CSTb7100DENC *,
                            CGammaMixer *,
                            CSTmFSynth *,
                            CSTmHDFormatterAWG *);

  virtual ~CSTi7111HDFormatterOutput();

  bool Start(const stm_mode_line_t*, ULONG tvStandard);
  bool Stop(void);

  ULONG GetCapabilities(void) const;

  void  SetControl(stm_output_control_t, ULONG ulNewVal);
  ULONG GetControl(stm_output_control_t) const;

  const stm_mode_line_t* SupportedMode(const stm_mode_line_t *) const;

  void SetSlaveOutputs(COutput *dvo, COutput *hdmi);

  virtual void SetUpsampler(int);

  bool  SetFilterCoefficients(const stm_display_filter_setup_t *);

protected:
  CSTmSDVTG          *m_pVTG2; // Slave VTG for SD modes on HD display outputs
  COutput            *m_pDVO;
  COutput            *m_pHDMI;
  CSTmHDFormatterAWG *m_pAWGAnalog;

  bool  m_bDacHdPowerDisabled;
  ULONG m_ulAWGLock;

  ULONG m_ulMainTVOut;
  ULONG m_ulAuxTVOut;
  ULONG m_ulTVFmt;
  bool  m_bUseAlternate2XFilter;
  stm_display_hdf_filter_setup_t m_filters[6];

  bool StartHDDisplay(const stm_mode_line_t*);
  bool StartSDInterlacedDisplay(const stm_mode_line_t*, ULONG tvStandard);
  bool StartSDProgressiveDisplay(const stm_mode_line_t*, ULONG tvStandard);

  bool SetOutputFormat(ULONG);

  virtual void StartHDClocks(const stm_mode_line_t*)            = 0;
  virtual void StartSDInterlacedClocks(const stm_mode_line_t*)  = 0;
  virtual void StartSDProgressiveClocks(const stm_mode_line_t*) = 0;

  virtual void SetMainClockToHDFormatter(void) = 0;

  virtual void EnableDACs(void)  = 0;
  virtual void DisableDACs(void) = 0;

  void WriteAuxTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((m_ulAuxTVOut + reg)>>2), val); }
  ULONG ReadAuxTVOutReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((m_ulAuxTVOut + reg)>>2)); }

  void WriteMainTVOutReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((m_ulMainTVOut + reg)>>2), val); }
  ULONG ReadMainTVOutReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((m_ulMainTVOut + reg)>>2)); }

  void WriteTVFmtReg(ULONG reg, ULONG val) { g_pIOS->WriteRegister(m_pGammaReg + ((m_ulTVFmt + reg)>>2), val); }
  ULONG ReadTVFmtReg(ULONG reg) { return g_pIOS->ReadRegister(m_pGammaReg + ((m_ulTVFmt + reg)>>2)); }

private:
  CSTi7111HDFormatterOutput(const CSTi7111HDFormatterOutput&);
  CSTi7111HDFormatterOutput& operator=(const CSTi7111HDFormatterOutput&);
};


#endif //_STi7111HDFOUTPUT_H
