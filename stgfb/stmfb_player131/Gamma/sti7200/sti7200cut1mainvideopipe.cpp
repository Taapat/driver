/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cut1mainvideopipe.cpp
 * Copyright (c) 2008 by STMicroelectronics. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>

#include "sti7200mainvideopipe.h"



/*
 * The 7200c1 main video pipe contains a 7109DEI and an IQI.
 */
CSTi7200MainVideoPipe::CSTi7200MainVideoPipe (stm_plane_id_t   GDPid,
                                              CGammaVideoPlug *plug,
                                              ULONG            dei_base): CSTi7xxxMainVideoPipe (GDPid, 
                                                                                                 plug, 
                                                                                                 dei_base,
                                                                                                 IQI_CELL_REV1)
{
  DEBUGF2 (4, (FENTRY ", self: %p, planeID: %d, dei_base: %lx\n",
               __PRETTY_FUNCTION__, this, GDPid, dei_base));
  DEBUGF2 (4, (FEXIT ", self: %p\n", __PRETTY_FUNCTION__, this));
}


CSTi7200MainVideoPipe::~CSTi7200MainVideoPipe (void)
{
  DEBUGF2 (4, (FENTRY ", self: %p\n",
               __PRETTY_FUNCTION__, this));
  DEBUGF2 (4, (FEXIT ", self: %p\n", __PRETTY_FUNCTION__, this));
}
