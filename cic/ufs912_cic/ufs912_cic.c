#ifdef UFS912
/*
 * ufs912 ci controller handling.
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

#include "ufs912_cic.h"

static int debug=0;

#define TAGDEBUG "[ufs912_cic] "

#define dprintk(level, x...) do { \
if ((debug) && (debug > level)) printk(TAGDEBUG x); \
} while (0)


/* ***** emi for 7111 */
#define EMIConfigBaseAddress 0xfe700000

#define EMI_LCK 	0x0020
#define EMI_GEN_CFG 	0x0028

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

volatile unsigned long slot_a_r_mem  =	0x03028000; /* read */
volatile unsigned long slot_a_w_mem  =	0x03008000; /* write */
volatile unsigned long slot_a_w_stat =	0x03000000;
volatile unsigned long slot_a_r_stat =	0x03020000;

volatile unsigned long slot_b_r_mem  =	0x04028000;
volatile unsigned long slot_b_w_mem  =	0x04008000;
volatile unsigned long slot_b_w_stat =	0x04000000;
volatile unsigned long slot_b_r_stat =	0x04020000;

static struct ufs912_cic_core ci_core;
static struct ufs912_cic_state ci_state;
static struct mutex ufs912_cic_mutex;

/* *************************** */
/* map, write & read functions */
/* *************************** */

unsigned char ufs912_read_register_u8(unsigned long address)
{
    unsigned char result;

#ifdef address_not_mapped
    unsigned long mapped_register = (unsigned long) ioremap_nocache(address, 1);
#else
    unsigned long mapped_register = address;
#endif

    //dprintk(200, "%s > address = 0x%.8lx, mapped = 0x%.8lx\n", __FUNCTION__, (unsigned long) address, mapped_register);

    result = readb(mapped_register);
     
#ifdef address_not_mapped
    iounmap((void*) mapped_register);
#endif
    
    return result;
}

void ufs912_write_register_u8(unsigned long address, unsigned char value)
{
#ifdef address_not_mapped
    unsigned long mapped_register = (unsigned long)  ioremap_nocache(address, 1);
#else
    unsigned long mapped_register = address;
#endif

    writeb(value, mapped_register);
     
#ifdef address_not_mapped
    iounmap((void*) mapped_register);
#endif
}

void ufs912_write_register_u32(unsigned long address, unsigned int value)
{
#ifdef address_not_mapped
    unsigned long mapped_register = (unsigned long) ioremap_nocache(address, 4);
#else
    unsigned long mapped_register = address;
#endif

    writel(value, mapped_register);
     
#ifdef address_not_mapped
    iounmap((void*) mapped_register);
#endif
}

static int ufs912_cic_poll_slot_status(struct dvb_ca_en50221 *ca, int slot, int open)
{
   struct ufs912_cic_state *state = ca->data;
   int                     slot_status = 0;
   unsigned int            result;

   dprintk(100, "%s (%d; open = %d) >\n", __FUNCTION__, slot, open);

   mutex_lock(&ufs912_cic_mutex);

   result = stpio_get_pin(state->slot_status[slot]);

   dprintk(100, "Slot %d Status = 0x%x\n", slot, result);

   if (result == 0x01)
      slot_status = 1;

   if (slot_status != state->module_present[slot])
   {
	   if (slot_status)
	   {
		   stpio_set_pin(state->slot_enable[slot], 0);
		   dprintk(1, "Modul now present\n");
	           state->module_present[slot] = 1;
	   }
	   else
	   {
		   stpio_set_pin(state->slot_enable[slot], 1);
		   dprintk(1, "Modul now not present\n");
	           state->module_present[slot] = 0;
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
	state->detection_timeout[slot] = jiffies + HZ/2;
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
  dprintk(1, "Module %c: present = %d, ready = %d\n",
			  slot ? 'B' : 'A', slot_status,
			  state->module_ready[slot]);

  mutex_unlock(&ufs912_cic_mutex);
  return slot_status;
}

static int ufs912_cic_slot_reset(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs912_cic_state *state = ca->data;

	dprintk(1, "%s >\n", __FUNCTION__);

	mutex_lock(&ufs912_cic_mutex);

	stpio_set_pin(state->slot_reset[slot], 1);
	mdelay(50);
	stpio_set_pin(state->slot_reset[slot], 0);

	stpio_set_pin(state->slot_enable[slot], 1);

       /* reset status variables because module detection has to
        * be reported after a delay 
	*/
        state->module_ready[slot] = 0;
        state->module_present[slot] = 0;
        state->detection_timeout[slot] = 0;

	mutex_unlock(&ufs912_cic_mutex);

	dprintk(1, "%s <\n", __FUNCTION__);
	return 0;
}

static int ufs912_cic_read_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address)
{
	unsigned char res = 0;

	mutex_lock(&ufs912_cic_mutex);

	if (slot == 0)
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, (unsigned long) slot_a_r_mem + address);
	   
	   res = ufs912_read_register_u8(slot_a_r_mem + address);

        } else
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, (unsigned long) slot_b_r_mem + address);

	   res = ufs912_read_register_u8(slot_b_r_mem + address);
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

	mutex_unlock(&ufs912_cic_mutex);
	return (int) res;
}

