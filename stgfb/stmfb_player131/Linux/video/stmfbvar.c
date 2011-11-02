/***********************************************************************
 *
 * File: stgfb/Linux/video/stmfbvar.c
 * Copyright (c) 2007 STMicroelectronics Limited.
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <linux/version.h>
#include <linux/fb.h>
#include <asm/div64.h>

#include <stmdisplay.h>

#include "stmfb.h"
#include "stmfbinfo.h"


/*****************************************************************************
 * Processing of framebuffer var structures to ST core driver information
 */
#define DEBUG_VAR 0
#if DEBUG_VAR == 1

static void stmfb_display_var(const struct fb_var_screeninfo* var)
{
	printk( "stmfb: - struct fb_var_screeninfo var = %p ---------------------\n",var);
   
	printk( "stmfb: resolution: %ux%ux%u (virtual %ux%u+%u+%u)\n",
			var->xres, var->yres, var->bits_per_pixel,
			var->xres_virtual, var->yres_virtual,
			var->xoffset, var->yoffset);
            
	printk( "stmfb: color: %c%c R(%u,%u,%u), G(%u,%u,%u), B(%u,%u,%u), T(%u,%u,%u)\n",
			var->grayscale?'G':'C', var->nonstd?'N':'S',
			var->red.offset,    var->red.length,   var->red.msb_right,
			var->green.offset,  var->green.length, var->green.msb_right,
			var->blue.offset,   var->blue.length,  var->blue.msb_right,
			var->transp.offset, var->transp.length,var->transp.msb_right);
          
	printk( "stmfb: timings: %ups (%u,%u)-(%u,%u)+%u+%u\n",
		var->pixclock,
		var->left_margin, var->upper_margin, var->right_margin,
		var->lower_margin, var->hsync_len, var->vsync_len);
      
	printk(	"stmfb: activate %08x accel_flags %08x sync %08x vmode %08x\n",
		var->activate, var->accel_flags, var->sync, var->vmode);
     
	printk(	"stmfb: ----------------------------------------------------------------------\n");
}


static void stmfb_display_vm(const struct stmfb_videomode *vm)
{
  printk( "stmfb: mode (%p) %u x %u x %u bpp (virtual %u x %u+%u+%u)\n", vm, 
    (__u32)vm->ulxres, (__u32)vm->ulyres, (__u32)vm->uldepth,
    (__u32)vm->ulxvres, (__u32)vm->ulyvres, (__u32)vm->ulxoffset, (__u32)vm->ulyoffset);
  printk( "stmfb:             %uHz %ulines %upixels %s\n", 
    (__u32)vm->ulpixclock,(__u32)vm->ultotallines,(__u32)vm->ultotalpixels,
    (vm->scanType == SCAN_P)?"progressive":"interlaced");
}

#define STGFB_DISPLAY_VAR(a) stmfb_display_var(a)
#define STGFB_DISPLAY_VM(a) stmfb_display_vm(a)

#else

#define STGFB_DISPLAY_VAR(a)
#define STGFB_DISPLAY_VM(a)

#endif

typedef struct {
   SURF_FMT fmt;
   int      depth;
   int      aoffset;
   int      alength;
   int      roffset;
   int      rlength;
   int      goffset;
   int      glength;
   int      boffset;
   int      blength;
} COLOUR_DESC;


static COLOUR_DESC colour_formats[] = {
    { SURF_RGB565,   16,  0, 0, 11, 5, 5, 6, 0, 5},
    { SURF_ARGB1555, 16, 15, 1, 10, 5, 5, 5, 0, 5},
    { SURF_ARGB4444, 16, 12, 4,  8, 4, 4, 4, 0, 4},
    { SURF_RGB888,   24,  0, 0, 16, 8, 8, 8, 0, 8},
    { SURF_ARGB8565, 24, 16, 8, 11, 5, 5, 6, 0, 5},
    { SURF_ARGB8888, 32, 24, 8, 16, 8, 8, 8, 0, 8}
};


static const int nColourFormats = sizeof(colour_formats)/sizeof(COLOUR_DESC);

