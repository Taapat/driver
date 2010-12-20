/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111auxoutput.cpp
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
#include <STMCommon/stmhdfawg.h>

#include "sti7111reg.h"
#include "sti7111tvoutreg.h"
#include "sti7111device.h"
#include "sti7111auxoutput.h"
#include "sti7111hdfoutput.h"
#include "sti7111mixer.h"
#include "sti7111denc.h"

#define DENC_DELAY (-4)

CSTi7111AuxOutput::CSTi7111AuxOutput(
    CGenericGammaDevice       *pDev,
    CSTmSDVTG                 *pVTG,
    CSTi7111DENC              *pDENC,
    CSTi7111AuxMixer          *pMixer,
    CSTmFSynth                *pFSynth,
    CSTmHDFormatterAWG        *pAWG,
    CSTi7111HDFormatterOutput *pHDFOutput): CGenericGammaOutput(pDev, pDENC, pVTG, pMixer, pFSynth)
{
  DEBUGF2(2,("CSTi7111AuxOutput::CSTi7111AuxOutput - m_pVTG   = %p\n", m_pVTG));
  DEBUGF2(2,("CSTi7111AuxOutput::CSTi7111AuxOutput - m_pDENC  = %p\n", m_pDENC));
  DEBUGF2(2,("CSTi7111AuxOutput::CSTi7111AuxOutput - m_pMixer = %p\n", m_pMixer));

  m_pAWG = pAWG;
  m_pHDFOutput = pHDFOutput;

  m_ulInputSource = STM_AV_SOURCE_MAIN_INPUT;
  CSTi7111AuxOutput::SetControl(STM_CTRL_VIDEO_OUT_SELECT,(STM_VIDEO_OUT_YC | STM_VIDEO_OUT_CVBS));

  /*
   * Default sync selection, note the HD Formatter sync will change depending
   * on which pipeline is actually using the HD DACs.
   *
   * Sync1  - Positive H sync, V sync is actually a top not bot, needed by the
   *          DENC. Yes you might think it might use the ref bnott signal but it
   *          doesn't.
   * Sync2  - External syncs, polarity as specified in standards, could be moved
   *          independently to Sync1 to compensate for signal delays.
   * Sync3  - Internal signal generator sync, again movable relative to Sync1
   *          to allow for delays in the generator block.
   */
  WriteAuxTVOutReg(TVOUT_SYNC_SEL, (TVOUT_AUX_SYNC_DENC_SYNC1      |
                                    TVOUT_AUX_SYNC_VTG_SLAVE_SYNC1 |
                                    TVOUT_AUX_SYNC_AWG_SYNC3       |
                                    TVOUT_AUX_SYNC_HDF_VTG1_REF));

  m_pVTG->SetSyncType(1, STVTG_SYNC_TOP_NOT_BOT);
  m_pVTG->SetSyncType(2, STVTG_SYNC_TIMING_MODE);
  m_pVTG->SetSyncType(3, STVTG_SYNC_POSITIVE);

  /*
   * Default pad configuration
   */
  WriteAuxTVOutReg(TVOUT_PADS_CTL,(TVOUT_AUX_PADS_HSYNC_SYNC2 |
                                   TVOUT_PADS_VSYNC_SYNC2     |
                                   TVOUT_AUX_PADS_DAC_POFF    |
                                   TVOUT_AUX_PADS_SD_DAC_456_N_123 ));

  /*
   * CVBS delay for RGB SCART due to the additional latency of routing RGB
   * through the HD Formatter to the HD DACs
   */
  WriteAuxTVOutReg(TVOUT_SD_DEL,2);

  /*
   * We set up SD to be divided down from 108MHz, so we can run the
   * HD DACs at the same speed as when used from the main output.
   */
  m_pFSynth->SetDivider(8);
  m_pFSynth->Start(13500000);

  ULONG val = ReadClkReg(CKGB_DISPLAY_CFG);

  val &= ~(CKGB_CFG_PIX_SD_FS1(CKGB_CFG_MASK) | CKGB_CFG_DISP_ID(CKGB_CFG_MASK));
  val |=  (CKGB_CFG_PIX_SD_FS1(CKGB_CFG_DIV4) | CKGB_CFG_DISP_ID(CKGB_CFG_DIV8));

  WriteClkReg(CKGB_DISPLAY_CFG, val);

  m_maxDACVoltage  = 1400; // in mV
  m_DACSaturation  = 1023;
  RecalculateDACSetup();

  DEXIT();
}


CSTi7111AuxOutput::~CSTi7111AuxOutput() {}


const stm_mode_line_t* CSTi7111AuxOutput::SupportedMode(const stm_mode_line_t *mode) const
{
  if((mode->ModeParams.OutputStandards & STM_OUTPUT_STD_SD_MASK) == 0)
    return 0;

  if(mode->ModeParams.ScanType == SCAN_P)
    return 0;

  if(mode->TimingParams.ulPixelClock == 13500000 ||
     mode->TimingParams.ulPixelClock == 13513500)
    return mode;

  return 0;
}