static int ufs912_cic_write_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address, u8 value)
{
	mutex_lock(&ufs912_cic_mutex);

	if (slot == 0)
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) slot_a_w_mem + address, value);
	   ufs912_write_register_u8(slot_a_w_mem + address, value);

        } else
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) slot_b_w_mem + address, value);
	   ufs912_write_register_u8(slot_b_w_mem + address, value);
	}

	mutex_unlock(&ufs912_cic_mutex);
	return 0;
}

static int ufs912_cic_read_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address)
{
	unsigned char res = 0;
	
	mutex_lock(&ufs912_cic_mutex);

	if (slot == 0)
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, (unsigned long) slot_a_r_stat + address);

	   res = ufs912_read_register_u8(slot_a_r_stat + address);

        } else
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, (unsigned long) slot_b_r_stat + address);
	   res = ufs912_read_register_u8(slot_b_r_stat + address);
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

	mutex_unlock(&ufs912_cic_mutex);
	return (int) res;
}

static int ufs912_cic_write_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address, u8 value)
{
	mutex_lock(&ufs912_cic_mutex);

	if (slot == 0)
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) slot_a_w_stat + address, value);
	   ufs912_write_register_u8(slot_a_w_stat + address, value);

        } else
	{
	   dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) slot_b_w_stat + address, value);
	   ufs912_write_register_u8(slot_b_w_stat + address, value);
	}

	mutex_unlock(&ufs912_cic_mutex);
	return 0;
}

static int ufs912_cic_slot_shutdown(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs912_cic_state *state = ca->data;
	dprintk(20, "%s > slot = %d\n", __FUNCTION__, slot);
	mutex_lock(&ufs912_cic_mutex);

//fixme

	mutex_unlock(&ufs912_cic_mutex);
	return 0;
}

static int ufs912_cic_slot_ts_enable(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs912_cic_state *state = ca->data;

	dprintk(20, "%s > slot = %d\n", __FUNCTION__, slot);

        /* noop */

	return 0;
}

int setCiSource(int slot, int source)
{
   return 0;
}

void getCiSource(int slot, int* source)
{
   *source = 0;
}

#ifdef use_poll_irq
void ufs912_slot_a_irq(struct stpio_pin *pin, void *dev)
{
   struct ufs912_cic_state *state = (struct ufs912_cic_state*) dev;
   int    slot_status = 0;
   unsigned int result;
   
   printk("%s >\n", __func__);

   result = stpio_get_pin(state->slot_a_status);

   dprintk(10, "Slot A Status = 0x%x\n", result);

/* fixme: see poll_slot */

   /* inform the api */
   dvb_ca_en50221_camchange_irq(state->core->ca, 0 /slot */);
todo/fixme
dvb_ca_en50221_camready_irq
dvb_ca_en50221_frda_irq

   printk("%s <\n", __func__);
}

void ufs912_slot_b_irq(struct stpio_pin *pin, void *dev)
{
   printk("%s >\n", __func__);

   printk("%s <\n", __func__);
}
#endif

int cic_init_hw(void)
{
	struct ufs912_cic_state *state = &ci_state;
	
        ufs912_write_register_u32(EMIConfigBaseAddress + EMI_LCK, 0x00);
        ufs912_write_register_u32(EMIConfigBaseAddress + EMI_GEN_CFG, 0x18);
	
        ufs912_write_register_u32(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA0, 0x04f446d9);
        ufs912_write_register_u32(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA1, 0xfd44ffff);
        ufs912_write_register_u32(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA2, 0xfd88ffff);
        ufs912_write_register_u32(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA3, 0x00000000);
        ufs912_write_register_u32(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA0, 0x04f446d9);
        ufs912_write_register_u32(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA1, 0xfd44ffff);
        ufs912_write_register_u32(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA2, 0xfd88ffff);
        ufs912_write_register_u32(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA3, 0x00000000);

/* fixme: use constants here */
        ufs912_write_register_u32(EMIConfigBaseAddress + 0x800, 0x00000000);
        ufs912_write_register_u32(EMIConfigBaseAddress + 0x810, 0x00000008);
        ufs912_write_register_u32(EMIConfigBaseAddress + 0x820, 0x0000000c);
        ufs912_write_register_u32(EMIConfigBaseAddress + 0x830, 0x00000010);
        ufs912_write_register_u32(EMIConfigBaseAddress + 0x840, 0x00000012);
        ufs912_write_register_u32(EMIConfigBaseAddress + 0x850, 0x000000ff);

        ufs912_write_register_u32(EMIConfigBaseAddress + EMI_LCK, 0x1f);
   
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
	msleep(50);
	stpio_set_pin(state->slot_enable[0], 1);
	
	stpio_set_pin(state->slot_enable[1], 0);
	msleep(50);
	stpio_set_pin(state->slot_enable[1], 1);

#ifdef use_poll_irq
/* Dagobert: trial startup for irq slot status handling */
        stpio_flagged_request_irq(state->slot_status[0], 1,
		       ufs912_slot_a_irq,
		       state, 
		       IRQ_DISABLED);
        		       
        stpio_flagged_request_irq(state->slot_status[1], 0,
		       ufs912_slot_b_irq,
		       state, 
		       IRQ_DISABLED);
#endif
	
	return 0;
}

