#ifdef UFS922
/*
 * ufs922 ci controller handling. currently I dont know which
 * ci controller this is ;-)
 *
 * So the "controller" seems to be a self constructed one. The
 * communication to the modules is done via EMI. The stream
 * routing is controlled via i2c-2 0x23.The internal name in
 * the logfiles is FPGACharon. I think it will be controlled
 * by the Actel ProASIC3.
 *
 * Registers:
 *
 * 0x01 ->Input Control Register
 * 0x02 ->CAM Routing Control Register
 *
 * Dagobert
 * GPL
 * 2009
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

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include <asm/system.h>
#include <asm/io.h>
#include <asm/semaphore.h>
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

#include "ufs922_cic.h"

void set_ts_path(int route);
void set_cam_path(int route);

static int debug=0;
#define dprintk(args...) \
	do { \
		if (debug) printk (args); \
	} while (0)


/* emi */
#define EMIConfigBaseAddress 0x1A100000
#define EMI_STA_CFG	0x0010
#define EMI_STA_LCK 	0x0018
#define EMI_LCK 	0x0020
/* general purpose config register
 * 32Bit, R/W, reset=0x00
 * Bit 31-5 reserved
 * Bit 4 = PCCB4_EN: Enable PC card bank 4
 * Bit 3 = PCCB3_EN: Enable PC card bank 3 
 * Bit 2 = EWAIT_RETIME
 * Bit 1/0 reserved
 */
#define EMI_GEN_CFG 	0x0028

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

/* fixme: to be continued */

unsigned long slot_a_command = 0xa2800000;  //write
unsigned long slot_a_status =  0xa2820000;  //read
unsigned long slot_a_mem =     0xa2828000;  //read
unsigned long slot_a_w_mem =   0xa2808000;  //write

unsigned long slot_b_command = 0xa2000000;
unsigned long slot_b_status =  0xa2020000;
unsigned long slot_b_mem =     0xa2028000;
unsigned long slot_b_w_mem =   0xa2008000;

static struct ufs922_cic_core ci_core;
static struct ufs922_cic_state ci_state;
static struct mutex ufs922_cic_mutex;

static int
ufs922_cic_readN (struct ufs922_cic_state *state, u8 * buf, u16 len)
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

static int ufs922_cic_writereg(struct ufs922_cic_state* state, int reg, int data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = state->i2c_addr,
		.flags = 0, .buf = buf, .len = 2 };
	int err;

#ifdef my_debug
		dprintk("ufs922_cic: %s:  write reg 0x%02x, value 0x%02x\n",
						__FUNCTION__,reg, data);

#endif

	if ((err = i2c_transfer(state->i2c, &msg, 1)) != 1) {
		dprintk("%s: writereg error(err == %i, reg == 0x%02x,"
			 " data == 0x%02x)\n", __FUNCTION__, err, reg, data);
		return -EREMOTEIO;
	}

	return 0;
}

static int ufs922_cic_poll_slot_status(struct dvb_ca_en50221 *ca, int slot, int open)
{
	struct ufs922_cic_state *state = ca->data;
	int    slot_status = 0;
	u8     buf[2];

	dprintk("%s (%d; open = %d) >\n", __FUNCTION__, slot, open);

        mutex_lock(&ufs922_cic_mutex);

/* hacky workaround for stmfb problem (yes I really mean stmfb ;-) ):
 * switching the hdmi output of my lcd to the ufs922 while running or
 * switching the resolution leads to a modification of register
 * 0x00 which disable the stream input for ci controller. it seems so
 * that the ci controller has no bypass in this (and in any) case.
 * so we poll it here and set it back.
 */ 
	buf[0] = 0x00;
	ufs922_cic_readN (state, buf, 1);

	if (buf[0] != 0x3f)
	{
		printk("ALERT: CI Controller loosing input %02x\n", buf[0]);
		
		ufs922_cic_writereg(state, 0x00, 0x3f);
	}	
		
	if (slot == 0)
	{
		unsigned int result = stpio_get_pin(state->slot_a_status);

		dprintk("Slot A Status = 0x%x\n", result);
		
		if (result == 0x01)
		   slot_status = 1;

		if (slot_status != state->module_a_present)
		{
			if (slot_status)
			{
				stpio_set_pin(state->slot_a_enable, 0);
		   		dprintk(KERN_ERR "Modul A present\n");
			}
			else
		   	{
				stpio_set_pin(state->slot_a_enable, 1);
				dprintk(KERN_ERR "Modul A not present\n");
			}
			
			state->module_a_present = slot_status;
		}
	}
	else
	{
		unsigned int result = stpio_get_pin(state->slot_b_status);

		dprintk("Slot B Status = 0x%x\n", result);
		
		if (result == 0x01)
		   slot_status = 1;

		if (slot_status != state->module_b_present)
		{
			if (slot_status)
			{
				stpio_set_pin(state->slot_b_enable, 0);
		   		dprintk(KERN_ERR "Modul B present\n");
			}
			else
			{
				stpio_set_pin(state->slot_b_enable, 1);
		   		dprintk(KERN_ERR "Modul B not present\n");
			}
			
			state->module_b_present = slot_status;
		}
	}

	if (slot_status == 1)
	{
#ifdef my_debug	
		dprintk("%s <\n", __FUNCTION__);

#endif
   		mutex_unlock(&ufs922_cic_mutex);
		return DVB_CA_EN50221_POLL_CAM_PRESENT | DVB_CA_EN50221_POLL_CAM_READY;
	}
#ifdef my_debug	
	dprintk("%s <\n", __FUNCTION__);
#endif
   	mutex_unlock(&ufs922_cic_mutex);
	return 0;
}

