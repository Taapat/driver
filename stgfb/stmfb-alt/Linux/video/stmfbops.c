/***********************************************************************
 *
 * File: stgfb/Linux/video/stmfbops.c
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pm.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include <stmdisplay.h>
#include <linux/stm/stmcoredisplay.h>

#include "stmfb.h"
#include "stmfbinfo.h"

#define DEBUG_CLUT 0

/* ------------------- chipset specific functions -------------------------- */
/*
 * stmfb_check_var
 * determine if the video mode is valid, possibly altering the
 * incoming var to the closest match.
 */
static int stmfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
  struct stmfb_info*     i = (struct stmfb_info*)info;
  struct stmfb_videomode vm;
  int ret;
  __u32 saved_activate;

  DPRINTK("in current process/pid: %p/%d, %u(+%u)x%u@%u\n",
          current,current->pid,
          var->xres,var->right_margin,var->yres,var->pixclock);

  /*
   * Get hardware description of videomode to set
   */
  if((ret = stmfb_decode_var(var, &vm, i)) < 0)
  {
    DPRINTK("failed\n");
    return ret;
  }

  /*
   * Re-create the VAR from the exact hardware description, this gives a
   * completely clean var, which is why we have to save and restore the
   * activate flags. Without this we get spurious mode changes when they
   * should have been just tests.
   */
  saved_activate = var->activate;
  stmfb_encode_var(var, &vm, i);
  var->activate = saved_activate;

  DPRINTK("finished OK\n");

  return 0;
}


/*
 * Callback to indicate when a new framebuffer has made it onto the display
 * Currently only used to wait for the display to "pan".
 */
static void stmfb_frame_displayed(void *pData, TIME64 vsyncTime)
{
  struct stmfb_info* i = (struct stmfb_info*)pData;

#if defined(DEBUG_UPDATES)
  DPRINTK("in i = %p outstanding updates = %d\n",i,i->num_outstanding_updates);
#endif

  i->num_outstanding_updates--;
  wake_up(&i->framebuffer_updated_wait_queue);
}


/*
 * Helper to pan the framebuffer, this is called with the driver lock
 * already taken from both the framebuffer pan display and set mode
 * entrypoints.
 */
static int stmfb_do_pan_display(struct fb_var_screeninfo* var, struct stmfb_info* i)
{
  int ret;
#if defined(DEBUG_UPDATES)
  DPRINTK("in i = %p xoffset = %d yoffset = %d\n",i,var->xoffset,var->yoffset);
#endif

  if(var->xoffset != 0 || (var->yoffset+i->current_videomode.ulyres > i->current_videomode.ulyvres))
  {
    DPRINTK("requested offsets invalid xoffset = %d yoffset = %d\n",var->xoffset,var->yoffset);
    return -EINVAL;
  }

  if(var->yoffset == i->current_videomode.ulyoffset)
  {
#if defined(DEBUG_UPDATES)
    DPRINTK("Already panned to required position, doing nothing\n");
#endif
    return 0;
  }

  /*
   * Only actually do the pan if we are powered up, otherwise just return
   * success. When the device is resumed the correct pan will be set.
   */
  if(i->platformDevice->dev.power.power_state.event != PM_EVENT_ON)
  {
    DPRINTK("Device suspended, doing nothing\n");
    return 0;
  }

  i->current_buffer_setup.src.ulVideoBufferAddr = i->ulPFBBase + (var->yoffset * i->current_videomode.ulstride);

  i->current_buffer_setup.info.pDisplayCallback = stmfb_frame_displayed;
  i->current_buffer_setup.info.pUserData        = i;

  if((ret = stmfb_queuebuffer(i))<0)
    return ret;

  i->current_videomode.ulyoffset = var->yoffset;

  if(var->activate & FB_ACTIVATE_VBL)
    wait_event(i->framebuffer_updated_wait_queue, i->num_outstanding_updates < 2);
  else
    wait_event(i->framebuffer_updated_wait_queue, (i->num_outstanding_updates == 0));

  return 0;
}


/*
 * stmfb_pan_display
 * Pan (or wrap, depending on the `vmode' field) the display using the
 * `xoffset' and `yoffset' fields of the `var' structure.
 * If the values don't fit, return -EINVAL.
 */
static int stmfb_pan_display(struct fb_var_screeninfo* var, struct fb_info* info)
{
  struct stmfb_info* i = (struct stmfb_info* )info;
  int ret;

  if(down_interruptible(&i->framebufferLock))
    return -ERESTARTSYS;

  if(!i->current_videomode_valid)
  {
    DPRINTK("video mode not valid\n");
    up(&i->framebufferLock);
    return -ENODEV;
  }

  ret = stmfb_do_pan_display(var,i);

  up(&i->framebufferLock);

  return ret;
}


