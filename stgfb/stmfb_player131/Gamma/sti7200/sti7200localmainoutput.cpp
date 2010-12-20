/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200analogueouts.cpp
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>

#include <STMCommon/stmhdmiregs.h>
#include <STMCommon/stmsdvtg.h>
#include <STMCommon/stmhdfawg.h>
#include <STMCommon/stmfsynth.h>

#include "sti7200reg.h"
#include "sti7200cut1tvoutreg.h"
#include "sti7200device.h"
#include "sti7200localmainoutput.h"
#include "sti7200mixer.h"
#include "sti7200denc.h"

#define X2_COEFF_PHASE1_123  0x00fa83fd
#define X2_COEFF_PHASE1_456  0x00a7da0f
#define X2_COEFF_PHASE1_7    0x01eb
#define X2_COEFF_PHASE2_123  0x00000000
#define X2_COEFF_PHASE2_456  0x00000000
#define X2_COEFF_PHASE3_123  0x00f306fb
#define X2_COEFF_PHASE3_456  0x0a1f3c31
#define X2_COEFF_PHASE4_123  0x00000000
#define X2_COEFF_PHASE4_456  0x00000000

#define X4_COEFF_PHASE1_123  0x00fc827f
#define X4_COEFF_PHASE1_456  0x008fe20b
#define X4_COEFF_PHASE1_7    0x01ed
#define X4_COEFF_PHASE2_123  0x00f684fc
#define X4_COEFF_PHASE2_456  0x050f7c24
#define X4_COEFF_PHASE3_123  0x00f4857c
#define X4_COEFF_PHASE3_456  0x0a1f402e
#define X4_COEFF_PHASE4_123  0x00fa027f
#define X4_COEFF_PHASE4_456  0x0e076e1d

/*
 * Note that these are the same as 4x, we do not have any specific 8x
 * coefficients at this time.
 */
#define X8_COEFF_PHASE1_123  0x00fc827f
#define X8_COEFF_PHASE1_456  0x008fe20b
#define X8_COEFF_PHASE1_7    0x01ed
#define X8_COEFF_PHASE2_123  0x00f684fc
#define X8_COEFF_PHASE2_456  0x050f7c24
#define X8_COEFF_PHASE3_123  0x00f4857c
#define X8_COEFF_PHASE3_456  0x0a1f402e
#define X8_COEFF_PHASE4_123  0x00fa027f
#define X8_COEFF_PHASE4_456  0x0e076e1d

/*
 * Local Aux TVOut specific config registers definitions
 */
#define TVOUT_AUX_PADS_DAC_POFF (1L<<9)
#define TVOUT_AUX_CLK_SEL_PIX1X_SHIFT (0)
#define TVOUT_AUX_CLK_SEL_DENC_SHIFT  (2)

/*
 * Local Main TVOut specific config registers definitions
 */
#define TVOUT_MAIN_PADS_AUX_NOT_MAIN_SYNC (1L<<2)
#define TVOUT_MAIN_PADS_DAC_POFF          (1L<<11)

#define TVOUT_CLK_SEL_PIX_MAIN_SHIFT   (0)
#define TVOUT_CLK_SEL_PIX_MAIN_MASK    (3L<<TVOUT_CLK_SEL_PIX_MAIN_SHIFT)
#define TVOUT_CLK_SEL_TMDS_MAIN_SHIFT  (2)
#define TVOUT_CLK_SEL_TMDS_MAIN_MASK   (3L<<TVOUT_CLK_SEL_TMDS_MAIN_SHIFT)
#define TVOUT_CLK_SEL_BCH_MAIN_SHIFT   (4)
#define TVOUT_CLK_SEL_BCH_MAIN_MASK    (3L<<TVOUT_CLK_SEL_BCH_MAIN_SHIFT)
#define TVOUT_CLK_SEL_FDVO_MAIN_SHIFT  (6)
#define TVOUT_CLK_SEL_FDVO_MAIN_MASK   (3L<<TVOUT_CLK_SEL_FDVO_MAIN_SHIFT)

#define TVOUT_SYNC_SEL_FDVO_SHIFT (0)
#define TVOUT_SYNC_SEL_FDVO_MASK  (3L<<TVOUT_SYNC_SEL_DENC_SHIFT)
#define TVOUT_SYNC_SEL_FMT_SHIFT  (2)
#define TVOUT_SYNC_SEL_FMT_MASK   (3L<<TVOUT_SYNC_SEL_FMT_SHIFT)
#define TVOUT_SYNC_SEL_HDMI_SHIFT (4)
#define TVOUT_SYNC_SEL_HDMI_MASK  (3L<<TVOUT_SYNC_SEL_HDMI_SHIFT)
#define TVOUT_SYNC_SEL_AWG_SHIFT  (6)
#define TVOUT_SYNC_SEL_AWG_MASK   (3L<<TVOUT_SYNC_SEL_AWG_SHIFT)

#define VTG_AWG_SYNC_ID  2 /* for syncs sent to TVOUT_SYNC_SEL_SYNC2 */
#define VTG_HDMI_SYNC_ID 3 /* for syncs sent to TVOUT_SYNC_SEL_SYNC3 */

#define MAIN_TO_DENC_DELAY  (-9)


