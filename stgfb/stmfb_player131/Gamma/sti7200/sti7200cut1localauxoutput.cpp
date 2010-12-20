/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cut1localauxoutput.cpp
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
#include <STMCommon/stmfsynth.h>

#include "sti7200reg.h"
#include "sti7200cut1tvoutreg.h"
#include "sti7200cut1localdevice.h"
#include "sti7200cut1localauxoutput.h"
#include "sti7200mixer.h"
#include "sti7200denc.h"

/*
 * Local Aux TVOut specific config registers definitions
 */
#define TVOUT_PADS_DAC_POFF (1L<<9)

#define TVOUT_CLK_SEL_PIX1X_SHIFT (0)
#define TVOUT_CLK_SEL_PIX1X_MASK  (3L<<TVOUT_CLK_SEL_PIX1X_SHIFT)
#define TVOUT_CLK_SEL_DENC_SHIFT  (2)
#define TVOUT_CLK_SEL_DENC_MASK   (3L<<TVOUT_CLK_SEL_DENC_SHIFT)

#define TVOUT_SYNC_SEL_DENC_SHIFT (0)
#define TVOUT_SYNC_SEL_DENC_MASK  (7L<<TVOUT_SYNC_SEL_DENC_SHIFT)
#define TVOUT_SYNC_SEL_DENC_PADS  (7L)
#define TVOUT_SYNC_SEL_AWG_SHIFT  (3)
#define TVOUT_SYNC_SEL_AWG_MASK   (3L<<TVOUT_SYNC_SEL_AWG_SHIFT)
#define TVOUT_SYNC_SEL_VTG_SHIFT  (5)
#define TVOUT_SYNC_SEL_VTG_MASK   (3L<<TVOUT_SYNC_SEL_VTG_SHIFT)

#define DENC_DELAY (-4)

CSTi7200Cut1LocalAuxOutput::CSTi7200Cut1LocalAuxOutput(
    CSTi7200Cut1LocalDevice   *pDev,
    CSTmSDVTG                 *pVTG,
    CSTi7200DENC              *pDENC,
    CSTi7200LocalAuxMixer     *pMixer,
    CSTmFSynthType2           *pFSynth): CGenericGammaOutput(pDev, pDENC, pVTG, pMixer, pFSynth)
{

  DEBUGF2(2,("CSTi7200Cut1LocalAuxOutput::CSTi7200Cut1LocalAuxOutput - m_pVTG   = %p\n", m_pVTG));
  DEBUGF2(2,("CSTi7200Cut1LocalAuxOutput::CSTi7200Cut1LocalAuxOutput - m_pDENC  = %p\n", m_pDENC));
  DEBUGF2(2,("CSTi7200Cut1LocalAuxOutput::CSTi7200Cut1LocalAuxOutput - m_pMixer = %p\n", m_pMixer));

  /*
   * The clock tree has a fixed /8 after the fsynth, so for a 27MHz input clock
   * we need to multiply the pixelclock by 16 to get the required FSynth
   * frequency.
   */
  m_pFSynth->SetDivider(16);

  m_ulInputSource = STM_AV_SOURCE_MAIN_INPUT;
  CSTi7200Cut1LocalAuxOutput::SetControl(STM_CTRL_VIDEO_OUT_SELECT,(STM_VIDEO_OUT_YC | STM_VIDEO_OUT_CVBS));

  /*
   * Fixed setup of syncs and clock divides to subblocks as we are only
   * supporting SD TV modes, we might as well do it once here.
   *
   * Sync1  - Positive H sync, V sync is actually a top not bot, needed by the
   *          DENC. Yes you might think it might use the ref bnott signal but it
   *          doesn't.
   * Sync2  - External syncs, polarity as specified in standards, could be moved
   *          independently to Sync1 to compensate for signal delays.
   * Sync3  - Internal signal generator sync, again movable relative to Sync1
   *          to allow for delays in the generator block.
   */
  WriteTVOutReg(TVOUT_SYNC_SEL, ((TVOUT_SYNC_SEL_SYNC1 << TVOUT_SYNC_SEL_DENC_SHIFT) |
                                 (TVOUT_SYNC_SEL_SYNC2 << TVOUT_SYNC_SEL_VTG_SHIFT)  |
                                 (TVOUT_SYNC_SEL_SYNC3 << TVOUT_SYNC_SEL_AWG_SHIFT)));

  m_pVTG->SetSyncType(1, STVTG_SYNC_TOP_NOT_BOT);
  m_pVTG->SetSyncType(2, STVTG_SYNC_TIMING_MODE);
  m_pVTG->SetSyncType(3, STVTG_SYNC_POSITIVE);

  /*
   * Default pad configuration
   */
  WriteTVOutReg(TVOUT_PADS_CTL, TVOUT_PADS_HSYNC_HSYNC2 |
                                TVOUT_PADS_VSYNC_VSYNC2 |
                                TVOUT_PADS_DAC_POFF);

  DEXIT();
}


CSTi7200Cut1LocalAuxOutput::~CSTi7200Cut1LocalAuxOutput() {}


const stm_mode_line_t* CSTi7200Cut1LocalAuxOutput::SupportedMode(const stm_mode_line_t *mode) const
{
  if((mode->ModeParams.OutputStandards & STM_OUTPUT_STD_SD_MASK) == 0)
    return 0;

  if(mode->ModeParams.ScanType == SCAN_P)
    return 0;

  if(mode->TimingParams.ulPixelClock == 13500000)
    return mode;

  return 0;
}


