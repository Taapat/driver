/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200denc.cpp
 * Copyright (c) 2005 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>

#include <STMCommon/stmdencregs.h>

#include "sti7200reg.h"
#include "sti7200denc.h"

#define DENC_CFG13_RGB_MAXDYN 0x40

#define DENC_CFG13_DAC123_CONF_YC_CVBS (0x0 << 3)
#define DENC_CFG13_DAC123_CONF_YUV     (0x3 << 3)


CSTi7200DENC::CSTi7200DENC(CDisplayDevice* pDev, ULONG regoffset, CSTmTeletext *pTeletext): CSTb7100DENC(pDev, regoffset, pTeletext)
{
  /*
   * Don't override the default for 7200
   */
  m_bEnableLumaFilter = false;
}


CSTi7200DENC::~CSTi7200DENC() {}


bool CSTi7200DENC::Start(COutput *parent, const stm_mode_line_t *pModeLine, ULONG tvStandard)
{
  DENTRY();

  return CSTmDENC::Start(parent, pModeLine, tvStandard, DENC_CFG0_ODHS_SLV, STVTG_SYNC_POSITIVE, STVTG_SYNC_TOP_NOT_BOT);
}


void CSTi7200DENC::ProgramOutputFormats(void)
{
  /*
   * On sti7200 remote DENC we are setting DACs 123, but we can set RGB,YUV
   * or Y/C-CVBS.
   */
  UCHAR Cfg13 = 0;

  if(m_mainOutputFormat == STM_VIDEO_OUT_YUV)
  {
    DEBUGF2(2,("CSTi7200DENC::ProgramOutputFormats output is YUV\n"));
    Cfg13 |= DENC_CFG13_DAC123_CONF_YUV;

    WriteDAC123Scale(false);
    WriteDAC456Scale(false); // For Ch3/4 CVBS from DAC4
  }
  else
  {
    DEBUGF2(2,("CSTi7200DENC::ProgramOutputFormats output is Y/C-CVBS\n"));
    Cfg13 |= DENC_CFG13_DAC123_CONF_YC_CVBS;

    WriteDAC123Scale(false);
    WriteDAC456Scale(false); // For Ch3/4 CVBS from DAC4
  }

  WriteDENCReg(DENC_CFG13, Cfg13);
}


bool CSTi7200DENC::SetMainOutputFormat(ULONG format)
{
  /*
   * The 7200Cut1 DENCs can support either YUV or Y/C-CVBS.
   * There are no other useful combinations (RGB is useless without CVBS to
   * provide the syncs), which makes this much simpler than our other DENC
   * specializations.
   */
  if((format != 0                  &&
      format != STM_VIDEO_OUT_YUV  &&
      format != STM_VIDEO_OUT_YC   &&
      format != STM_VIDEO_OUT_CVBS &&
      format != (STM_VIDEO_OUT_YC | STM_VIDEO_OUT_CVBS)))
    return false;

  m_mainOutputFormat = format;

  ProgramOutputFormats();

  return true;
}


bool CSTi7200DENC::SetAuxOutputFormat(ULONG format)
{
  /*
   * Remote DENC only has one set of outputs, the DAC4 output is fixed to
   * CVBS for the CH3/4 DAC.
   */
  return false;
}


//-----------------------------------------------------------------------------

bool CSTi7200Cut2LocalDENC::Start(COutput *parent, const stm_mode_line_t *pModeLine, ULONG tvStandard)
{
  DENTRY();

  return CSTmDENC::Start(parent, pModeLine, tvStandard, DENC_CFG0_ODHS_SLV, STVTG_SYNC_POSITIVE, STVTG_SYNC_TOP_NOT_BOT);
}

