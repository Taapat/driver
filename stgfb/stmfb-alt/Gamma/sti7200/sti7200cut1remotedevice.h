/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cut1remotedevice.h
 * Copyright (c) 2007,2008 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STI7200CUT1REMOTEDEVICE_H
#define _STI7200CUT1REMOTEDEVICE_H

#include "sti7200device.h"

class CSTmSDVTG;
class CSTmFSynthType2;
class CSTi7200DENC;
class CSTi7200RemoteMixer;
class CGammaVideoPlug;

class CSTi7200Cut1RemoteDevice : public CSTi7200Device
{
public:
  CSTi7200Cut1RemoteDevice(void);
  virtual ~CSTi7200Cut1RemoteDevice(void);

  bool Create(void);

private:
  CSTmFSynthType2     *m_pFSynth;
  CSTmSDVTG           *m_pVTG;
  CSTi7200DENC        *m_pDENC;
  CSTi7200RemoteMixer *m_pMixer;
  CGammaVideoPlug     *m_pVideoPlug;

  CSTi7200Cut1RemoteDevice(const CSTi7200Cut1RemoteDevice&);
  CSTi7200Cut1RemoteDevice& operator=(const CSTi7200Cut1RemoteDevice&);
};

#endif // _STI7200CUT1REMOTEDEVICE_H