/*
 * stmfb_coldstart_display
 * Start the display running, where we know it is currently stopped.
 */
static int stmfb_coldstart_display(struct stmfb_info *i,
                                   struct stmfb_videomode *vm)
{
  int ret = 0;

  DPRINTK ("\n");

  if(i->main_config.activate == STMFBIO_ACTIVATE_ON_NEXT_CHANGE)
  {
    i->main_config.activate = STMFBIO_ACTIVATE_IMMEDIATE;
    ret = stmfb_set_output_configuration(&i->main_config, i);
    if(ret != 0)
    {
      DPRINTK("Setting main output configuration failed\n");
    }
  }

  ret += stm_display_output_start(i->pFBMainOutput, vm->pModeLine, vm->ulTVStandard);

  if(ret == 0)
  {
    DPRINTK("Start Main video output successful\n");
  }

  if(i->pFBAuxOutput)
  {
    if(i->aux_config.activate == STMFBIO_ACTIVATE_ON_NEXT_CHANGE)
    {
      i->aux_config.activate = STMFBIO_ACTIVATE_IMMEDIATE;
      ret = stmfb_set_output_configuration(&i->aux_config, i);
      if(ret != 0)
      {
        DPRINTK("Setting aux output configuration failed\n");
      }
    }

    ret += stm_display_output_start(i->pFBAuxOutput, vm->pModeLine, vm->ulTVStandard);

    if(ret == 0)
    {
      DPRINTK("Start Aux video output successful\n");
    }
  }

  if(i->pFBDVO && !(i->main_config.dvo_config & STMFBIO_OUTPUT_DVO_DISABLED))
  {
    ret += stm_display_output_start(i->pFBDVO, vm->pModeLine, vm->ulTVStandard);

    if(ret == 0)
    {
      DPRINTK("Start DVO successful\n");
    }
  }

  if(ret < 0)
  {
    DPRINTK("Failed\n");
    if(signal_pending(current))
      return -ERESTARTSYS;
    else
      return -EIO;
  }

  DPRINTK("Initialised display output\n");
  return 0;
}


/*
 * stmfb_stop_display
 * Completely stop the display output
 */
static int stmfb_stop_display(struct stmfb_info *i)
{
  unsigned long saveFlags;

  DPRINTK("Flushing FB plane\n");

  wait_event(i->framebuffer_updated_wait_queue, (i->num_outstanding_updates == 0));

  if(stm_display_plane_flush(i->pFBPlane)<0)
    return -ERESTARTSYS;

  if(stm_display_output_stop(i->pFBMainOutput)<0)
  {
    if(!signal_pending(current) && i->current_videomode_valid)
    {
      /*
       * We couldn't stop the main output because some other plane was
       * active on the display (e.g. an open, streaming, V4L2 output).
       * However we flushed the framebuffer plane in preparation to stop
       * the output. In order that the system isn't left in an inconsistent
       * state, reinstate the previous framebuffer onto the display.
       */
      stmfb_queuebuffer(i);
    }

    /*
     * Test signal pending state again, in case a signal came in and
     * disturbed the queue buffer call. Remember this is still an
     * error path which needs to return -EBUSY unless a signal is pending.
     */
    if(signal_pending(current))
      return -ERESTARTSYS;
    else
      return -EBUSY;
  }

  /*
   * Invalidate the current video mode and reset the HDMI hotplug
   * status (note this is done under interrupt lock, although there
   * shouldn't be any VTG interrupts at this point because we have stopped
   * the outputs). This forces HDMI to re-start on a mode change as the
   * Vsync handling will think there has been a new hotplug event if the
   * TV is actually connected.
   */
  spin_lock_irqsave(&(i->framebufferSpinLock), saveFlags);

  i->current_videomode_valid = 0;
  i->info.mode = NULL;
  if(i->hdmi)
  {
    i->hdmi->status_changed = 0;
    i->hdmi->status         = STM_DISPLAY_DISCONNECTED;
  }

  spin_unlock_irqrestore(&(i->framebufferSpinLock), saveFlags);

  return 0;
}

/*
 * stmfb_restart_display
 * helper for stmfb_set_videomode
 */
static int stmfb_restart_display(struct stmfb_info *i,
                                 struct stmfb_videomode *vm)
{
  int ret;
  DPRINTK("Doing display restart\n");

