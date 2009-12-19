/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111hdmi.cpp
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>

#include <STMCommon/stmsdvtg.h>
#include <STMCommon/stmfsynth.h>
#include <STMCommon/stmhdmiregs.h>
#include <STMCommon/stmdmaiframes.h>

#include "sti7111reg.h"
#include "sti7111device.h"
#include "sti7111mainoutput.h"
#include "sti7111hdmi.h"
#include "sti7111tvoutreg.h"
/*
 * DLL Rejection PLL setup
 */
#define SYS_CFG3_PLL_S_HDMI_PDIV_DEFAULT (2L   << SYS_CFG3_PLL_S_HDMI_PDIV_SHIFT)
#define SYS_CFG3_PLL_S_HDMI_NDIV_DEFAULT (0x64 << SYS_CFG3_PLL_S_HDMI_NDIV_SHIFT)
#define SYS_CFG3_PLL_S_HDMI_MDIV_DEFAULT (0x32 << SYS_CFG3_PLL_S_HDMI_MDIV_SHIFT)

/*
 * Some offsets into the audio hardware so we can determine the HDMI audio
 * setup without making a dependency between the display and audio drivers.
 */
#define AUD_FSYN_CFG   0x00
#define AUD_FSYN0      0x10
#define AUD_FSYN1      0x20
#define AUD_FSYN2      0x30
#define AUD_FSYN_MD    0x0
#define AUD_FSYN_PE    0x4
#define AUD_FSYN_SDIV  0x8
#define AUD_FSYN_EN    0xC

#define AUD_FSYN_CFG_RST     (1L<<0)
#define AUD_FSYN_CFG_FS0_SEL (1L<<2)
#define AUD_FSYN_CFG_FS1_SEL (1L<<3)
#define AUD_FSYN_CFG_FS2_SEL (1L<<4)
#define AUD_FSYN_CFG_FS0_EN  (1L<<6)
#define AUD_FSYN_CFG_FS1_EN  (1L<<7)
#define AUD_FSYN_CFG_FS2_EN  (1L<<8)
#define AUD_FSYN_CFG_FS0_NSB (1L<<10)
#define AUD_FSYN_CFG_FS1_NSB (1L<<11)
#define AUD_FSYN_CFG_FS2_NSB (1L<<12)
#define AUD_FSYN_CFG_NPDA    (1L<<14)
#define AUD_FSYN_CFG_SYSB_IN (1L<<23)
#define AUD_FSYN_CFG_IN_MASK (3L<<23)

#define MD_48KHZ_256FS    0xF3
#define PE_48KHZ_256FS    0x3C00
#define SDIV_48KHZ_256FS  0x4

#define AUD_PLAYER_CTRL   0x1C

#define AUD_PCMP_CTRL_MODE_MASK    0x3
#define AUD_PCMP_CTRL_CLKDIV_SHIFT 4
#define AUD_PCMP_CTRL_CLKDIV_MASK  (0xFF << AUD_PCMP_CTRL_CLKDIV_SHIFT)

#define AUD_SPDIF_CTRL_MODE_MASK    0x7
#define AUD_SPDIF_CTRL_CLKDIV_SHIFT 5
#define AUD_SPDIF_CTRL_CLKDIV_MASK  (0xFF << AUD_SPDIF_CTRL_CLKDIV_SHIFT)

////////////////////////////////////////////////////////////////////////////////
//

