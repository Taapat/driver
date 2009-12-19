/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111dei.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/
#ifndef _STI7111_DEI_H
#define _STI7111_DEI_H

#include <Gamma/stb7100/stb7109deireg.h>
#include <Gamma/sti7200/sti7200mainvideopipe.h>


/*
 * The 7111c1 main video pipe contains a 7109DEI with 1920 pixel line buffers and a
 * new IQI as well as a P2I block, i.e. it is the same as on 7200c2.
 */
class CSTi7111DEI: public CSTi7200MainVideoPipe
{
public:
  CSTi7111DEI (stm_plane_id_t   GDPid,
               CGammaVideoPlug *plug,
               ULONG            baseAddr): CSTi7200MainVideoPipe (GDPid,
                                                                  plug,
                                                                  baseAddr)
  {
    /* we want to be sure we are linked against the correct implementation of
       the MainVideoPipe class (7200cut2) */
    ASSERTF (m_nDEIPixelBufferLength = 1920,
             ("something went wrong during compilation!\n"));

    /* motion data allocated from system partition, there is only one LMI on
       sti7111. */
    m_ulMotionDataFlag = SDAAF_NONE;

    /*
     * (mostly) validation recommended T3I plug settings.
     */
    m_nMBChunkSize          = 0x3; // Validation actually recommend 0xf, this is better for extreme downscales
    m_nRasterOpcodeSize     = 32;
    m_nRasterChunkSize      = 0x7;
    m_nPlanarChunkSize      = 0x3;// Validation do not test this mode so do not have a recommendation

    m_nMotionOpcodeSize     = 32;
    WriteVideoReg (DEI_T3I_MOTION_CTL, (DEI_T3I_CTL_OPCODE_SIZE_32 |
                                        (0x3 << DEI_T3I_CTL_CHUNK_SIZE_SHIFT)));
  }

private:
  CSTi7111DEI (const CSTi7111DEI &);
  CSTi7111DEI& operator= (const CSTi7111DEI &);
};

#endif // _STI7111_DEI_H
