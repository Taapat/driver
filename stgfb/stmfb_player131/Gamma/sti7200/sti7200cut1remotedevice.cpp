/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cut1remotedevice.cpp
 * Copyright (c) 2007,2008 STMicroelectronics Limited.
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

#include "sti7200reg.h"
#include "sti7200cut1remotedevice.h"
#include "sti7200remoteoutput.h"
#include "sti7200denc.h"
#include "sti7200mixer.h"
#include "sti7200bdisp.h"
#include "sti7200gdp.h"
#include "sti7200vdp.h"


//////////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSTi7200Cut1RemoteDevice::CSTi7200Cut1RemoteDevice(void): CSTi7200Device()

{
  DENTRY();

  m_pFSynth     = 0;
  m_pDENC       = 0;
  m_pVTG        = 0;
  m_pMixer      = 0;
  m_pVideoPlug  = 0;

  DEXIT();
}


CSTi7200Cut1RemoteDevice::~CSTi7200Cut1RemoteDevice()
{
  DENTRY();

  delete m_pDENC;
  delete m_pVTG;
  delete m_pMixer;
  delete m_pVideoPlug;
  delete m_pFSynth;

  DEXIT();
}


bool CSTi7200Cut1RemoteDevice::Create()
{ 
  CGammaCompositorGDP  *gdp;
  CSTb7109DEI *disp;

  DENTRY();

  if(!CSTi7200Device::Create())
    return false;

  if((m_pFSynth = new CSTmFSynthType2(this, STi7200_CLKGEN_BASE+CLKB_FS1_CLK2_CFG)) == 0)
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - failed to create FSynth\n"));
    return false;
  }

  m_pFSynth->SetClockReference(STM_CLOCK_REF_30MHZ,0);
  
  /*
   * VTG is clocked at 1xpixelclock on 7200
   */
  if((m_pVTG = new CSTmSDVTG(this, STi7200C1_VTG3_BASE, m_pFSynth, false, STVTG_SYNC_POSITIVE)) == 0)
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - failed to create VTG\n"));
    return false;
  }

  m_pDENC = new CSTi7200DENC(this, STi7200C1_REMOTE_DENC_BASE);
  if(!m_pDENC || !m_pDENC->Create())
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - failed to create DENC\n"));
    return false;
  }

  if((m_pMixer = new CSTi7200RemoteMixer(this)) == 0)
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - failed to create main mixer\n"));
    return false;
  }


  m_numPlanes = 2;
  ULONG ulCompositorBaseAddr = ((ULONG)m_pGammaReg) + STi7200_REMOTE_COMPOSITOR_BASE;

  gdp = new CSTi7200RemoteGDP(OUTPUT_GDP1, ulCompositorBaseAddr+STi7200_GDP1_OFFSET);
  if(!gdp || !gdp->Create())
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - failed to create GDP1\n"));
    return false;
  }
  m_hwPlanes[0] = gdp;

  m_pVideoPlug = new CGammaVideoPlug(ulCompositorBaseAddr+STi7200_VID1_OFFSET, true, true);
  if(!m_pVideoPlug)
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - failed to create video plugs\n"));
    return false;
  }

  disp = new CSTi7200VDP(OUTPUT_VID1,m_pVideoPlug, ((ULONG)m_pGammaReg) + STi7200_REMOTE_DEI_BASE);
  if(!disp || !disp->Create())
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - failed to create VID1\n"));
    return false;
  }
  m_hwPlanes[1] = disp;

  CSTi7200Cut1RemoteOutput *pMain;

  if((pMain = new CSTi7200Cut1RemoteOutput(this, m_pVTG, m_pDENC, m_pMixer, m_pFSynth)) == 0)
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - failed to create primary output\n"));
    return false;
  }

  m_pOutputs[STi7200_OUTPUT_IDX_VDP2_MAIN] = pMain;

  m_nOutputs = 1;


  m_graphicsAccelerators[STi7200c1_BLITTER_IDX] = m_pBDisp->GetAQ(STi7200_BLITTER_AQ_VDP0_MAIN);
  if(!m_graphicsAccelerators[STi7200c1_BLITTER_IDX])
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - failed to get AQ\n"));
    return false;
  }

  m_numAccelerators = 1;

  if(!CGenericGammaDevice::Create())
  {
    DEBUGF(("CSTi7200Cut1RemoteDevice::Create - base class Create failed\n"));
    return false;
  }

  DEXIT();

  return true;
}
