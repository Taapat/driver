/*
 *  Driver for the TKDMA
 *
 * Copyright (C) 2006 STMicroelectronics Limited
 * Author: Peter Bennett <peter.bennett@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */

#include <linux/module.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <linux/delay.h>

#include <linux/stm/slim.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#define SLIM_PERS_MBOX_SET      (0x387)
#define SLIM_PERS_MBOX_STATUS   (0x386)
#define SLIM_PERS_MBOX_CLEAR    (0x388)
#define SLIM_PERS_MBOX_MASK     (0x389)
#define SLIM_PERS_COUNTER       (0x380)
#define SLIM_PERS_DREQ          (0xFE0)


extern int  slim_boot_core (struct slim_module *module);
extern void slim_core_go   (struct slim_module *module);

struct tkdma_module {
  struct slim_module smodule;

  volatile int *idle_count;
  volatile int *counter;
  int cycles_per_idle;

  volatile int *mask;
  volatile int *status;
  volatile int *clear;

  volatile int *read_address;
  volatile int *write_address;
  volatile int *command;
  volatile int *num_packets;

  volatile int *temp;
  volatile unsigned int *cpikey;

  volatile unsigned int *iv;
  volatile unsigned int *cpcw_key;

  int key_slot;

  int *data;

  struct semaphore irq; 
  struct mutex    mutex;
  
};

static void convert_key(unsigned char *dest, unsigned char *key)
{
  int n;
  for (n=0;n<16;n++) dest[n] = key[15-n];
}

/*
static void load_key(unsigned char *dest, unsigned char *key)
{
  int n;
  for (n=0;n<16;n++) dest[n] = key[n];
}
*/

static struct tkdma_module *mod;

void tkdma_set_iv(char *iv)
{
  unsigned int data[4];
  int i;
  //  printk("%s: enter\n",__FUNCTION__);

  convert_key((unsigned char*)data,(unsigned char*)iv);
  for (i=0;i<4;i++) mod->iv[i] = data[i];
}

void tkdma_set_key(char *key,int slot)
{
  unsigned int data[4];
  int i;
  //   printk("%s: enter\n",__FUNCTION__);

  convert_key((unsigned char*)data,(unsigned char*)key);
  for (i=0;i<4;i++) mod->cpcw_key[i+(4*slot)] = data[i];
}

void tkdma_set_cpikey(char *key)
{
  unsigned int data[4];
  int i;

  convert_key((unsigned char*)data,(unsigned char*)key);
  for (i=0;i<4;i++) mod->cpikey[i] = data[i];
}

unsigned char iv2[]={0x0B,0xA0,0xF8,0xDD,0xFE,0xA6,0x1F,0xB3,0xD8,0xDF,0x9F,0x56,0x6A,0x05,0x0F,0x78};

int tkdma_bluray_decrypt_data(char *dest, char *src,int num_packets, int device)
{
#if 0
  if (mutex_lock_interruptible(&mod->mutex))
    return -ERESTARTSYS;
#else
  mutex_lock(&mod->mutex);
#endif

  dma_cache_wback_inv(src,192*32*num_packets);
  dma_cache_wback_inv(dest,192*32*num_packets);

  *mod->read_address  = virt_to_phys(src);
  *mod->write_address = virt_to_phys(dest);

   if (mod->num_packets)
	   *mod->num_packets   = num_packets;

   *mod->command       = 1;

  down(&mod->irq);

  mutex_unlock(&mod->mutex);

  return 0;

}

int tkdma_hddvd_decrypt_data(char *dest, char *src,int num_packets, int channel, int device)
{
#if 0
  if (mutex_lock_interruptible(&mod->mutex))
    return -ERESTARTSYS;
#else
  mutex_lock(&mod->mutex);
#endif

  dma_cache_wback_inv(src,2048*num_packets);
  dma_cache_wback_inv(dest,2048*num_packets);

  *mod->read_address  = virt_to_phys(src);
  *mod->write_address = virt_to_phys(dest);

   if (mod->num_packets)
	   *mod->num_packets   = num_packets;

   *mod->command       = 2;

   down(&mod->irq);
#if 0
  printk("Decrypted Packet\n");
  printk("    CPCW: 0x%08x 0x%08x 0x%08x 0x%08x\n",mod->cpcw_key[0],mod->cpcw_key[1],mod->cpcw_key[2],mod->cpcw_key[3]);
  printk("  CPIKey: 0x%08x 0x%08x 0x%08x 0x%08x\n",mod->cpikey[0],mod->cpikey[1],mod->cpikey[2],mod->cpikey[3]);
  printk("     Key: 0x%08x 0x%08x 0x%08x 0x%08x\n",mod->temp[0],mod->temp[1],mod->temp[2],mod->temp[3]);
#endif

   mutex_unlock(&mod->mutex);

  return 0;
}

EXPORT_SYMBOL(tkdma_bluray_decrypt_data);
EXPORT_SYMBOL(tkdma_hddvd_decrypt_data);
EXPORT_SYMBOL(tkdma_set_iv);
EXPORT_SYMBOL(tkdma_set_key);
EXPORT_SYMBOL(tkdma_set_cpikey);

static struct slim_module *tkdma_create_driver(struct slim_driver *driver, int version )
{
  struct tkdma_module *main;

