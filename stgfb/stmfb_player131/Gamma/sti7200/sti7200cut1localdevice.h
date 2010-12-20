/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cut1localdevice.h
 * Copyright (c) 2007,2008 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STI7200CUT1LOCALDEVICE_H
#define _STI7200CUT1LOCALDEVICE_H

#include "sti7200device.h"

class CSTmSDVTG;
class CSTmFSynthType2;
class CSTi7200DENC;
class CSTi7200LocalMainMixer;
class CSTi7200LocalAuxMixer;
class CGammaVideoPlug;
class CSTmHDFormatterAWG;
class CGDPBDispOutput;

class CSTi7200Cut1LocalDevice : public CSTi7200Device
{
public:
  CSTi7200Cut1LocalDevice(void);
  virtual ~CSTi7200Cut1LocalDevice(void);

  bool Create(void);

  CDisplayPlane *GetPlane(stm_plane_id_t planeID) const;

  void UpdateDisplay(COutput *);

private:
  CSTmFSynthType2        *m_pFSynthHD0;
  CSTmFSynthType2        *m_pFSynthSD;
  CSTmSDVTG              *m_pVTG1;
  CSTmSDVTG              *m_pVTG2;
  CSTi7200DENC           *m_pDENC;
  CSTi7200LocalMainMixer *m_pMainMixer;
  CSTi7200LocalAuxMixer  *m_pAuxMixer;
  CGammaVideoPlug        *m_pVideoPlug1;
  CGammaVideoPlug        *m_pVideoPlug2;
  CSTmHDFormatterAWG     *m_pAWGAnalog;
  CGDPBDispOutput        *m_pGDPBDispOutput;

  CSTi7200Cut1LocalDevice(const CSTi7200Cut1LocalDevice&);
  CSTi7200Cut1LocalDevice& operator=(const CSTi7200Cut1LocalDevice&);
};

#endif // _STI7200CUT1LOCALDEVICE_H
