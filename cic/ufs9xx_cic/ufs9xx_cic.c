/*
 * ufs912/922 ci controller handling.
 *
 * Registers:
 *
 *
 * Dagobert
 * GPL
 * 2010
 */



#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/platform_device.h>

#include <linux/interrupt.h>
#include <linux/i2c.h> 
#include <linux/i2c-algo-bit.h>
#include <linux/firmware.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#include <linux/stpio.h>
#else
#include <linux/stm/pio.h>
#endif

#include <asm/system.h>
#include <asm/io.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <linux/mutex.h>
#include <linux/dvb/dmx.h>

#include "dvb_frontend.h"
#include "dvbdev.h"
#include "demux.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_filter.h"
#include "dvb_net.h"
#include "dvb_ca_en50221.h"

#include "ufs9xx_cic.h"

static int debug=0;

#define TAGDEBUG "[ufs9xx_cic] "

#define dprintk(level, x...) do { \
if ((debug) && (debug >= level)) printk(TAGDEBUG x); \
} while (0)


/* ***** emi for 7111 */
#if defined(UFS912)
#define EMIConfigBaseAddress 0xfe700000
#elif defined(UFS922)
#define EMIConfigBaseAddress 0x1A100000
#endif


#define EMI_LCK 	0x0020
#define EMI_GEN_CFG 0x0028

#define EMI_FLASH_CLK_SEL 0x0050 /* WO: 00, 10, 01 */
#define EMI_CLK_EN 0x0068 /* WO: must only be set once !!*/

#define EMIBank0 0x100
#define EMIBank1 0x140
#define EMIBank2 0x180
#define EMIBank3 0x1C0
#define EMIBank4 0x200
#define EMIBank5 0x240 /* virtual */

#define EMI_CFG_DATA0	0x0000
#define EMI_CFG_DATA1	0x0008
#define EMI_CFG_DATA2	0x0010
#define EMI_CFG_DATA3	0x0018

unsigned long reg_config = 0;

/* ***** end 7111 emi */

/* *************************************************
 * For the reason I dont understand the bits in the
 * i2c registers I use this constants.
 */
#define  TUNER_1_VIEW 			0
#define  TUNER_1_CAM_A_VIEW 		1
#define  TUNER_1_CAM_B_VIEW 		2
#define  TUNER_1_CAM_A_CAM_B_VIEW 	3
#define  TUNER_2_CAM_A 		        4
#define  TUNER_2_CAM_B 	 	        5
#define  TUNER_2_CAM_A_B 		6
#define  TUNER_2_VIEW                   7

#if defined(UFS912)
volatile unsigned long slot_a_r_mem  =	0x03028000; /* read */
volatile unsigned long slot_a_w_mem  =	0x03008000; /* write */
volatile unsigned long slot_a_w_stat =	0x03000000;
volatile unsigned long slot_a_r_stat =	0x03020000;

volatile unsigned long slot_b_r_mem  =	0x04028000;
volatile unsigned long slot_b_w_mem  =	0x04008000;
volatile unsigned long slot_b_w_stat =	0x04000000;
volatile unsigned long slot_b_r_stat =	0x04020000;
#elif defined(UFS922)
unsigned long slot_a_w_stat =  0xa2800000;  //write
unsigned long slot_a_r_stat =  0xa2820000;  //read
unsigned long slot_a_r_mem =   0xa2828000;  //read
unsigned long slot_a_w_mem =   0xa2808000;  //write

unsigned long slot_b_w_stat =  0xa2000000;
unsigned long slot_b_r_stat =  0xa2020000;
unsigned long slot_b_r_mem =   0xa2028000;
unsigned long slot_b_w_mem =   0xa2008000;
#endif

static struct ufs9xx_cic_core ci_core;
static struct ufs9xx_cic_state ci_state;
static struct mutex ufs9xx_cic_mutex;

#ifdef UFS922
void set_ts_path(int route);
void set_cam_path(int route);
#endif

//#define address_not_mapped

#ifdef UFS912
static int waitMS = 200; //100 enough for AClight but not for Diablo
#else
static int waitMS = 100;
#endif

/* *************************** */
/* map, write & read functions */
/* *************************** */

