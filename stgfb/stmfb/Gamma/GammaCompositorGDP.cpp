/***********************************************************************
 *
 * File: stgfb/Gamma/GammaCompositorGDP.cpp
 * Copyright (c) 2004-2009 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>
#include <Generic/DisplayDevice.h>

#include "GenericGammaDefs.h"
#include "GenericGammaReg.h"

#include "GenericGammaOutput.h"
#include "GammaCompositorGDP.h"
#include "STRefGDPFilters.h"

static const SURF_FMT g_surfaceFormats[] = {
  SURF_RGB565,
  SURF_ARGB1555,
  SURF_ARGB4444,
  SURF_ARGB8888,
  SURF_BGRA8888,
  SURF_RGB888,
  SURF_ARGB8565,
  SURF_YCBCR422R,
  SURF_CRYCB888,
  SURF_ACRYCB8888
};


static const SURF_FMT g_surfaceFormatsWithClut[] = {
  SURF_RGB565,
  SURF_ARGB1555,
  SURF_ARGB4444,
  SURF_ARGB8888,
  SURF_BGRA8888,
  SURF_RGB888,
  SURF_ARGB8565,
  SURF_YCBCR422R,
  SURF_CRYCB888,
  SURF_ACRYCB8888,
  SURF_CLUT8,
  SURF_ACLUT88
};

#define GDP_HFILTER_TABLE_SIZE (NB_HSRC_FILTERS * (NB_HSRC_COEFFS + HFILTERS_ALIGN))
#define GDP_VFILTER_TABLE_SIZE (NB_VSRC_FILTERS * (NB_VSRC_COEFFS + VFILTERS_ALIGN))
#define GDP_FFILTER_TABLE_SIZE (sizeof(ULONG)*6)
#define GDP_CLUT_SIZE          (256*sizeof(ULONG))

CGammaCompositorGDP::CGammaCompositorGDP(stm_plane_id_t GDPid,
                                         ULONG baseAddr,
                                         ULONG PKZVal,
                                         bool  bHasClut):CGammaCompositorPlane(GDPid)
{
  DEBUGF2(2,("CGammaCompositorGDP::CGammaCompositorGDP id = %d\n",GDPid));

  m_GDPBaseAddr = baseAddr;
  m_bHasClut    = bHasClut;
  if(!m_bHasClut)
  {
    m_pSurfaceFormats = g_surfaceFormats;
    m_nFormats = sizeof(g_surfaceFormats)/sizeof(SURF_FMT);
  }
  else
  {
    m_pSurfaceFormats = g_surfaceFormatsWithClut;
    m_nFormats = sizeof(g_surfaceFormatsWithClut)/sizeof(SURF_FMT);
  }

  g_pIOS->WriteRegister((volatile ULONG *)(m_GDPBaseAddr + GDPn_PKZ_OFFSET),PKZVal);

  g_pIOS->ZeroMemory(&m_HFilter,       sizeof(DMA_Area));
  g_pIOS->ZeroMemory(&m_VFilter,       sizeof(DMA_Area));
  g_pIOS->ZeroMemory(&m_FlickerFilter, sizeof(DMA_Area));
  g_pIOS->ZeroMemory(&m_Registers,     sizeof(DMA_Area));

  m_bHasVFilter       = false;
  m_bHasFlickerFilter = false;

  /*
   * The GDP sample rate converters have an n.8 fixed point format,
   * but to get better registration with video planes we do the maths in n.13
   * and then round it before use to reduce the fixed point error between the
   * two. Not doing this is particularly noticeable with DVD menu highlights
   * when upscaling to HD output.
   */
  m_fixedpointONE     = 1<<13;

  /*
   * Do not assume scaling is available, SoC specific subclasses will
   * override this in their constructors.
   */
  m_ulMaxHSrcInc   = m_fixedpointONE;
  m_ulMinHSrcInc   = m_fixedpointONE;
  m_ulMaxVSrcInc   = m_fixedpointONE;
  m_ulMinVSrcInc   = m_fixedpointONE;

  m_ulGain           = 255;
  m_ulAlphaRamp      = 0x0000ff00;
  m_ulStaticAlpha[0] = 0;
  m_ulStaticAlpha[1] = 0x80;

  m_ColorKeyConfig.flags  = static_cast<enum StmColorKeyConfigFlags>(SCKCF_ENABLE | SCKCF_FORMAT | SCKCF_R_INFO | SCKCF_G_INFO | SCKCF_B_INFO);
  m_ColorKeyConfig.enable = 0;
  m_ColorKeyConfig.format = SCKCVF_RGB;
  m_ColorKeyConfig.r_info = SCKCCM_DISABLED;
  m_ColorKeyConfig.g_info = SCKCCM_DISABLED;
  m_ColorKeyConfig.b_info = SCKCCM_DISABLED;

  m_capabilities.ulCaps      = (PLANE_CAPS_RESIZE                 |
                                PLANE_CAPS_SRC_COLOR_KEY          |
                                PLANE_CAPS_PREMULTIPLED_ALPHA     |
                                PLANE_CAPS_RESCALE_TO_VIDEO_RANGE |
                                PLANE_CAPS_GLOBAL_ALPHA);

  m_capabilities.ulControls  = (PLANE_CTRL_CAPS_GAIN |
                                PLANE_CTRL_CAPS_ALPHA_RAMP);

  m_capabilities.ulMinFields =    1;
  m_capabilities.ulMinWidth  =   32;
  m_capabilities.ulMinHeight =   24;
  m_capabilities.ulMaxWidth  = 1920;
  m_capabilities.ulMaxHeight = 1088;

  /*
   * Note: the scaling capabilities are calculated in the Create method as
   *       the sample rate limits may be overriden.
   */

  DEBUGF2(2,("CGammaCompositorGDP::CGammaCompositorGDP out\n"));
}