/*
 * stmfb_decode_var
 * Get the video params out of 'var' and construct a hw specific video
 * mode record.
 */
int stmfb_decode_var(struct fb_var_screeninfo *v, 
                     struct stmfb_videomode   *vm,
                     struct stmfb_info        *i)
{
  const SURF_FMT *formats;
  int nFormats;
  int c;
  u64 timingtmp;
  unsigned long outputStandards;
  
  if(i==0)
    return -ENODEV;

  STGFB_DISPLAY_VAR(v);

  /* Virtual resolution */
  vm->ulxvres   = v->xres_virtual;
  vm->ulyvres   = v->yres_virtual;
  vm->ulxoffset = v->xoffset;
  vm->ulyoffset = v->yoffset;

  /* Actual resolution */
  vm->ulxres = v->xres;
  vm->ulyres = v->yres;

  /*
   * We do not support panning in the X direction so the
   * real and virtual sizes must be the same
   */
  if((vm->ulxres != vm->ulxvres) || (vm->ulyres > vm->ulyvres))
    return -EINVAL;


  /*
   * Check that the panning offsets are usable
   */
  if(vm->ulxoffset != 0 || (vm->ulyoffset+vm->ulyres) > vm->ulyvres)
    return -EINVAL;

    
  vm->uldepth = v->bits_per_pixel;

  /*
   * search for the exact pixel format, only look at the bit length
   * of each component so we can use fbset -rgba to change mode
   */
  for(c=0;c<nColourFormats;c++)
  {
    if(v->transp.length != colour_formats[c].alength)
      continue;
    if(v->red.length    != colour_formats[c].rlength)
      continue;
    if(v->green.length  != colour_formats[c].glength)
      continue;
    if(v->blue.length   != colour_formats[c].blength)
      continue;
      
    /* The colour format matches so break out of the loop. */
    break;
  }

  if(c != nColourFormats && colour_formats[c].depth == vm->uldepth)
  {
    vm->pixformat = colour_formats[c].fmt;
  }
  else
  {
    /*
     * The vars bitdepth and rgba specification is not consistent.
     * Catching this allows us to just use fbset -depth to change quickly
     * between the colour depths without having to specify -rgba as well or
     * to use fb.modes settings which only pass the bitdepth (colour lengths
     * come in as 0/0/0/0 in that case)
     */
    switch(vm->uldepth)
    {
      case 8:
        vm->pixformat = SURF_CLUT8;
        break;
      case 16:
        vm->pixformat = SURF_RGB565;
        break;
      case 24:
        vm->pixformat = SURF_RGB888;
        break;
      case 32:
        vm->pixformat = SURF_ARGB8888;
        break;
      default:
        return -EINVAL;
    }
  }

  /*
   * Now check that the surface format is actually supported by the hardware.
   */

  nFormats = stm_display_plane_get_image_formats(i->pFBPlane, &formats);
  if(signal_pending(current))
    return -ERESTARTSYS;
    
  for(c=0;c<nFormats;c++)
  {
    if(formats[c] == vm->pixformat)
      break;
  }
  
  if(c == nFormats)
    return -EINVAL;
  
  /*
   * Potential problem with this - if we have to pad the display
   * for some reason, this will be incorrect.
   */
  vm->ulstride  = vm->ulxres*(vm->uldepth>>3);

  /*
   * Check there is enough video memory to support the virtual
   * screen size. If the video memory hasn't been set yet, we are trying
   * to find a default mode from the module parameters line, so ignore this
   * test.
   */
  if(i->ulFBSize != 0 && (vm->ulyvres * vm->ulstride) > i->ulFBSize)
  {
    DPRINTK("Insufficient memory for requested mode: %lu available %lu requested\n",i->ulFBSize,(vm->ulyvres * vm->ulstride));
    DPRINTK("                                      : vyres = %lu xres = %lu stride = %lu\n",vm->ulyvres,vm->ulxres,vm->ulstride);
    return -EINVAL;
  }