CSTi7200LocalMainOutput::CSTi7200LocalMainOutput(CDisplayDevice         *pDev,
                                                 CSTmSDVTG              *pVTG1,
                                                 CSTmSDVTG              *pVTG2,
                                                 CSTi7200DENC           *pDENC,
                                                 CSTi7200LocalMainMixer *pMixer,
                                                 CSTmFSynthType2        *pFSynth1,
                                                 CSTmHDFormatterAWG     *pAWG): CGenericGammaOutput(pDev, pDENC, pVTG1, pMixer, pFSynth1)
{
  m_pVTG2      = pVTG2;
  m_pHDMI      = 0;
  m_pDVO       = 0;
  m_pAWGAnalog = pAWG;
  m_ulAWGLock  = g_pIOS->CreateResourceLock();

  m_bDacHdPowerDisabled = false;

  /*
   * Recommended STi7200 design is for a 1.4v max output voltage
   * from the video DACs. By default we set the saturation point
   * to the full range of the 10bit DAC. See RecalculateDACSetup for
   * more details.
   */
  m_maxDACVoltage  = 1400; // in mV
  m_DACSaturation  = 1023;
  RecalculateDACSetup();

  DEBUGF2(2,("CSTi7200LocalMainOutput::CSTi7200LocalMainOutput - m_pVTG   = %p\n", m_pVTG));
  DEBUGF2(2,("CSTi7200LocalMainOutput::CSTi7200LocalMainOutput - m_pVTG2  = %p\n", m_pVTG2));
  DEBUGF2(2,("CSTi7200LocalMainOutput::CSTi7200LocalMainOutput - m_pDENC  = %p\n", m_pDENC));
  DEBUGF2(2,("CSTi7200LocalMainOutput::CSTi7200LocalMainOutput - m_pMixer = %p\n", m_pMixer));

  /* Select Main compositor input and output defaults */
  m_ulInputSource = STM_AV_SOURCE_MAIN_INPUT;
  CSTi7200LocalMainOutput::SetControl(STM_CTRL_VIDEO_OUT_SELECT, STM_VIDEO_OUT_YUV);

  /*
   * Default DAC routing and analogue output config
   */
  WriteTVFmtReg(TVFMT_ANA_CFG, (ANA_CFG_RCTRL_R << ANA_CFG_REORDER_RSHIFT) |
                               (ANA_CFG_RCTRL_G << ANA_CFG_REORDER_GSHIFT) |
                               (ANA_CFG_RCTRL_B << ANA_CFG_REORDER_BSHIFT));

  /*
   * Default pad configuration, note that the H/VSYNC1 output of the VTG
   * appears to be unusable, so use H/VSYNC2 for VGA syncs instead.
   */
  WriteTVOutReg(TVOUT_PADS_CTL, TVOUT_PADS_HSYNC_HSYNC2 |
                                TVOUT_PADS_VSYNC_VSYNC2 |
                                TVOUT_MAIN_PADS_DAC_POFF);

  /*
   * Default sync selection to subblocks
   */
  WriteTVOutReg(TVOUT_SYNC_SEL, (TVOUT_SYNC_SEL_REF   << TVOUT_SYNC_SEL_FDVO_SHIFT)  |
                                (TVOUT_SYNC_SEL_SYNC2 << TVOUT_SYNC_SEL_FMT_SHIFT)   |
                                (TVOUT_SYNC_SEL_SYNC3 << TVOUT_SYNC_SEL_HDMI_SHIFT)  |
                                (TVOUT_SYNC_SEL_SYNC2 << TVOUT_SYNC_SEL_AWG_SHIFT));

  /*
   * The syncs to the HDMI and HDFormatter blocks must be positive regardless
   * of the display standard.
   */
  m_pVTG->SetSyncType(VTG_HDMI_SYNC_ID, STVTG_SYNC_POSITIVE);
  m_pVTG->SetSyncType(VTG_AWG_SYNC_ID, STVTG_SYNC_POSITIVE);

  m_pAWGAnalog->CacheFirmwares(this);
}


CSTi7200LocalMainOutput::~CSTi7200LocalMainOutput()
{
  g_pIOS->DeleteResourceLock(m_ulAWGLock);
}


void CSTi7200LocalMainOutput::SetSlaveOutputs(COutput *hdmi, COutput *dvo)
{
  m_pHDMI = hdmi;
  m_pDVO  = dvo;
}


const stm_mode_line_t* CSTi7200LocalMainOutput::SupportedMode(const stm_mode_line_t *mode) const
{
  DEBUGF2(3,("CSTi7200LocalMainOutput::SupportedMode\n"));

  if(mode->ModeParams.OutputStandards & STM_OUTPUT_STD_HD_MASK)
  {
    DEBUGF2(3,("CSTi7200LocalMainOutput::SupportedMode Looking for valid HD mode, pixclock = %lu\n",mode->TimingParams.ulPixelClock));

    if(mode->ModeParams.OutputStandards & STM_OUTPUT_STD_AS4933)
    {
      DEBUGF2(3,("CSTi7200LocalMainOutput::SupportedMode AS4933 not supported\n"));
      return 0;
    }

    if(mode->TimingParams.ulPixelClock <= 74250000)
    {
      DEBUGF2(3,("CSTb7100MainOutput::SupportedMode Found valid HD mode\n"));
      return mode;
    }
  }
  else
  {
    /*
     * We support SD interlaced modes based on a 13.5Mhz pixel clock
     * and progressive modes at 27Mhz or 27.027Mhz. We also report support
     * for SD interlaced modes over HDMI where the clock is doubled and
     * pixels are repeated. Finally we support VGA (640x480p@59.94Hz)
     * for digital displays (25.18MHz pixclock)
     */
    if(mode->ModeParams.OutputStandards & (STM_OUTPUT_STD_SMPTE293M | STM_OUTPUT_STD_VESA))
    {
      DEBUGF2(3,("CSTi7200LocalMainOutput::SupportedMode Looking for valid SD progressive mode\n"));
      if(mode->TimingParams.ulPixelClock == 27000000 ||
         mode->TimingParams.ulPixelClock == 27027000 ||
         mode->TimingParams.ulPixelClock == 25174800 ||
         mode->TimingParams.ulPixelClock == 25200000)
        return mode;
    }
    else if(mode->ModeParams.OutputStandards & STM_OUTPUT_STD_TMDS_PIXELREP_2X)
    {
      DEBUGF2(3,("CSTi7200LocalMainOutput::SupportedMode Looking for valid SD HDMI mode\n"));
      if(mode->TimingParams.ulPixelClock == 27000000)
        return mode;
    }
    else
    {
      DEBUGF2(3,("CSTi7200LocalMainOutput::SupportedMode Looking for valid SD interlaced mode\n"));
      if(mode->TimingParams.ulPixelClock == 13500000)
        return mode;
    }
  }

  return 0;
}