  ret = stmfb_stop_display(i);
  if(ret<0)
    return ret;

  ret = stmfb_coldstart_display(i,vm);
  if(ret<0)
    return ret;

  return 0;
}


static int stmfb_queue_new_fb_configuration(struct stmfb_info *i,
                                            struct stmfb_videomode *vm)
{
  int ret;
  DPRINTK("Setting new framebuffer configuration\n");

  /*
   * Queue a new buffer configuration on the framebuffer display plane.
   */
  memset(&i->current_buffer_setup, 0, sizeof(stm_display_buffer_t));

  i->current_buffer_setup.src.ulVideoBufferAddr = i->ulPFBBase + (vm->ulyoffset * vm->ulstride);
  i->current_buffer_setup.src.ulVideoBufferSize = vm->ulstride * vm->ulyres;
  i->current_buffer_setup.src.ulClutBusAddress  = i->dmaFBCLUT;

  i->current_buffer_setup.src.ulStride     = vm->ulstride;
  i->current_buffer_setup.src.ulPixelDepth = vm->uldepth;
  i->current_buffer_setup.src.ulColorFmt   = vm->pixformat;
  i->current_buffer_setup.src.Rect.width   = vm->ulxres;
  i->current_buffer_setup.src.Rect.height  = vm->ulyres;

  i->current_buffer_setup.dst.Rect.width   = vm->ulxres;
  i->current_buffer_setup.dst.Rect.height  = vm->ulyres;

  /*
   * The persistent flag keeps this buffer on the display until
   * another buffer is queued or the plane is released.
   *
   * The presentation as graphics changes the behaviour when the display is
   * interlaced; it will only update the display on top fields and therefore
   * will be visible on a matched top/bottom field pair.
   */
  i->current_buffer_setup.info.ulFlags          = (STM_PLANE_PRESENTATION_PERSISTENT |
                                                   STM_PLANE_PRESENTATION_GRAPHICS);
  i->current_buffer_setup.info.pDisplayCallback = stmfb_frame_displayed;
  i->current_buffer_setup.info.pUserData        = i;

  i->current_var_ex.activate = STMFBIO_ACTIVATE_IMMEDIATE;

  /*
   * Override the flicker filter state for 8bit modes, we need to force the
   * filter off.
   */
  if(vm->uldepth == 8)
    i->current_var_ex.ff_state = STMFBIO_FF_OFF;

  if((ret = stmfb_set_var_ex(&i->current_var_ex, i))<0)
    return ret;


  /* Wait for the flip to happen */
  wait_event(i->framebuffer_updated_wait_queue, (i->num_outstanding_updates == 0));

  /*
   * Create a new drawing surface (note this has the virtual Y size)
   */
  i->fbSurface.ulMemory = i->ulPFBBase;
  i->fbSurface.ulSize   = i->ulFBSize;
  i->fbSurface.ulWidth  = vm->ulxres;
  i->fbSurface.ulHeight = vm->ulyvres;
  i->fbSurface.ulStride = vm->ulstride;
  i->fbSurface.format   = vm->pixformat;

  i->current_videomode       = *vm;
  i->current_videomode_valid = 1;
  i->info.mode = fb_match_mode(&i->info.var, &i->info.modelist);

  i->info.fix.line_length    = vm->ulstride;
  i->info.fix.visual         = (i->info.var.bits_per_pixel == 8)?FB_VISUAL_PSEUDOCOLOR:FB_VISUAL_TRUECOLOR;
  i->info.par                = &i->current_videomode;
  return 0;
}


/*
 * stmfb_set_videomode
 * Set the hardware to display the videomode the mode passed in
 */