  switch (v->vmode & FB_VMODE_MASK)
  {
    case FB_VMODE_NONINTERLACED:
        vm->scanType = SCAN_P;
        break;
    case FB_VMODE_INTERLACED:
        vm->scanType = SCAN_I;
        break;
    case FB_VMODE_DOUBLE:
    default:
        DPRINTK("Unsupported FB VMODE flags: %lu\n",(unsigned long)v->vmode);
        return -EINVAL;
  }

  /* Convert pixelclock from picoseconds to Hz
   * Note that on the sh4 we can do an unsigned 64bit/32bit 
   * divide.
   */
  timingtmp = 1000000000000ULL;
  do_div(timingtmp,v->pixclock);
  vm->ulpixclock = (unsigned long)timingtmp;

  vm->ultotallines  = v->yres + v->upper_margin + v->lower_margin + v->vsync_len;
  vm->ultotalpixels = v->xres + v->left_margin + v->right_margin + v->hsync_len;

  vm->pModeLine = stm_display_output_find_display_mode(i->pFBMainOutput,
                                                       vm->ulxres,
                                                       vm->ulyres,
                                                       vm->ultotallines,
                                                       vm->ultotalpixels,
                                                       vm->ulpixclock,
                                                       vm->scanType);
  if(signal_pending(current))
    return -ERESTARTSYS;

  if(!vm->pModeLine)
  {
#if DEBUG_VAR == 1
    DPRINTK("Unable to find video mode from the following descriptor\n");
    STGFB_DISPLAY_VM(vm);
#endif
    return -EINVAL;
  }

  /*
   * We have to make a sensible decision about the TV standard to use
   * if the timing mode can support more than one. The Linux framebuffer
   * setup gives us no help with this. If the configured or default encoding
   * isn't supported on the mode, fall back to the most sensible default.
   */
  outputStandards = vm->pModeLine->ModeParams.OutputStandards;


  if(outputStandards & i->main_config.sdtv_encoding)
    vm->ulTVStandard = i->main_config.sdtv_encoding;
  else if(outputStandards & i->default_sd_encoding)
    i->main_config.sdtv_encoding = vm->ulTVStandard = i->default_sd_encoding;
  else if(outputStandards & STM_OUTPUT_STD_NTSC_M) 
    i->main_config.sdtv_encoding = vm->ulTVStandard = STM_OUTPUT_STD_NTSC_M;
  else if(outputStandards & STM_OUTPUT_STD_PAL_BDGHI)
    i->main_config.sdtv_encoding = vm->ulTVStandard = STM_OUTPUT_STD_PAL_BDGHI;
  else if((outputStandards & STM_OUTPUT_STD_HD_MASK) != 0)
    vm->ulTVStandard = (outputStandards & STM_OUTPUT_STD_HD_MASK);
  else if(outputStandards & STM_OUTPUT_STD_VESA)
    vm->ulTVStandard = STM_OUTPUT_STD_VESA;

  /*
   * Finally add in SMPTE293M for ED progressive modes.
   * 
   * This is additive to the normal SD standard to support chips such as
   * the STm8000 which can show progressive modes automatically re-interlaced
   * back to an interlaced TV standard. In which case we need to know both
   * that it is SMPTE293M and which standard to use for the normal TV output.
   */
  if(outputStandards & STM_OUTPUT_STD_SMPTE293M)
    vm->ulTVStandard |= STM_OUTPUT_STD_SMPTE293M;

  STGFB_DISPLAY_VM(vm);

  return 0;
}


/*
 *  stmfb_encode_var
 *  Fill a 'var' structure based on the values in 'vm' and maybe other
 *  values read out of the hardware. This is the reverse of stmfb_decode_var
 */
int stmfb_encode_var(struct fb_var_screeninfo *v,
                     struct stmfb_videomode   *vm,
                     struct stmfb_info        *i)
{
  int c;
  u64 timingtmp;

  STGFB_DISPLAY_VM(vm);

  // Clear structure
  memset(v, 0, sizeof(struct fb_var_screeninfo));

  // Virtual resolution
  v->xres_virtual=vm->ulxvres;
  v->yres_virtual=vm->ulyvres;
  v->xoffset = 0;
  v->yoffset = vm->ulyoffset;

  // Actual resolution
  v->xres=vm->ulxres;
  v->yres=vm->ulyres;

