/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111vdp.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/
#ifndef _STI7111_VDP_H
#define _STI7111_VDP_H

#include <Gamma/stb7100/stb7109dei.h>

class CSTi7111VDP: public CSTb7109DEI
{
public:
    CSTi7111VDP(stm_plane_id_t GDPid, CGammaVideoPlug *plug, ULONG vdpBaseAddr): CSTb7109DEI(GDPid,plug,vdpBaseAddr)
    {
      m_bHaveDeinterlacer = false;
      m_keepHistoricBufferForDeinterlacing = false;
    }

    ~CSTi7111VDP() {}
};

#endif // _STI7111_VDP_H