unsigned char ufs9xx_read_register_u8(unsigned long address)
{
    unsigned char result;

#ifdef address_not_mapped
    volatile unsigned long mapped_register = (unsigned long) ioremap_nocache(address, 1);
#else
    volatile unsigned long mapped_register = address;
#endif

    //dprintk(200, "%s > address = 0x%.8lx, mapped = 0x%.8lx\n", __FUNCTION__, (unsigned long) address, mapped_register);

    result = readb(mapped_register);
     
#ifdef address_not_mapped
    iounmap((void*) mapped_register);
#endif
    
    return result;
}

void ufs9xx_write_register_u8(unsigned long address, unsigned char value)
{
#ifdef address_not_mapped
    volatile unsigned long mapped_register = (unsigned long)  ioremap_nocache(address, 1);
#else
    volatile unsigned long mapped_register = address;
#endif

    writeb(value, mapped_register);
     
#ifdef address_not_mapped
    iounmap((void*) mapped_register);
#endif
}

void ufs9xx_write_register_u32(unsigned long address, unsigned int value)
{
#ifdef address_not_mapped
    volatile unsigned long mapped_register = (unsigned long) ioremap_nocache(address, 4);
#else
    volatile unsigned long mapped_register = address;
#endif

    writel(value, mapped_register);
     
#ifdef address_not_mapped
    iounmap((void*) mapped_register);
#endif
}


#ifdef UFS922
static int ufs9xx_cic_readN (struct ufs9xx_cic_state *state, u8 * buf, u16 len)
{
  int ret = -EREMOTEIO;
  struct i2c_msg msg;

  msg.addr = state->i2c_addr;
  msg.flags = I2C_M_RD;
  msg.buf = buf;
  msg.len = len;

  if ((ret = i2c_transfer (state->i2c, &msg, 1)) != 1)
  {
    printk ("%s: writereg error(err == %i)\n",
            __FUNCTION__, ret);
    ret = -EREMOTEIO;
  }

  return ret;
}

static int ufs9xx_cic_writereg(struct ufs9xx_cic_state* state, int reg, int data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = state->i2c_addr,
		.flags = 0, .buf = buf, .len = 2 };
	int err;

	if ((err = i2c_transfer(state->i2c, &msg, 1)) != 1) {
		printk("%s: writereg error(err == %i, reg == 0x%02x,"
			 " data == 0x%02x)\n", __FUNCTION__, err, reg, data);
		return -EREMOTEIO;
	}

	return 0;
}
#endif