CGammaCompositorGDP::~CGammaCompositorGDP(void)
{
  DENTRY();

  g_pIOS->FreeDMAArea(&m_HFilter);
  g_pIOS->FreeDMAArea(&m_VFilter);
  g_pIOS->FreeDMAArea(&m_FlickerFilter);
  g_pIOS->FreeDMAArea(&m_Registers);

  DENTRY();
}


bool CGammaCompositorGDP::Create(void)
{
  DENTRY();

  if(!CDisplayPlane::Create())
    return false;

  SetScalingCapabilities(&m_capabilities);

  g_pIOS->AllocateDMAArea(&m_HFilter, GDP_HFILTER_TABLE_SIZE, 16, SDAAF_NONE);
  if(!m_HFilter.pMemory)
  {
    DERROR("failed to allocate HFilter memory");
    return false;
  }

  g_pIOS->MemcpyToDMAArea(&m_HFilter,0,&stlayer_HSRC_Coeffs,sizeof(stlayer_HSRC_Coeffs));
  g_pIOS->FlushCache(m_HFilter.pData, GDP_HFILTER_TABLE_SIZE);

  if(m_bHasVFilter)
  {
    g_pIOS->AllocateDMAArea(&m_VFilter, GDP_VFILTER_TABLE_SIZE, 16, SDAAF_NONE);
    if(!m_VFilter.pMemory)
    {
      DERROR("failed to allocate VFilter memory");
      return false;
    }

    g_pIOS->MemcpyToDMAArea(&m_VFilter,0,&stlayer_VSRC_Coeffs,sizeof(stlayer_VSRC_Coeffs));
    g_pIOS->FlushCache(m_VFilter.pData, GDP_VFILTER_TABLE_SIZE);
  }

  if(m_bHasFlickerFilter)
  {
    m_capabilities.ulCaps |= PLANE_CAPS_FLICKER_FILTER;

    g_pIOS->AllocateDMAArea(&m_FlickerFilter, GDP_FFILTER_TABLE_SIZE, 16, SDAAF_NONE);
    if(!m_FlickerFilter.pMemory)
    {
      DERROR("failed to allocate VFilter memory");
      return false;
    }

    unsigned long *filter = (unsigned long *)m_FlickerFilter.pData;
    /*
     * The flicker filter set is identical to that found in the Gamma blitter.
     */
    filter[0] = 0x04008000; // Threshold1 = 4, Taps = 0%   ,100%,0%
    filter[1] = 0x06106010; // Threshold2 = 6, Taps = 12.5%,75% ,12.5%
    filter[2] = 0x08185018; // Threshold3 = 8, Taps = 19%  ,62% ,19%
    filter[3] = 0x00204020; //                 Taps = 25%  ,50% ,25%

    g_pIOS->FlushCache(m_FlickerFilter.pData, GDP_FFILTER_TABLE_SIZE);
  }

  g_pIOS->AllocateDMAArea(&m_Registers, sizeof(GENERIC_GDP_LLU_NODE), 16, SDAAF_NONE);
  if(!m_Registers.pMemory)
  {
    DERROR("failed to allocate register memory");
    return false;
  }

  /*
   * Set the next node pointer back to itself for now. This will change if we
   * support a VBI node as well.
   */
  ((GENERIC_GDP_LLU_NODE *)m_Registers.pData)->GDPn_NVN = m_Registers.ulPhysical;

  DEXIT();
  return true;
}


static inline ULONG round_src(ULONG val)
{
  /*
   * val is a sample rate converter step in n.13 fixed point. We want to round
   * it as if it were n.8, but leave it in n.13 format for now.
   */
  val += 1L<<4; // change bit 6, this will be the LSB in n.8
  val &= ~0x1f; // zero the bottom 5 bits that will be lost in n.8
  return val;
}