CSTi7111HDMI::CSTi7111HDMI(CDisplayDevice *pDev,
                           COutput        *pMainOutput,
                           CSTmSDVTG      *pVTG): CSTmHDFormatterHDMI(pDev, STM_HDMI_HW_V_1_4, STM_HDF_HDMI_REV2,
                                                                      STi7111_HDMI_BASE, STi7111_HD_FORMATTER_BASE,
                                                                      pMainOutput, pVTG)
{
  ULONG val;

  DEBUGF2 (2, (FENTRY " @ %p: pDev = %p  main output = %p\n", __PRETTY_FUNCTION__, this, pDev, pMainOutput));

  m_ulSysCfgOffset = STi7111_SYSCFG_BASE;
  m_ulAudioOffset  = STi7111_AUDCFG_BASE;
  m_ulPCMPOffset   = STi7111_HDMI_PCMPLAYER_BASE;
  m_ulSPDIFOffset  = STi7111_HDMI_SPDIF_BASE;
  m_ulTVOutOffset  = STi7111_MAIN_TVOUT_BASE;
  m_ulClkOffset    = STi7111_CLKGEN_BASE;

  /*
   * We can support hardware CTS generation for input audio clock frequencies
   * above 128*Fs, for the moment just support 256*Fs which matches the usual
   * analogue DAC master clock.
   */
  m_maxAudioClockDivide = 2;

  /*
   * Start with HDMI Serializer powered down.
   */
  val = ReadSysCfgReg(SYS_CFG2) | SYS_CFG2_HDMI_POFF;
  WriteSysCfgReg(SYS_CFG2, val);

  StartAudioClocks();

  DEBUGF2 (2, (FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));
}


CSTi7111HDMI::~CSTi7111HDMI()
{
}


bool CSTi7111HDMI::Create(void)
{
  DENTRY();

  /*
   * Try and create an FDMA driver InfoFrame manager before we call the
   * base class Create().
   */
  m_pIFrameManager = new CSTmDMAIFrames(m_pDisplayDevice,m_ulHDMIOffset,STi7111_REGISTER_BASE);
  if(!m_pIFrameManager || !m_pIFrameManager->Create(this,m_pMasterOutput))
  {
    DEBUGF2(2,("Unable to create a DMA based Info Frame manager\n"));
    /*
     * Reset m_pIFrameManager so the base class will create the CPU driven
     * version instead.
     */
    delete m_pIFrameManager;
    m_pIFrameManager = 0;
  }

  return CSTmHDFormatterHDMI::Create();
}


void CSTi7111HDMI::StartAudioClocks(void)
{
  /*
   * If we do not yet have a HDMI audio clock running, we must start it
   * with a default frequency, otherwise the SW reset of the HDMI block
   * does not work. Note we do the pcm and spdif clocks on 7111; it could
   * be sufficient to find the correct pcm clock, but lets be safe and
   * do all of them.
   */
  ULONG val = ReadAUDReg(AUD_FSYN_CFG);
  /*
   * Bring out of power down and reset if needed
   */
  if((val & AUD_FSYN_CFG_NPDA) == 0)
  {
    val &= ~AUD_FSYN_CFG_IN_MASK;
    val |= AUD_FSYN_CFG_SYSB_IN | AUD_FSYN_CFG_NPDA | AUD_FSYN_CFG_RST;

    WriteAUDReg(AUD_FSYN_CFG,val);
    g_pIOS->StallExecution(10);
    val &= ~AUD_FSYN_CFG_RST;
    WriteAUDReg(AUD_FSYN_CFG,val);
  }

  if((val & AUD_FSYN_CFG_FS0_NSB) == 0)
  {
    /*
     * Bring Fsynth0 out of digital standby
     */
    val |= AUD_FSYN_CFG_FS0_NSB | AUD_FSYN_CFG_FS0_SEL | AUD_FSYN_CFG_FS0_EN;
    WriteAUDReg(AUD_FSYN_CFG,val);

    WriteAUDReg(AUD_FSYN0+AUD_FSYN_EN,   0);
    WriteAUDReg(AUD_FSYN0+AUD_FSYN_SDIV, SDIV_48KHZ_256FS);
    WriteAUDReg(AUD_FSYN0+AUD_FSYN_MD,   MD_48KHZ_256FS);
    WriteAUDReg(AUD_FSYN0+AUD_FSYN_PE,   PE_48KHZ_256FS);
    WriteAUDReg(AUD_FSYN0+AUD_FSYN_EN,   1);
  }

  if((val & AUD_FSYN_CFG_FS1_NSB) == 0)
  {
    /*
     * Bring Fsynth1 out of digital standby
     */
    val |= AUD_FSYN_CFG_FS1_NSB | AUD_FSYN_CFG_FS1_SEL | AUD_FSYN_CFG_FS1_EN;
    WriteAUDReg(AUD_FSYN_CFG,val);

    WriteAUDReg(AUD_FSYN1+AUD_FSYN_EN,   0);
    WriteAUDReg(AUD_FSYN1+AUD_FSYN_SDIV, SDIV_48KHZ_256FS);
    WriteAUDReg(AUD_FSYN1+AUD_FSYN_MD,   MD_48KHZ_256FS);
    WriteAUDReg(AUD_FSYN1+AUD_FSYN_PE,   PE_48KHZ_256FS);
    WriteAUDReg(AUD_FSYN1+AUD_FSYN_EN,   1);
  }

  if((val & AUD_FSYN_CFG_FS2_NSB) == 0)
  {
    /*
     * Bring Fsynth2 out of digital standby
     */
    val |= AUD_FSYN_CFG_FS2_NSB | AUD_FSYN_CFG_FS2_SEL | AUD_FSYN_CFG_FS2_EN;
    WriteAUDReg(AUD_FSYN_CFG,val);

    WriteAUDReg(AUD_FSYN2+AUD_FSYN_EN,   0);
    WriteAUDReg(AUD_FSYN2+AUD_FSYN_SDIV, SDIV_48KHZ_256FS);
    WriteAUDReg(AUD_FSYN2+AUD_FSYN_MD,   MD_48KHZ_256FS);
    WriteAUDReg(AUD_FSYN2+AUD_FSYN_PE,   PE_48KHZ_256FS);
    WriteAUDReg(AUD_FSYN2+AUD_FSYN_EN,   1);
  }
}


