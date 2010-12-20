/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200mainvideopipe.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/
#ifndef _STI7200_MAINVIDEOPIPE_H
#define _STI7200_MAINVIDEOPIPE_H

#include <Gamma/MainVideoPipe.h>


/*
 * The 7200c1 main video pipe contains a 7109DEI and an IQI.
 * The 7200c2 main video pipe contains a 7109DEI with 1920 pixel line buffers and a
 * new IQI as well as a P2I block.
 */
class CSTi7200MainVideoPipe: public CSTi7xxxMainVideoPipe
{
public:
    CSTi7200MainVideoPipe  (stm_plane_id_t   GDPid,
                            CGammaVideoPlug *plug,
                            ULONG            dei_base);
    virtual ~CSTi7200MainVideoPipe (void);
};


#endif /* _STI7200_MAINVIDEOPIPE_H */