bool CGammaCompositorGDP::QueueBuffer(const stm_display_buffer_t* const pBuffer,
                                      const void                * const user)
{
  GENERIC_GDP_LLU_NODE     nodes[2] = {{0}};
  GENERIC_GDP_LLU_NODE     &topNode = nodes[0];
  GENERIC_GDP_LLU_NODE     &botNode = nodes[1];
  GAMMA_QUEUE_BUFFER_INFO  qbinfo    = {0};

  DEBUGF2(DEBUG_GDPNODE_LEVEL,("CGammaCompositorGDP::QueueBuffer 'pBuffer = %p'\n",pBuffer));

  if(!GetQueueBufferInfo(pBuffer, user, qbinfo))
    return false;

  AdjustBufferInfoForScaling(pBuffer, qbinfo);
  qbinfo.hsrcinc        = round_src(qbinfo.hsrcinc);
  qbinfo.vsrcinc        = round_src(qbinfo.vsrcinc);
  qbinfo.chroma_hsrcinc = round_src(qbinfo.chroma_hsrcinc);
  qbinfo.chroma_vsrcinc = round_src(qbinfo.chroma_vsrcinc);

  DEBUGF2(3,("%s: one = 0x%lx hsrcinc = 0x%lx chsrcinc = 0x%lx\n",__PRETTY_FUNCTION__,m_fixedpointONE, qbinfo.hsrcinc,qbinfo.chroma_hsrcinc));
  DEBUGF2(3,("%s: one = 0x%lx vsrcinc = 0x%lx cvsrcinc = 0x%lx\n",__PRETTY_FUNCTION__,m_fixedpointONE, qbinfo.vsrcinc,qbinfo.chroma_vsrcinc));

  //Wait for Next VSync before loading the next node
  topNode.GDPn_CTL = botNode.GDPn_CTL = GDP_CTL_WAIT_NEXT_VSYNC;

  /*
   * This may seem pointless and convoluted, but it is to cope with future
   * functionality where we have a VBI region as well as a graphics region.
   */
  topNode.GDPn_NVN = botNode.GDPn_NVN = ((GENERIC_GDP_LLU_NODE *)m_Registers.pData)->GDPn_NVN;

  /*
   * Note that the resize and filter setup, may set state in the nodes which
   * is inspected by the following calls to setOutputViewport and
   * setMemoryAddressing, so don't reorder these!
   */
  if(!setNodeResizeAndFilters(topNode, botNode, pBuffer, qbinfo))
    return false;

  if(!setOutputViewport(topNode, botNode, qbinfo))
    return false;

  if(!setMemoryAddressing(topNode, botNode, pBuffer, qbinfo))
    return false;

  if(!setNodeColourFmt(topNode, botNode, pBuffer))
    return false;

  if(!setNodeColourKeys(topNode, botNode, pBuffer))
    return false;

  if(!setNodeAlpha(topNode, botNode, pBuffer))
    return false;

  DEBUGF2(DEBUG_GDPNODE_LEVEL,("QUEUING ----------------- Top Node -----------------------\n"));
  DEBUGGDP(&nodes[0]);
  DEBUGF2(DEBUG_GDPNODE_LEVEL,("---------------------- Bottom Node -----------------------\n"));
  DEBUGGDP(&nodes[1]);
  DEBUGF2(DEBUG_GDPNODE_LEVEL,("----------------------------------------------------------\n"));

  if(!qbinfo.isSourceInterlaced)
    return QueueProgressiveContent(nodes, sizeof(nodes), qbinfo);

  if(qbinfo.firstFieldOnly || (qbinfo.isDisplayInterlaced && !qbinfo.interpolateFields))
    return QueueSimpleInterlacedContent(nodes, sizeof(nodes), qbinfo);
  else
    return QueueInterpolatedInterlacedContent(nodes, sizeof(nodes), qbinfo);

  DEBUGF2(DEBUG_GDPNODE_LEVEL,("CGammaCompositorGDP::QueueBuffer %p completed\n",pBuffer));

  return true;
}


void CGammaCompositorGDP::UpdateHW(bool isDisplayInterlaced,
                                   bool isTopFieldOnDisplay,
                                   const TIME64 &vsyncTime)
{
  /* This is only called as part of the display interrupt processing
   * bottom half and is not re-enterant. Given limitations on stack usage in
   * interrupt context on certain systems, we keep as much off the stack
   * as possible.
   */
  static stm_plane_node displayNode;

  void *nextfieldsetup = 0;

  UpdateCurrentNode(vsyncTime);

  if(m_currentNode.isValid && isDisplayInterlaced)
  {
    if(!isTopFieldOnDisplay)
      nextfieldsetup = (void*)m_currentNode.dma_area.pData;
    else
      nextfieldsetup = (void*)(m_currentNode.dma_area.pData+sizeof(GENERIC_GDP_LLU_NODE));
  }

  if(m_isPaused || (m_currentNode.isValid && m_currentNode.info.nfields > 1))
  {
    /*
     * We have got a node on display that is going to be there for more than
     * one frame/field, so we do not get another from the queue yet.
     */
    goto write_next_field;
  }

  if(!PeekNextNodeFromDisplayList(vsyncTime, displayNode))
    goto write_next_field;

  if(isDisplayInterlaced)
  {
    /*
     * If we are being asked to present graphics on an interlaced display
     * then only allow changes on the top field. This is to prevent visual
     * artifacts when animating vertical movement.
     */
    if(isTopFieldOnDisplay && (displayNode.info.ulFlags & STM_PLANE_PRESENTATION_GRAPHICS))
      goto write_next_field;

    if((isTopFieldOnDisplay  && displayNode.nodeType == GNODE_TOP_FIELD) ||
       (!isTopFieldOnDisplay && displayNode.nodeType == GNODE_BOTTOM_FIELD))
    {
      DEBUGF2(3,("%s: Waiting for correct field\n", __PRETTY_FUNCTION__));
      DEBUGF2(3,("%s: isTopFieldOnDisplay = %s\n", __PRETTY_FUNCTION__, isTopFieldOnDisplay?"true":"false"));
      DEBUGF2(3,("%s: nodeType            = %s\n", __PRETTY_FUNCTION__, (displayNode.nodeType==GNODE_TOP_FIELD)?"top":"bottom"));

      goto write_next_field;
    }

    if(!isTopFieldOnDisplay)
      nextfieldsetup = (void*)displayNode.dma_area.pData;
    else
      nextfieldsetup = (void*)(displayNode.dma_area.pData+sizeof(GENERIC_GDP_LLU_NODE));
  }
  else
  {
    if(displayNode.nodeType == GNODE_PROGRESSIVE || displayNode.nodeType == GNODE_TOP_FIELD)
      nextfieldsetup = (void*)displayNode.dma_area.pData;
    else
      nextfieldsetup = (void*)(displayNode.dma_area.pData+sizeof(GENERIC_GDP_LLU_NODE));
  }

  if(!m_isEnabled)
    EnableHW();

  SetPendingNode(displayNode);
  PopNextNodeFromDisplayList();

write_next_field:
  if(nextfieldsetup)
    writeFieldSetup((GENERIC_GDP_LLU_NODE*)nextfieldsetup);

}