static int stmfb_set_videomode(struct fb_info *info)
{
  struct stmfb_info*     i = (struct stmfb_info*)info;
  struct stmfb_videomode vm;
  int ret;

  DPRINTK("in current process = %p pid = %d i = %p\n",current,(current!=NULL)?current->pid:0,i);

  if(!i->platformDevice)
  {
    DPRINTK("Missing platform device pointer???\n");
    return -ENODEV;
  }

  if(i->platformDevice->dev.power.power_state.event != PM_EVENT_ON)
  {
    DPRINTK("Changing display mode while in suspend state???\n");
    return -EBUSY;
  }

  /*
   * Get hardware description of videomode to set
   */
  if(stmfb_decode_var(&i->info.var, &vm, i) < 0)
  {
    printk(KERN_ERR "stmfb_set_videomode: unable to decode videomode, corrupted state?\n");
    return -EIO;
  }

  if(down_interruptible(&i->framebufferLock))
    return -ERESTARTSYS;

  if(i->current_videomode_valid &&
     i->current_videomode.pModeLine->Mode == vm.pModeLine->Mode &&
     i->current_videomode.pixformat       == vm.pixformat)
  {
    /*
     * Firstly set the virtual y resolution, this can change between VTs
     * but has no effect on the display beyond limiting the vertical panning.
     */
    i->current_videomode.ulyvres = vm.ulyvres;

    if(i->current_videomode.ulyoffset == vm.ulyoffset)
    {
      DPRINTK("Display mode already set, doing nothing\n");
      up(&i->framebufferLock);
      return 0;
    }

    /*
     * All we have been asked to change in effect is the panning, this often
     * happens with VT switches, so just do a pan. This might make switching
     * between DirectFB applications very marginally faster.
     */
    DPRINTK("Display mode already set, just panning display to %ld\n",vm.ulyoffset);
    ret = stmfb_do_pan_display(&i->info.var, i);
    up(&i->framebufferLock);

    return ret;
  }

  DPRINTK("Updating Display, synchronising with graphics engine\n");

  /*
   * Firstly make sure we have no blitter operations pending on the
   * current framebuffer.
   */
  if(i->pBlitter)
    stm_display_blitter_sync(i->pBlitter);

  DPRINTK("Synchronised with graphics engine\n");

  if(!i->current_videomode_valid)
  {
    ret = stmfb_coldstart_display(i,&vm);
    if(ret<0)
    {
      up(&i->framebufferLock);
      return ret;
    }
  }
  else if(i->current_videomode.pModeLine->Mode != vm.pModeLine->Mode)
  {
    /*
     * Try and change the display mode on the fly, to allow us to do a low
     * impact change between 60/59.94Hz variants of HD modes.
     */
    ret = stm_display_output_start (i->pFBMainOutput, vm.pModeLine, vm.ulTVStandard);
    if (ret == 0)
    {
      if(i->main_config.activate == STMFBIO_ACTIVATE_ON_NEXT_CHANGE)
      {
        i->main_config.activate = STMFBIO_ACTIVATE_IMMEDIATE;
        stmfb_set_output_configuration(&i->main_config, i);
        /*
         * If a signal happened then reset the activate flag so it
         * will happen next time. But other than that keep going.
         */
        if(signal_pending(current))
          i->main_config.activate = STMFBIO_ACTIVATE_ON_NEXT_CHANGE;
      }

      if(i->aux_config.activate == STMFBIO_ACTIVATE_ON_NEXT_CHANGE)
      {
        i->aux_config.activate = STMFBIO_ACTIVATE_IMMEDIATE;
        stmfb_set_output_configuration(&i->aux_config, i);

        if(signal_pending(current))
          i->aux_config.activate = STMFBIO_ACTIVATE_ON_NEXT_CHANGE;
      }


      /* It might be that HDMI was not active before, in particular, because
         it has never been started because the very first video mode we
         tried to set was a mode which the connected TV doesn't support. In
         this case, we need to do a full display restart. */
      if (i->hdmi && i->hdmi->status == STM_DISPLAY_NEEDS_RESTART)
      {
        DPRINTK("Signalling Display change to HDMI\n");
        i->hdmi->status_changed = 1;
        wake_up (&i->hdmi->status_wait_queue);
      }

      /*
       * Success, we should really wait for the display to actually switch as
       * this will be synchronized with the vsync. TDB Later!
       */
      DPRINTK("On the fly display change successful\n");
      if(i->current_videomode.pixformat == vm.pixformat &&
         i->current_videomode.ulyoffset == vm.ulyoffset)
      {
      	/*
      	 * No change necessary to the plane configuration so we are done.
      	 */
        i->current_videomode = vm;
        i->info.mode = fb_match_mode(&i->info.var, &i->info.modelist);
        up(&i->framebufferLock);
        return 0;
      }

      if(signal_pending(current))
      {
        up(&i->framebufferLock);
        return -EINTR;
      }
    }
    else
    {
      if(signal_pending(current))
      {
        up(&i->framebufferLock);
        return -EINTR;
      }

      if((ret = stmfb_restart_display(i,&vm))<0)
      {
        up(&i->framebufferLock);
        return ret;
      }
    }
  }

  ret = stmfb_queue_new_fb_configuration(i,&vm);
  up(&i->framebufferLock);

  DPRINTK("out\n");
  return ret;
}


