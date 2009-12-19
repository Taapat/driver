/***********************************************************************
 *
 * File: stgfb/Gamma/MainVideoPipe.h
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

#include "MainVideoPipe.h"


#define IQI_OFFSET 0x200   // offset from DEI



CSTi7xxxMainVideoPipe::CSTi7xxxMainVideoPipe (stm_plane_id_t        GDPid,
                                              CGammaVideoPlug      *plug,
                                              ULONG                 dei_base,
                                              enum IQICellRevision  iqi_rev): CSTb7109DEI (GDPid, 
                                                                                           plug, 
                                                                                           dei_base)
{
  DEBUGF2 (4, (FENTRY ", planeID: %d, dei_base: %lx, IQI rev/base: %d/%lx\n",
               __PRETTY_FUNCTION__,
               GDPid, dei_base, iqi_rev, dei_base + IQI_OFFSET));

  m_IQIRev = iqi_rev;
  m_IQI    = 0;

  DEBUGF2 (4, (FEXIT "\n", __PRETTY_FUNCTION__));
}


CSTi7xxxMainVideoPipe::~CSTi7xxxMainVideoPipe (void)
{
  DEBUGF2 (4, (FENTRY "\n", __PRETTY_FUNCTION__));

  delete (m_IQI);

  DEBUGF2 (4, (FEXIT "\n", __PRETTY_FUNCTION__));
}


bool
CSTi7xxxMainVideoPipe::Create (void)
{
  DEBUGF2 (4, (FENTRY "\n", __PRETTY_FUNCTION__));

  if (!CSTb7109DEI::Create ())
    return false;

  m_IQI = new CSTi7200IQI (m_IQIRev, m_baseAddress + IQI_OFFSET);
  if (!m_IQI)
    {
      DEBUGF (("%s - failed to create IQI\n", __PRETTY_FUNCTION__));
      return false;
    }
  m_IQI->Create ();

  DEBUGF2 (4, (FEXIT "\n", __PRETTY_FUNCTION__));

  return true;
}


bool
CSTi7xxxMainVideoPipe::QueuePreparedNode (const stm_display_buffer_t       * const pFrame,
                                          const GAMMA_QUEUE_BUFFER_INFO    &qbi,
                                          struct STi7xxxMainVideoPipeSetup *node)
{
  DEBUGF2 (8, ("%s\n", __PRETTY_FUNCTION__));

  m_IQI->CalculateSetup (pFrame, &node->iqi_setup);
  return QueueNode (pFrame,
                    qbi,
                    node->dei_setup, sizeof (struct STi7xxxMainVideoPipeSetup));
}


bool
CSTi7xxxMainVideoPipe::QueueBuffer (const stm_display_buffer_t * const pFrame,
                                    const void                 * const user)
{
  DEBUGF2 (8, ("%s\n", __PRETTY_FUNCTION__));

  struct STi7xxxMainVideoPipeSetup node = { dei_setup : { 0 } };
  struct GAMMA_QUEUE_BUFFER_INFO   qbi;

  if (UNLIKELY (!PrepareNodeForQueueing (pFrame,
                                         user,
                                         node.dei_setup,
                                         qbi)))
    return false;

  return QueuePreparedNode (pFrame, qbi, &node);
}


void
CSTi7xxxMainVideoPipe::UpdateHW (bool          isDisplayInterlaced,
                                 bool          isTopFieldOnDisplay,
                                 const TIME64 &vsyncTime)
{
  DEBUGF2 (9, (FENTRY "\n", __PRETTY_FUNCTION__));

  CSTb7109DEI::UpdateHW (isDisplayInterlaced, isTopFieldOnDisplay, vsyncTime);

  ULONG width;
  const struct IQISetup *iqi_setup;
  if (LIKELY (m_pendingNode.isValid))
    {
      const struct STi7xxxMainVideoPipeSetup * const nextMVPnode
        = reinterpret_cast<struct STi7xxxMainVideoPipeSetup *> (m_pendingNode.dma_area.pData);
      iqi_setup = &nextMVPnode->iqi_setup;

      width     = (nextMVPnode->dei_setup.TARGET_SIZE >> 0) & 0x7ff;
    }
  else 
    iqi_setup = 0, width = 0;

  m_IQI->UpdateHW (iqi_setup, width);

  DEBUGF2 (9, (FEXIT "\n", __PRETTY_FUNCTION__));
}


stm_plane_caps_t
CSTi7xxxMainVideoPipe::GetCapabilities (void) const
{
  DEBUGF2 (4, ("%s\n", __PRETTY_FUNCTION__));

  stm_plane_caps_t caps = CSTb7109DEI::GetCapabilities ();
  caps.ulControls |= m_IQI->GetCapabilities ();

  return caps;
}

bool
CSTi7xxxMainVideoPipe::SetControl (stm_plane_ctrl_t control,
                                   ULONG            value)
{
  bool retval;

  DEBUGF2 (8, (FENTRY "\n", __PRETTY_FUNCTION__));

  switch (control)
    {
    case PLANE_CTRL_IQI_FIRST ... PLANE_CTRL_IQI_LAST:
      retval = m_IQI->SetControl (control, value);
      break;

    default:
      retval = CSTb7109DEI::SetControl (control, value);
      break;
    }

  return retval;
}


bool
CSTi7xxxMainVideoPipe::GetControl (stm_plane_ctrl_t  control,
                                   ULONG            *value) const
{
  bool retval;

  DEBUGF2 (4, (FENTRY "\n", __PRETTY_FUNCTION__));

  switch (control)
    {
    case PLANE_CTRL_IQI_FIRST ... PLANE_CTRL_IQI_LAST:
      retval = m_IQI->GetControl (control, value);
      break;

    default:
      retval = CSTb7109DEI::GetControl (control, value);
      break;
    }

  return retval;
}