static int ufs922_cic_slot_reset(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs922_cic_state *state = ca->data;

	printk("%s >\n", __FUNCTION__);

	mutex_lock(&ufs922_cic_mutex);

	if (slot == 0)
	{
		stpio_set_pin(state->slot_a, 1);
		mdelay(50);
		stpio_set_pin(state->slot_a, 0);
	} else
	{
		stpio_set_pin(state->slot_b, 1);
		mdelay(50);
		stpio_set_pin(state->slot_b, 0);
	}

	mdelay(50);
	dprintk("%s <\n", __FUNCTION__);

	mutex_unlock(&ufs922_cic_mutex);
	return 0;
}

static int ufs922_cic_read_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address)
{
	struct ufs922_cic_state *state = ca->data;
	int res = 0;

	mutex_lock(&ufs922_cic_mutex);

	if (slot == 0)
	{
	   stpio_set_pin(state->slot_a, 0);

	   dprintk("%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, slot_a_mem + address);
	   
	   res = ctrl_inb(slot_a_mem + (address));

           res &= 0x00FF;

        } else
	{
	   stpio_set_pin(state->slot_b, 0);

	   dprintk("%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, slot_b_mem + address);

	   res = ctrl_inb(slot_b_mem + (address));

           res &= 0x00FF;
	}

	if (address <= 2)
	   dprintk ("address = %d: res = 0x%.x\n", address, res);
	else
	{
	   if ((res > 31) && (res < 127))
		dprintk("%c", res);
	   else
		dprintk(".");
	}

	mutex_unlock(&ufs922_cic_mutex);
	return res;
}

static int ufs922_cic_write_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address, u8 value)
{
	mutex_lock(&ufs922_cic_mutex);

	if (slot == 0)
	{
	   dprintk("%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, slot_a_w_mem + address, value);
	   ctrl_outb(value, slot_a_w_mem + (address));

        } else
	{
	   dprintk("%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, slot_b_w_mem + address, value);
	   ctrl_outb(value, slot_b_w_mem + (address));
	}

	mutex_unlock(&ufs922_cic_mutex);
	return 0;
}

static int ufs922_cic_read_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address)
{
	int res = 0;
	
	mutex_lock(&ufs922_cic_mutex);

	if (slot == 0)
	{
	   dprintk("%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, slot_a_status + address);

	   res = ctrl_inb(slot_a_status + (address));

           res &= 0x00FF;

        } else
	{
	   dprintk("%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, slot_b_status + address);
	   res = ctrl_inb(slot_b_status + (address));

           res &= 0x00FF;
	}

	if (address <= 2)
	   dprintk ("address = 0x%x: res = 0x%x\n", (address), res);
	else
	{
	   if ((res > 31) && (res < 127))
		dprintk("%c", res);
	   else
		dprintk(".");
	}

	mutex_unlock(&ufs922_cic_mutex);
	return res;
}

static int ufs922_cic_write_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address, u8 value)
{
	mutex_lock(&ufs922_cic_mutex);

	if (slot == 0)
	{
	   dprintk("%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, slot_a_command + address, value);
	   ctrl_outb(value, slot_a_command + (address));

        } else
	{
	   dprintk("%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, slot_b_command + address, value);
	   ctrl_outb(value, slot_b_command + (address));
	}

	mutex_unlock(&ufs922_cic_mutex);
	return 0;
}

static int ufs922_cic_slot_shutdown(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs922_cic_state *state = ca->data;
	dprintk("%s > slot = %d\n", __FUNCTION__, slot);
	mutex_lock(&ufs922_cic_mutex);

        if ((state->module_a_present) && (!state->module_b_present))
	{
	   set_cam_path(TUNER_1_CAM_A_VIEW);
	   set_ts_path(TUNER_1_CAM_A_VIEW);
	} else
        if ((!state->module_a_present) && (state->module_b_present))
	{
	   set_cam_path(TUNER_1_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_B_VIEW);
	} else
        if ((state->module_a_present) && (state->module_b_present))
	{
	   set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
	} else
        if ((!state->module_a_present) && (!state->module_b_present))
	{
	   set_cam_path(TUNER_1_VIEW);
	   set_ts_path(TUNER_1_VIEW);
	}

	mutex_unlock(&ufs922_cic_mutex);
	return 0;
}

static int ufs922_cic_slot_ts_enable(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs922_cic_state *state = ca->data;

	dprintk("%s > slot = %d\n", __FUNCTION__, slot);

	mutex_lock(&ufs922_cic_mutex);

        if ((state->module_a_present) && (!state->module_b_present))
	{
	   set_cam_path(TUNER_1_CAM_A_VIEW);
	   set_ts_path(TUNER_1_CAM_A_VIEW);
	} else
        if ((!state->module_a_present) && (state->module_b_present))
	{
	   set_cam_path(TUNER_1_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_B_VIEW);
	} else
        if ((state->module_a_present) && (state->module_b_present))
	{
	   set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
	   set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
	} else
        if ((!state->module_a_present) && (!state->module_b_present))
	{
	   set_cam_path(TUNER_1_VIEW);
	   set_ts_path(TUNER_1_VIEW);
	}
	
	mutex_unlock(&ufs922_cic_mutex);
	return 0;
}

/* This function sets the CI source
   Params:
     slot - CI slot number (0|1)
     source - tuner number (0|1)
*/
int setCiSource(int slot, int source)
{
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
      if ((ci_state.module_a_present) && (!ci_state.module_b_present))
      {
	  set_cam_path(TUNER_1_CAM_A_VIEW);
	  set_ts_path(TUNER_1_CAM_A_VIEW);
      } else
      if ((!ci_state.module_a_present) && (ci_state.module_b_present))
      {
	 set_cam_path(TUNER_1_CAM_B_VIEW);
	 set_ts_path(TUNER_1_CAM_B_VIEW);
      } else
      if ((ci_state.module_a_present) && (ci_state.module_b_present))
      {
	 set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
	 set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
      } else
      if ((!ci_state.module_a_present) && (!ci_state.module_b_present))
      {
	 set_cam_path(TUNER_1_VIEW);
	 set_ts_path(TUNER_1_VIEW);
      }
   } else
   {
      if ((ci_state.module_a_present) && (!ci_state.module_b_present))
      {
	  set_cam_path(TUNER_2_CAM_A);
	  set_ts_path(TUNER_2_CAM_A);
      } else
      if ((!ci_state.module_a_present) && (ci_state.module_b_present))
      {
	 set_cam_path(TUNER_2_CAM_B);
	 set_ts_path(TUNER_2_CAM_B);
      } else
      if ((ci_state.module_a_present) && (ci_state.module_b_present))
      {
	 set_cam_path(TUNER_2_CAM_A_B);
	 if (slot == 0) 
	    set_ts_path(TUNER_2_CAM_A);
         else
	    set_ts_path(TUNER_2_CAM_B);
      } else
      if ((!ci_state.module_a_present) && (!ci_state.module_b_present))
      {
/* ??? */
	 set_ts_path(TUNER_2_VIEW);
      }
   }
   return 0;
}

void getCiSource(int slot, int* source)
{
   if (slot == 0)
   {
 	  *source = ci_state.module_a_source;
   } else
   {
 	  *source = ci_state.module_b_source;
   }
}

void set_cam_path(int route)
{
	struct ufs922_cic_state *state = &ci_state;

	switch (route)
	{
		case TUNER_1_VIEW:
		   printk("%s: TUNER_1_VIEW\n", __func__);
		   ufs922_cic_writereg(state, 0x02, 0x00);
		break;
		case TUNER_1_CAM_A_VIEW:
		   printk("%s: TUNER_1_CAM_A_VIEW\n", __func__);
		   ufs922_cic_writereg(state, 0x02, 0x01);
		break;
		case TUNER_1_CAM_B_VIEW:
		   printk("%s: TUNER_1_CAM_B_VIEW\n", __func__);
		   ufs922_cic_writereg(state, 0x02, 0x10);
		break;
		case TUNER_1_CAM_A_CAM_B_VIEW:
		   printk("%s: TUNER_1_CAM_A_CAM_B_VIEW\n", __func__);
//FIXME: maruapp sets first 0x11 and then 0x14 ???
		   ufs922_cic_writereg(state, 0x02, 0x14);
		break;
		case TUNER_2_CAM_A:
		   printk("%s: TUNER_2_CAM_A\n", __func__);
		   ufs922_cic_writereg(state, 0x02, 0x12);
		break;
		case TUNER_2_CAM_B:
		   printk("%s: TUNER_2_CAM_B\n", __func__);
		   ufs922_cic_writereg(state, 0x02, 0x20);
		break;
		case TUNER_2_CAM_A_B:
/* fixme maurapp sets first 0x22 */
		   printk("%s: TUNER_2_CAM_A_B\n", __func__);
		   ufs922_cic_writereg(state, 0x02, 0x24);
		break;
		default:
		   printk("%s: TUNER_1_VIEW\n", __func__);
		   ufs922_cic_writereg(state, 0x02, 0x00);
		break;
	}
}


void set_ts_path(int route)
{
	struct ufs922_cic_state *state = &ci_state;

	switch (route)
	{
		case TUNER_1_VIEW:
		   printk("%s: TUNER_1_VIEW\n", __func__);
		   ufs922_cic_writereg(state, 0x01, 0x21);
		break;
		case TUNER_1_CAM_A_VIEW:
		   printk("%s: TUNER_1_CAM_A_VIEW\n", __func__);
		   ufs922_cic_writereg(state, 0x01, 0x23);
		break;
		case TUNER_1_CAM_B_VIEW:
		   printk("%s: TUNER_1_CAM_B_VIEW\n", __func__);
		   ufs922_cic_writereg(state, 0x01, 0x24);
		break;
		case TUNER_1_CAM_A_CAM_B_VIEW:
		   ufs922_cic_writereg(state, 0x01, 0x23);
		   printk("%s: TUNER_1_CAM_A_CAM_B_VIEW\n", __func__);
		break;
		case TUNER_2_VIEW:
/* fixme: maruapp sets sometimes 0x11 before */
		   ufs922_cic_writereg(state, 0x01, 0x12);
		   printk("%s: TUNER_2_VIEW\n", __func__);
		break;
		case TUNER_2_CAM_A:
		   ufs922_cic_writereg(state, 0x01, 0x31);
		   printk("%s: TUNER_2_CAM_A\n", __func__);
		break;
		case TUNER_2_CAM_B:
		   ufs922_cic_writereg(state, 0x01, 0x41);
		   printk("%s: TUNER_2_CAM_B\n", __func__);
		break;
		default:
		   printk("%s: TUNER_1_VIEW\n", __func__);
		   ufs922_cic_writereg(state, 0x01, 0x21);
		break;
	}
}

int cic_init_hw(void)
{
	struct ufs922_cic_state *state = &ci_state;
	
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

	/* for external reset purpose look if they are
	 * already requested.
	 */
	if (state->enable_pin == NULL)
	{
	   /* enables the ci controller itself */	
	   state->enable_pin = stpio_request_pin (3, 4, "CI_enable", STPIO_OUT);

	   /* these one goes to 0 for the time the cam is busy otherwise they are 1 */
	   state->slot_a_busy = stpio_request_pin (0, 1, "SLOT_A_BUSY", STPIO_IN);
	   state->slot_b_busy = stpio_request_pin (0, 2, "SLOT_B_BUSY", STPIO_IN);

	   /* will be set to one if a module is present in slot a/b.
	    */
	   state->slot_a_status = stpio_request_pin (0, 0, "SLOT_A_STA", STPIO_IN);
	   state->slot_b_status = stpio_request_pin (2, 6, "SLOT_B_STA", STPIO_IN);

	   /* these one will set and then cleared if a module is presented
	    * or for reset purpose. in our case its ok todo this only 
	    * in reset function because it will be called after a module
	    * is inserted (in e2 case, if other applications does not do
	    * this we must set and clear it also in the poll function).
	    */
	   state->slot_a = stpio_request_pin (5, 4, "SLOT_A", STPIO_OUT);
	   state->slot_b = stpio_request_pin (5, 5, "SLOT_B", STPIO_OUT);

	   /* must be cleared when a module is present and vice versa
	    * ->setting this bit during runtime gives output from maruapp
	    * isBusy ...
	    */
	   state->slot_a_enable = stpio_request_pin (5, 2, "SLOT_A_EN", STPIO_OUT);
	   state->slot_b_enable = stpio_request_pin (5, 3, "SLOT_B_EN", STPIO_OUT);
	}
	
	stpio_set_pin(state->slot_a, 1);
	stpio_set_pin(state->slot_b, 1);
	
	//reset fpga charon
	printk("reset fpga charon\n");
	
	stpio_set_pin(state->enable_pin, 0);
	mdelay(50); //necessary?
	stpio_set_pin(state->enable_pin, 1);

	set_ts_path(TUNER_1_VIEW);
	set_cam_path(TUNER_1_VIEW);

	return 0;
}

EXPORT_SYMBOL(cic_init_hw);

int init_ci_controller(struct dvb_adapter* dvb_adap)
{
	struct ufs922_cic_state *state = &ci_state;
	struct ufs922_cic_core *core = &ci_core;
	int result;

	printk("init_ufs922_cic >\n");

        mutex_init (&ufs922_cic_mutex);

	core->dvb_adap = dvb_adap;
	state->i2c = i2c_get_adapter(2);
	state->i2c_addr = 0x23;

	memset(&core->ca, 0, sizeof(struct dvb_ca_en50221));

	/* register CI interface */
	core->ca.owner = THIS_MODULE;

	core->ca.read_attribute_mem 	= ufs922_cic_read_attribute_mem;
	core->ca.write_attribute_mem 	= ufs922_cic_write_attribute_mem;
	core->ca.read_cam_control 	= ufs922_cic_read_cam_control;
	core->ca.write_cam_control 	= ufs922_cic_write_cam_control;
	core->ca.slot_shutdown 		= ufs922_cic_slot_shutdown;
	core->ca.slot_ts_enable 	= ufs922_cic_slot_ts_enable;

	core->ca.slot_reset 		= ufs922_cic_slot_reset;
	core->ca.poll_slot_status 	= ufs922_cic_poll_slot_status;

	state->core 			= core;
	core->ca.data 			= state;

	state->module_a_present = 0;
	state->module_b_present = 0;

	state->module_b_source = 0; /* tuner 1 */
	state->module_b_source = 1; /* tuner 2 */

	cic_init_hw();
	
	dprintk("init_ufs922_cic: call dvb_ca_en50221_init\n");

	if ((result = dvb_ca_en50221_init(core->dvb_adap,
					  &core->ca, 0, 2)) != 0) {
		printk(KERN_ERR "ca0 initialisation failed.\n");
		goto error;
	}

	dprintk(KERN_INFO "ufs922_cic: ca0 interface initialised.\n");

	dprintk("init_ufs922_cic <\n");

	return 0;

error:

	dprintk("init_ufs922_cic <\n");

	return result;
}

EXPORT_SYMBOL(init_ci_controller);
EXPORT_SYMBOL(setCiSource);
EXPORT_SYMBOL(getCiSource);

int __init ufs922_cic_init(void)
{
    printk("ufs922_cic loaded\n");
    return 0;
}

static void __exit ufs922_cic_exit(void)
{  
   printk("ufs922_cic unloaded\n");
}

module_init             (ufs922_cic_init);
module_exit             (ufs922_cic_exit);

MODULE_DESCRIPTION      ("CI Controller");
MODULE_AUTHOR           ("Dagobert");
MODULE_LICENSE          ("GPL");

#endif