static int stmfb_sync(struct fb_info *info)
{
  struct stmfb_info *i = (struct stmfb_info *)info;
  int ret = 0;

  if(down_interruptible(&i->framebufferLock))
    return -ERESTARTSYS;

  if(i->pBlitter)
  {
    if((ret = stm_display_blitter_sync(i->pBlitter)) < 0)
    {
      if(signal_pending(current))
        ret = -ERESTARTSYS;
      else
        ret = -EIO;
    }
  }

  up(&i->framebufferLock);

  return ret;
}


static void stmfb_set_pseudo_palette(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info* info)
{
  struct stmfb_info* i = (struct stmfb_info* )info;
  struct stmfb_videomode vm;

  /*
   * We have to decode the var to get the full display format, as we may be
   * called before any videomode has been set.
   */
  if(stmfb_decode_var(&i->info.var, &vm, i) < 0)
  {
    printk(KERN_ERR "%s: unable to decode videomode, corrupted state?\n",__FUNCTION__);
    return;
  }


  /*
   * We need to maintain the pseudo palette as this is what is used
   * by the cfb routines to draw on the framebuffer for the VT code.
   */
  switch (vm.pixformat)
  {
    case SURF_RGB565:
    {
      i->pseudo_palette[regno] = (red   & 0xf800       ) |
                                ((green & 0xfc00) >> 5 ) |
                                ((blue  & 0xf800) >> 11);
      break;
    }
    case SURF_ARGB1555:
    {
      int alpha = (transp>0)?1:0;
      i->pseudo_palette[regno] = ((red   & 0xf800) >> 1 ) |
                                 ((green & 0xf800) >> 6 ) |
                                 ((blue  & 0xf800) >> 11) |
                                  (alpha           << 15);
      break;
    }
    case SURF_ARGB4444:
    {
      i->pseudo_palette[regno] = ((red    & 0xe000) >> 4 ) |
                                 ((green  & 0xe000) >> 8 ) |
                                 ((blue   & 0xe000) >> 12) |
                                 ((transp & 0xe000)      );
      break;
    }
    case SURF_RGB888:
    {
      i->pseudo_palette[regno] = ((red    & 0xff00) << 8) |
                                 ((green  & 0xff00)     ) |
                                 ((blue   & 0xff00) >> 8);
      break;
    }
    case SURF_ARGB8565:
    {
      i->pseudo_palette[regno] = (red   & 0xf800       ) |
                                ((green & 0xfc00) >> 5 ) |
                                ((blue  & 0xf800) >> 11) |
                                ((transp& 0xff00) << 16);
      break;
    }
    case SURF_ARGB8888:
    {
      i->pseudo_palette[regno] = ((red    & 0xff00) << 8 ) |
                                 ((green  & 0xff00)      ) |
                                 ((blue   & 0xff00) >> 8 ) |
                                 ((transp & 0xff00) << 16);
      break;
    }
    default:
      break;
  }
}


/*
 * stmfb_setcolreg
 * Set a single color register. The values supplied have a 16 bit magnitude
 * Return != 0 for invalid regno.
 */
static int stmfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info* info)
{
  struct stmfb_info* i = (struct stmfb_info* )info;
  unsigned long alpha;

  if (regno > 255)
    return -EINVAL;

  /*
   * First set the console palette (first 16 entries only) for RGB framebuffers
   */
  if (regno < 16)
    stmfb_set_pseudo_palette(regno, red, green, blue, transp, info);

  /*
   * Now set the real CLUT for 8bit surfaces.
   *
   * Note that transparency is converted to a range of 0-128 for the hardware
   * CLUT. This is different from the pseudo palette where alpha image formats
   * use the full 0-255 8bit range where available.
   */
  alpha = ((transp>>8)+1)/2;
  i->pFBCLUT[regno] = ( alpha     <<24) |
                      ((red>>8)   <<16) |
                      ((green>>8) <<8 ) |
                       (blue>>8);

#if DEBUG_CLUT
  DPRINTK("pFCLUT[%u] = 0x%08lx\n",regno,i->pFBCLUT[regno]);
#endif

  return 0;
}

static int stmfb_iomm_addressable(const struct stmfb_info * const i, unsigned long physaddr)
{
  int n;
  struct stmcore_display_pipeline_data *pd;

  pd = *((struct stmcore_display_pipeline_data **)i->platformDevice->dev.platform_data);

  for (n=0; n<pd->whitelist_size; n++)
    if (physaddr == pd->whitelist[n])
      return 1;

  return 0;
}

