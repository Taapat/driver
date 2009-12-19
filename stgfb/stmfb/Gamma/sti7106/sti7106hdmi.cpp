/***********************************************************************
 *
 * File: stgfb/Gamma/sti7106/sti7106hdmi.cpp
 * Copyright (c) 2009 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>
#include <STMCommon/stmv29iframes.h>

#include "sti7106reg.h"
#include "sti7106hdmi.h"

#include <Gamma/sti7111/sti7111tvoutreg.h>

////////////////////////////////////////////////////////////////////////////////
//

CSTi7106HDMI::CSTi7106HDMI(CDisplayDevice *pDev,
                           COutput        *pMainOutput,
                           CSTmSDVTG      *pVTG): CSTi7111HDMI(pDev, pMainOutput, pVTG)
{
  DEBUGF2 (2, (FENTRY " @ %p: pDev = %p  main output = %p\n", __PRETTY_FUNCTION__, this, pDev, pMainOutput));

  /*
   * Override the HDMI hardware version set by the 7111 class.
   */
  m_HWVersion = STM_HDMI_HW_V_2_9;

  DEBUGF2 (2, (FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));
}


CSTi7106HDMI::~CSTi7106HDMI()
{
  DENTRY();

  DEXIT();
}


bool CSTi7106HDMI::Create(void)
{
  DENTRY();

  m_pIFrameManager = new CSTmV29IFrames(m_pDisplayDevice,m_ulHDMIOffset);
  if(!m_pIFrameManager || !m_pIFrameManager->Create(this,m_pMasterOutput))
  {
    DEBUGF2(2,("Unable to create HDMI v2.9 Info Frame manager\n"));
    delete m_pIFrameManager;
    m_pIFrameManager = 0;
    return false;
  }

  /*
   * Bypass the 7111 Create as we do not want the FDMA driven IFrames
   */
  return CSTmHDFormatterHDMI::Create();
}


bool CSTi7106HDMI::SetInputSource(ULONG src)
{
  DENTRY();

  /*
   * 7106 does correctly support 8ch PCM over HDMI, so bypass the 7111 code
   */
  if(!CSTmHDFormatterHDMI::SetInputSource(src))
    return false;

  /*
   * TODO: check the effect of SYSCFG2 bit 30 on 2ch/8ch PCM, we thought
   * that this switched between two different PCM players rather than effecting
   * the I2S converters or the HDMI cell itself (which has its own configuration
   * bit for this). But on 7105 which also has this configuration bit documented
   * that doesn't seem to be the case, changing this bit does not effect
   * PCM player0 feeding the I2S converter.
   */

  DEXIT();
  return true;
}


bool CSTi7106HDMI::SetupRejectionPLL(const stm_mode_line_t *mode,
                                     ULONG                  tvStandard,
                                     ULONG                  divide)
{
  ULONG val;

  /*
   * The old BCH/rejection PLL is now reused to provide the CLKPXPLL clock
   * input to the new PHY PLL that generates the serializer clock (TMDS*10) and
   * the TMDS clock which is now fed back into the HDMI formatter instead of
   * the TMDS clock line from ClockGenB.
   *
   */
  val  = ReadSysCfgReg(SYS_CFG3);
  val &= ~(SYS_CFG3_PLL_S_HDMI_EN | SYS_CFG3_S_HDMI_RST_N);
  WriteSysCfgReg(SYS_CFG3, val);

  DEBUGF2(2,("%s: waiting for BCH powerdown\n", __PRETTY_FUNCTION__));

  while((ReadSysCfgReg(SYS_STA9) & SYS_STA9_HDMI_PLL_LOCK) != 0);

  DEBUGF2(2,("%s: got BCH powerdown\n", __PRETTY_FUNCTION__));

  /*
   * Power up the BCH PLL
   */
  val = ReadSysCfgReg(SYS_CFG3) & ~(SYS_CFG3_PLL_S_HDMI_PDIV_MASK |
                                    SYS_CFG3_PLL_S_HDMI_NDIV_MASK |
                                    SYS_CFG3_PLL_S_HDMI_MDIV_MASK);

  /*
   * PLLout = (Fin*Mdiv) / ((2 * Ndiv) / 2^Pdiv)
   */
  ULONG mdiv = 32;
  ULONG ndiv = 64;
  ULONG pdiv;

  switch(divide)
  {
    case TVOUT_CLK_BYPASS:
      DEBUGF2(2,("%s: PLL out = fsynth\n", __PRETTY_FUNCTION__));
      pdiv = 2;
      break;
    case TVOUT_CLK_DIV_2:
      DEBUGF2(2,("%s: PLL out = fsynth/2\n", __PRETTY_FUNCTION__));
      pdiv = 3;
      break;
    case TVOUT_CLK_DIV_4:
      DEBUGF2(2,("%s: PLL out = fsynth/4\n", __PRETTY_FUNCTION__));
      pdiv = 4;
      break;
    default:
      DEBUGF2(1,("%s: unsupported TMDS clock divide (0x%x)\n", __PRETTY_FUNCTION__,divide));
      return false;
  }

  val |= ((pdiv << SYS_CFG3_PLL_S_HDMI_PDIV_SHIFT) |
          (ndiv << SYS_CFG3_PLL_S_HDMI_NDIV_SHIFT) |
          (mdiv << SYS_CFG3_PLL_S_HDMI_MDIV_SHIFT) |
          SYS_CFG3_PLL_S_HDMI_EN);

  WriteSysCfgReg(SYS_CFG3, val);

  DEBUGF2(2,("%s: waiting for rejection PLL Lock\n", __PRETTY_FUNCTION__));

  while((ReadSysCfgReg(SYS_STA9) & SYS_STA9_HDMI_PLL_LOCK) == 0);

  DEBUGF2(2,("%s: got rejection PLL Lock\n", __PRETTY_FUNCTION__));

  return true;
}