ULONG CSTi7200LocalMainOutput::GetCapabilities(void) const
{
  ULONG caps =  (STM_OUTPUT_CAPS_SD_ANALOGUE       |
                 STM_OUTPUT_CAPS_ED_ANALOGUE       |
                 STM_OUTPUT_CAPS_HD_ANALOGUE       |
                 STM_OUTPUT_CAPS_PLANE_MIXER       |
                 STM_OUTPUT_CAPS_MODE_CHANGE       |
                 STM_OUTPUT_CAPS_SD_RGB_CVBS_YC    | /* SCART, YC optional */
                 STM_OUTPUT_CAPS_YPrPb_EXCLUSIVE   | /* YPrPb, _no_ CVBS/YC available at the same time    */
                 STM_OUTPUT_CAPS_CVBS_YC_EXCLUSIVE | /* SD CVBS/YC, _no_ YPrPb available at the same time */
                 STM_OUTPUT_CAPS_RGB_EXCLUSIVE);     /* VGA via DSub connector */

  return caps;
}


bool CSTi7200LocalMainOutput::SetOutputFormat(ULONG format)
{
  DEBUGF2(2,("CSTi7200LocalMainOutput::SetOutputFormat - format = 0x%lx\n",format));

  switch(format)
  {
    case 0:
    case STM_VIDEO_OUT_RGB:
    case STM_VIDEO_OUT_YUV:
    case STM_VIDEO_OUT_YC:
    case STM_VIDEO_OUT_CVBS:
    case (STM_VIDEO_OUT_YC  | STM_VIDEO_OUT_CVBS):
    case (STM_VIDEO_OUT_RGB | STM_VIDEO_OUT_CVBS):
    case (STM_VIDEO_OUT_RGB | STM_VIDEO_OUT_YC | STM_VIDEO_OUT_CVBS):
      break;
    default:
      return false;
  }

  if(!m_pCurrentMode)
    return true;

  ULONG cfgana = ReadTVFmtReg(TVFMT_ANA_CFG) & ~(ANA_CFG_INPUT_MASK|ANA_CFG_PRPB_SYNC_OFFSET_MASK);

  if(m_bUsingDENC)
  {
    if(format & STM_VIDEO_OUT_RGB)
    {
      /*
       * If we are doing SD and want RGB, this actually means we are trying
       * to do euro SCART output. This needs RGB with no syncs and CVBS with
       * syncs and optionally colour (which at the moment we don't control,
       * you always get colour.) S-Video can also be output at the same time
       * and also carried on SCART or on a separate connector. In this case
       * the DENC provides the CVBS and S-Video and the HD formatter must
       * take the RGB straight from the mixer and rescale it.
       */
      if(!m_pDENC->SetMainOutputFormat(format & ~STM_VIDEO_OUT_RGB))
      {
        DEBUGF2(2,("CSTi7200LocalMainOutput::SetOutputFormat - format unsupported on DENC\n"));
        return false;
      }

      /*
       * Upsample compositor RGB output from 13.5MHz pixel clock to 108MHz DAC
       * clock.
       */
      SetUpsampler(8);

      /*
       * Drop through to the RGB setup below.
       */
      format = STM_VIDEO_OUT_RGB;
    }
    else
    {
      /*
       * Either YUV or CVBS/S-Video from the DENC. YUV will get output from the
       * HD DACs and CVBS/S-Video from the SD DACs, this is sorted out by a call
       * to EnableDACs() later.
       */
      if(!m_pDENC->SetMainOutputFormat(format))
      {
        DEBUGF2(2,("CSTi7200LocalMainOutput::SetOutputFormat - format unsupported on DENC\n"));
        return false;
      }

      /*
       * If this was the 710x then we would have to upsample the DENC output 4x
       * for the HD DACs. But that doesn't work, so we have to disable the
       * upsampler on the 7200, still to be explained.
       */
      SetUpsampler(0);

      cfgana |= ANA_CFG_INPUT_AUX;
      WriteTVFmtReg(TVFMT_ANA_CFG, cfgana);
      WriteTVFmtReg(TVFMT_ANA_Y_SCALE, 0x400);
      WriteTVFmtReg(TVFMT_ANA_C_SCALE, 0x400);
      return true;
    }
  }

  /*
   * Select the colour format for use with progressive/HD modes. We also
   * need to change the Chroma/Red/Blue DAC level scaling. Note we only
   * support sync on Luma/Green for consumer applications at the moment.
   */
  switch(format & (STM_VIDEO_OUT_RGB | STM_VIDEO_OUT_YUV))
  {
    case STM_VIDEO_OUT_RGB:
    {
      cfgana |= ANA_CFG_INPUT_RGB | ANA_CFG_CLIP_GB_N_CRCB;

      if(m_ulTVStandard & STM_OUTPUT_STD_VESA)
      {
      	/*
      	 * For VESA modes use the full RGB range mapped to 0-0.7v
      	 */
        WriteTVFmtReg(TVFMT_ANA_Y_SCALE, m_DAC_RGB_Scale);
        WriteTVFmtReg(TVFMT_ANA_C_SCALE, m_DAC_RGB_Scale);
      }
      else
      {
       /*
        * For TV modes using component RGB use the same scaling and offset
        * as Luma (Y) for all three signals.
        */
        WriteTVFmtReg(TVFMT_ANA_Y_SCALE, m_DAC_Y_Scale | (m_DAC_Y_Offset<<ANA_SCALE_OFFSET_SHIFT));
        WriteTVFmtReg(TVFMT_ANA_C_SCALE, m_DAC_Y_Scale | (m_DAC_Y_Offset<<ANA_SCALE_OFFSET_SHIFT));
      }
      break;
    }
    case STM_VIDEO_OUT_YUV:
    {
      cfgana |= ANA_CFG_INPUT_YCBCR;
      cfgana &= ~ANA_CFG_CLIP_GB_N_CRCB;

      if(m_ulTVStandard & STM_OUTPUT_STD_VESA)
      {
      	/*
      	 * VESA modes not defined in YUV so blank the signal at 0V
      	 */
        WriteTVFmtReg(TVFMT_ANA_Y_SCALE, 0);
        WriteTVFmtReg(TVFMT_ANA_C_SCALE, 0);
      }
      else
      {
        WriteTVFmtReg(TVFMT_ANA_Y_SCALE, m_DAC_Y_Scale | (m_DAC_Y_Offset<<ANA_SCALE_OFFSET_SHIFT));
        WriteTVFmtReg(TVFMT_ANA_C_SCALE, m_DAC_C_Scale | (m_DAC_C_Offset<<ANA_SCALE_OFFSET_SHIFT));

        cfgana |= (m_AWG_Y_C_Offset  << ANA_CFG_PRPB_SYNC_OFFSET_SHIFT) & ANA_CFG_PRPB_SYNC_OFFSET_MASK;
      }

      break;
    }
    default:
    {
      /*
       * No HD DAC output selected, blank the signal, although the DACs will
       * not actually get switched on.
       */
      WriteTVFmtReg(TVFMT_ANA_Y_SCALE, 0);
      WriteTVFmtReg(TVFMT_ANA_C_SCALE, 0);
      break;
    }
  }

  DEBUGF2(2,("CSTi7200LocalMainOutput::SetOutputFormat TVFMT_YSCALE\t\t%#.8lx\n", ReadTVFmtReg(TVFMT_ANA_Y_SCALE)));
  DEBUGF2(2,("CSTi7200LocalMainOutput::SetOutputFormat TVFMT_CSCALE\t\t%#.8lx\n", ReadTVFmtReg(TVFMT_ANA_C_SCALE)));

  if(m_signalRange == STM_SIGNAL_VIDEO_RANGE)
    cfgana |= ANA_CFG_CLIP_EN;
  else
    cfgana &= ~ANA_CFG_CLIP_EN;

  WriteTVFmtReg(TVFMT_ANA_CFG, cfgana);
  DEBUGF2(2,("CSTi7200LocalMainOutput::SetOutputFormat TVFMT_ANA_CFG\t\t%#.8lx\n", ReadTVFmtReg(TVFMT_ANA_CFG)));

  /*
   * Make sure syncs (if programmed) are enabled.
   */
  m_pAWGAnalog->Enable();

  DEBUGF2(2,("CSTi7200LocalMainOutput::SetOutputFormat - out\n"));
  return true;
}