void CGammaCompositorGDP::writeFieldSetup(GENERIC_GDP_LLU_NODE *fieldsetup)
{

  /* Write the main registers */
  g_pIOS->MemcpyToDMAArea(&m_Registers, 0, fieldsetup, sizeof(GENERIC_GDP_LLU_NODE));

  /*
   * Now sort out the gain and RGB1555 alpha ramp, which can change
   * independently of the buffer.
   */
  ULONG staticgain = (m_ulGain+1)/2; // convert 0-255 to 0-128 (note not 127)
  /*
   * Extract the gain associated with the buffer and scale it by the global
   * gain for the plane.
   */
  ULONG buffergain = (fieldsetup->GDPn_AGC & GDP_AGC_GAIN_MASK) >> GDP_AGC_GAIN_SHIFT;
        buffergain = (buffergain*staticgain) / 128;

  ULONG agc = (fieldsetup->GDPn_AGC & ~GDP_AGC_GAIN_MASK) | (buffergain << GDP_AGC_GAIN_SHIFT);

  if((fieldsetup->GDPn_CTL & GDP_CTL_FORMAT_MASK) == GDP_CTL_ARGB_1555)
  {
      ULONG bufferalpha = fieldsetup->GDPn_AGC & 0xff;
      agc &= 0xffff0000; // Clear the global alpha bits
      /*
       * Scale the alpha ramp map by the requested buffer alpha
       */
      agc |= (m_ulStaticAlpha[0] * bufferalpha)/128;
      agc |= ((m_ulStaticAlpha[1] * bufferalpha)/128)<<8;
  }

  g_pIOS->MemcpyToDMAArea(&m_Registers, (ULONG)&((GENERIC_GDP_LLU_NODE *)0)->GDPn_AGC, &agc, sizeof(ULONG));
}


bool CGammaCompositorGDP::setNodeColourFmt(GENERIC_GDP_LLU_NODE       &topNode,
                                           GENERIC_GDP_LLU_NODE       &botNode,
                                     const stm_display_buffer_t* const pBuffer)
{
  ULONG ulCtrl = 0;
  ULONG alphaRange = (pBuffer->src.ulFlags & STM_PLANE_SRC_LIMITED_RANGE_ALPHA)?0:GDP_CTL_ALPHA_RANGE;

  switch(pBuffer->src.ulColorFmt)
  {
    case SURF_RGB565:
      ulCtrl = GDP_CTL_RGB_565;
      break;

    case SURF_RGB888:
      ulCtrl = GDP_CTL_RGB_888;
      break;

    case SURF_ARGB8565:
      ulCtrl = GDP_CTL_ARGB_8565 | alphaRange;
      break;

    case SURF_ARGB8888:
      ulCtrl = GDP_CTL_ARGB_8888 | alphaRange;
      break;

    case SURF_ARGB1555:
      ulCtrl = GDP_CTL_ARGB_1555;
      break;

    case SURF_ARGB4444:
      ulCtrl = GDP_CTL_ARGB_4444;
      break;

    case SURF_YCBCR422R:
      ulCtrl = GDP_CTL_YCbCr422R;

      if(pBuffer->src.ulFlags & STM_PLANE_SRC_COLORSPACE_709)
        ulCtrl |= GDP_CTL_709_SELECT;

      break;

    case SURF_CRYCB888:
      ulCtrl = GDP_CTL_YCbCr888;

      if(pBuffer->src.ulFlags & STM_PLANE_SRC_COLORSPACE_709)
        ulCtrl |= GDP_CTL_709_SELECT;

      break;

    case SURF_ACRYCB8888:
      ulCtrl = GDP_CTL_AYCbCr8888 | alphaRange;

      if(pBuffer->src.ulFlags & STM_PLANE_SRC_COLORSPACE_709)
        ulCtrl |= GDP_CTL_709_SELECT;

      break;

    case SURF_CLUT8:
      if(m_bHasClut)
      {
      	ulCtrl |= GDP_CTL_CLUT8 | GDP_CTL_EN_CLUT_UPDATE;
      }
      else
      {
        DEBUGF2(1,("CGammaCompositorGDP::setNodeColourFmt - Clut not supported on hardware.\n"));
        return false;
      }
      break;

    case SURF_ACLUT88:
      if(m_bHasClut)
      {
        ulCtrl |= GDP_CTL_ACLUT88 | GDP_CTL_EN_CLUT_UPDATE;
      }
      else
      {
        DEBUGF2(1,("CGammaCompositorGDP::setNodeColourFmt - Clut not supported on hardware.\n"));
        return false;
      }
      break;

    case SURF_BGRA8888:
      ulCtrl = GDP_CTL_ARGB_8888 | GDP_CTL_BIGENDIAN | alphaRange;
      break;

    default:
      DERROR("Unknown colour format.");
      return false;
  }

  if(pBuffer->src.ulFlags & STM_PLANE_SRC_PREMULTIPLIED_ALPHA)
  {
    DEBUGF2(3,("CGammaCompositorGDP::setNodeColourFmt - Setting Premultiplied Alpha.\n"));
    ulCtrl |= GDP_CTL_PREMULT_FORMAT;
  }

  topNode.GDPn_CTL |= ulCtrl;
  botNode.GDPn_CTL |= ulCtrl;

  DEBUGF2(3,("CGammaCompositorGDP::setNodeColourFmt - Setting Clut Address = 0x%08lx.\n",pBuffer->src.ulClutBusAddress));
  topNode.GDPn_CML  = botNode.GDPn_CML = pBuffer->src.ulClutBusAddress;

  return true;
}


