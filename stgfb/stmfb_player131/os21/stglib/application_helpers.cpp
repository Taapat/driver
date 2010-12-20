/***********************************************************************
 *
 * File: stgfb/os21/stglib/application_helpers.cpp
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include "application_helpers.h"

/*
 * Colour bars in RGB, with the range clamped to 16-235 for video output.
 */
unsigned long colourbars100[] = { 0xffebebeb,
                                  0xffebeb10,
                                  0xff10ebeb,
                                  0xff10eb10,
                                  0xffeb10eb,
                                  0xffeb1010,
                                  0xff1010eb,
                                  0xff101010 };

void create_test_pattern(char *pBuf,unsigned long width,unsigned long height, unsigned long stride)
{
unsigned long *pixel;
unsigned long pixelVal;
int nbars = sizeof(colourbars100)/sizeof(unsigned long);

  /* Greyscale ramp */
  for(int y=0;y<height/2;y++)
  {
    for(int x=0;x<width;x++)
    {
      unsigned long tmp = x%256;
      pixel = (unsigned long *)(pBuf+y*stride+x*sizeof(unsigned long));
      pixelVal = (0xff<<24) | (tmp<<16) | (tmp<<8) | tmp;

      *pixel = pixelVal;
    }
  }

  for(int i=0;i<nbars;i++)
  {
    int barwidth = width/nbars;
    for(int y=height/2;y<height;y++)
    {
      for(int x=(barwidth*i);x<(barwidth*(i+1));x++)
      {
    	pixel = (unsigned long *)(pBuf+y*stride+x*sizeof(unsigned long));

    	*pixel = colourbars100[i];
      }
    }
  }
}


volatile unsigned char *hotplug_poll_pio = 0;
int hotplug_pio_pin = 0;



void setup_soc(void)
{
#if defined(CONFIG_STB7100)
  {
    /*
     * Enable HDMI hotplug PIO function
     */
    volatile UCHAR *pio2 = (volatile UCHAR *)vmem_create((void*)(STb7100_REGISTER_BASE+STb7100_PIO2_BASE),
                                                         1024,NULL,
                                                         (VMEM_CREATE_UNCACHED|VMEM_CREATE_WRITE|VMEM_CREATE_READ));


    hotplug_poll_pio = pio2+PIO_INPUT;
    hotplug_pio_pin = (1L<<2);

    /*
     * PIO2 pin 2 to input with weak pullup for HDMI hotplug
     */
    *(pio2+PIO_PnC0) = *(pio2+PIO_PnC0) & ~(1L<<2);
    *(pio2+PIO_PnC1) = *(pio2+PIO_PnC1) & ~(1L<<2);
    *(pio2+PIO_PnC2) = *(pio2+PIO_PnC2) & ~(1L<<2);
  }

#elif defined(CONFIG_STI7111)
  {
    /*
     * Enable HDMI hotplug PIO function
     */
    volatile UCHAR *pio4 = (volatile UCHAR *)(STi7111_REGISTER_BASE+STi7111_PIO4);

    /*
     * PIO4 pin 7 to input for HDMI hotplug
     */
    *(pio4+PIO_PnC0) = *(pio4+PIO_PnC0) & ~(1L<<7);
    *(pio4+PIO_PnC1) = *(pio4+PIO_PnC1) & ~(1L<<7);
    *(pio4+PIO_PnC2) = *(pio4+PIO_PnC2) | (1L<<7);

    /*
     * Enable hotplug pio in syscfg
     */
    volatile ULONG *syscfg2 = (volatile ULONG *)(STi7111_REGISTER_BASE+STi7111_SYSCFG_BASE+SYS_CFG2);

    *syscfg2 = *syscfg2 | SYS_CFG2_HDMI_HOTPLUG_EN;

  }

#elif defined(CONFIG_STI7105) || defined(CONFIG_STI7106)
  {
    /*
     * Enable HDMI hotplug PIO function
     */
    volatile UCHAR *pio9 = (volatile UCHAR *)(STi7111_REGISTER_BASE+0x29000);

    /*
     * PIO9 pin 7 to input for HDMI hotplug
     */
    *(pio9+PIO_PnC0) = *(pio9+PIO_PnC0) & ~(1L<<7);
    *(pio9+PIO_PnC1) = *(pio9+PIO_PnC1) & ~(1L<<7);
    *(pio9+PIO_PnC2) = *(pio9+PIO_PnC2) | (1L<<7);

    /*
     * Enable hotplug pio in syscfg
     */
    volatile ULONG *syscfg2 = (volatile ULONG *)(STi7111_REGISTER_BASE+STi7111_SYSCFG_BASE+SYS_CFG2);

    *syscfg2 = *syscfg2 | SYS_CFG2_HDMI_HOTPLUG_EN;

  }

#elif defined(CONFIG_STI7141)
  {
    /*
     * Enable HDMI hotplug PIO function
     */
    volatile ULONG *syscfg20 = (volatile ULONG *)(STi7111_REGISTER_BASE+STi7111_SYSCFG_BASE+0x150);

    *syscfg20 = *syscfg20 | (1L<<24);

  }

#elif defined(CONFIG_STI7200)
  {
    /*
     * This malarky enabled H&V sync on the DSub connector on MB519/MB520
     * when J35 & J36 on the MB520 are set to the 1-2 position.
     */
    volatile UCHAR *reg = (volatile UCHAR *)STi7200_REGISTER_BASE;

    volatile UCHAR *pio3 = reg+STi7200_PIO3;
    volatile UCHAR *pio4 = reg+STi7200_PIO4;
    volatile ULONG *syscfg7 = (volatile ULONG *)(reg+STi7200_SYSCFG_BASE+SYS_CFG7);

    /*
     * PIO3 pin 7 to alternate output for vsync
     */
    *(pio3+PIO_PnC0) = *(pio3+PIO_PnC0) & ~(1L<<7);
    *(pio3+PIO_PnC1) = *(pio3+PIO_PnC1) | (1L<<7);
    *(pio3+PIO_PnC2) = *(pio3+PIO_PnC2) | (1L<<7);

    /*
     * PIO4 pin 0 to alternate output for hsync
     */
    *(pio4+PIO_PnC0) = *(pio4+PIO_PnC0) & ~(1L<<0);
    *(pio4+PIO_PnC1) = *(pio4+PIO_PnC1) | (1L<<0);
    *(pio4+PIO_PnC2) = *(pio4+PIO_PnC2) | (1L<<0);

    /*
     * Route H/V sync to PIO pads
     */
    *syscfg7 = *syscfg7 | SYS_CFG7_PIO_PAD_0;

  }

  {
    /*
     * Enable HDMI hotplug PIO function
     */
    volatile UCHAR *pio5 = (volatile UCHAR *)STi7200_REGISTER_BASE+STi7200_PIO5;

    /*
     * PIO5 pin 7 to alternate bidirectional (input) for HDMI hotplug
     */
    *(pio5+PIO_PnC0) = *(pio5+PIO_PnC0) | (1L<<7);
    *(pio5+PIO_PnC1) = *(pio5+PIO_PnC1) | (1L<<7);
    *(pio5+PIO_PnC2) = *(pio5+PIO_PnC2) | (1L<<7);

  }

#endif
}