static int ufs9xx_cic_poll_slot_status(struct dvb_ca_en50221 *ca, int slot, int open)
{
   struct ufs9xx_cic_state *state = ca->data;
   int                     slot_status = 0;
   unsigned int            result;
#ifdef UFS922
   u8                      buf[2];
#endif

   dprintk(100, "%s (%d; open = %d) >\n", __FUNCTION__, slot, open);

   mutex_lock(&ufs9xx_cic_mutex);

#ifdef UFS922
/* hacky workaround for stmfb problem (yes I really mean stmfb ;-) ):
 * switching the hdmi output of my lcd to the ufs922 while running or
 * switching the resolution leads to a modification of register
 * 0x00 which disable the stream input for ci controller. it seems so
 * that the ci controller has no bypass in this (and in any) case.
 * so we poll it here and set it back.
 */ 
	buf[0] = 0x00;
	ufs9xx_cic_readN (state, buf, 1);

	if (buf[0] != 0x3f)
	{
		printk("ALERT: CI Controller loosing input %02x\n", buf[0]);
		
		ufs9xx_cic_writereg(state, 0x00, 0x3f);
	}	
#endif

   result = stpio_get_pin(state->slot_status[slot]);

   dprintk(100, "Slot %d Status = 0x%x\n", slot, result);

   if (result == 0x01)
      slot_status = 1;

   if (slot_status != state->module_present[slot])
   {
	   if (slot_status)
	   {
		   stpio_set_pin(state->slot_enable[slot], 0);

                   mdelay(waitMS);

#ifdef UFS912
                   stpio_set_pin(state->slot_reset[slot], 1);

                   mdelay(waitMS);

	           stpio_set_pin(state->slot_reset[slot], 0);
                   
                   mdelay(waitMS);
#endif
		   dprintk(1, "Modul now present\n");
	           state->module_present[slot] = 1;
	   }
	   else
	   {
		   stpio_set_pin(state->slot_enable[slot], 1);
		   dprintk(1, "Modul now not present\n");
	           state->module_present[slot] = 0;

#ifdef UFS922
                   if ((state->module_present[0]) && (!state->module_present[1]))
	           {
	              set_cam_path(TUNER_1_CAM_A_VIEW);
	              set_ts_path(TUNER_1_CAM_A_VIEW);
	           } else
                   if ((!state->module_present[0]) && (state->module_present[1]))
	           {
	              set_cam_path(TUNER_1_CAM_B_VIEW);
	              set_ts_path(TUNER_1_CAM_B_VIEW);
	           } else
                   if ((state->module_present[0]) && (state->module_present[1]))
	           {
	              set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
	              set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
	           } else
                   if ((!state->module_present[0]) && (!state->module_present[1]))
	           {
	              set_cam_path(TUNER_1_VIEW);
	              set_ts_path(TUNER_1_VIEW);
	           }
#endif
	   }
   }

  /* Phantomias: an insertion should not be reported immediately
    because the module needs time to power up. Therefore the
    detection is reported after the module has been present for
    the specified period of time (to be confirmed in tests). */
  if(slot_status == 1)
  {
    if(state->module_ready[slot] == 0)
    {
      if(state->detection_timeout[slot] == 0)
      {
	/* detected module insertion, set the detection
	 *  timeout (500 ms) 
	 */
	state->detection_timeout[slot] = jiffies + HZ / 2;
      }
      else
      {
	/* timeout in progress */
	if(time_after(jiffies, state->detection_timeout[slot]))
	{
	  /* timeout expired, report module present */
	  state->module_ready[slot] = 1;
	}
      }
    }
    /* else: state->module_ready[slot] == 1 */
  }
  else
  {
    /* module not present, reset everything */
    state->module_ready[slot] = 0;
    state->detection_timeout[slot] = 0;
  }

  slot_status = slot_status ? DVB_CA_EN50221_POLL_CAM_PRESENT : 0;
slot_status |= DVB_CA_EN50221_POLL_CAM_READY;
/*
  if(state->module_ready[slot])
     slot_status |= DVB_CA_EN50221_POLL_CAM_READY;
*/
  dprintk(1, "Module %c (%d): present = %d, ready = %d\n",
			  slot ? 'B' : 'A', slot, slot_status,
			  state->module_ready[slot]);

  mutex_unlock(&ufs9xx_cic_mutex);
  return slot_status;
}

static int ufs9xx_cic_slot_reset(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs9xx_cic_state *state = ca->data;

	dprintk(1, "%s >\n", __FUNCTION__);

	mutex_lock(&ufs9xx_cic_mutex);

        /* reset status variables because module detection has to
         * be reported after a delay 
	 */
        state->module_ready[slot] = 0;
        state->module_present[slot] = 0;
        state->detection_timeout[slot] = 0;

#ifdef UFS922
        if ((state->module_present[0]) && (!state->module_present[1]))
	{
	   set_cam_path(TUNER_1_CAM_A_VIEW);
	   set_ts_path(TUNER_1_CAM_A_VIEW);
	} else
        if ((!state->module_present[0]) && (state->module_present[1]))
	{
	   set_cam_path(TUNER_1_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_B_VIEW);
	} else
        if ((state->module_present[0]) && (state->module_present[1]))
	{
	   set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
	} else
        if ((!state->module_present[0]) && (!state->module_present[1]))
	{
	   set_cam_path(TUNER_1_VIEW);
	   set_ts_path(TUNER_1_VIEW);
	}
#endif

        stpio_set_pin(state->slot_reset[slot], 1);
        mdelay(waitMS);
	stpio_set_pin(state->slot_reset[slot], 0);
        mdelay(waitMS);
#ifdef UFS912
        stpio_set_pin(state->slot_enable[slot], 1);
        mdelay(waitMS);
#endif

	mutex_unlock(&ufs9xx_cic_mutex);

	dprintk(1, "%s <\n", __FUNCTION__);
	return 0;
}