void CGammaCompositorGDP::updateColorKeyState (stm_color_key_config_t       * const dst,
					       const stm_color_key_config_t * const src) const
{
  /* argh, this should be possible in a nicer way... */
  dst->flags = static_cast<StmColorKeyConfigFlags> (src->flags | dst->flags);
  if (src->flags & SCKCF_ENABLE)
    dst->enable = src->enable;
  if (src->flags & SCKCF_FORMAT)
    dst->format = src->format;
  if (src->flags & SCKCF_R_INFO)
    dst->r_info = src->r_info;
  if (src->flags & SCKCF_G_INFO)
    dst->g_info = src->g_info;
  if (src->flags & SCKCF_B_INFO)
    dst->b_info = src->b_info;
  if (src->flags & SCKCF_MINVAL)
    dst->minval = src->minval;
  if (src->flags & SCKCF_MAXVAL)
    dst->maxval = src->maxval;
}

bool CGammaCompositorGDP::setNodeColourKeys(GENERIC_GDP_LLU_NODE       &topNode,
                                            GENERIC_GDP_LLU_NODE       &botNode,
                                      const stm_display_buffer_t* const pBuffer)
{
  UCHAR ucRCR = 0;
  UCHAR ucGY  = 0;
  UCHAR ucBCB = 0;

  // GDPs do not support destination colour keying
  if((pBuffer->dst.ulFlags & (STM_PLANE_DST_COLOR_KEY | STM_PLANE_DST_COLOR_KEY_INV)) != 0)
    return false;

  updateColorKeyState (&m_ColorKeyConfig, &pBuffer->src.ColorKey);

  // If colour keying not required, do nothing.
  if (m_ColorKeyConfig.enable == 0)
    return true;

  //Get Min Key value
  if (!(m_ColorKeyConfig.flags & SCKCF_MINVAL)
      || !GetRGBYCbCrKey (ucRCR, ucGY, ucBCB, m_ColorKeyConfig.minval,
			  pBuffer->src.ulColorFmt,
			  m_ColorKeyConfig.format == SCKCVF_RGB))
  {
    DERROR("Min key value not obtained.");
    return false;
  }
  botNode.GDPn_KEY1 = topNode.GDPn_KEY1 = (ucBCB | (ucGY<<8) | (ucRCR<<16));

  //Get Max Key value
  if (!(m_ColorKeyConfig.flags & SCKCF_MAXVAL)
      || !GetRGBYCbCrKey (ucRCR, ucGY, ucBCB, m_ColorKeyConfig.maxval,
			  pBuffer->src.ulColorFmt,
			  m_ColorKeyConfig.format == SCKCVF_RGB))
  {
    DERROR("Max key value not obtained");
    return false;
  }
  botNode.GDPn_KEY2 = topNode.GDPn_KEY2 = (ucBCB | (ucGY<<8) | (ucRCR<<16));

  ULONG ulCtrl = GDP_CTL_EN_COLOR_KEY;

  switch (m_ColorKeyConfig.r_info) {
  case SCKCCM_DISABLED: break;
  case SCKCCM_ENABLED: ulCtrl |= GDP_CTL_RCR_COL_KEY_1; break;
  case SCKCCM_INVERSE: ulCtrl |= GDP_CTL_RCR_COL_KEY_3; break;
  default: return false;
  }

  switch (m_ColorKeyConfig.g_info) {
  case SCKCCM_DISABLED: break;
  case SCKCCM_ENABLED: ulCtrl |= GDP_CTL_GY_COL_KEY_1; break;
  case SCKCCM_INVERSE: ulCtrl |= GDP_CTL_GY_COL_KEY_3; break;
  default: return false;
  }

  switch (m_ColorKeyConfig.b_info) {
  case SCKCCM_DISABLED: break;
  case SCKCCM_ENABLED: ulCtrl |= GDP_CTL_BCB_COL_KEY_1; break;
  case SCKCCM_INVERSE: ulCtrl |= GDP_CTL_BCB_COL_KEY_3; break;
  default: return false;
  }

  topNode.GDPn_CTL |= ulCtrl;
  botNode.GDPn_CTL |= ulCtrl;

  return true;
}