bool CSTi7106HDMI::PreConfiguration(const stm_mode_line_t *mode,
                                    ULONG                  tvStandard)
{
  ULONG val;

  DEBUGF2(2,(FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  /*
   * We are re-using the 7111/7105 ClockGenB and HDFormatter clock setup,
   * but now the TMDS clock is going to come from HDMI PHY PLL not ClockGenB
   * in order to be able to generate the deepcolor TMDS clock multiples. So
   * we do not want to divide the TMDS clock in the HDFormatter any more, it
   * will always be correct. However we do need to know the divide set by the
   * master output clock setup so we can replicate it in the RejectionPLL setup
   * for the input clock to the HDMI PHY's TMDS clock generation.
   */
  ULONG divide = (ReadTVOutReg(TVOUT_CLK_SEL) & TVOUT_MAIN_CLK_SEL_TMDS(TVOUT_CLK_MASK)) >> TVOUT_MAIN_CLK_TMDS_SHIFT;

  /*
   * Adjust divide for pixel repetition. 2x pixel repetition for interlaced
   * modes (480i, 576i) will already have the correct divide (4) from 108MHz set
   * as that is the default.
   */
  if(((tvStandard & STM_OUTPUT_STD_TMDS_PIXELREP_2X) && mode->ModeParams.ScanType == SCAN_P) ||
     ((tvStandard & STM_OUTPUT_STD_TMDS_PIXELREP_4X) && mode->ModeParams.ScanType == SCAN_I))
  {
    divide = TVOUT_CLK_DIV_2;
    DEBUGF2(2,("%s: setting TMDS clock divide to 2 (54MHz)\n", __PRETTY_FUNCTION__));
  }
  else if((tvStandard & STM_OUTPUT_STD_TMDS_PIXELREP_4X) && mode->ModeParams.ScanType == SCAN_P)
  {
    divide = TVOUT_CLK_BYPASS;
    DEBUGF2(2,("%s: setting TMDS clock divide to bypass (108MHz)\n", __PRETTY_FUNCTION__));
  }

  ULONG clk = ReadTVOutReg(TVOUT_CLK_SEL) & ~TVOUT_MAIN_CLK_SEL_TMDS(TVOUT_CLK_MASK);
  clk |= TVOUT_MAIN_CLK_SEL_TMDS(TVOUT_CLK_BYPASS);
  WriteTVOutReg(TVOUT_CLK_SEL,clk);


  if(!SetupRejectionPLL(mode, tvStandard, divide))
    return false;

  DEBUGF2(2,(FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));

  return true;
}


bool CSTi7106HDMI::PostConfiguration(const stm_mode_line_t*, ULONG tvStandard)
{
  /*
   * Remove the serializer reset now the generic code has fully set up the PHY.
   */
  ULONG val = ReadSysCfgReg(SYS_CFG3) | SYS_CFG3_S_HDMI_RST_N;
  WriteSysCfgReg(SYS_CFG3, val);

  return true;
}


bool CSTi7106HDMI::Stop(void)
{
  ULONG val;

  DEBUGF2(2,(FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  CSTmHDMI::Stop();

  val = ReadSysCfgReg(SYS_CFG3);
  val &= ~(SYS_CFG3_PLL_S_HDMI_EN | SYS_CFG3_S_HDMI_RST_N);
  WriteSysCfgReg(SYS_CFG3, val);

  DEBUGF2(2,(FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));

  return true;
}
