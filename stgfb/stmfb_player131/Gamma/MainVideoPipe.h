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
#ifndef _MAIN_VIDEO_PIPE_H
#define _MAIN_VIDEO_PIPE_H

#include <Gamma/stb7100/stb7109dei.h>
#include <Gamma/sti7200/sti7200iqi.h>


/* the main video pipeline on newer chips has a dei and an iqi embedded */
struct STi7xxxMainVideoPipeSetup
{
  struct DEISetup dei_setup; /* must be first! */
  struct IQISetup iqi_setup;
};


class CSTi7xxxMainVideoPipe: public CSTb7109DEI
{
public:
  CSTi7xxxMainVideoPipe (stm_plane_id_t        GDPid,
                         CGammaVideoPlug      *plug,
                         ULONG                 dei_base,
                         enum IQICellRevision  iqi_rev);
  virtual ~CSTi7xxxMainVideoPipe (void);

  bool Create (void);

  bool QueuePreparedNode (const stm_display_buffer_t       * const pFrame,
                          const GAMMA_QUEUE_BUFFER_INFO    &qbi,
                          struct STi7xxxMainVideoPipeSetup *node);
  bool QueueBuffer (const stm_display_buffer_t * const pFrame,
                    const void                 * const user);
  void UpdateHW (bool          isDisplayInterlaced,
                 bool          isTopFieldOnDisplay,
                 const TIME64 &vsyncTime);

  stm_plane_caps_t GetCapabilities (void) const;

  bool SetControl (stm_plane_ctrl_t control, ULONG  value);
  bool GetControl (stm_plane_ctrl_t control, ULONG *value) const;

private:
  enum IQICellRevision  m_IQIRev; /* need to remember this between the
                                     constructor and Create () */
  CSTi7200IQI          *m_IQI;
};


#endif /* _MAIN_VIDEO_PIPE_H */