bool CSTi7200Cut1LocalAuxOutput::Start(const stm_mode_line_t *mode, ULONG tvStandard)
{
  ULONG tmp;
  DENTRY();

  if(!m_pCurrentMode && (m_pVTG->GetCurrentMode() != 0))
  {
    DEBUGF2(1,("CSTi7200Cut1LocalAuxOutput::Start - hardware in use by main output\n"));
    return false;
  }

  /*
   * Ensure we have the right clock routing to start SD mode from the
   * 27MHz clock source.
   */
  tmp = ReadHDMIReg(STM_HDMI_SYNC_CFG) & ~(HDMI_GPOUT_LOCAL_SDTVO_HD | HDMI_GPOUT_LOCAL_AUX_PIX_HD);
  WriteHDMIReg(STM_HDMI_SYNC_CFG, tmp);

  tmp = ReadTVOutReg(TVOUT_PADS_CTL) & ~(TVOUT_CLK_MASK << TVOUT_AUX_PADS_LOCAL_AUX_DIV_SHIFT);
  tmp |= (TVOUT_CLK_DIV_2 << TVOUT_AUX_PADS_LOCAL_AUX_DIV_SHIFT);
  WriteTVOutReg(TVOUT_PADS_CTL, tmp);

  WriteTVOutReg(TVOUT_CLK_SEL, ((TVOUT_CLK_BYPASS << TVOUT_CLK_SEL_DENC_SHIFT) |
                                (TVOUT_CLK_DIV_2  << TVOUT_CLK_SEL_PIX1X_SHIFT)));

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
    DEBUGF2(2,("CSTi7200Cut1LocalAuxOutput::Start - failed\n"));
    return false;
  }

  DEXIT();

  return true;
}


ULONG CSTi7200Cut1LocalAuxOutput::GetCapabilities(void) const
{
  ULONG caps = (STM_OUTPUT_CAPS_SD_ANALOGUE       |
                STM_OUTPUT_CAPS_PLANE_MIXER       |
                STM_OUTPUT_CAPS_MODE_CHANGE       |
                STM_OUTPUT_CAPS_CVBS_YC_EXCLUSIVE |
                STM_OUTPUT_CAPS_YPrPb_EXCLUSIVE   );

  return caps;
}


bool CSTi7200Cut1LocalAuxOutput::SetOutputFormat(ULONG format)
{
  DEBUGF2(2,("CSTi7200Cut1LocalAuxOutput::SetOutputFormat - format = 0x%lx\n",format));

  switch(format)
  {
    case 0:
    case STM_VIDEO_OUT_YUV:
    case STM_VIDEO_OUT_YC:
    case STM_VIDEO_OUT_CVBS:
    case (STM_VIDEO_OUT_YC | STM_VIDEO_OUT_CVBS):
      break;
    default:
      return false;
  }

  if(m_pCurrentMode)
    m_pDENC->SetMainOutputFormat(format);

  return true;
}


bool CSTi7200Cut1LocalAuxOutput::ShowPlane(stm_plane_id_t planeID)
{
  ULONG val;
  DEBUGF2(2,("CSTi7200LocalMainOutput::ShowPlane %d\n",(int)planeID));

  if(planeID == OUTPUT_GDP3)
  {
    val = ReadHDMIReg(STM_HDMI_SYNC_CFG) | HDMI_GPOUT_LOCAL_GDP3_SD;
    WriteHDMIReg(STM_HDMI_SYNC_CFG, val);
  }
  else if (planeID == OUTPUT_VID2)
  {
    val = ReadHDMIReg(STM_HDMI_SYNC_CFG) | HDMI_GPOUT_LOCAL_VDP_SD;
    WriteHDMIReg(STM_HDMI_SYNC_CFG, val);
  }

  return CGenericGammaOutput::m_pMixer->EnablePlane(planeID);
}


void CSTi7200Cut1LocalAuxOutput::EnableDACs(void)
{
  ULONG val;

  DENTRY();

  val = ReadSysCfgReg(SYS_CFG1);

  if(m_ulOutputFormat & STM_VIDEO_OUT_YUV)
  {
    DEBUGF2(2,("%s: Enabling Aux YPrPb DACs\n",__PRETTY_FUNCTION__));
    val &= ~(SYS_CFG1_DAC_SD0_HZU_DISABLE |
             SYS_CFG1_DAC_SD0_HZV_DISABLE |
             SYS_CFG1_DAC_SD0_HZW_DISABLE);
  }
  else
  {
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
  }

  WriteSysCfgReg(SYS_CFG1,val);

  val = ReadTVOutReg(TVOUT_PADS_CTL) & ~TVOUT_PADS_DAC_POFF;
  WriteTVOutReg(TVOUT_PADS_CTL, val);

  DEXIT();
}


void CSTi7200Cut1LocalAuxOutput::DisableDACs(void)
{
  DENTRY();

  ULONG val = ReadTVOutReg(TVOUT_PADS_CTL) | TVOUT_PADS_DAC_POFF;

  WriteTVOutReg(TVOUT_PADS_CTL, val);

  DEXIT();
}
