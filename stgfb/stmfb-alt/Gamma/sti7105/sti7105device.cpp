/***********************************************************************
 *
 * File: stmfb/Gamma/sti7105/sti7105device.cpp
 * Copyright (c) 2008,2009 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>

#include "sti7105reg.h"
#include "sti7105device.h"


CSTi7105Device::CSTi7105Device (void): CSTi7111Device ()
{
  DEBUGF2 (2, (FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  DEBUGF2 (2, (FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));
}


CSTi7105Device::~CSTi7105Device (void)
{
  DEBUGF2 (2, (FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  DEBUGF2 (2, (FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));
}



bool CSTi7105Device::CreateOutputs(void)
{
  if(!CSTi7111Device::CreateOutputs())
    return false;

  /*
   * TODO: Create DVO1 output
   */
  return true;
}
