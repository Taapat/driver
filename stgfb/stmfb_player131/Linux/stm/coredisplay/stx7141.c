/***********************************************************************
 *
 * File: stmfb/Linux/stm/coredisplay/sti7141.c
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

/*
 * Note that the STi7141 is only supported on 2.6.23 and above kernels
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/stm/sysconf.h>
#include <linux/stm/pio.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/irq-ilc.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include <stmdisplay.h>
#include <linux/stm/stmcoredisplay.h>

/*
 * Yes we are using 7111 unmodified at the moment, the display is that similar
 */
#include "Gamma/sti7111/sti7111reg.h"
#include "Gamma/sti7111/sti7111device.h"

static const unsigned long whitelist[] = {
    STi7111_REGISTER_BASE + STi7111_DENC_BASE,
    STi7111_REGISTER_BASE + STi7111_DENC_BASE+PAGE_SIZE,
    STi7111_REGISTER_BASE + STi7111_DENC_BASE+(PAGE_SIZE*2),
    STi7111_REGISTER_BASE + STi7111_HDMI_BASE,
    _ALIGN_DOWN(STi7111_REGISTER_BASE + STi7111_BLITTER_BASE, PAGE_SIZE),
};


/* BDisp IRQs on 7141: aq1 115, aq2 116, aq3 117, aq4 118, cq1 119, cq2 120 */
static struct stmcore_display_pipeline_data platform_data[] = {
  {
    .owner                    = THIS_MODULE,
    .name                     = "STi7141-main",
    .device                   = 0,
    .vtg_irq                  = ILC_IRQ(143),
    .blitter_irq              = ILC_IRQ(115),
    .hdmi_irq                 = ILC_IRQ(139),
    .hdmi_i2c_adapter_id      = 4,
    .main_output_id           = STi7111_OUTPUT_IDX_VDP0_MAIN,
    .aux_output_id            = STi7111_OUTPUT_IDX_VDP0_AUX,
    .hdmi_output_id           = STi7111_OUTPUT_IDX_VDP0_HDMI,
    .dvo_output_id            = STi7111_OUTPUT_IDX_DVO0,

    .blitter_id               = STi7111_BLITTER_IDX_VDP0_MAIN,
    .blitter_type             = STMCORE_BLITTER_BDISPII,

    .preferred_graphics_plane = OUTPUT_GDP1,
    .preferred_video_plane    = OUTPUT_VID1,
    .planes                   = {
        { OUTPUT_GDP1, STMCORE_PLANE_GFX | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS},
        { OUTPUT_GDP2, STMCORE_PLANE_GFX | STMCORE_PLANE_MEM_SYS},
        { OUTPUT_GDP3, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_MEM_SYS},
        { OUTPUT_VID1, STMCORE_PLANE_VIDEO | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS},
        { OUTPUT_VID2, STMCORE_PLANE_VIDEO | STMCORE_PLANE_SHARED | STMCORE_PLANE_MEM_SYS},
        { OUTPUT_CUR , STMCORE_PLANE_GFX | STMCORE_PLANE_MEM_SYS}
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0,

    /* this is later mmap()ed, so it must be page aligned! */
    .mmio                     = _ALIGN_DOWN (STi7111_REGISTER_BASE + STi7111_BLITTER_BASE, PAGE_SIZE),
    .mmio_len                 = PAGE_SIZE,
  },
  {
    .owner                    = THIS_MODULE,
    .name                     = "STi7141-aux",
    .device                   = 0,
    .vtg_irq                  = ILC_IRQ(125),
    .blitter_irq              = ILC_IRQ(116),
    .hdmi_irq                 = -1,
    .hdmi_i2c_adapter_id      = -1,
    .main_output_id           = STi7111_OUTPUT_IDX_VDP1_MAIN,
    .aux_output_id            = STi7111_OUTPUT_IDX_VDP1_AUX,
    .hdmi_output_id           = -1,
    .dvo_output_id            = -1,

    .blitter_id               = STi7111_BLITTER_IDX_VDP1_MAIN,
    .blitter_type             = STMCORE_BLITTER_BDISPII,

    .preferred_graphics_plane = OUTPUT_GDP3,
    .preferred_video_plane    = OUTPUT_VID2,
    .planes                   = {
        { OUTPUT_GDP3, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS},
        { OUTPUT_VID2, STMCORE_PLANE_VIDEO | STMCORE_PLANE_SHARED | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS}
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0,

    /* this is later mmap()ed, so it must be page aligned! */
    .mmio                     = _ALIGN_DOWN (STi7111_REGISTER_BASE + STi7111_BLITTER_BASE, PAGE_SIZE),
    .mmio_len                 = PAGE_SIZE,
  }
};


static const int maxDAC123Voltage = 1360;
static const int maxDAC456Voltage = 1360;

static const int DAC123SaturationPoint = 0; // Use Default (1023 for a 10bit DAC)
static const int DAC456SaturationPoint = 0;

static struct stpio_pin *hotplug_pio = 0;
static struct sysconf_field *syscfg20_24;

int __init stmcore_probe_device(struct stmcore_display_pipeline_data **pd, int *nr_platform_devices)
{

  if(boot_cpu_data.type == CPU_STX7141)
  {
    unsigned long syscfg20;
    *pd = platform_data;
    *nr_platform_devices = N_ELEMENTS (platform_data);

    /*
     * Setup HDMI hotplug
     */
    hotplug_pio = stpio_request_pin(5,6,"HDMI Hotplug",STPIO_IN);
    /*
     * Enable hotplug pio in syscfg
     */
    syscfg20_24 = sysconf_claim(SYS_CFG,20,24,24,"HDMI Hotplug PIO ALT Function");
    sysconf_write(syscfg20_24,1);

    printk(KERN_WARNING "stmcore-display: STi7141 display probed\n");
    return 0;
  }

  printk(KERN_WARNING "stmcore-display: STi7141 display not found\n");

  return -ENODEV;
}


int __init stmcore_display_postinit(struct stmcore_display *p)
{
  /*
   * Setup internal configuration controls
   */
  if(maxDAC123Voltage != 0)
    stm_display_output_set_control(p->main_output, STM_CTRL_DAC123_MAX_VOLTAGE, maxDAC123Voltage);

  if(maxDAC456Voltage != 0)
    stm_display_output_set_control(p->main_output, STM_CTRL_DAC456_MAX_VOLTAGE, maxDAC456Voltage);

  if(DAC123SaturationPoint != 0)
    stm_display_output_set_control(p->main_output, STM_CTRL_DAC123_SATURATION_POINT, DAC123SaturationPoint);

  if(DAC456SaturationPoint != 0)
    stm_display_output_set_control(p->main_output, STM_CTRL_DAC456_SATURATION_POINT, DAC456SaturationPoint);

  return 0;
}


void stmcore_cleanup_device(void)
{
  if(hotplug_pio)
    stpio_free_pin(hotplug_pio);

  if (syscfg20_24)
    sysconf_release (syscfg20_24);

}