bool CGammaCompositorGDP::setNodeResizeAndFilters(GENERIC_GDP_LLU_NODE    &topNode,
                                                  GENERIC_GDP_LLU_NODE    &botNode,
                                            const stm_display_buffer_t* const pBuffer,
                                            const GAMMA_QUEUE_BUFFER_INFO &qbinfo)
{
  ULONG ulCtrl = 0;
  ULONG hw_srcinc;
  unsigned filter_index;

  if(qbinfo.hsrcinc != (ULONG)m_fixedpointONE)
  {
    DEBUGF2(3,("CGammaCompositorGDP::setNodeResizeAndFilters H Resize Enabled.\n"));

    hw_srcinc = (qbinfo.hsrcinc >> 5);
    ulCtrl |= GDP_CTL_EN_H_RESIZE;
    botNode.GDPn_HSRC = topNode.GDPn_HSRC = hw_srcinc | GDP_HSRC_FILTER_EN;

    for(filter_index=0;filter_index<N_ELEMENTS(HSRC_index);filter_index++)
    {
      if(hw_srcinc <= HSRC_index[filter_index])
      {
        DEBUGF2(3,("CGammaCompositorGDP::setNodeResizeAndFilters: HSRC = %lx filter index = %d.\n",hw_srcinc,filter_index));
        botNode.GDPn_HFP = topNode.GDPn_HFP = m_HFilter.ulPhysical + (filter_index*HFILTERS_ENTRY_SIZE);
        ulCtrl |= GDP_CTL_EN_HFILTER_UPD;
        break;
      }
    }
  }

  if(qbinfo.vsrcinc != (ULONG)m_fixedpointONE)
  {
    DEBUGF2(3,("CGammaCompositorGDP::setNodeResizeAndFilterss V Resize Enabled.\n"));

    if((pBuffer->dst.ulFlags & STM_PLANE_DST_FLICKER_FILTER) &&
       (m_bHasFlickerFilter && qbinfo.isDisplayInterlaced && !qbinfo.isSourceInterlaced) &&
       (qbinfo.vsrcinc == (ULONG)m_fixedpointONE*2))
    {
      DEBUGF2(3,("CGammaCompositorGDP::setNodeResizeAndFilterss Flicker Filtering Enabled.\n"));
      ulCtrl |= (GDP_CTL_EN_FLICKERFIL | GDP_CTL_EN_VFILTER_UPD);

      if((pBuffer->dst.ulFlags & STM_PLANE_DST_ADAPTIVE_FF) &&
         (pBuffer->src.ulColorFmt != SURF_YCBCR422R))
      {
        DEBUGF2(3,("CGammaCompositorGDP::setNodeResizeAndFilterss Adaptive Flicker Filtering Enabled.\n"));
        botNode.GDPn_VSRC = topNode.GDPn_VSRC = GDP_VSRC_ADAPTIVE_FLICKERFIL;
      }

      botNode.GDPn_VFP = topNode.GDPn_VFP = m_FlickerFilter.ulPhysical;
    }
    else
    {
      DEBUGF2(3,("CGammaCompositorGDP::setNodeResizeAndFilterss 3-Tap Polyphase Filtering Enabled.\n"));

      hw_srcinc = (qbinfo.vsrcinc >> 5);
      botNode.GDPn_VSRC  = topNode.GDPn_VSRC = hw_srcinc | GDP_VSRC_FILTER_EN;
      ulCtrl |= GDP_CTL_EN_V_RESIZE;

      if(m_bHasVFilter)
      {
        for(filter_index=0;filter_index<N_ELEMENTS(VSRC_index);filter_index++)
        {
          if(hw_srcinc <= VSRC_index[filter_index])
          {
            DEBUGF2(3,("CGammaCompositorGDP::setNodeResizeAndFilters: VSRC = %lx filter index = %d.\n",hw_srcinc,filter_index));
            botNode.GDPn_VFP   = topNode.GDPn_VFP = m_VFilter.ulPhysical + (filter_index*VFILTERS_ENTRY_SIZE);
            ulCtrl |= GDP_CTL_EN_VFILTER_UPD;
            break;
          }
        }
      }

      if(qbinfo.isDisplayInterlaced && !qbinfo.isSourceInterlaced)
      {
        /*
         * When putting progressive content on an interlaced display
         * we adjust the filter phase of the bottom field to start
         * an appropriate distance lower in the source bitmap, based on the
         * scaling factor. If the scale means that the bottom field
         * starts >1 source bitmap line lower then this will get dealt
         * with in the memory setup by adjusting the source bitmap address.
         */
        ULONG bottomfieldinc   = ScaleVerticalSamplePosition(m_fixedpointONE,qbinfo);
        ULONG integerinc       = bottomfieldinc/m_fixedpointONE;
        ULONG fractionalinc    = bottomfieldinc - integerinc*m_fixedpointONE;
        ULONG bottomfieldphase = (((fractionalinc*8)+(m_fixedpointONE/2))/m_fixedpointONE) % 8;

        botNode.GDPn_VSRC |= (bottomfieldphase<<GDP_VSRC_INITIAL_PHASE_SHIFT);
      }
    }
  }

  topNode.GDPn_CTL |= ulCtrl;
  botNode.GDPn_CTL |= ulCtrl;

  return true;
}


