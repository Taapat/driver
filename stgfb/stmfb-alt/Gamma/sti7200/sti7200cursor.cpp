/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cursor.cpp
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
#include <Gamma/GenericGammaReg.h>

#include "sti7200cursor.h"


CSTi7200Cursor::CSTi7200Cursor(ULONG baseAddr): CGammaCompositorCursor(baseAddr)
{
  DENTRY();

  /*
   * Cursor plug registers are at the same offset as GDPs
   */
  WriteCurReg(GDPn_PAS ,6);
  WriteCurReg(GDPn_MAOS,5);
  WriteCurReg(GDPn_MIOS,3);
  WriteCurReg(GDPn_MACS,0);
  WriteCurReg(GDPn_MAMS,3);

  DEXIT();
}


CSTi7200Cursor::~CSTi7200Cursor() {}
