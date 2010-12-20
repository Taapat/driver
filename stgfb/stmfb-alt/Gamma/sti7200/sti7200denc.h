/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200denc.h
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7200DENC_H
#define _STi7200DENC_H

#include <Gamma/stb7100/stb7100denc.h>

class CDisplayDevice;
class COutput;
class CSTmTeletext;

/*
 * Both DENCs on Cut1 and the remote output DENC on Cut2 use this, it only
 * supports Y/C-CVBS or YPrPb on a single set of DAC outputs.
 */
class CSTi7200DENC : public CSTb7100DENC
{
public:
  CSTi7200DENC(CDisplayDevice* pDev, ULONG regoffset, CSTmTeletext *pTeletext = 0);
  ~CSTi7200DENC(void);

  bool Start(COutput *,const stm_mode_line_t *pModeLine, ULONG tvStandard);

  bool SetMainOutputFormat(ULONG);
  bool SetAuxOutputFormat(ULONG);

private:
  void ProgramOutputFormats(void);

  CSTi7200DENC(const CSTi7200DENC&);
  CSTi7200DENC& operator=(const CSTi7200DENC&);
};


/*
 * The Cut2 local DENC is much simpler, we have the full set of DENC outputs
 * available as on the 7111, with flexible routing to the DACs/HDFormatter in
 * the surrounding glue.
 */
class CSTi7200Cut2LocalDENC: public CSTb7100DENC
{
public:
  CSTi7200Cut2LocalDENC(CDisplayDevice* pDev, ULONG regoffset, CSTmTeletext *pTeletext = 0): CSTb7100DENC(pDev, regoffset, pTeletext)
  {
    m_bEnableLumaFilter = false;
  }

  ~CSTi7200Cut2LocalDENC(void) {}

  bool Start(COutput *,const stm_mode_line_t *pModeLine, ULONG tvStandard);

private:
  CSTi7200Cut2LocalDENC(const CSTi7200Cut2LocalDENC&);
  CSTi7200Cut2LocalDENC& operator=(const CSTi7200Cut2LocalDENC&);
};

#endif // _STi7200DENC_H