static int ufs9xx_cic_read_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address)
{
	unsigned char res = 0;

	mutex_lock(&ufs9xx_cic_mutex);

	if (slot == 0)
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, (unsigned long) slot_a_r_mem + address);
	   
	   res = ufs9xx_read_register_u8(slot_a_r_mem + address);

        } else
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, (unsigned long) slot_b_r_mem + address);

	   res = ufs9xx_read_register_u8(slot_b_r_mem + address);
	}

	if (address <= 2)
	   dprintk (100, "address = %d: res = 0x%.x\n", address, res);
	else
	{
	   if ((res > 31) && (res < 127))
		dprintk(100, "%c", res);
	   else
		dprintk(150, ".");
	}

	mutex_unlock(&ufs9xx_cic_mutex);
	return (int) res;
}

static int ufs9xx_cic_write_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address, u8 value)
{
	mutex_lock(&ufs9xx_cic_mutex);

	if (slot == 0)
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) slot_a_w_mem + address, value);
	   ufs9xx_write_register_u8(slot_a_w_mem + address, value);

        } else
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) slot_b_w_mem + address, value);
	   ufs9xx_write_register_u8(slot_b_w_mem + address, value);
	}

	mutex_unlock(&ufs9xx_cic_mutex);
	return 0;
}

static int ufs9xx_cic_read_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address)
{
	unsigned char res = 0;
	
	mutex_lock(&ufs9xx_cic_mutex);

	if (slot == 0)
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, (unsigned long) slot_a_r_stat + address);

	   res = ufs9xx_read_register_u8(slot_a_r_stat + address);

        } else
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, (unsigned long) slot_b_r_stat + address);
	   res = ufs9xx_read_register_u8(slot_b_r_stat + address);
	}

	if (address <= 2)
	   dprintk (100, "address = 0x%x: res = 0x%x (0x%x)\n", address, res, (int) res);
	else
	{
	   if ((res > 31) && (res < 127))
		dprintk(100, "%c", res);
	   else
		dprintk(150, ".");
	}

	mutex_unlock(&ufs9xx_cic_mutex);
	return (int) res;
}

static int ufs9xx_cic_write_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address, u8 value)
{
	mutex_lock(&ufs9xx_cic_mutex);

	if (slot == 0)
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) slot_a_w_stat + address, value);
	   ufs9xx_write_register_u8(slot_a_w_stat + address, value);

        } else
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) slot_b_w_stat + address, value);
	   ufs9xx_write_register_u8(slot_b_w_stat + address, value);
	}

	mutex_unlock(&ufs9xx_cic_mutex);
	return 0;
}

static int ufs9xx_cic_slot_shutdown(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs9xx_cic_state *state = ca->data;
	dprintk(20, "%s > slot = %d\n", __FUNCTION__, slot);
	mutex_lock(&ufs9xx_cic_mutex);

        stpio_set_pin(state->slot_enable[slot], 1);

#ifdef UFS922
        if ((state->module_present[0]) && (!state->module_present[1]))
	{
	   set_cam_path(TUNER_1_CAM_A_VIEW);
	   set_ts_path(TUNER_1_CAM_A_VIEW);
	} else
        if ((!state->module_present[0]) && (state->module_present[1]))
	{
	   set_cam_path(TUNER_1_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_B_VIEW);
	} else
        if ((state->module_present[0]) && (state->module_present[1]))
	{
	   set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
	} else
        if ((!state->module_present[0]) && (!state->module_present[1]))
	{
	   set_cam_path(TUNER_1_VIEW);
	   set_ts_path(TUNER_1_VIEW);
	}
#endif
	mutex_unlock(&ufs9xx_cic_mutex);
	return 0;
}

static int ufs9xx_cic_slot_ts_enable(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs9xx_cic_state *state = ca->data;

	dprintk(20, "%s > slot = %d\n", __FUNCTION__, slot);

#ifdef UFS922
        if ((state->module_present[0]) && (!state->module_present[1]))
	{
	   set_cam_path(TUNER_1_CAM_A_VIEW);
	   set_ts_path(TUNER_1_CAM_A_VIEW);
	} else
        if ((!state->module_present[0]) && (state->module_present[1]))
	{
	   set_cam_path(TUNER_1_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_B_VIEW);
	} else
        if ((state->module_present[0]) && (state->module_present[1]))
	{
	   set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
	} else
        if ((!state->module_present[0]) && (!state->module_present[1]))
	{
	   set_cam_path(TUNER_1_VIEW);
	   set_ts_path(TUNER_1_VIEW);
	}
#endif

	return 0;
}

