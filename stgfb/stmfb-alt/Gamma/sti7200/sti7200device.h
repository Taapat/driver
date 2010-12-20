/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200device.h
 * Copyright (c) 2007 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STI7200DEVICE_H
#define _STI7200DEVICE_H

#ifdef __cplusplus

#include <Gamma/GenericGammaDevice.h>


class CSTi7200BDisp;

class CSTi7200Device : public CGenericGammaDevice  
{
public:
  CSTi7200Device(void);
  virtual ~CSTi7200Device(void);

  bool Create(void);

protected:
  static CSTi7200BDisp *m_pBDisp;
  static ULONG         *m_pBDispRegMapping;
  static int            m_nBDispUseCount;
};

#endif /* __cplusplus */


#define STi7200_OUTPUT_IDX_VDP0_MAIN      ( 0)
#define STi7200_OUTPUT_IDX_VDP0_HDMI      ( 2)
#define STi7200_OUTPUT_IDX_VDP0_DVO0      ( 3)
#define STi7200_OUTPUT_IDX_VDP0_GDP       ( 4)
#define STi7200_OUTPUT_IDX_VDP1_MAIN      ( 1)

#define STi7200_OUTPUT_IDX_VDP2_MAIN      ( 0)

/* cut 2 has 4 AQs, cut 1 only 1. So on cut 1 it is shared between all the
   outputs. */
#define STi7200c1_BLITTER_IDX             ( 0)

#define STi7200c2_BLITTER_IDX_VDP0_MAIN   ( 0)
#define STi7200c2_BLITTER_IDX_VDP1_MAIN   ( 1)
#define STi7200c2_BLITTER_IDX_VDP2_MAIN   ( 0)

#define STi7200_BLITTER_AQ_VDP0_MAIN      ( 1)
#define STi7200c2_BLITTER_AQ_VDP1_MAIN    ( 2)
#define STi7200c2_BLITTER_AQ_VDP2_MAIN    ( 3)


#endif // _STI7200DEVICE_H