/* The following couple lines of code make sure that in case a DirectFB
   application crashes/gets killed/interrupted while being in the process
   of updating the BDisp LNA, bdisp_running is _not_ left set, which
   would give ourselves a hard time and leave us in a funny state. We
   have no other way of finding this out. It's important to do sth about
   that here, since else e.g. unloading this module could hang forever. */
static void stmfb_iomm_vma_open(struct vm_area_struct *vma)
{
  unsigned long physaddr;

  struct stmfb_info * const i = vma->vm_private_data;
  const struct stmcore_display_pipeline_data * const pd =
    *((struct stmcore_display_pipeline_data **)i->platformDevice->dev.platform_data);

  physaddr = (vma->vm_pgoff << PAGE_SHIFT) + pd->io_offset;
  if (physaddr == pd->mmio)
    atomic_inc (&i->mmio_users);
}

static void stmfb_iomm_vma_close(struct vm_area_struct *vma)
{
  unsigned long physaddr;

  struct stmfb_info * const i = vma->vm_private_data;
  const struct stmcore_display_pipeline_data * const pd =
    *((struct stmcore_display_pipeline_data **)i->platformDevice->dev.platform_data);

  physaddr = (vma->vm_pgoff << PAGE_SHIFT) + pd->io_offset;
  if (physaddr == pd->mmio
      && atomic_dec_and_test (&i->mmio_users)
      && i->pVSharedAreaBase)
  {
    /* now that nobody has the fb device open for BDisp access anymore,
       it's a good idea to clear the BDisp shared area, just in case the
       previous user crashed or so and left it in a state inconsistent
       w/ the hardware ... */
    i->pVSharedAreaBase->bdisp_running
      = i->pVSharedAreaBase->updating_lna
      = 0;
    atomic_set ((atomic_t *) &i->pVSharedAreaBase->lock, 0);
  }
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
static struct page *stmfb_iomm_vma_nopage(struct vm_area_struct *vma, unsigned long address, int *type)
{
  /* we want to provoke a bus error rather than give the client the zero page */
  return NOPAGE_SIGBUS;
}
#else
static int stmfb_iomm_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
  return VM_FAULT_SIGBUS;
}
#endif

static struct vm_operations_struct stmfb_iomm_nopage_ops = {
  .open = stmfb_iomm_vma_open,
  .close = stmfb_iomm_vma_close,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
  .nopage = stmfb_iomm_vma_nopage,
#else
  .fault = stmfb_iomm_vma_fault,
#endif
};

static int stmfb_iomm_mmap(struct stmfb_info * const i, struct vm_area_struct * vma)
{
  unsigned long rawaddr, physaddr, vsize, off;
  const struct stmcore_display_pipeline_data * const pd =
    *((struct stmcore_display_pipeline_data **)i->platformDevice->dev.platform_data);

  vma->vm_flags |= VM_IO | VM_RESERVED | VM_DONTEXPAND;
  vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

  rawaddr  = (vma->vm_pgoff << PAGE_SHIFT);
  physaddr = rawaddr + pd->io_offset;
  vsize = vma->vm_end - vma->vm_start;

  for (off=0; off<vsize; off+=PAGE_SIZE)
  {
    if (stmfb_iomm_addressable(i, rawaddr+off))
      io_remap_pfn_range(vma, vma->vm_start+off, (physaddr+off) >> PAGE_SHIFT, PAGE_SIZE, vma->vm_page_prot);
  }

  vma->vm_private_data = i;

  // ensure we get bus errors when we access illegal memory address
  vma->vm_ops = &stmfb_iomm_nopage_ops;
  stmfb_iomm_vma_open(vma);

  return 0;
}


/* This is more or less a verbatim copy of fb_mmap().
 * Sadly we have to copy it on order to hook any attempt
 * to mmap() the registers. We also want to allow to mmap the
 * auxmem, which the linux fb subsystem knows nothing about.
 * In the process we are also forced to hook attempts to
 * mmap() the framebuffer itself.
 *
 * Sigh.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
// FIXME: this should be removed once we don't support < 2.6.23 anymore
static int stmfb_mmap(struct fb_info *info, struct vm_area_struct * vma)
{
  struct stmfb_info * const i = container_of(info, struct stmfb_info, info);
  unsigned long      off;
  unsigned int       idx;
  int                is_auxmem = 0; /* 0: fbmem or mmio (uncached),
                                       1: auxmem (uncached),
                                       2: STMFBBDispSharedArea (cached),
                                       3: BDisp nodelist itself (uncached),
                                    */
  unsigned int       len_requested = vma->vm_end - vma->vm_start;
#if !defined(__sparc__) || defined(__sparc_v9__)
  unsigned long start;
  u32 len;