bool CGammaCompositorGDP::setOutputViewport(GENERIC_GDP_LLU_NODE    &topNode,
                                            GENERIC_GDP_LLU_NODE    &botNode,
                                      const GAMMA_QUEUE_BUFFER_INFO &qbinfo)
{
  /*
   * Set the destination viewport, which is in terms of frame line numbering,
   * regardless of interlaced/progressive.
   */
  topNode.GDPn_VPO = PackRegister(qbinfo.viewportStartLine, qbinfo.viewportStartPixel);
  botNode.GDPn_VPO = topNode.GDPn_VPO;

  topNode.GDPn_VPS = PackRegister(qbinfo.viewportStopLine, qbinfo.viewportStopPixel);
  botNode.GDPn_VPS = topNode.GDPn_VPS;

  /*
   * The number of pixels per line to read is recalculated from the number of
   * output samples required to fill the viewport, which may have been clipped
   * by the active video area.
   */
  ULONG horizontalInputSamples = (qbinfo.horizontalFilterOutputSamples * qbinfo.hsrcinc) / m_fixedpointONE;
  if(qbinfo.src.width < 0 || horizontalInputSamples > (ULONG) qbinfo.src.width)
    horizontalInputSamples = qbinfo.src.width;

  DEBUGF2(3,("%s: H input samples = %lu V input samples = %lu\n",__PRETTY_FUNCTION__,horizontalInputSamples, qbinfo.verticalFilterInputSamples));

  topNode.GDPn_SIZE = PackRegister(qbinfo.verticalFilterInputSamples, horizontalInputSamples);
  botNode.GDPn_SIZE = topNode.GDPn_SIZE;

  return true;
}


bool CGammaCompositorGDP::setMemoryAddressing(GENERIC_GDP_LLU_NODE    &topNode,
                                              GENERIC_GDP_LLU_NODE    &botNode,
                                        const stm_display_buffer_t* const pBuffer,
                                        const GAMMA_QUEUE_BUFFER_INFO &qbinfo)
{
  ULONG pitch = pBuffer->src.ulStride;

  if(qbinfo.isSourceInterlaced)
  {
    DEBUGF2(3,("CGammaCompositorGDP::setMemoryAddressing Programming for interlaced output.\n"));
    topNode.GDPn_PMP = pitch*2;
  }
  else
  {
    DEBUGF2(3,("CGammaCompositorGDP::setMemoryAddressing Programming for progressive output.\n"));
    topNode.GDPn_PMP = pitch;
  }

  botNode.GDPn_PMP = topNode.GDPn_PMP;

  // Set up the pixmap memory
  bool  usingFlickerFilter = (topNode.GDPn_CTL & GDP_CTL_EN_FLICKERFIL) != 0;
  ULONG ulXStart        = qbinfo.src.x/m_fixedpointONE;
  ULONG ulYStart        = qbinfo.src.y/m_fixedpointONE;
  ULONG ulBytesPerPixel = pBuffer->src.ulPixelDepth>>3;
  ULONG ulScanLine      = topNode.GDPn_PMP * ulYStart;
  ULONG bufferPhysAddr  = pBuffer->src.ulVideoBufferAddr;

  if(!usingFlickerFilter)
  {
    topNode.GDPn_PML = bufferPhysAddr + ulScanLine + (ulBytesPerPixel * ulXStart);

    if(qbinfo.isSourceInterlaced)
    {
      /*
       * When accessing the buffer content as interlaced fields the bottom
       * field start pointer must be one line down the buffer.
       */
      botNode.GDPn_PML = topNode.GDPn_PML + pitch;
    }
    else if(qbinfo.isDisplayInterlaced)
    {
      /*
       * Progressive on interlaced display, the start position of the
       * bottom field depends on the scaling factor involved. Note that
       * we round to the next line if the sample position is in the last 1/16th.
       * The filter phase (which has 8 positions) will have been rounded up
       * and the wrapped to 0 in the filter setup.
       *
       * Note: this could potentially cause us to read beyond the end of the
       * image buffer.
       */
      ULONG linestobottomfield = (ScaleVerticalSamplePosition(m_fixedpointONE,qbinfo)+(m_fixedpointONE/16))/m_fixedpointONE;
      botNode.GDPn_PML = topNode.GDPn_PML + (pitch*linestobottomfield);
    }
    else
    {
      /*
       * Progressive content on a progressive display has the same start pointer
       * for both nodes, i.e. the content is repeated exactly.
       */
      botNode.GDPn_PML = topNode.GDPn_PML;
    }
  }
  else
  {
    /*
     * We can only flicker filter progressive images on an interlaced display,
     * with no rescale applied, so there is only one case to cater for.
     *
     * Somewhat strangely we set the same pointer for both top and bottom fields
     * and the hardware just err works it out... there has to be a catch.
     */
    topNode.GDPn_PML = botNode.GDPn_PML = bufferPhysAddr + ulScanLine + (ulBytesPerPixel * ulXStart);
  }

  DEBUGF2(3,("CGammaCompositorGDP::setMemoryAddressing top PML = %#08lx bot PML = %#08lx.\n",topNode.GDPn_PML,botNode.GDPn_PML));

  return true;
}