void CSTi7200LocalMainOutput::SetControl(stm_output_control_t ctrl, ULONG val)
{
  DEBUGF2(2,("CSTi7200LocalMainOutput::SetControl ctrl = %u val = %lu\n",(unsigned)ctrl,val));
  switch(ctrl)
  {
    case STM_CTRL_SIGNAL_RANGE:
    {
      CGenericGammaOutput::SetControl(ctrl,val);
      /*
       * Enable the change in the HD formatter
       */
      SetOutputFormat(m_ulOutputFormat);
      break;
    }
    case STM_CTRL_DAC_HD_POWER:
    {
      m_bDacHdPowerDisabled = (val != 0);
      if (m_pCurrentMode)
      {
        if(!m_bDacHdPowerDisabled)
          EnableDACs();
        else
        {
          val = ReadTVOutReg(TVOUT_PADS_CTL) | TVOUT_MAIN_PADS_DAC_POFF;
          WriteTVOutReg(TVOUT_PADS_CTL, val);
        }
      }
      break;
    }
    case STM_CTRL_YCBCR_COLORSPACE:
    {
      if(m_pHDMI)
        m_pHDMI->SetControl(ctrl,val);
      // Fallthrough to base class in order to configure the mixer
    }
    default:
      CGenericGammaOutput::SetControl(ctrl,val);
      break;
  }
}


ULONG CSTi7200LocalMainOutput::GetControl(stm_output_control_t ctrl) const
{
  switch(ctrl)
  {
    case STM_CTRL_DAC_HD_POWER:
      return m_bDacHdPowerDisabled?1:0;
    default:
      return CGenericGammaOutput::GetControl(ctrl);
  }

  return 0;
}


bool CSTi7200LocalMainOutput::Start(const stm_mode_line_t *mode, ULONG tvStandard)
{
  DEBUGF2(2,("CSTi7200LocalMainOutput::Start - in\n"));

  /*
   * Strip any secondary mode flags out when SMPTE293M is defined.
   * Other chips need additional modes for simultaneous re-interlaced DENC
   * output. Stripping that out lets us use tvStandard more sensibly throughout,
   * making the code more readable.
   */
  if(tvStandard & STM_OUTPUT_STD_SMPTE293M)
    tvStandard = STM_OUTPUT_STD_SMPTE293M;

  if((mode->ModeParams.OutputStandards & tvStandard) != tvStandard)
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::Start - requested standard not supported by mode\n"));
    return false;
  }

  if(m_bIsSuspended)
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::Start output is suspended\n"));
    return false;
  }

  /*
   * First try to change the display mode on the fly, if that works there is
   * nothing else to do.
   */
  if(TryModeChange(mode, tvStandard))
  {
    DEBUGF2(2,("CSTi7200LocalMainOutput::Start - mode change successfull\n"));
    return true;
  }

  if(m_pCurrentMode)
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::Start - failed, output is active\n"));
    return false;
  }

  if(mode->ModeParams.OutputStandards & STM_OUTPUT_STD_HD_MASK)
  {
    if(!StartHDDisplay(mode))
      return false;
  }
  else if(tvStandard & STM_OUTPUT_STD_SD_MASK)
  {
    if(!StartSDInterlacedDisplay(mode, tvStandard))
      return false;
  }
  else
  {
    /*
     * Note that this path also deals with VESA (VGA) modes.
     */
    if(!StartSDProgressiveDisplay(mode, tvStandard))
      return false;
  }

  if(!m_pMixer->Start(mode))
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::Start Mixer start failed\n"));
    return false;
  }

  /*
   * We don't want anything from CGenericGammaOutput::Start, but we do
   * need to call the base class Start.
   */
  COutput::Start(mode, tvStandard);

  /*
   * Now update the output DAC scaling and colour format, which is dependent
   * on exactly which display standard is set.
   */
  SetOutputFormat(m_ulOutputFormat);

  /*
   * Now we have the analogue setup finialized, turn on the DACs
   */
  EnableDACs();

  DEBUGF2(2,("CSTi7200LocalMainOutput::Start - out\n"));
  return true;
}