  main = kmalloc(sizeof(struct tkdma_module),GFP_KERNEL);
  if (!main) return NULL;

  memset(main,0,sizeof(struct tkdma_module));

  mod = main;

  main->smodule.core = 0;
  main->smodule.name = "tkdma";
  main->smodule.firmware = "tkdma.o";

  driver->private = main;
  
  if (slim_elf_load(&main->smodule))
    goto err1;

  main->idle_count    = slim_get_symbol_ptr(&main->smodule,"idle_count");  
  main->counter       = slim_get_peripheral(&main->smodule,SLIM_PERS_COUNTER);
  main->mask          = slim_get_peripheral(&main->smodule,SLIM_PERS_MBOX_MASK);
  main->clear         = slim_get_peripheral(&main->smodule,SLIM_PERS_MBOX_CLEAR);
  main->status        = slim_get_peripheral(&main->smodule,SLIM_PERS_MBOX_STATUS);

  main->command       = slim_get_symbol_ptr(&main->smodule,"command");
  main->num_packets   = slim_get_symbol_ptr(&main->smodule,"num_packets");
  main->cpikey        = slim_get_symbol_ptr(&main->smodule,"cpikey");
  *main->mask         = 0xffffffff;

  main->read_address  = slim_get_symbol_ptr(&main->smodule,"read_address");  
  main->write_address = slim_get_symbol_ptr(&main->smodule,"write_address");

  main->temp          = slim_get_symbol_ptr(&main->smodule,"htemp0");

  main->cpcw_key      = slim_get_peripheral(&main->smodule,(0x3420-0x5000)/4);
  main->iv            = slim_get_peripheral(&main->smodule,(0x3004-0x5000)/4);

  sema_init(&main->irq,0);
  mutex_init(&main->mutex);

  slim_boot_core(&main->smodule);

  slim_core_go(&main->smodule);
  msleep(1);

  {
    unsigned int cycles,idle_count;

    idle_count = *main->idle_count;
    cycles     = *main->counter;
    
    msleep(100);

    idle_count = *main->idle_count - idle_count;
    cycles     = *main->counter - cycles;

    main->cycles_per_idle = cycles / idle_count;
  }

  return &main->smodule;

 err1:
  kfree(main);

  return NULL;
}

static void                tkdma_delete_driver(struct slim_driver *driver )
{
  struct tkdma_module *main = (struct tkdma_module*)driver->private;

  slim_elf_unload(&main->smodule);

  kfree(main);
}

static int                 tkdma_interrupt    (struct slim_driver *driver )
{
  struct tkdma_module *main = driver->private;

  *main->clear = *main->status;
  up(&mod->irq);
  
  return 0;
}

static char               *tkdma_debug_driver (struct slim_driver *driver, char *buffer, char *line_indent)
{
  struct tkdma_module *main = driver->private;
  //  main->debug(main,buffer,line_indent);
  char *ptr = buffer;
  
  ptr +=sprintf(ptr,"%s: ic %x\n",__FUNCTION__,*main->idle_count);  ptr +=sprintf(ptr,"%s: command %x\n",__FUNCTION__,*main->command);

  ptr +=sprintf(ptr,"%s: read_address:0x%08x write_address:0x%08x\n",__FUNCTION__,
		*main->read_address,
		*main->write_address);

  ptr += sprintf(ptr,"temp0: 0x%x\n",mod->temp[0]);
  ptr += sprintf(ptr,"temp1: 0x%x\n",mod->temp[1]);
  ptr += sprintf(ptr,"temp2: 0x%x\n",mod->temp[2]);
  ptr += sprintf(ptr,"temp3: 0x%x\n",mod->temp[3]);

  ptr += sprintf(ptr,"cpcw0: 0x%x\n",mod->cpcw_key[0]);
  ptr += sprintf(ptr,"cpcw1: 0x%x\n",mod->cpcw_key[1]);
  ptr += sprintf(ptr,"cpcw2: 0x%x\n",mod->cpcw_key[2]);
  ptr += sprintf(ptr,"cpcw3: 0x%x\n",mod->cpcw_key[3]);

  ptr += sprintf(ptr,"cpikey: 0x%x\n",mod->cpikey[4]);
  ptr += sprintf(ptr,"cpikey: 0x%x\n",mod->cpikey[5]);
  ptr += sprintf(ptr,"cpikey: 0x%x\n",mod->cpikey[6]);
  ptr += sprintf(ptr,"cpikey: 0x%x\n",mod->cpikey[7]);

  return ptr;
}


struct slim_driver tkdma = {
  .name          = "tkdma", /* It's the YKDMA IP */
  .create_driver = tkdma_create_driver,
  .delete_driver = tkdma_delete_driver,
  .debug         = tkdma_debug_driver,
  .interrupt     = tkdma_interrupt
};

static __init int tkdma_init(void)
{
  return slim_register_driver(&tkdma);
}

static void __exit tkdma_cleanup(void)
{ 
  slim_unregister_driver(&tkdma);
}

MODULE_DESCRIPTION("Tkdma, a SLIM Driver");
MODULE_AUTHOR("STMicroelectronics Limited");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

module_init(tkdma_init);
module_exit(tkdma_cleanup);