  //pixel depth
  v->bits_per_pixel=vm->uldepth;

  for(c=0;c < nColourFormats;c++)
  {
    if(colour_formats[c].fmt == vm->pixformat)
    {
      v->transp.offset = colour_formats[c].aoffset;
      v->transp.length = colour_formats[c].alength;
      v->red.offset    = colour_formats[c].roffset;
      v->red.length    = colour_formats[c].rlength;
      v->green.offset  = colour_formats[c].goffset;
      v->green.length  = colour_formats[c].glength;
      v->blue.offset   = colour_formats[c].boffset;
      v->blue.length   = colour_formats[c].blength;
    }
  }
  

  /* Height/Width of picture in mm */
  v->height=v->width=-1;

  /* Convert pixel clock (Hz) to pico seconds, correctly rounding around 1/2 */
  timingtmp = 1000000000000ULL+(vm->pModeLine->TimingParams.ulPixelClock/2);
  do_div(timingtmp,vm->pModeLine->TimingParams.ulPixelClock);
  v->pixclock= (unsigned long)timingtmp;

  /*
   * The timing parameters for the ST VTG don't quite match up to the linux
   * values, so we have to cheat a bit. The actual start of the active video
   * is left_margin+hsync_len and upper_margin+vsync_len.
   */
  v->hsync_len    = vm->pModeLine->TimingParams.HSyncPulseWidth;
  v->left_margin  = vm->pModeLine->ModeParams.ActiveAreaXStart - v->hsync_len;
  v->right_margin = vm->pModeLine->TimingParams.PixelsPerLine -
                    vm->pModeLine->ModeParams.ActiveAreaXStart -
                    vm->pModeLine->ModeParams.ActiveAreaWidth;

  v->vsync_len    = vm->pModeLine->TimingParams.VSyncPulseWidth;
  v->upper_margin = vm->pModeLine->ModeParams.FullVBIHeight - v->vsync_len;
  v->lower_margin = vm->pModeLine->TimingParams.LinesByFrame -
                    vm->pModeLine->ModeParams.FullVBIHeight -
                    vm->pModeLine->ModeParams.ActiveAreaHeight;


  if (vm->pModeLine->TimingParams.HSyncPolarity)
    v->sync|=FB_SYNC_HOR_HIGH_ACT;

  if (vm->pModeLine->TimingParams.VSyncPolarity)
    v->sync|=FB_SYNC_VERT_HIGH_ACT;

  v->vmode = (vm->pModeLine->ModeParams.ScanType == SCAN_I)?FB_VMODE_INTERLACED:FB_VMODE_NONINTERLACED;

  STGFB_DISPLAY_VAR(v);

  return 0;
}


/******************************************************************************
 * Extended var information to control ST specific plane properties
 */
int stmfb_queuebuffer(struct stmfb_info* i)
{
  spin_lock_irq(&(i->framebufferSpinLock));
  i->num_outstanding_updates++;
  spin_unlock_irq(&(i->framebufferSpinLock));
        
  if(stm_display_plane_queue_buffer(i->pFBPlane,&i->current_buffer_setup)<0)
  {
    spin_lock_irq(&(i->framebufferSpinLock));
    i->num_outstanding_updates--;
    spin_unlock_irq(&(i->framebufferSpinLock));

    if(signal_pending(current))
      return -ERESTARTSYS;
    else
      return -EINVAL;
  }

  return 0;
}