bool CSTi7111AuxOutput::Start(const stm_mode_line_t *mode, ULONG tvStandard)
{
  ULONG tmp;
  DENTRY();

  if(!m_pCurrentMode && (m_pVTG->GetCurrentMode() != 0))
  {
    DEBUGF2(1,("CSTi7111AuxOutput::Start - hardware in use by main output\n"));
    return false;
  }

  /*
   * Set the SD pixel clock to be sourced from the SD FSynth.
   */
  ULONG val = ReadClkReg(CKGB_CLK_SRC) | CKGB_SRC_PIXSD_FS1_NOT_FS0;
  WriteClkReg(CKGB_CLK_SRC, val);

  /*
   * Configure TVOut clocks driven from SD pixel clock from FS1,
   * which will be 27MHz.
   */
  WriteAuxTVOutReg(TVOUT_CLK_SEL, (TVOUT_AUX_CLK_SEL_PIX1X(TVOUT_CLK_DIV_2) |
                                   TVOUT_AUX_CLK_SEL_DENC(TVOUT_CLK_BYPASS) |
                                   TVOUT_AUX_CLK_SEL_SD_N_HD));

  /*
   * Ensure the DENC is getting the video from the Aux mixer
   */
  tmp = ReadTVFmtReg(TVFMT_ANA_CFG) & ~ANA_CFG_SEL_MAIN_TO_DENC;
  WriteTVFmtReg(TVFMT_ANA_CFG,tmp);

  m_pDENC->SetMainOutputFormat(m_ulOutputFormat);

  /*
   * Make sure the VTG ref signals are +ve for master mode
   */
  m_pVTG->SetSyncType(0, STVTG_SYNC_POSITIVE);

  /*
   * Make sure the offsets are reset in case the VTG has previously been in use
   * by the main display output.
   */
  m_pVTG->SetHSyncOffset(1, DENC_DELAY);
  m_pVTG->SetVSyncHOffset(1, DENC_DELAY);

  if(!CGenericGammaOutput::Start(mode, tvStandard))
  {
    DEBUGF2(2,("CSTi7111AuxOutput::Start - failed\n"));
    return false;
  }

  DEXIT();

  return true;
}


ULONG CSTi7111AuxOutput::GetCapabilities(void) const
{
  ULONG caps = (STM_OUTPUT_CAPS_SD_ANALOGUE       |
                STM_OUTPUT_CAPS_PLANE_MIXER       |
                STM_OUTPUT_CAPS_MODE_CHANGE       |
                STM_OUTPUT_CAPS_SD_RGB_CVBS_YC    |
                STM_OUTPUT_CAPS_SD_YPrPb_CVBS_YC);

  return caps;
}


bool CSTi7111AuxOutput::SetOutputFormat(ULONG format)
{
  DEBUGF2(2,("CSTi7111AuxOutput::SetOutputFormat - format = 0x%lx\n",format));

  /*
   * Cannot support RGB+YUV together
   */
  if(format & (STM_VIDEO_OUT_RGB | STM_VIDEO_OUT_YUV) == (STM_VIDEO_OUT_RGB | STM_VIDEO_OUT_YUV))
    return false;

  if(m_pCurrentMode)
  {
    m_pDENC->SetMainOutputFormat(format);

    /*
     * Switch HD formatter for RGB(SCART) or YPrPb output from the aux pipeline
     */
    if(format & (STM_VIDEO_OUT_RGB | STM_VIDEO_OUT_YUV))
    {
      ULONG val;
      /*
       * We must disable any sync program running as this will corrupt the
       * signal from the DENC.
       */
      m_pAWG->Disable();

      /*
       * We have to clear this bit to get output on the HD DACs.
       */
      val = ReadAuxTVOutReg(TVOUT_CLK_SEL) & ~TVOUT_AUX_CLK_SEL_SD_N_HD;
      WriteAuxTVOutReg(TVOUT_CLK_SEL, val);

      /*
       * Switch the HD DAC clock to the SD clock
       */
      val = ReadClkReg(CKGB_DISPLAY_CFG) | CKGB_CFG_PIX_HD_FS1_N_FS0;
      WriteClkReg(CKGB_DISPLAY_CFG,val);

      val = ReadAuxTVOutReg(TVOUT_SYNC_SEL) & ~TVOUT_AUX_SYNC_HDF_MASK;
      val |= TVOUT_AUX_SYNC_HDF_VTG2_SYNC1;
      WriteAuxTVOutReg(TVOUT_SYNC_SEL,val);

      ULONG cfgana = ReadTVFmtReg(TVFMT_ANA_CFG) & ~(ANA_CFG_INPUT_MASK |
                                                     ANA_CFG_CLIP_EN);
      cfgana |= ANA_CFG_INPUT_AUX;
      WriteTVFmtReg(TVFMT_ANA_CFG, cfgana);

      WriteTVFmtReg(TVFMT_ANA_Y_SCALE, 0x400);
      WriteTVFmtReg(TVFMT_ANA_C_SCALE, 0x400);

      /*
       * Set up the HD formatter SRC configuration
       */
      m_pHDFOutput->SetUpsampler(4);

      val  = ReadMainTVOutReg(TVOUT_SYNC_SEL) & ~TVOUT_MAIN_SYNC_DCSED_MASK;
      val |= TVOUT_MAIN_SYNC_DCSED_VTG2_SYNC3;
      WriteMainTVOutReg(TVOUT_SYNC_SEL, val);
    }
  }

  return true;
}