#endif

  off = vma->vm_pgoff << PAGE_SHIFT;

  /* frame buffer memory */
  start = info->fix.smem_start;
  len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);

  if (off >= len)
  {
    /* check if mmap of auxmem was requested */
    for (idx = 0; !is_auxmem && idx < ARRAY_SIZE (i->AuxPart); ++idx)
    {
      if (i->AuxPart[idx]
          && off >= i->AuxBase[idx]
          && (off + len_requested) >= i->AuxBase[idx] /* fight hackers 'n crackers */
          && (off + len_requested) <= (i->AuxBase[idx] + i->AuxSize[idx]))
      {
        /* yes! */
        start = i->AuxBase[idx];
        off -= (start & PAGE_MASK);
        len = PAGE_ALIGN((start & ~PAGE_MASK) + i->AuxSize[idx]);
        is_auxmem = 1;
        if ((len_requested + off) > len)
          printk ("will fail\n");
      }
    }
    if (!is_auxmem)
    {
      if (i->pVSharedAreaBase)
      {
        /* check if mmap of shared area was requested */
        /* we mmap() the first page (shared area) cached, and the actual
           nodelist uncached */
        if (off == i->ulPSharedAreaBase && len_requested == PAGE_SIZE)
          {
            start = i->ulPSharedAreaBase;
            off -= (start & PAGE_MASK);
            len = PAGE_ALIGN ((start & ~PAGE_MASK) + i->ulSharedAreaSize);
            is_auxmem = 2;
          }
        else if (off >= i->ulPSharedAreaBase
                 && (off + len_requested) >= i->ulPSharedAreaBase /* fight hackers 'n crackers */
                 && (off + len_requested) <= (i->ulPSharedAreaBase + i->ulSharedAreaSize))
        {
          start = i->ulPSharedAreaBase;
          off -= (start & PAGE_MASK);
          len = PAGE_ALIGN ((start & ~PAGE_MASK) + i->ulSharedAreaSize);
          is_auxmem = 3;
        }
      }

      if (!is_auxmem)
      {
        if (off + len_requested <= info->fix.smem_len + info->fix.mmio_len)
          /* mmap() of mmio @ offset == info->fix.smem_len */
          vma->vm_pgoff = info->fix.mmio_start >> PAGE_SHIFT;

        return stmfb_iomm_mmap(i, vma);
      }
    }
  }

  start &= PAGE_MASK;
  if ((len_requested + off) > len)
    return -EINVAL;
  off += start;
  vma->vm_pgoff = off >> PAGE_SHIFT;
  /* This is an IO map - tell maydump to skip this VMA */
  vma->vm_flags |= VM_IO | VM_RESERVED | VM_DONTEXPAND;
#if defined(__sparc_v9__)
  if (io_remap_page_range(vma, vma->vm_start, off,
			  len_requested, vma->vm_page_prot, 0))
    return -EAGAIN;
#else
#if defined(__mc68000__)
#if defined(CONFIG_SUN3)
  pgprot_val(vma->vm_page_prot) |= SUN3_PAGE_NOCACHE;
#elif defined(CONFIG_MMU)
  if (CPU_IS_020_OR_030)
    pgprot_val(vma->vm_page_prot) |= _PAGE_NOCACHE030;
  if (CPU_IS_040_OR_060) {
    pgprot_val(vma->vm_page_prot) &= _CACHEMASK040;
    /* Use no-cache mode, serialized */
    pgprot_val(vma->vm_page_prot) |= _PAGE_NOCACHE_S;
  }
#endif
#elif defined(__powerpc__)
  pgprot_val(vma->vm_page_prot) |= _PAGE_NO_CACHE|_PAGE_GUARDED;
#elif defined(__alpha__)
  /* Caching is off in the I/O space quadrant by design.  */
#elif defined(__i386__) || defined(__x86_64__)
  if (boot_cpu_data.x86 > 3)
    pgprot_val(vma->vm_page_prot) |= _PAGE_PCD;
#elif defined(__mips__)
  vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#elif defined(__hppa__)
  pgprot_val(vma->vm_page_prot) |= _PAGE_NO_CACHE;
#elif defined(__arm__) || defined(__sh__) || defined(__m32r__)
  vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#elif defined(__ia64__)
  if (efi_range_is_wc(vma->vm_start, len_requested))
    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
  else
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#else
#warning What do we have to do here??
#endif
  if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                         len_requested, vma->vm_page_prot))
    return -EAGAIN;
