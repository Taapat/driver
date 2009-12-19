/***********************************************************************
 *
 * File: stgfb/Gamma/sti7111/sti7111denc.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7111DENC_H
#define _STi7111DENC_H

#include <STMCommon/stmdencregs.h>
#include <Gamma/stb7100/stb7100denc.h>

class CDisplayDevice;
class COutput;
class CSTmTeletext;

class CSTi7111DENC: public CSTb7100DENC
{
public:
  CSTi7111DENC(CDisplayDevice* pDev, ULONG regoffset, CSTmTeletext *pTeletext): CSTb7100DENC(pDev, regoffset, pTeletext)
  {
    m_bEnableLumaFilter = false;
  }

  ~CSTi7111DENC(void) {}

  bool Start(COutput *parent, const stm_mode_line_t *pModeLine, ULONG tvStandard)
  {
    return CSTmDENC::Start(parent, pModeLine, tvStandard, DENC_CFG0_ODHS_SLV, STVTG_SYNC_POSITIVE, STVTG_SYNC_TOP_NOT_BOT);
  }

private:
  CSTi7111DENC(const CSTi7111DENC&);
  CSTi7111DENC& operator=(const CSTi7111DENC&);
};

#endif // _STi7111DENC_H