bool CSTi7111AuxOutput::ShowPlane(stm_plane_id_t planeID)
{
  ULONG val;
  DEBUGF2(2,("CSTi7111LocalMainOutput::ShowPlane %d\n",(int)planeID));

  if(planeID == OUTPUT_GDP3)
  {
    // Change the clock source for the shared GDP to the SD/Aux video clock
    ULONG val = ReadClkReg(CKGB_CLK_SRC) | CKGB_SRC_GDP3_FS1_NOT_FS0;
    WriteClkReg(CKGB_CLK_SRC, val);
  }
  else if (planeID == OUTPUT_VID2)
  {
    val = ReadSysCfgReg(SYS_CFG6) & ~SYS_CFG6_VIDEO2_MAIN_N_AUX;
    WriteSysCfgReg(SYS_CFG6, val);
  }

  return CGenericGammaOutput::m_pMixer->EnablePlane(planeID);
}


void CSTi7111AuxOutput::EnableDACs(void)
{
  ULONG val;

  DENTRY();

  val = ReadSysCfgReg(SYS_CFG3);

  if(m_ulOutputFormat & (STM_VIDEO_OUT_YUV | STM_VIDEO_OUT_RGB))
  {
    /*
     * We can blindly enable the HD DACs if we need them, it doesn't
     * matter if they were already in use by the HD pipeline.
     */
    DEBUGF2(2,("%s: Enabling HD DACs\n",__PRETTY_FUNCTION__));
    val &= ~(SYS_CFG3_DAC_HD_HZU_DISABLE |
             SYS_CFG3_DAC_HD_HZV_DISABLE |
             SYS_CFG3_DAC_HD_HZW_DISABLE);

    ULONG hddacs = ReadMainTVOutReg(TVOUT_PADS_CTL) & ~TVOUT_MAIN_PADS_DAC_POFF;
    WriteMainTVOutReg(TVOUT_PADS_CTL, hddacs);
  }

  if(m_ulOutputFormat & STM_VIDEO_OUT_CVBS)
  {
    DEBUGF2(2,("%s: Enabling CVBS DAC\n",__PRETTY_FUNCTION__));
    val &= ~SYS_CFG3_DAC_SD_HZU_DISABLE;
  }
  else
  {
    DEBUGF2(2,("%s: Tri-Stating CVBS DAC\n",__PRETTY_FUNCTION__));
    val |=  SYS_CFG3_DAC_SD_HZU_DISABLE;
  }

  if(m_ulOutputFormat & STM_VIDEO_OUT_YC)
  {
    DEBUGF2(2,("%s: Enabling Y/C DACs\n",__PRETTY_FUNCTION__));
    val &= ~(SYS_CFG3_DAC_SD_HZV_DISABLE |
             SYS_CFG3_DAC_SD_HZW_DISABLE);
  }
  else
  {
    DEBUGF2(2,("%s: Tri-Stating Y/C DACs\n",__PRETTY_FUNCTION__));
    val |=  (SYS_CFG3_DAC_SD_HZV_DISABLE |
             SYS_CFG3_DAC_SD_HZW_DISABLE);
  }

  WriteSysCfgReg(SYS_CFG3,val);

  ULONG sddacs = ReadAuxTVOutReg(TVOUT_PADS_CTL) & ~TVOUT_AUX_PADS_DAC_POFF;
  WriteAuxTVOutReg(TVOUT_PADS_CTL, sddacs);

  DEXIT();
}


void CSTi7111AuxOutput::DisableDACs(void)
{
  DENTRY();
  ULONG val;

  if(m_ulOutputFormat & (STM_VIDEO_OUT_YUV | STM_VIDEO_OUT_RGB))
  {
    /*
     * Disable HD DACs only if this pipeline using them
     */
    if(ReadClkReg(CKGB_DISPLAY_CFG) & CKGB_CFG_PIX_HD_FS1_N_FS0)
    {
      DEBUGF2(2,("%s: Disabling HD DACs\n",__PRETTY_FUNCTION__));
      val = ReadMainTVOutReg(TVOUT_PADS_CTL) | TVOUT_MAIN_PADS_DAC_POFF;
      WriteMainTVOutReg(TVOUT_PADS_CTL,val);
    }
  }

  val = ReadAuxTVOutReg(TVOUT_PADS_CTL) | TVOUT_AUX_PADS_DAC_POFF;
  WriteAuxTVOutReg(TVOUT_PADS_CTL, val);

  DEXIT();
}