bool CSTi7200LocalMainOutput::StartSDInterlacedDisplay(const stm_mode_line_t *mode, ULONG tvStandard)
{
  ULONG val;

  DEBUGF2(2,("CSTi7200LocalMainOutput::StartSDInterlacedDisplay\n"));

  /*
   * Just make sure someone hasn't specified an SD progressive mode
   * without specifying OUTPUT_CAPS_SD_SMPTE293M
   */
  if(mode->ModeParams.ScanType != SCAN_I)
    return false;

  if(m_pVTG2->GetCurrentMode() != 0)
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::StartSDInterlacedDisplay - cannot set interlaced SD mode, SD path in use\n"));
    return false;
  }

  m_pFSynth->SetDivider(8);

  /*
   * First Set the AuxTVout to take the 108MHz HD clock
   */
  val = ReadHDMIReg(STM_HDMI_SYNC_CFG) | HDMI_GPOUT_LOCAL_SDTVO_HD | HDMI_GPOUT_LOCAL_AUX_PIX_HD;
  WriteHDMIReg(STM_HDMI_SYNC_CFG, val);

  val = ReadAuxTVOutReg(TVOUT_PADS_CTL) & ~(TVOUT_CLK_MASK << TVOUT_AUX_PADS_LOCAL_AUX_DIV_SHIFT);
  val |= (TVOUT_CLK_DIV_8 << TVOUT_AUX_PADS_LOCAL_AUX_DIV_SHIFT);
  WriteAuxTVOutReg(TVOUT_PADS_CTL, val);

  WriteAuxTVOutReg(TVOUT_CLK_SEL, ((TVOUT_CLK_DIV_4 << TVOUT_AUX_CLK_SEL_DENC_SHIFT) |
                                   (TVOUT_CLK_DIV_8 << TVOUT_AUX_CLK_SEL_PIX1X_SHIFT)));


  /*
   * Now set all the main pipeline clock divides
   */
  WriteTVOutReg(TVOUT_CLK_SEL, ((TVOUT_CLK_DIV_8  << TVOUT_CLK_SEL_PIX_MAIN_SHIFT)   |   // 13.5MHz
                                (TVOUT_CLK_DIV_4  << TVOUT_CLK_SEL_TMDS_MAIN_SHIFT)  |   // 27MHz (Pixel doubled)
                                (TVOUT_CLK_DIV_2  << TVOUT_CLK_SEL_BCH_MAIN_SHIFT)   |   // 54MHz (2*TMDS)
                                (TVOUT_CLK_DIV_4  << TVOUT_CLK_SEL_FDVO_MAIN_SHIFT)));   // 13.5MHz

  /*
   * We need to set the TMDS divide in two places, why?
   */
  val = ReadAuxTVOutReg(TVOUT_PADS_CTL) & ~(TVOUT_CLK_MASK << TVOUT_AUX_PADS_TMDS_DIV_SHIFT);
  val |= (TVOUT_CLK_DIV_4 << TVOUT_AUX_PADS_TMDS_DIV_SHIFT);
  WriteAuxTVOutReg(TVOUT_PADS_CTL, val);

  SetFlexVPDivider(TVOUT_CLK_DIV_8);

  /*
   * Now the analogue formatter configuration
   */
  val = ReadTVFmtReg(TVFMT_ANA_CFG) & ~(ANA_CFG_INPUT_MASK           |
                                        ANA_CFG_SYNC_FILTER_DEL_MASK |
                                        ANA_CFG_SYNC_EN              |
                                        ANA_CFG_CLIP_EN              |
                                        ANA_CFG_PREFILTER_EN         |
                                        ANA_CFG_SYNC_ON_PRPB);

  val |= (ANA_CFG_SEL_MAIN_TO_DENC | ANA_CFG_SYNC_FILTER_DEL_SD);

  WriteTVFmtReg(TVFMT_ANA_CFG, val);

  /*
   * We are going to reuse the AWG syncs here to generate the signals that will
   * drive VTG2 and then the DENC. The auxilary output must be configured to
   * receive the correct sync set.
   */
  m_pVTG->SetHSyncOffset(VTG_AWG_SYNC_ID,0);
  m_pVTG->SetVSyncHOffset(VTG_AWG_SYNC_ID,0);
  /*
   * Setup sync signals sent to VTG2 for it to lock to.
   *
   * Note: using TopNotBot produces a much more reliable startup of
   * the slave VTG than BotNotTop, possibly because you get an immediate
   * rising edge as soon as VTG1 starts.
   */
  m_pVTG->SetSyncType(VTG_AWG_SYNC_ID, STVTG_SYNC_TOP_NOT_BOT);
  m_pVTG2->SetSyncType(0, STVTG_SYNC_TOP_NOT_BOT);

  m_pVTG2->SetHSyncOffset(1, MAIN_TO_DENC_DELAY);
  m_pVTG2->SetVSyncHOffset(1, MAIN_TO_DENC_DELAY);

  m_bUsingDENC = true;

  if(!m_pVTG2->Start(mode,0))
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::StartSDInterlacedDisplay VTG2 start failed\n"));
    return false;
  }

  if(!m_pDENC->Start(this, mode, tvStandard))
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::StartSDInterlacedDisplay DENC start failed\n"));
    return false;
  }

  if(!m_pVTG->Start(mode))
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::StartSDInterlacedDisplay VTG start failed\n"));
    return false;
  }

  /*
   * Now we have ownership of the DENC, set this output's PSI control setup.
   */
  ProgramPSIControls();

  m_pDENC->SetControl(STM_CTRL_SIGNAL_RANGE, (ULONG)m_signalRange);

  DEBUGF2(2,("CSTi7200LocalMainOutput::StartSDInterlacedDisplay DENC switched to main video\n"));

  return true;
}