int setCiSource(int slot, int source)
{
#ifdef UFS922
/* FIXME: Slot Shutdown must be modified, because otherwise
 * removing a module will overwrite this settings here
 * ->and also ts_enabled
 */
   printk("%s slot %d source %d\n", __FUNCTION__, slot, source);
  
   if (slot == 0)
   {
 	  ci_state.module_a_source = source;
   } else
   {
 	  ci_state.module_b_source = source;
   }

   if (source == 0)
   {
      if ((ci_state.module_present[0]) && (!ci_state.module_present[1]))
      {
	  set_cam_path(TUNER_1_CAM_A_VIEW);
	  set_ts_path(TUNER_1_CAM_A_VIEW);
      } else
      if ((!ci_state.module_present[0]) && (ci_state.module_present[1]))
      {
	 set_cam_path(TUNER_1_CAM_B_VIEW);
	 set_ts_path(TUNER_1_CAM_B_VIEW);
      } else
      if ((ci_state.module_present[0]) && (ci_state.module_present[1]))
      {
	 set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
	 set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
      } else
      if ((!ci_state.module_present[0]) && (!ci_state.module_present[1]))
      {
	 set_cam_path(TUNER_1_VIEW);
	 set_ts_path(TUNER_1_VIEW);
      }
   } else
   {
      if ((ci_state.module_present[0]) && (!ci_state.module_present[1]))
      {
	  set_cam_path(TUNER_2_CAM_A);
	  set_ts_path(TUNER_2_CAM_A);
      } else
      if ((!ci_state.module_present[0]) && (ci_state.module_present[1]))
      {
	 set_cam_path(TUNER_2_CAM_B);
	 set_ts_path(TUNER_2_CAM_B);
      } else
      if ((ci_state.module_present[0]) && (ci_state.module_present[1]))
      {
	 set_cam_path(TUNER_2_CAM_A_B);
	 if (slot == 0) 
	    set_ts_path(TUNER_2_CAM_A);
         else
	    set_ts_path(TUNER_2_CAM_B);
      } else
      if ((!ci_state.module_present[0]) && (!ci_state.module_present[1]))
      {
/* ??? */
	 set_ts_path(TUNER_2_VIEW);
      }
   }
#endif
   return 0;
}

void getCiSource(int slot, int* source)
{
#ifdef UFS922
   if (slot == 0)
   {
 	  *source = ci_state.module_a_source;
   } else
   {
 	  *source = ci_state.module_b_source;
   }
#else
   *source = 0;
#endif
}

#ifdef UFS922
void set_cam_path(int route)
{
	struct ufs9xx_cic_state *state = &ci_state;

	switch (route)
	{
		case TUNER_1_VIEW:
		   printk("%s: TUNER_1_VIEW\n", __func__);
		   ufs9xx_cic_writereg(state, 0x02, 0x00);
		break;
		case TUNER_1_CAM_A_VIEW:
		   printk("%s: TUNER_1_CAM_A_VIEW\n", __func__);
		   ufs9xx_cic_writereg(state, 0x02, 0x01);
		break;
		case TUNER_1_CAM_B_VIEW:
		   printk("%s: TUNER_1_CAM_B_VIEW\n", __func__);
		   ufs9xx_cic_writereg(state, 0x02, 0x10);
		break;
		case TUNER_1_CAM_A_CAM_B_VIEW:
		   printk("%s: TUNER_1_CAM_A_CAM_B_VIEW\n", __func__);
//FIXME: maruapp sets first 0x11 and then 0x14 ???
		   ufs9xx_cic_writereg(state, 0x02, 0x14);
		break;
		case TUNER_2_CAM_A:
		   printk("%s: TUNER_2_CAM_A\n", __func__);
		   ufs9xx_cic_writereg(state, 0x02, 0x12);
		break;
		case TUNER_2_CAM_B:
		   printk("%s: TUNER_2_CAM_B\n", __func__);
		   ufs9xx_cic_writereg(state, 0x02, 0x20);
		break;
		case TUNER_2_CAM_A_B:
/* fixme maurapp sets first 0x22 */
		   printk("%s: TUNER_2_CAM_A_B\n", __func__);
		   ufs9xx_cic_writereg(state, 0x02, 0x24);
		break;
		default:
		   printk("%s: TUNER_1_VIEW\n", __func__);
		   ufs9xx_cic_writereg(state, 0x02, 0x00);
		break;
	}
}