void CSTi7111HDMI::SetupRejectionPLL(const stm_mode_line_t *mode,
                                     ULONG                  tvStandard)
{
  ULONG val;

  DENTRY();

  /*
   * The main HDMI clocks have been setup when the main output video mode
   * was started. However there is an extra PLL, fed by the main output
   * FSynth which needs to be enabled. This unfortunately lives in the sys
   * config registers, not the clockgen B registers. The startup sequence is
   * quite complex....
   *
   * First Power down the BCH PLL
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

  val |= (SYS_CFG3_PLL_S_HDMI_PDIV_DEFAULT |
          SYS_CFG3_PLL_S_HDMI_NDIV_DEFAULT |
          SYS_CFG3_PLL_S_HDMI_MDIV_DEFAULT |
          SYS_CFG3_PLL_S_HDMI_EN);


  WriteSysCfgReg(SYS_CFG3, val);

  DEBUGF2(2,("%s: waiting for rejection PLL Lock\n", __PRETTY_FUNCTION__));

  while((ReadSysCfgReg(SYS_STA9) & SYS_STA9_HDMI_PLL_LOCK) == 0);

  DEBUGF2(2,("%s: got rejection PLL Lock\n", __PRETTY_FUNCTION__));

  /*
   * Bring the serializer out of reset
   */
  val = ReadSysCfgReg(SYS_CFG3) | SYS_CFG3_S_HDMI_RST_N;
  WriteSysCfgReg(SYS_CFG3, val);

  DEBUGF2(2,("%s: waiting for HDMI DLL Lock\n", __PRETTY_FUNCTION__));

  while((ReadHDMIReg(STM_HDMI_STA) & STM_HDMI_STA_DLL_LCK) == 0);

  DEBUGF2(2,("%s: got HDMI DLL Lock\n", __PRETTY_FUNCTION__));

  DEXIT();
}


