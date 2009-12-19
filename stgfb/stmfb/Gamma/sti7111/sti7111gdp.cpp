/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111gdp.cpp
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
#include <Gamma/GenericGammaReg.h>

#include "sti7111gdp.h"


CSTi7111GDP::CSTi7111GDP(stm_plane_id_t GDPid,
                         ULONG          baseAddr): CGammaCompositorGDP(GDPid, baseAddr, 0, true)
{
  DENTRY();

  m_bHasVFilter       = true;
  m_bHasFlickerFilter = true;

  m_ulMaxHSrcInc = m_fixedpointONE*4; // downscale to 1/4
  m_ulMinHSrcInc = m_fixedpointONE/8; // upscale 8x
  m_ulMaxVSrcInc = m_fixedpointONE*4; // downscale to 1/4, yes really!
  m_ulMinVSrcInc = m_fixedpointONE/8;

  /*
   * Standard STBus transaction setup
   * TODO: Does this need to change for 1080p @50/60Hz?
   */
  g_pIOS->WriteRegister((volatile ULONG *)(m_GDPBaseAddr+GDPn_PAS) ,6); // 4K page
  g_pIOS->WriteRegister((volatile ULONG *)(m_GDPBaseAddr+GDPn_MAOS),5); // Max opcode = 32bytes
  g_pIOS->WriteRegister((volatile ULONG *)(m_GDPBaseAddr+GDPn_MIOS),3); // Min opcode = 8bytes
  g_pIOS->WriteRegister((volatile ULONG *)(m_GDPBaseAddr+GDPn_MACS),3); // Max chunk size = 8*max opcode size
  g_pIOS->WriteRegister((volatile ULONG *)(m_GDPBaseAddr+GDPn_MAMS),0); // Max message size = chunk size

  DEXIT();
}