static int stmfb_decode_var_ex(struct stmfbio_var_screeninfo_ex *v, stm_display_buffer_t *b, struct stmfb_info *i)
{
  stm_display_plane_t *plane;
  int layerid = v->layerid;

  /*
   * Currently only support the framebuffer plane, but are leaving open the
   * possibility of accessing other planes in the future.
   */
  if(layerid != 0 || v->activate == STMFBIO_ACTIVATE_TEST)
    return -EINVAL;

  plane = i->pFBPlane;
  
  v->failed = 0;

  if(v->caps & STMFBIO_VAR_CAPS_COLOURKEY)
  {
    b->src.ColorKey.flags = SCKCF_ENABLE;
    if (v->colourKeyFlags & STMFBIO_COLOURKEY_FLAGS_ENABLE)
    {
      b->src.ColorKey.enable = 1;

      b->src.ColorKey.flags  |= SCKCF_FORMAT;
      b->src.ColorKey.format  = SCKCVF_RGB;

      b->src.ColorKey.flags  |= (SCKCF_MINVAL | SCKCF_MAXVAL);
      b->src.ColorKey.minval  = v->min_colour_key;
      b->src.ColorKey.minval  = v->max_colour_key;

      b->src.ColorKey.flags |= (SCKCF_R_INFO | SCKCF_G_INFO | SCKCF_B_INFO);
      if(v->colourKeyFlags & STMFBIO_COLOURKEY_FLAGS_INVERT)
      {
	DPRINTK("Inverse colour key.\n");
	b->src.ColorKey.r_info  = SCKCCM_INVERSE;
	b->src.ColorKey.g_info  = SCKCCM_INVERSE;
	b->src.ColorKey.b_info  = SCKCCM_INVERSE;
      }
      else
      {
	DPRINTK("Normal colour key.\n");
	b->src.ColorKey.r_info  = SCKCCM_ENABLED;
	b->src.ColorKey.g_info  = SCKCCM_ENABLED;
	b->src.ColorKey.b_info  = SCKCCM_ENABLED;
      }
    }
    else
    {
      b->src.ColorKey.enable = 0;
    }

    i->current_var_ex.min_colour_key = v->min_colour_key;
    i->current_var_ex.max_colour_key = v->max_colour_key;
    i->current_var_ex.colourKeyFlags = v->colourKeyFlags;
  }
  else
    b->src.ColorKey.flags = SCKCF_NONE;

  if(v->caps & STMFBIO_VAR_CAPS_FLICKER_FILTER)
  {
    if((i->info.var.bits_per_pixel == 8) && (v->ff_state != STMFBIO_FF_OFF))
      v->failed |= STMFBIO_VAR_CAPS_FLICKER_FILTER;
 
    switch(v->ff_state)
    {
      case STMFBIO_FF_OFF:
        DPRINTK("Flicker Filter Off.\n");
        b->dst.ulFlags &= ~(STM_PLANE_DST_FLICKER_FILTER | STM_PLANE_DST_ADAPTIVE_FF);
        break;
      case STMFBIO_FF_SIMPLE:
        DPRINTK("Flicker Filter Simple.\n");
        b->dst.ulFlags |= STM_PLANE_DST_FLICKER_FILTER;
        break;
      case STMFBIO_FF_ADAPTIVE:
        DPRINTK("Flicker Filter Adaptive.\n");
        b->dst.ulFlags |= (STM_PLANE_DST_FLICKER_FILTER | STM_PLANE_DST_ADAPTIVE_FF);
        break;
      default:
        DPRINTK("Flicker Filter state invalid.\n");
        v->failed |= STMFBIO_VAR_CAPS_FLICKER_FILTER;
        break;
    }
    
    if(!(v->failed & STMFBIO_VAR_CAPS_FLICKER_FILTER))
      i->current_var_ex.ff_state = v->ff_state;
  }

  if(v->caps & STMFBIO_VAR_CAPS_PREMULTIPLIED)
  {
    if(v->premultiplied_alpha)
    {
      DPRINTK("Premultiplied Alpha.\n");
      b->src.ulFlags |= STM_PLANE_SRC_PREMULTIPLIED_ALPHA;
    }
    else
    {
      DPRINTK("Non-Premultiplied Alpha.\n");
      b->src.ulFlags &= ~STM_PLANE_SRC_PREMULTIPLIED_ALPHA;
    }

    i->current_var_ex.premultiplied_alpha = v->premultiplied_alpha;
  }


  if(v->caps & STBFBIO_VAR_CAPS_RESCALE_COLOUR_TO_VIDEO_RANGE)
  {
    if(v->rescale_colour_to_video_range)
    {
      DPRINTK("Rescale colour to video output range.\n");
      b->dst.ulFlags |= STM_PLANE_DST_RESCALE_TO_VIDEO_RANGE;
    }
    else
    {
      DPRINTK("Full colour output range.\n");
      b->dst.ulFlags &= ~STM_PLANE_DST_RESCALE_TO_VIDEO_RANGE;
    }
    
    i->current_var_ex.rescale_colour_to_video_range = v->rescale_colour_to_video_range;
  }


  if(v->caps & STMFBIO_VAR_CAPS_OPACITY)
  {
    if (v->opacity <255)
    {
      b->src.ulFlags     |= STM_PLANE_SRC_CONST_ALPHA;
      b->src.ulConstAlpha = v->opacity;
    }
    else
    {
      b->src.ulFlags &= ~STM_PLANE_SRC_CONST_ALPHA;
    }

    DPRINTK("Opacity = %d.\n",v->opacity);
    
    i->current_var_ex.opacity = v->opacity;
  }

  if(v->caps & STMFBIO_VAR_CAPS_GAIN)
  {
    if(v->activate == STMFBIO_ACTIVATE_IMMEDIATE)
    {
      if(stm_display_plane_set_control(plane, PLANE_CTRL_GAIN, v->gain)<0)
        v->failed |= STMFBIO_VAR_CAPS_GAIN;
    }
    
    if(!(v->failed & STMFBIO_VAR_CAPS_GAIN))
    {
      DPRINTK("Gain = %d.\n",v->gain);
      i->current_var_ex.gain = v->gain;
    }
  }

  if(v->caps & STMFBIO_VAR_CAPS_BRIGHTNESS)
  {
    if(v->activate == STMFBIO_ACTIVATE_IMMEDIATE)
    {
      if(stm_display_plane_set_control(plane, PLANE_CTRL_PSI_BRIGHTNESS, v->brightness)<0)
        v->failed |= STMFBIO_VAR_CAPS_BRIGHTNESS;
    }

    if(!(v->failed & STMFBIO_VAR_CAPS_BRIGHTNESS))
    {
      DPRINTK("Brightness = %d.\n",v->brightness);
      i->current_var_ex.brightness = v->brightness;
    }
  }

  if(v->caps & STMFBIO_VAR_CAPS_SATURATION)
  {
    if(v->activate == STMFBIO_ACTIVATE_IMMEDIATE)
    {
      if(stm_display_plane_set_control(plane, PLANE_CTRL_PSI_SATURATION, v->saturation)<0)
        v->failed |= STMFBIO_VAR_CAPS_SATURATION;
    }

    if(!(v->failed & STMFBIO_VAR_CAPS_SATURATION))
    {
      DPRINTK("Saturation = %d.\n",v->saturation);
      i->current_var_ex.saturation = v->saturation;
    }
  }

  if(v->caps & STMFBIO_VAR_CAPS_CONTRAST)
  {
    if(v->activate == STMFBIO_ACTIVATE_IMMEDIATE)
    {
      if(stm_display_plane_set_control(plane, PLANE_CTRL_PSI_CONTRAST, v->contrast)<0)
        v->failed |= STMFBIO_VAR_CAPS_CONTRAST;
    }

    if(!(v->failed & STMFBIO_VAR_CAPS_CONTRAST))
    {
      DPRINTK("Contrast = %d.\n",v->contrast);
      i->current_var_ex.contrast = v->contrast;
    }
  }

  if(v->caps & STMFBIO_VAR_CAPS_TINT)
  {
    if(v->activate == STMFBIO_ACTIVATE_IMMEDIATE)
    {
      if(stm_display_plane_set_control(plane, PLANE_CTRL_PSI_TINT, v->tint)<0)
        v->failed |= STMFBIO_VAR_CAPS_TINT;
    }

    if(!(v->failed & STMFBIO_VAR_CAPS_TINT))
    {
      DPRINTK("Tint = %d.\n",v->tint);
      i->current_var_ex.tint = v->tint;
    }
  }

  if(v->caps & STMFBIO_VAR_CAPS_ALPHA_RAMP)
  {
    if(v->activate == STMFBIO_ACTIVATE_IMMEDIATE)
    {
      if(stm_display_plane_set_control(plane, PLANE_CTRL_ALPHA_RAMP, v->alpha_ramp[0] | (v->alpha_ramp[1]<<8))<0)
        v->failed |= STMFBIO_VAR_CAPS_ALPHA_RAMP;
    }

    if(!(v->failed & STMFBIO_VAR_CAPS_ALPHA_RAMP))
    {
      DPRINTK("Alpha Ramp = [%d,%d].\n",v->alpha_ramp[0],v->alpha_ramp[1]);
      i->current_var_ex.alpha_ramp[0] = v->alpha_ramp[0];
      i->current_var_ex.alpha_ramp[1] = v->alpha_ramp[1];
    }
  }

  if(v->caps & STMFBIO_VAR_CAPS_ZPOSITION)
  {
    if(v->activate == STMFBIO_ACTIVATE_IMMEDIATE)
    {
      if(stm_display_plane_set_depth(plane, i->pFBMainOutput, v->z_position, 1)<0)
      {
        DPRINTK("FAILED: Set Z Position = %d.\n",v->z_position);
        v->failed |= STMFBIO_VAR_CAPS_ZPOSITION;
      }
      else
      {
        if(stm_display_plane_get_depth(plane, i->pFBMainOutput, &i->current_var_ex.z_position)<0)
        {
          DPRINTK("FAILED: Get Z Position\n");
          v->failed |= STMFBIO_VAR_CAPS_ZPOSITION;
        }
        else
        {
          DPRINTK("Real Z Position = %d.\n",i->current_var_ex.z_position);
        }
      }
    }
    else
    {
      DPRINTK("Z Position = %d.\n",v->z_position);
      i->current_var_ex.z_position = v->z_position;
    }
  }

  if(v->failed != 0)
  {
    if(signal_pending(current))
      return -EINTR;
    else
      return -EINVAL;
  }

  return 0;
}