EXPORT_SYMBOL(cic_init_hw);

int init_ci_controller(struct dvb_adapter* dvb_adap)
{
	struct ufs912_cic_state *state = &ci_state;
	struct ufs912_cic_core *core = &ci_core;
	int result;

	printk("init_ufs912_cic >\n");

        mutex_init (&ufs912_cic_mutex);

	core->dvb_adap = dvb_adap;
	state->i2c = i2c_get_adapter(2);
	state->i2c_addr = 0x23;

	memset(&core->ca, 0, sizeof(struct dvb_ca_en50221));

	/* register CI interface */
	core->ca.owner = THIS_MODULE;

	core->ca.read_attribute_mem 	= ufs912_cic_read_attribute_mem;
	core->ca.write_attribute_mem 	= ufs912_cic_write_attribute_mem;
	core->ca.read_cam_control 	= ufs912_cic_read_cam_control;
	core->ca.write_cam_control 	= ufs912_cic_write_cam_control;
	core->ca.slot_shutdown 		= ufs912_cic_slot_shutdown;
	core->ca.slot_ts_enable 	= ufs912_cic_slot_ts_enable;

	core->ca.slot_reset 		= ufs912_cic_slot_reset;
	core->ca.poll_slot_status 	= ufs912_cic_poll_slot_status;

	state->core 			= core;
	core->ca.data 			= state;

	cic_init_hw();
	
#ifndef address_not_mapped
        slot_a_r_mem  =	(volatile unsigned long) ioremap(slot_a_r_mem, 0x200);
        slot_a_w_mem  =	(volatile unsigned long) ioremap(slot_a_w_mem, 0x200);
        slot_a_w_stat = (volatile unsigned long) ioremap(slot_a_w_stat, 0x200);
        slot_a_r_stat = (volatile unsigned long) ioremap(slot_a_r_stat, 0x200);

        slot_b_r_mem  =	(volatile unsigned long) ioremap(slot_b_r_mem, 0x200);
        slot_b_w_mem  =	(volatile unsigned long) ioremap(slot_b_w_mem, 0x200);
        slot_b_w_stat = (volatile unsigned long) ioremap(slot_b_w_stat, 0x200);
        slot_b_r_stat = (volatile unsigned long) ioremap(slot_b_r_stat, 0x200);
#endif
	
	dprintk(1, "init_ufs912_cic: call dvb_ca_en50221_init\n");

#ifdef use_poll_irq
	if ((result = dvb_ca_en50221_init(core->dvb_adap,
					  &core->ca, DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE, 2)) != 0) {
		printk(KERN_ERR "ca0 initialisation failed.\n");
		goto error;
	}
#else
	if ((result = dvb_ca_en50221_init(core->dvb_adap,
					  &core->ca, 0, 2)) != 0) {
		printk(KERN_ERR "ca0 initialisation failed.\n");
		goto error;
	}
#endif

	dprintk(1, "ufs912_cic: ca0 interface initialised.\n");

	dprintk(10, "init_ufs912_cic <\n");

	return 0;

error:

	printk("init_ufs912_cic < error\n");

	return result;
}

EXPORT_SYMBOL(init_ci_controller);
EXPORT_SYMBOL(setCiSource);
EXPORT_SYMBOL(getCiSource);

int __init ufs912_cic_init(void)
{
    printk("ufs912_cic loaded\n");
    return 0;
}

static void __exit ufs912_cic_exit(void)
{  
   printk("ufs912_cic unloaded\n");
}

module_init             (ufs912_cic_init);
module_exit             (ufs912_cic_exit);

MODULE_DESCRIPTION      ("CI Controller");
MODULE_AUTHOR           ("Dagobert");
MODULE_LICENSE          ("GPL");

module_param(debug, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(debug, "Debug Output 0=disabled >0=enabled(debuglevel)");

#endif