bool CSTi7200LocalMainOutput::StartSDProgressiveDisplay(const stm_mode_line_t *mode, ULONG tvStandard)
{
  ULONG val;

  DEBUGF2(2,("CSTi7200LocalMainOutput::StartSDProgressiveDisplay\n"));

  m_pFSynth->SetDivider(4);

  WriteTVOutReg(TVOUT_CLK_SEL, ((TVOUT_CLK_DIV_4  << TVOUT_CLK_SEL_PIX_MAIN_SHIFT)   |   // e.g. 27MHz
                                (TVOUT_CLK_DIV_4  << TVOUT_CLK_SEL_TMDS_MAIN_SHIFT)  |   // As Pixel clock
                                (TVOUT_CLK_DIV_2  << TVOUT_CLK_SEL_BCH_MAIN_SHIFT)   |   // 2*TMDS
                                (TVOUT_CLK_DIV_2  << TVOUT_CLK_SEL_FDVO_MAIN_SHIFT)));   // As Pixel clock

  /*
   * We need to set the TMDS divide in two places, why?
   */
  val = ReadAuxTVOutReg(TVOUT_PADS_CTL) & ~(TVOUT_CLK_MASK << TVOUT_AUX_PADS_TMDS_DIV_SHIFT);
  val |= (TVOUT_CLK_DIV_4 << TVOUT_AUX_PADS_TMDS_DIV_SHIFT);
  WriteAuxTVOutReg(TVOUT_PADS_CTL, val);

  SetFlexVPDivider(TVOUT_CLK_DIV_4);
  SetUpsampler(4);

  val = ReadTVFmtReg(TVFMT_ANA_CFG) & ~(ANA_CFG_INPUT_MASK           |
                                        ANA_CFG_SEL_MAIN_TO_DENC     |
                                        ANA_CFG_SYNC_FILTER_DEL_MASK |
                                        ANA_CFG_SYNC_EN              |
                                        ANA_CFG_CLIP_EN              |
                                        ANA_CFG_SYNC_HREF_IGNORED    |
                                        ANA_CFG_SYNC_FIELD_N_FRAME   |
                                        ANA_CFG_PREFILTER_EN         |
                                        ANA_CFG_SYNC_ON_PRPB);

  WriteTVFmtReg(TVFMT_ANA_CFG, val);

  /*
   * Enable sync generation for ED modes but not VESA (VGA) modes
   */
  if((tvStandard & STM_OUTPUT_STD_ED_MASK) != 0)
  {
    /*
     * Try and start a sync program for the mode. If one doesn't exist then
     * this isn't a failure because we may only support the mode on slaved
     * digital outputs (e.g. HDMI).
     */
    m_pAWGAnalog->Start(mode);
    m_pVTG->SetHSyncOffset(VTG_AWG_SYNC_ID, AWG_DELAY_ED);
    m_pVTG->SetVSyncHOffset(VTG_AWG_SYNC_ID, AWG_DELAY_ED);
    m_pVTG->SetSyncType(VTG_AWG_SYNC_ID, STVTG_SYNC_POSITIVE);
  }
  else
  {
    /*
     * For the moment set a zero offset between the VTG ref signals and the sync
     * pulses sent to the DSub connector. Actually the sync probably should be
     * delayed to take into account the video delay in the HD formatter and
     * DACs.
     */
    m_pVTG->SetSyncType(VTG_AWG_SYNC_ID, STVTG_SYNC_TIMING_MODE);
    m_pVTG->SetHSyncOffset(VTG_AWG_SYNC_ID,0);
    m_pVTG->SetVSyncHOffset(VTG_AWG_SYNC_ID,0);
  }

  if(!m_pVTG->Start(mode))
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::StartSDProgressiveDisplay VTG start failed\n"));
    return false;
  }

  return true;
}


bool CSTi7200LocalMainOutput::StartHDDisplay(const stm_mode_line_t *mode)
{
  ULONG val = 0;
  DEBUGF2(2,("CSTi7200LocalMainOutput::StartHDdisplay\n"));

  m_pFSynth->SetDivider(2);

  WriteTVOutReg(TVOUT_CLK_SEL, ((TVOUT_CLK_DIV_2  << TVOUT_CLK_SEL_PIX_MAIN_SHIFT)   |   // e.g. 74MHz
                                (TVOUT_CLK_DIV_2  << TVOUT_CLK_SEL_TMDS_MAIN_SHIFT)  |   // As Pixel clock
                                (TVOUT_CLK_BYPASS << TVOUT_CLK_SEL_BCH_MAIN_SHIFT)   |   // 2*TMDS
                                (TVOUT_CLK_DIV_2  << TVOUT_CLK_SEL_FDVO_MAIN_SHIFT)));   // As Pixel clock

  /*
   * We need to set the TMDS divide in two places, why?
   */
  val = ReadAuxTVOutReg(TVOUT_PADS_CTL) & ~(TVOUT_CLK_MASK << TVOUT_AUX_PADS_TMDS_DIV_SHIFT);
  val |= (TVOUT_CLK_DIV_2 << TVOUT_AUX_PADS_TMDS_DIV_SHIFT);
  WriteAuxTVOutReg(TVOUT_PADS_CTL, val);

  SetFlexVPDivider(TVOUT_CLK_DIV_2);
  SetUpsampler(2);

  val = ReadTVFmtReg(TVFMT_ANA_CFG) & ~(ANA_CFG_INPUT_MASK           |
                                        ANA_CFG_SEL_MAIN_TO_DENC     |
                                        ANA_CFG_SYNC_FILTER_DEL_MASK |
                                        ANA_CFG_SYNC_EN              |
                                        ANA_CFG_CLIP_EN              |
                                        ANA_CFG_SYNC_HREF_IGNORED    |
                                        ANA_CFG_SYNC_FIELD_N_FRAME   |
                                        ANA_CFG_PREFILTER_EN         |
                                        ANA_CFG_SYNC_ON_PRPB);

  WriteTVFmtReg(TVFMT_ANA_CFG, val);

  /*
   * Try and start a sync program for the mode. If one doesn't exist then
   * this isn't a failure because we may only support the mode on slaved
   * digital outputs (e.g. HDMI).
   */
  m_pAWGAnalog->Start(mode);
  m_pVTG->SetHSyncOffset(VTG_AWG_SYNC_ID, AWG_DELAY_HD);

  /*
   * For 1080i we cannot move the VSync backwards, that would put the top field
   * vsync active edge in the bottom field (as the VTG sees it). This doesn't
   * work because the AWG is set up in frame mode for this and only sees the
   * active edge when the VTG reference signal bot_not_top indicates a top field.
   */
  if(mode->ModeParams.ScanType == SCAN_I)
    m_pVTG->SetVSyncHOffset(VTG_AWG_SYNC_ID, 0);
  else
    m_pVTG->SetVSyncHOffset(VTG_AWG_SYNC_ID, AWG_DELAY_HD);

  m_pVTG->SetSyncType(VTG_AWG_SYNC_ID, STVTG_SYNC_POSITIVE);

  if(!m_pVTG->Start(mode))
  {
    DEBUGF2(1,("CSTi7200LocalMainOutput::StartHDdisplay VTG start failed\n"));
    return false;
  }

  return true;
}