void set_ts_path(int route)
{
	struct ufs9xx_cic_state *state = &ci_state;

	switch (route)
	{
		case TUNER_1_VIEW:
		   printk("%s: TUNER_1_VIEW\n", __func__);
		   ufs9xx_cic_writereg(state, 0x01, 0x21);
		break;
		case TUNER_1_CAM_A_VIEW:
		   printk("%s: TUNER_1_CAM_A_VIEW\n", __func__);
		   ufs9xx_cic_writereg(state, 0x01, 0x23);
		break;
		case TUNER_1_CAM_B_VIEW:
		   printk("%s: TUNER_1_CAM_B_VIEW\n", __func__);
		   ufs9xx_cic_writereg(state, 0x01, 0x24);
		break;
		case TUNER_1_CAM_A_CAM_B_VIEW:
		   ufs9xx_cic_writereg(state, 0x01, 0x23);
		   printk("%s: TUNER_1_CAM_A_CAM_B_VIEW\n", __func__);
		break;
		case TUNER_2_VIEW:
/* fixme: maruapp sets sometimes 0x11 before */
		   ufs9xx_cic_writereg(state, 0x01, 0x12);
		   printk("%s: TUNER_2_VIEW\n", __func__);
		break;
		case TUNER_2_CAM_A:
		   ufs9xx_cic_writereg(state, 0x01, 0x31);
		   printk("%s: TUNER_2_CAM_A\n", __func__);
		break;
		case TUNER_2_CAM_B:
		   ufs9xx_cic_writereg(state, 0x01, 0x41);
		   printk("%s: TUNER_2_CAM_B\n", __func__);
		break;
		default:
		   printk("%s: TUNER_1_VIEW\n", __func__);
		   ufs9xx_cic_writereg(state, 0x01, 0x21);
		break;
	}
}
#endif

#if defined(UFS912)
int cic_init_hw_ufs912(void)
{
	struct ufs9xx_cic_state *state = &ci_state;
	
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMI_LCK, 0x00);
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMI_GEN_CFG, 0x18);
	
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA0, 0x04f446d9);
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA1, 0xfd44ffff);
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA2, 0xfd88ffff);
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA3, 0x00000000);
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA0, 0x04f446d9);
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA1, 0xfd44ffff);
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA2, 0xfd88ffff);
        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA3, 0x00000000);

        ufs9xx_write_register_u32(EMIConfigBaseAddress + EMI_LCK, 0x1f);
   
        state->module_ready[0] = 0;
        state->module_ready[1] = 0;
        state->module_present[0] = 0;
        state->module_present[1] = 0;

        state->detection_timeout[0] = 0;
        state->detection_timeout[1] = 0;

	/* will be set to one if a module is present in slot a/b.
	 */
	state->slot_status[0] = stpio_request_pin (6, 0, "SLOT_A_STA", STPIO_IN);
	state->slot_status[1] = stpio_request_pin (6, 1, "SLOT_B_STA", STPIO_IN);

	/* these one will set and then cleared if a module is presented
	 * or for reset purpose. in our case its ok todo this only 
	 * in reset function because it will be called after a module
	 * is inserted (in e2 case, if other applications does not do
	 * this we must set and clear it also in the poll function).
	 */