void setup_analogue_voltages(stm_display_output_t *pOutput)
{
#if defined(CONFIG_STB7100)
#if defined(CONFIG_MB602)
  stm_display_output_set_control(pOutput, STM_CTRL_DAC123_MAX_VOLTAGE, 1402);
  stm_display_output_set_control(pOutput, STM_CTRL_DAC456_MAX_VOLTAGE, 1402);
#else
  stm_display_output_set_control(pOutput, STM_CTRL_DAC123_MAX_VOLTAGE, 1409);
  stm_display_output_set_control(pOutput, STM_CTRL_DAC456_MAX_VOLTAGE, 1409);
#endif
#endif

#if defined(CONFIG_MB519)
  stm_display_output_set_control(pOutput, STM_CTRL_DAC123_MAX_VOLTAGE, 1350);
  stm_display_output_set_control(pOutput, STM_CTRL_DAC456_MAX_VOLTAGE, 1350);
#endif

#if defined(CONFIG_MB671)
  stm_display_output_set_control(pOutput, STM_CTRL_DAC123_MAX_VOLTAGE, 1350);
  stm_display_output_set_control(pOutput, STM_CTRL_DAC456_MAX_VOLTAGE, 1350);
#endif

#if defined(CONFIG_MB618)
  stm_display_output_set_control(pOutput, STM_CTRL_DAC123_MAX_VOLTAGE, 1360);
  /*
   * The following is for the CVBS phono output with Jumper 38 set to the 2-3
   * position. For some reason there is an op-amp on this path which one assumes
   * is the reason why set such an add value for the voltage range.
   */
  stm_display_output_set_control(pOutput, STM_CTRL_DAC456_MAX_VOLTAGE, 1600);
#endif

#if defined(CONFIG_STI7141)
  stm_display_output_set_control(pOutput, STM_CTRL_DAC123_MAX_VOLTAGE, 1360);
  stm_display_output_set_control(pOutput, STM_CTRL_DAC456_MAX_VOLTAGE, 1360);
#endif

#if defined(CONFIG_MB680)
  stm_display_output_set_control(pOutput, STM_CTRL_DAC123_MAX_VOLTAGE, 1360);
  stm_display_output_set_control(pOutput, STM_CTRL_DAC456_MAX_VOLTAGE, 1360);
#endif

#if defined(CONFIG_SDK7105)
  stm_display_output_set_control(pOutput, STM_CTRL_DAC123_MAX_VOLTAGE, 1320);
  stm_display_output_set_control(pOutput, STM_CTRL_DAC456_MAX_VOLTAGE, 1320);
#endif

}

stm_display_output_t *get_hdmi_output(stm_display_device_t *pDev)
{
  int i = 0;
  stm_display_output_t *out;
  while((out = stm_display_get_output(pDev,i++)) != 0)
  {
    ULONG caps;
    stm_display_output_get_capabilities(out, &caps);
    if((caps & STM_OUTPUT_CAPS_TMDS) != 0)
    {
      return out;
    }

    stm_display_output_release(out);
  }

  return 0;
}


int hdmi_isr(void *data)
{
  stm_display_output_t *o = (stm_display_output_t *)data;
  stm_display_output_handle_interrupts(o);
  return OS21_SUCCESS;
}


int get_yesno(void)
{
  int ch=0;
  while(1)
  {
    printf("Did the test succeed y/n?\n");
    ch = getchar();
    switch(ch)
    {
      case 'y':
      case 'Y':
        return 0;
      case 'n':
      case 'N':
        return 1;
    }
  }
}