bool CGammaCompositorGDP::setNodeAlpha(GENERIC_GDP_LLU_NODE       &topNode,
                                       GENERIC_GDP_LLU_NODE       &botNode,
                                 const stm_display_buffer_t* const pBuffer)
{
  static const ULONG GDP_AGC_FULL_RANGE  = 128<<GDP_AGC_GAIN_SHIFT;
  /*
   * Map 0-255 to 16-235, 6.25% offset and 85.88% gain, approximately.
   */
  static const ULONG GDP_AGC_VIDEO_RANGE = (64 << GDP_AGC_CONSTANT_SHIFT) |
                                           (110<< GDP_AGC_GAIN_SHIFT);

  ULONG ulAlpha = 128; // default is fully opaque
  ULONG ulAGC;

  if (pBuffer->src.ulFlags & STM_PLANE_SRC_CONST_ALPHA)
  {
    // The input range is 0-255, this must be scaled to 0-128 for the hardware
    ulAlpha = ((pBuffer->src.ulConstAlpha + 1) >> 1) & 0xFF;
  }

  if (pBuffer->dst.ulFlags & STM_PLANE_DST_RESCALE_TO_VIDEO_RANGE)
    ulAGC = GDP_AGC_VIDEO_RANGE;
  else
    ulAGC = GDP_AGC_FULL_RANGE;

  ulAGC |= ulAlpha;

  // Note: We sort out ARGB1555 alpha ramp setup in the hardware update.
  topNode.GDPn_AGC = botNode.GDPn_AGC = ulAGC;

  return true;
}


bool CGammaCompositorGDP::SetControl(stm_plane_ctrl_t control, ULONG value)
{
  DEBUGF2(2,(FENTRY ": control = %d ulNewVal = %lu (0x%08lx)\n",__PRETTY_FUNCTION__,(int)control,value,value));

  switch(control)
  {
    case PLANE_CTRL_GAIN:
      if(value>255)
        return false;
      m_ulGain = value;
      break;

    case PLANE_CTRL_ALPHA_RAMP:
      m_ulAlphaRamp = value;
      m_ulStaticAlpha[0] = ((m_ulAlphaRamp & 0xff)+1)/2;
      m_ulStaticAlpha[1] = (((m_ulAlphaRamp>>8) & 0xff)+1)/2;
      break;

    case PLANE_CTRL_COLOR_KEY:
      {
	const stm_color_key_config_t * const config = reinterpret_cast<stm_color_key_config_t *> (value);
	if (!config)
	  return false;
	updateColorKeyState (&m_ColorKeyConfig, config);
      }
      break;

    default:
      return CGammaCompositorPlane::SetControl(control,value);
  }

  return true;
}


bool CGammaCompositorGDP::GetControl(stm_plane_ctrl_t control, ULONG *value) const
{
  DEBUGF2(2,(FENTRY ": control = %d\n",__PRETTY_FUNCTION__,(int)control));

  switch(control)
  {
    case PLANE_CTRL_GAIN:
      *value = m_ulGain;
      break;
    case PLANE_CTRL_ALPHA_RAMP:
      *value = m_ulAlphaRamp;
      break;

    case PLANE_CTRL_COLOR_KEY:
      {
	stm_color_key_config_t * const config = reinterpret_cast<stm_color_key_config_t *> (value);
	if (!config)
	  return false;
	config->flags = SCKCF_NONE;
	updateColorKeyState (config, &m_ColorKeyConfig);
      }
      break;

    default:
      return CGammaCompositorPlane::GetControl(control,value);
  }

  return true;
}


void CGammaCompositorGDP::EnableHW(void)
{
  DENTRY();

  if(!m_isEnabled && m_pOutput)
  {
    DEBUGF2(2,("CGammaCompositorGDP::EnableHW 'START GDP id = %d NVN = %#08x'\n",GetID(),m_Registers.ulPhysical));
    g_pIOS->WriteRegister((volatile ULONG*)(m_GDPBaseAddr+GDPn_NVN_OFFSET),m_Registers.ulPhysical);

    CDisplayPlane::EnableHW();
  }

  DEXIT();
}