#endif /* !__sparc_v9__ */
  return 0;
}
#else
/* much more readable code from 2.6.23 */
#include <asm/fb.h>
static int stmfb_mmap(struct fb_info *info, struct vm_area_struct * vma)
{
  struct stmfb_info * const i = container_of(info, struct stmfb_info, info);
  unsigned long      off;
  unsigned long      start;
  u32                len;
  unsigned int       idx;
  int                is_auxmem = 0; /* 0: fbmem or mmio (uncached),
                                       1: auxmem (uncached),
                                       2: STMFBBDispSharedArea (cached),
                                       3: BDisp nodelist itself (uncached),
                                    */
  unsigned int       len_requested = vma->vm_end - vma->vm_start;

  off = vma->vm_pgoff << PAGE_SHIFT;

  /* frame buffer memory */
  start = info->fix.smem_start;
  len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);

  if (off >= len)
  {
    /* check if mmap of auxmem was requested */
    for (idx = 0; !is_auxmem && idx < ARRAY_SIZE (i->AuxPart); ++idx)
    {
      if (i->AuxPart[idx]
          && off >= i->AuxBase[idx]
          && (off + len_requested) >= i->AuxBase[idx] /* fight hackers 'n crackers */
          && (off + len_requested) <= (i->AuxBase[idx] + i->AuxSize[idx]))
      {
        /* yes! */
        start = i->AuxBase[idx];
        off -= (start & PAGE_MASK);
        len = PAGE_ALIGN((start & ~PAGE_MASK) + i->AuxSize[idx]);
        is_auxmem = 1;
      }
    }

    if (!is_auxmem)
    {
      if (i->pVSharedAreaBase)
      {
        /* check if mmap of shared area was requested */
        /* we mmap() the first page (shared area) cached, and the actual
           nodelist uncached */
        if (off == i->ulPSharedAreaBase && len_requested == PAGE_SIZE)
          {
            start = i->ulPSharedAreaBase;
            off -= (start & PAGE_MASK);
            len = PAGE_ALIGN ((start & ~PAGE_MASK) + i->ulSharedAreaSize);
            is_auxmem = 2;
          }
        else if (off >= i->ulPSharedAreaBase
                 && (off + len_requested) >= i->ulPSharedAreaBase /* fight hackers 'n crackers */
                 && (off + len_requested) <= (i->ulPSharedAreaBase + i->ulSharedAreaSize))
        {
          start = i->ulPSharedAreaBase;
          off -= (start & PAGE_MASK);
          len = PAGE_ALIGN ((start & ~PAGE_MASK) + i->ulSharedAreaSize);
          is_auxmem = 3;
        }
      }

      if (!is_auxmem)
      {
        /* memory mapped io */
        if (off + len_requested <= info->fix.smem_len + info->fix.mmio_len)
          /* mmap() of mmio @ offset == info->fix.smem_len */
          vma->vm_pgoff = info->fix.mmio_start >> PAGE_SHIFT;

        return stmfb_iomm_mmap(i, vma);
      }
    }
  }

  start &= PAGE_MASK;
  if ((len_requested + off) > len)
    return -EINVAL;
  off += start;
  vma->vm_pgoff = off >> PAGE_SHIFT;
  /* This is an IO map - tell maydump to skip this VMA */
  vma->vm_flags |= VM_IO | VM_RESERVED | VM_DONTEXPAND;
  /* mmap() shared area cached, framebuffer uncached */
  if (is_auxmem != 2)
    fb_pgprotect(NULL, vma, off); /* using NULL here breaks powerpc! but we
                                     don't have file :( Since we're not powerpc,
                                     we don't care :) */
  if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                         len_requested, vma->vm_page_prot))
    return -EAGAIN;

  return 0;
}
#endif


/*
 * Framebuffer device structure.
 */
struct fb_ops stmfb_ops = {
	.owner			= THIS_MODULE,
	.fb_open		= NULL,
	.fb_release		= NULL,
	.fb_read		= NULL,
	.fb_write		= NULL,
	.fb_check_var		= stmfb_check_var,
	.fb_set_par		= stmfb_set_videomode,
	.fb_setcolreg		= stmfb_setcolreg,
	.fb_blank		= NULL,
	.fb_pan_display		= stmfb_pan_display,
	.fb_fillrect		= cfb_fillrect,
	.fb_copyarea		= cfb_copyarea,
	.fb_imageblit		= cfb_imageblit,
	.fb_rotate		= NULL,
	.fb_sync		= stmfb_sync,
	.fb_ioctl		= stmfb_ioctl,
	.fb_mmap		= stmfb_mmap,
};
