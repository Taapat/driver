/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200vdp.h
 * Copyright (c) 2007 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/
#ifndef _STB7200_VDP_H
#define _STB7200_VDP_H

#include <Gamma/stb7100/stb7109dei.h>

/*
 * The 7200VDP is a 7109DEI but without the de-interlacing capability, sigh...
 */
class CSTi7200VDP: public CSTb7109DEI
{
public:
  CSTi7200VDP(stm_plane_id_t   GDPid,
              CGammaVideoPlug *plug,
              ULONG            vdpBaseAddr): CSTb7109DEI(GDPid, plug, vdpBaseAddr)
  {
    m_bHaveDeinterlacer = false;
    m_keepHistoricBufferForDeinterlacing = false;
  }

private:
  CSTi7200VDP (const CSTi7200VDP &);
  CSTi7200VDP& operator= (const CSTi7200VDP &);
};

#endif // _STB7200_VDP_H