/* fixme: not sure here */
	state->slot_reset[0] = stpio_request_pin (6, 2, "SLOT_A", STPIO_OUT);
	state->slot_reset[1] = stpio_request_pin (6, 3, "SLOT_B", STPIO_OUT);

	/* must be cleared when a module is present and vice versa
	 */
	state->slot_enable[0] = stpio_request_pin (6, 4, "SLOT_A_EN", STPIO_OUT);
	state->slot_enable[1] = stpio_request_pin (6, 5, "SLOT_B_EN", STPIO_OUT);
	
	stpio_set_pin(state->slot_enable[0], 0);
	mdelay(50);
	stpio_set_pin(state->slot_enable[0], 1);
	
	stpio_set_pin(state->slot_enable[1], 0);
	mdelay(50);
	stpio_set_pin(state->slot_enable[1], 1);

	return 0;
}
#elif defined(UFS922)
int cic_init_hw_ufs922(void)
{
        struct stpio_pin* dummy_pin;
        struct stpio_pin* enable_pin;
	struct ufs9xx_cic_state *state = &ci_state;
	
	//first init emi settings
	reg_config = (unsigned long) ioremap(EMIConfigBaseAddress, 0x7ff);

	ctrl_outl(0x0, reg_config + EMI_LCK);
	ctrl_outl(0x18, reg_config + EMI_GEN_CFG);
	ctrl_outl(0x2, reg_config + EMI_FLASH_CLK_SEL);
	ctrl_outl(0x1, reg_config + EMI_CLK_EN);

	ctrl_outl(0x04f446d9,reg_config + EMIBank2+ EMI_CFG_DATA0);
	ctrl_outl(0xfd44ffff,reg_config + EMIBank2+ EMI_CFG_DATA1);
	ctrl_outl(0xfd88ffff,reg_config + EMIBank2+ EMI_CFG_DATA2);
	ctrl_outl(0x00000000,reg_config + EMIBank2+ EMI_CFG_DATA3);
	ctrl_outl(0x04f446d9,reg_config + EMIBank3+ EMI_CFG_DATA0);
	ctrl_outl(0xfd44ffff,reg_config + EMIBank3+ EMI_CFG_DATA1);
	ctrl_outl(0xfd88ffff,reg_config + EMIBank3+ EMI_CFG_DATA2);
	ctrl_outl(0x00000000,reg_config + EMIBank3+ EMI_CFG_DATA3);
	ctrl_outl(0x04f47ed1,reg_config + EMIBank4+ EMI_CFG_DATA0);
	ctrl_outl(0x9e113f3f,reg_config + EMIBank4+ EMI_CFG_DATA1);
	ctrl_outl(0x98339999,reg_config + EMIBank4+ EMI_CFG_DATA2);
	ctrl_outl(0x00000000,reg_config + EMIBank4+ EMI_CFG_DATA3);

	ctrl_outl(0x1F,reg_config + EMI_LCK);

        state->module_ready[0] = 0;
        state->module_ready[1] = 0;
        state->module_present[0] = 0;
        state->module_present[1] = 0;

        state->detection_timeout[0] = 0;
        state->detection_timeout[1] = 0;

	/* enables the ci controller itself */	
	enable_pin = stpio_request_pin (3, 4, "CI_enable", STPIO_OUT);

	/* these one goes to 0 for the time the cam is busy otherwise they are 1 */
	dummy_pin = stpio_request_pin (0, 1, "SLOT_A_BUSY", STPIO_IN);
	dummy_pin = stpio_request_pin (0, 2, "SLOT_B_BUSY", STPIO_IN);

	/* will be set to one if a module is present in slot a/b.
	 */
	state->slot_status[0] = stpio_request_pin (0, 0, "SLOT_A_STA", STPIO_IN);
	state->slot_status[1] = stpio_request_pin (2, 6, "SLOT_B_STA", STPIO_IN);

	/* these one will set and then cleared if a module is presented
	 * or for reset purpose. in our case its ok todo this only 
	 * in reset function because it will be called after a module
	 * is inserted (in e2 case, if other applications does not do
	 * this we must set and clear it also in the poll function).
	 */
	state->slot_reset[0] = stpio_request_pin (5, 4, "SLOT_A", STPIO_OUT);
	state->slot_reset[1] = stpio_request_pin (5, 5, "SLOT_B", STPIO_OUT);

	/* must be cleared when a module is present and vice versa
	 * ->setting this bit during runtime gives output from maruapp
	 * isBusy ...
	 */
	state->slot_enable[0] = stpio_request_pin (5, 2, "SLOT_A_EN", STPIO_OUT);
	state->slot_enable[1] = stpio_request_pin (5, 3, "SLOT_B_EN", STPIO_OUT);
	
	//reset fpga charon
	printk("reset fpga charon\n");
	
	stpio_set_pin(enable_pin, 0);
	mdelay(50); //necessary?
	stpio_set_pin(enable_pin, 1);

	set_ts_path(TUNER_1_VIEW);
	set_cam_path(TUNER_1_VIEW);

	return 0;
}
#endif