#ifdef __TDT__
EXPORT_SYMBOL(stmfb_set_var_ex); 
#endif
int stmfb_set_var_ex(struct stmfbio_var_screeninfo_ex *v, struct stmfb_info *i)
{
  int ret;
  stm_display_plane_t *plane;

  int layerid = v->layerid;
  /*
   * Currently only support the framebuffer plane, but are leaving open the
   * possibility of accessing other planes in the future.
   */
  if(layerid != 0)
    return -EINVAL;

  plane = i->pFBPlane;

  v->failed = v->caps & ~i->current_var_ex.caps; 
  if(v->failed != 0)
    return -EINVAL;

  if(v->activate == STMFBIO_ACTIVATE_TEST)
  {
    /*
     * Test is if a Z position change is valid.
     */
    if(v->caps & STMFBIO_VAR_CAPS_ZPOSITION)
    {
      ret = stm_display_plane_set_depth(plane, i->pFBMainOutput, v->z_position, 0);
      if(ret<0)
      {
        v->failed |= STMFBIO_VAR_CAPS_ZPOSITION;
        return ret;
      }
    }

    /*
     * Check that flicker filtering is not being switched on when the
     * framebuffer is in 8bit CLUT mode, flicker filtering is only usable
     * in true RGB modes.
     */
    if(v->caps & STMFBIO_VAR_CAPS_FLICKER_FILTER)
    {
      if((i->info.var.bits_per_pixel == 8) && (v->ff_state != STMFBIO_FF_OFF))
      {
        DPRINTK("Cannot enable flicker filter in 8bit modes\n");
      	v->failed |= STMFBIO_VAR_CAPS_FLICKER_FILTER;
      	return -EINVAL;
      }
    }

  }
  else
  {
    if((ret = stmfb_decode_var_ex(v, &i->current_buffer_setup, i))<0)
      return ret;

    if(v->activate == STMFBIO_ACTIVATE_IMMEDIATE)
    {
      ret = stmfb_queuebuffer(i);
      if(ret<0)
      {
        v->failed |= v->caps & (STMFBIO_VAR_CAPS_COLOURKEY                     |
                                STMFBIO_VAR_CAPS_FLICKER_FILTER                |
                                STMFBIO_VAR_CAPS_PREMULTIPLIED                 |
                                STBFBIO_VAR_CAPS_RESCALE_COLOUR_TO_VIDEO_RANGE |
                                STMFBIO_VAR_CAPS_OPACITY);
        return ret;
      }
      else
      {
      	/*
      	 * Don't let too many updates back up on the plane, but on the other
      	 * hand don't block by default.
      	 */
        wait_event(i->framebuffer_updated_wait_queue, i->num_outstanding_updates < 4);
      }
    }
  }

  return 0;
}