bool CSTi7200LocalMainOutput::Stop(void)
{
  DEBUGF2(2,("CSTi7200LocalMainOutput::Stop - in\n"));

  if(m_pCurrentMode)
  {
    ULONG planes = m_pMixer->GetActivePlanes();

    if((planes & ~(ULONG)OUTPUT_BKG) != 0)
    {
      DEBUGF2(1, ("CSTi7200LocalMainOutput::Stop error - mixer has active planes\n"));
      return false;
    }

    DisableDACs();

    // Stop slaved digital outputs.
    if(m_pHDMI)
      m_pHDMI->Stop();

    if(m_pDVO)
      m_pDVO->Stop();

    if(m_bUsingDENC)
    {
      // Stop SD Path when slaved to HD
      m_pDENC->Stop();
      m_pVTG2->Stop();
      m_bUsingDENC = false;
    }

    // Stop HD Path.
    m_pAWGAnalog->Stop();
    /* there's a hardware bug on STi7200 cut 1 where one has to wait for
       one VSync after disabling the AWG and before writing a new firmware
       into AWG's RAM. Doing so will prevent the System Bus from
       hanging and completely locking up the system.
       So we just force a new VSync here. */
    m_pVTG->ResetCounters();
    /* we just busy wait here... waiting for only one VSync is not always
     * enough, so we wait for 3 VSyncs :-(
     */
    for (int i = 0; i < 3; ++i)
    {
      TIME64 lastVSync = m_LastVSyncTime;
      while (lastVSync == m_LastVSyncTime)
        // would like to have some sort of cpu_relax() here
        ;
    }

    m_pMixer->Stop();
    m_pVTG->Stop();

    COutput::Stop();
  }

  DEBUGF2(2,("CSTi7200LocalMainOutput::Stop - out\n"));
  return true;
}


void CSTi7200LocalMainOutput::Suspend(void)
{
  DEBUGF2(2,("CSTi7200LocalMainOutput::Suspend\n"));

  if(m_bIsSuspended)
    return;

  /* FIXME: add AWG handling */

  CGenericGammaOutput::Suspend();
}


void CSTi7200LocalMainOutput::Resume(void)
{
  DEBUGF2(2,("CSTi7200LocalMainOutput::Resume\n"));

  if(!m_bIsSuspended)
    return;

  CGenericGammaOutput::Resume();

  /* FIXME: add AWG handling */
}


bool CSTi7200LocalMainOutput::ShowPlane(stm_plane_id_t planeID)
{
  DEBUGF2(2,("CSTi7200LocalMainOutput::ShowPlane %d\n",(int)planeID));
  ULONG val;

  if(planeID == OUTPUT_GDP3)
  {
    val = ReadHDMIReg(STM_HDMI_SYNC_CFG) & ~HDMI_GPOUT_LOCAL_GDP3_SD;
    WriteHDMIReg(STM_HDMI_SYNC_CFG, val);
  }
  else if (planeID == OUTPUT_VID2)
  {
    val = ReadHDMIReg(STM_HDMI_SYNC_CFG) & ~HDMI_GPOUT_LOCAL_VDP_SD;
    WriteHDMIReg(STM_HDMI_SYNC_CFG, val);
  }

  return CGenericGammaOutput::m_pMixer->EnablePlane(planeID);
}


void CSTi7200LocalMainOutput::DisableDACs()
{
  ULONG val;

  DENTRY();

  // Power down HD DACs
  val = ReadTVOutReg(TVOUT_PADS_CTL) | TVOUT_MAIN_PADS_DAC_POFF;
  WriteTVOutReg(TVOUT_PADS_CTL, val);

  if(m_bUsingDENC)
  {
    // Power down SD DACs
    val = ReadAuxTVOutReg(TVOUT_PADS_CTL) | TVOUT_AUX_PADS_DAC_POFF;
    WriteAuxTVOutReg(TVOUT_PADS_CTL, val);
  }

  DEXIT();
}


void CSTi7200LocalMainOutput::EnableDACs()
{
  ULONG val;

  DENTRY();

  if(!m_bDacHdPowerDisabled)
  {
    val = ReadSysCfgReg(SYS_CFG1);
    if(m_ulOutputFormat & (STM_VIDEO_OUT_RGB|STM_VIDEO_OUT_YUV))
    {
      DEBUGF2(2,("%s: Enabling HD DACs\n",__PRETTY_FUNCTION__));
      val &= ~(SYS_CFG1_DAC_HD_HZW_DISABLE |
               SYS_CFG1_DAC_HD_HZV_DISABLE |
               SYS_CFG1_DAC_HD_HZU_DISABLE);
    }
    else
    {
      DEBUGF2(2,("%s: Tri-Stating HD DACs\n",__PRETTY_FUNCTION__));
      val |=  (SYS_CFG1_DAC_HD_HZW_DISABLE |
               SYS_CFG1_DAC_HD_HZV_DISABLE |
               SYS_CFG1_DAC_HD_HZU_DISABLE);
    }
    WriteSysCfgReg(SYS_CFG1,val);

    val = ReadTVOutReg(TVOUT_PADS_CTL) & ~TVOUT_MAIN_PADS_DAC_POFF;
    WriteTVOutReg(TVOUT_PADS_CTL, val);
  }

  if(m_bUsingDENC)
  {
    val = ReadSysCfgReg(SYS_CFG1);
    if(m_ulOutputFormat & STM_VIDEO_OUT_CVBS)
    {
      DEBUGF2(2,("%s: Enabling CVBS DAC\n",__PRETTY_FUNCTION__));
      val &= ~SYS_CFG1_DAC_SD0_HZU_DISABLE;
    }
    else
    {
      DEBUGF2(2,("%s: Tri-Stating CVBS DAC\n",__PRETTY_FUNCTION__));
      val |=  SYS_CFG1_DAC_SD0_HZU_DISABLE;
    }

    if(m_ulOutputFormat & STM_VIDEO_OUT_YC)
    {
      DEBUGF2(2,("%s: Enabling Y/C DACs\n",__PRETTY_FUNCTION__));
      val &= ~(SYS_CFG1_DAC_SD0_HZV_DISABLE |
               SYS_CFG1_DAC_SD0_HZW_DISABLE);
    }
    else
    {
      DEBUGF2(2,("%s: Tri-Stating Y/C DACs\n",__PRETTY_FUNCTION__));
      val |=  (SYS_CFG1_DAC_SD0_HZV_DISABLE |
               SYS_CFG1_DAC_SD0_HZW_DISABLE);
    }

    WriteSysCfgReg(SYS_CFG1,val);

    val = ReadAuxTVOutReg(TVOUT_PADS_CTL) & ~TVOUT_AUX_PADS_DAC_POFF;
    WriteAuxTVOutReg(TVOUT_PADS_CTL, val);
  }

  DEXIT();
}


