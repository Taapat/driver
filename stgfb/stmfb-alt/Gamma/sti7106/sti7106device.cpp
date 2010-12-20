/***********************************************************************
 *
 * File: stmfb/Gamma/sti7106/sti7106device.cpp
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

#include "sti7106reg.h"
#include "sti7106device.h"
#include "sti7106hdmi.h"

CSTi7106Device::CSTi7106Device (void): CSTi7105Device ()
{
  DEBUGF2 (2, (FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  DEBUGF2 (2, (FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));
}


CSTi7106Device::~CSTi7106Device (void)
{
  DEBUGF2 (2, (FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  DEBUGF2 (2, (FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));
}



bool CSTi7106Device::CreateHDMIOutput(void)
{
  CSTmHDMI *pHDMI;

  if((pHDMI = new CSTi7106HDMI(this, m_pOutputs[STi7111_OUTPUT_IDX_VDP0_MAIN], m_pVTG1)) == 0)
  {
    DERROR("failed to allocate HDMI output\n");
    return false;
  }

  if(!pHDMI->Create())
  {
    DERROR("failed to create HDMI output\n");
    delete pHDMI;
    return false;
  }

  m_pOutputs[STi7111_OUTPUT_IDX_VDP0_HDMI] = pHDMI;
  return true;
}


/*
 * This is the top level of device creation.
 * There should be exactly one of these per kernel module.
 * When this is called only g_pIOS will have been initialised.
 */
CDisplayDevice *
AnonymousCreateDevice (unsigned deviceid)
{
  if (deviceid == 0)
    return new CSTi7106Device ();

  return 0;
}