int stmfb_encode_var_ex(struct stmfbio_var_screeninfo_ex *v, struct stmfb_info *i)
{
  stm_plane_caps_t caps;
  stm_display_plane_t *plane;

  int layerid = v->layerid;

  /*
   * Currently only support the framebuffer plane, but are leaving open the
   * possibility of accessing other planes in the future.
   */
  if(layerid != 0)
    return -EINVAL;

  plane = i->pFBPlane;

  memset(v, 0, sizeof(struct stmfbio_var_screeninfo_ex));
  v->layerid = layerid;

  if(stm_display_plane_get_capabilities(plane,&caps)<0)
    return -ERESTARTSYS;

  DPRINTK("Plane caps = 0x%08x\n",(unsigned)caps.ulCaps);

  if(caps.ulCaps & PLANE_CAPS_SRC_COLOR_KEY)
  {
    DPRINTK("Plane has Color Keying\n");
    v->caps |= STMFBIO_VAR_CAPS_COLOURKEY;
    v->colourKeyFlags = 0;
  }

  if(caps.ulCaps & PLANE_CAPS_FLICKER_FILTER)
  {
    DPRINTK("Plane has Flicker Filter\n");
    v->caps |= STMFBIO_VAR_CAPS_FLICKER_FILTER;
    v->ff_state = STMFBIO_FF_OFF;
  }

  if(caps.ulCaps & PLANE_CAPS_PREMULTIPLED_ALPHA)
  {
    DPRINTK("Plane supports premultipled alpha\n");
    v->caps |= STMFBIO_VAR_CAPS_PREMULTIPLIED;
#ifdef __TDT__
    v->premultiplied_alpha = 0; //Set nonpremulitplied as default as on dreambox
#else
    v->premultiplied_alpha = 1;
#endif
  }

  if(caps.ulCaps & PLANE_CAPS_RESCALE_TO_VIDEO_RANGE)
  {
    DPRINTK("Plane can rescale to video range\n");
    v->caps |= STBFBIO_VAR_CAPS_RESCALE_COLOUR_TO_VIDEO_RANGE;
    v->rescale_colour_to_video_range = 0;
  }

  if(caps.ulCaps & PLANE_CAPS_GLOBAL_ALPHA)
  {
    DPRINTK("Plane has global alpha\n");
    v->caps |= STMFBIO_VAR_CAPS_OPACITY;
    v->opacity = 255; /* fully opaque */
  }

  if(caps.ulControls & PLANE_CTRL_CAPS_GAIN)
  {
    DPRINTK("Plane has gain control\n");
    v->caps |= STMFBIO_VAR_CAPS_GAIN;
    v->gain = 255; /* 100% */
  }

  if(caps.ulControls & PLANE_CTRL_CAPS_PSI_CONTROLS)
  {
    DPRINTK("Plane has PSI control\n");
    v->caps |= STMFBIO_VAR_CAPS_BRIGHTNESS | 
               STMFBIO_VAR_CAPS_SATURATION |
               STMFBIO_VAR_CAPS_CONTRAST   |
               STMFBIO_VAR_CAPS_TINT;
               
    v->brightness = 128;
    v->saturation = 128;
    v->contrast   = 128;
    v->tint       = 128;
  }

  if(caps.ulControls & PLANE_CTRL_CAPS_ALPHA_RAMP)
  {
    v->caps |= STMFBIO_VAR_CAPS_ALPHA_RAMP;
    v->alpha_ramp[0] = 0;
    v->alpha_ramp[1] = 255;
  }

  v->caps |= STMFBIO_VAR_CAPS_ZPOSITION;
  stm_display_plane_get_depth(plane, i->pFBMainOutput, &v->z_position);
   
  return 0;
}