void CSTi7200LocalMainOutput::SetFlexVPDivider(ULONG div)
{
  ULONG val;

  div &= TVOUT_CLK_MASK;

  val = ReadAuxTVOutReg(TVOUT_PADS_CTL) & ~(1L << TVOUT_AUX_PADS_FVP_DIV_SHIFT_BIT1);
  val |= ((div>>1) << TVOUT_AUX_PADS_FVP_DIV_SHIFT_BIT1);
  WriteAuxTVOutReg(TVOUT_PADS_CTL, val);

  val = ReadHDMIReg(STM_HDMI_SYNC_CFG) & ~(1L << HDMI_GPOUT_FVP_DIV_SHIFT_BIT0);
  val |= ((div & 0x1) << HDMI_GPOUT_FVP_DIV_SHIFT_BIT0);
  WriteHDMIReg(STM_HDMI_SYNC_CFG, val);
}


void CSTi7200LocalMainOutput::SetUpsampler(int multiple)
{
  ULONG val;

  switch(multiple)
  {
    case 2:
      DEBUGF2(2,("CSTi7200LocalMainOutput::SetUpsampler 2X\n"));
      val = (X2_COEFF_PHASE1_7 << 16)| ANA_SRC_CFG_2X | ANA_SRC_CFG_DIV_512;
      WriteTVFmtReg(TVFMT_ANA_SRC_CFG, val);
      WriteTVFmtReg(TVFMT_COEFF_P1_T123, X2_COEFF_PHASE1_123);
      WriteTVFmtReg(TVFMT_COEFF_P1_T456, X2_COEFF_PHASE1_456);
      WriteTVFmtReg(TVFMT_COEFF_P2_T123, X2_COEFF_PHASE2_123);
      WriteTVFmtReg(TVFMT_COEFF_P2_T456, X2_COEFF_PHASE2_456);
      WriteTVFmtReg(TVFMT_COEFF_P3_T123, X2_COEFF_PHASE3_123);
      WriteTVFmtReg(TVFMT_COEFF_P3_T456, X2_COEFF_PHASE3_456);
      WriteTVFmtReg(TVFMT_COEFF_P4_T123, X2_COEFF_PHASE4_123);
      WriteTVFmtReg(TVFMT_COEFF_P4_T456, X2_COEFF_PHASE4_456);
      break;
    case 4:
      DEBUGF2(2,("CSTi7200LocalMainOutput::SetUpsampler 4X\n"));
      val = (X4_COEFF_PHASE1_7 << 16) | ANA_SRC_CFG_4X | ANA_SRC_CFG_DIV_512;
      WriteTVFmtReg(TVFMT_ANA_SRC_CFG, val);
      WriteTVFmtReg(TVFMT_COEFF_P1_T123, X4_COEFF_PHASE1_123);
      WriteTVFmtReg(TVFMT_COEFF_P1_T456, X4_COEFF_PHASE1_456);
      WriteTVFmtReg(TVFMT_COEFF_P2_T123, X4_COEFF_PHASE2_123);
      WriteTVFmtReg(TVFMT_COEFF_P2_T456, X4_COEFF_PHASE2_456);
      WriteTVFmtReg(TVFMT_COEFF_P3_T123, X4_COEFF_PHASE3_123);
      WriteTVFmtReg(TVFMT_COEFF_P3_T456, X4_COEFF_PHASE3_456);
      WriteTVFmtReg(TVFMT_COEFF_P4_T123, X4_COEFF_PHASE4_123);
      WriteTVFmtReg(TVFMT_COEFF_P4_T456, X4_COEFF_PHASE4_456);
      break;
    case 8:
      DEBUGF2(2,("CSTi7200LocalMainOutput::SetUpsampler 8X\n"));
      val = (X8_COEFF_PHASE1_7 << 16) | ANA_SRC_CFG_8X | ANA_SRC_CFG_DIV_512;
      WriteTVFmtReg(TVFMT_ANA_SRC_CFG, val);
      WriteTVFmtReg(TVFMT_COEFF_P1_T123, X8_COEFF_PHASE1_123);
      WriteTVFmtReg(TVFMT_COEFF_P1_T456, X8_COEFF_PHASE1_456);
      WriteTVFmtReg(TVFMT_COEFF_P2_T123, X8_COEFF_PHASE2_123);
      WriteTVFmtReg(TVFMT_COEFF_P2_T456, X8_COEFF_PHASE2_456);
      WriteTVFmtReg(TVFMT_COEFF_P3_T123, X8_COEFF_PHASE3_123);
      WriteTVFmtReg(TVFMT_COEFF_P3_T456, X8_COEFF_PHASE3_456);
      WriteTVFmtReg(TVFMT_COEFF_P4_T123, X8_COEFF_PHASE4_123);
      WriteTVFmtReg(TVFMT_COEFF_P4_T456, X8_COEFF_PHASE4_456);
      break;
    default:
      DEBUGF2(2,("CSTi7200LocalMainOutput::SetUpsampler disabled\n"));
      WriteTVFmtReg(TVFMT_ANA_SRC_CFG, ANA_SRC_CFG_DISABLE);
      break;
  }
}