bool CSTi7111HDMI::PreConfiguration(const stm_mode_line_t *mode,
                                    ULONG                  tvStandard)
{
  ULONG val;

  DEBUGF2(2,(FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  /*
   * The main output will have set the TMDS clock to the pixel clock for
   * HD/ED modes and 2x the pixel clock for SD modes. However we now have to
   * deal with 4x pixel repeats for SD modes and 2x or 4x repeats for ED modes
   * as well, to allow higher bitrate audio transmission.
   */
  if(((tvStandard & STM_OUTPUT_STD_TMDS_PIXELREP_2X) && mode->ModeParams.ScanType == SCAN_P) ||
     ((tvStandard & STM_OUTPUT_STD_TMDS_PIXELREP_4X) && mode->ModeParams.ScanType == SCAN_I))
  {
    ULONG clk = ReadTVOutReg(TVOUT_CLK_SEL) & ~TVOUT_MAIN_CLK_SEL_TMDS(TVOUT_CLK_MASK);

    clk |= TVOUT_MAIN_CLK_SEL_TMDS(TVOUT_CLK_DIV_2);
    WriteTVOutReg(TVOUT_CLK_SEL,clk);

    clk = ReadClkReg(CKGB_DISPLAY_CFG) & ~CKGB_CFG_TMDS_HDMI(CKGB_CFG_MASK);
    clk |= CKGB_CFG_TMDS_HDMI(CKGB_CFG_DIV2);
    WriteClkReg(CKGB_DISPLAY_CFG,clk);

    DEBUGF2(2,("%s: setting TMDS clock divide to 2 (54MHz)\n", __PRETTY_FUNCTION__));
  }
  else if((tvStandard & STM_OUTPUT_STD_TMDS_PIXELREP_4X) && mode->ModeParams.ScanType == SCAN_P)
  {
    ULONG clk = ReadTVOutReg(TVOUT_CLK_SEL) & ~TVOUT_MAIN_CLK_SEL_TMDS(TVOUT_CLK_MASK);

    clk |= TVOUT_MAIN_CLK_SEL_TMDS(TVOUT_CLK_BYPASS);
    WriteTVOutReg(TVOUT_CLK_SEL,clk);

    clk = ReadClkReg(CKGB_DISPLAY_CFG) & ~CKGB_CFG_TMDS_HDMI(CKGB_CFG_MASK);
    clk |= CKGB_CFG_TMDS_HDMI(CKGB_CFG_BYPASS);
    WriteClkReg(CKGB_DISPLAY_CFG,clk);

    DEBUGF2(2,("%s: setting TMDS clock divide to bypass (108MHz)\n", __PRETTY_FUNCTION__));
  }

  /*
   * Power up HDMI serializer and setup PHY parameters for mode
   */
  val = ReadSysCfgReg(SYS_CFG2) & ~(SYS_CFG2_HDMI_POFF               |
                                    SYS_CFG2_HDMIPHY_PREEMP_ON       |
                                    SYS_CFG2_HDMIPHY_PREEMP_STR_MASK |
                                    SYS_CFG2_HDMIPHY_BUFFER_SPEED_MASK);

  if(mode->TimingParams.ulPixelClock > 74250000)
  {
    /* 1080p 50/60Hz, 2880x480p or 2880x576p */
    DEBUGF2(2,("%s: PHY to 1.6Gb/s\n", __PRETTY_FUNCTION__));
    val |= SYS_CFG2_HDMIPHY_BUFFER_SPEED_16;

    /*
     * 1080p requires pre-emphasis to be enabled.
     */
    if(mode->TimingParams.ulPixelClock > 148000000)
    {
      DEBUGF2(2,("%s: Pre-emphasis enabled\n", __PRETTY_FUNCTION__));
      val |= (3 << SYS_CFG2_HDMIPHY_PREEMP_STR_SHIFT) | SYS_CFG2_HDMIPHY_PREEMP_ON;
    }

  }
  else if(mode->TimingParams.ulPixelClock > 27027000)
  {
    /* 720p, 1080i, 1440x480p, 1440x576p, 2880x480i, 2880x576i */
    DEBUGF2(2,("%s: PHY to 800Mb/s\n", __PRETTY_FUNCTION__));
    val |= SYS_CFG2_HDMIPHY_BUFFER_SPEED_8;
  }
  else
  {
    /* ED/SD pixel doubled modes */
    DEBUGF2(2,("%s: PHY to 400Mb/s\n", __PRETTY_FUNCTION__));
    val |= SYS_CFG2_HDMIPHY_BUFFER_SPEED_4;
  }

  WriteSysCfgReg(SYS_CFG2, val);

  SetupRejectionPLL(mode,tvStandard);

  DEBUGF2(2,(FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));

  return true;
}


bool CSTi7111HDMI::Stop(void)
{
  ULONG val;

  DEBUGF2(2,(FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  CSTmHDMI::Stop();

  val = ReadSysCfgReg(SYS_CFG3);
  val &= ~(SYS_CFG3_PLL_S_HDMI_EN | SYS_CFG3_S_HDMI_RST_N);
  WriteSysCfgReg(SYS_CFG3, val);

  val = ReadSysCfgReg(SYS_CFG2) | SYS_CFG2_HDMI_POFF;
  WriteSysCfgReg(SYS_CFG2, val);

  DEBUGF2(2,(FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));

  return true;
}


int CSTi7111HDMI::GetAudioFrequency(void)
{
  ULONG sdiv;
  long  md; /* Note this is signed */
  ULONG pe;

  if(m_ulInputSource & STM_AV_SOURCE_SPDIF_INPUT)
  {
    sdiv = ReadAUDReg(AUD_FSYN2+AUD_FSYN_SDIV);
    md   = ReadAUDReg(AUD_FSYN2+AUD_FSYN_MD);
    pe   = ReadAUDReg(AUD_FSYN2+AUD_FSYN_PE);
  }
  else
  {
    sdiv = ReadAUDReg(AUD_FSYN0+AUD_FSYN_SDIV);
    md   = ReadAUDReg(AUD_FSYN0+AUD_FSYN_MD);
    pe   = ReadAUDReg(AUD_FSYN0+AUD_FSYN_PE);
  }

  return stm_fsynth_frequency(STM_CLOCK_REF_30MHZ, sdiv, md, pe);
}


void CSTi7111HDMI::GetAudioHWState(stm_hdmi_audio_state_t *state)
{
  ULONG audctrl;

  /*
   * First update the audio info frame state, the HDMI can take its audio input
   * from either the SPDIF player or PCM Player 0 via an I2S to SPDIF converter.
   */
  if(m_ulInputSource & STM_AV_SOURCE_SPDIF_INPUT)
  {
    audctrl = ReadSPDIFReg(AUD_PLAYER_CTRL);
    state->status = ((audctrl & AUD_SPDIF_CTRL_MODE_MASK) != 0)?STM_HDMI_AUDIO_RUNNING:STM_HDMI_AUDIO_STOPPED;
    state->clock_divide = (audctrl & AUD_SPDIF_CTRL_CLKDIV_MASK) >> AUD_SPDIF_CTRL_CLKDIV_SHIFT;
  }
  else
  {
    audctrl = ReadPCMPReg(AUD_PLAYER_CTRL);
    state->status = ((audctrl & AUD_PCMP_CTRL_MODE_MASK) != 0)?STM_HDMI_AUDIO_RUNNING:STM_HDMI_AUDIO_STOPPED;
    state->clock_divide = (audctrl & AUD_PCMP_CTRL_CLKDIV_MASK) >> AUD_PCMP_CTRL_CLKDIV_SHIFT;
  }
}


bool CSTi7111HDMI::SetInputSource(ULONG src)
{
  DENTRY();

  /*
   * 7111 only has 2 channel PCM going to the HDMI
   */
  if(src & STM_AV_SOURCE_8CH_I2S_INPUT)
    return false;

  if(!CSTmHDFormatterHDMI::SetInputSource(src))
    return false;

  DEXIT();
  return true;
}