int cic_init_hw(void)
{
#if defined(UFS912)
     return cic_init_hw_ufs912();
#elif defined(UFS922)
     return cic_init_hw_ufs922();
#endif
     return -1;
}

EXPORT_SYMBOL(cic_init_hw);

int init_ci_controller(struct dvb_adapter* dvb_adap)
{
	struct ufs9xx_cic_state *state = &ci_state;
	struct ufs9xx_cic_core *core = &ci_core;
	int result;

	printk("init_ufs9xx_cic >\n");

        mutex_init (&ufs9xx_cic_mutex);

	core->dvb_adap = dvb_adap;

#ifdef UFS922
	state->i2c = i2c_get_adapter(2);
	state->i2c_addr = 0x23;
#endif

	memset(&core->ca, 0, sizeof(struct dvb_ca_en50221));

	/* register CI interface */
	core->ca.owner = THIS_MODULE;

	core->ca.read_attribute_mem 	= ufs9xx_cic_read_attribute_mem;
	core->ca.write_attribute_mem 	= ufs9xx_cic_write_attribute_mem;
	core->ca.read_cam_control 	= ufs9xx_cic_read_cam_control;
	core->ca.write_cam_control 	= ufs9xx_cic_write_cam_control;
	core->ca.slot_shutdown 		= ufs9xx_cic_slot_shutdown;
	core->ca.slot_ts_enable 	= ufs9xx_cic_slot_ts_enable;

	core->ca.slot_reset 		= ufs9xx_cic_slot_reset;
	core->ca.poll_slot_status 	= ufs9xx_cic_poll_slot_status;

	state->core 			= core;
	core->ca.data 			= state;

	cic_init_hw();
	
#if !defined(address_not_mapped) && !defined(UFS922)
        slot_a_r_mem  =	(volatile unsigned long) ioremap(slot_a_r_mem, 0x200);
        slot_a_w_mem  =	(volatile unsigned long) ioremap(slot_a_w_mem, 0x200);
        slot_a_w_stat = (volatile unsigned long) ioremap(slot_a_w_stat, 0x200);
        slot_a_r_stat = (volatile unsigned long) ioremap(slot_a_r_stat, 0x200);

        slot_b_r_mem  =	(volatile unsigned long) ioremap(slot_b_r_mem, 0x200);
        slot_b_w_mem  =	(volatile unsigned long) ioremap(slot_b_w_mem, 0x200);
        slot_b_w_stat = (volatile unsigned long) ioremap(slot_b_w_stat, 0x200);
        slot_b_r_stat = (volatile unsigned long) ioremap(slot_b_r_stat, 0x200);
#endif
	
	dprintk(1, "init_ufs9xx_cic: call dvb_ca_en50221_init\n");

	if ((result = dvb_ca_en50221_init(core->dvb_adap,
					  &core->ca, 0, 2)) != 0) {
		printk(KERN_ERR "ca0 initialisation failed.\n");
		goto error;
	}

	dprintk(1, "ufs9xx_cic: ca0 interface initialised.\n");

	dprintk(10, "init_ufs9xx_cic <\n");

	return 0;

error:

	printk("init_ufs9xx_cic < error\n");

	return result;
}

EXPORT_SYMBOL(init_ci_controller);
EXPORT_SYMBOL(setCiSource);
EXPORT_SYMBOL(getCiSource);

int __init ufs9xx_cic_init(void)
{
    printk("ufs9xx_cic loaded\n");
    return 0;
}

static void __exit ufs9xx_cic_exit(void)
{  
   printk("ufs9xx_cic unloaded\n");
}

module_init             (ufs9xx_cic_init);
module_exit             (ufs9xx_cic_exit);

MODULE_DESCRIPTION      ("CI Controller");
MODULE_AUTHOR           ("Dagobert");
MODULE_LICENSE          ("GPL");

module_param(debug, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(debug, "Debug Output 0=disabled >0=enabled(debuglevel)");

module_param(waitMS, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(waitMS, "waiting time between pio settings for reset/enable purpos in milliseconds (default=200)");
