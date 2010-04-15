/*
 *  Linux specific code for the SLIM driver.
 *
 * Copyright (C) 2006 STMicroelectronics Limited
 * Author: Peter Bennett <peter.bennett@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */

/* This file handles linux specific stuff */

#include <linux/version.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/platform_device.h>

#include <linux/delay.h>
#include <linux/debugfs.h>

#include <linux/cdev.h>

#include <linux/stm/slim.h>
#include <asm/dma.h>
#include <linux/firmware.h>

#include "slim_core.h"
#include "slim_elf.h"

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#define IRQF_DISABLED SA_INTERRUPT 
#endif
    
extern int slim_dump_core(struct slim_module *module);

static struct device *slim_device;

int slim_request_firmware(char *name, char **firmware_data)
{
	const struct firmware *fw_entry;

	if (!firmware_data)
		return -EINVAL;

	if (request_firmware(&fw_entry, name, slim_device) != 0) {
		printk(KERN_ERR "%s: Firmware '%s' not available\n",
		       __FUNCTION__, name);
		return -EACCES;
	}

	*firmware_data = kmalloc(fw_entry->size, GFP_KERNEL);

	if (!*firmware_data) {
		release_firmware(fw_entry);
		return -ENOMEM;
	}

	memcpy(*firmware_data, fw_entry->data, fw_entry->size);

	release_firmware(fw_entry);

	return 0;
}

enum debug_types {
	DEBUG_ELF = 0,
	DEBUG_CORE = 1,
	DEBUG_INFO = 2,
	DEBUG_REGS = 3,
	DEBUG_MEMORY = 4,
	DEBUG_CHANNEL = 5,
	DEBUG_SIZE = 6
};

struct slim_plat {
	int core;
	unsigned long base_address;
	int size;
	int irq;
	struct plat_slim *platform_data;
	struct slim_driver *driver;
	struct slim_module *module;

	struct dentry *slim_debugfs_dentry[DEBUG_SIZE];
	struct dentry *slim_debugfs_dir;
	char dbuf[4096];
};

struct slim_plat drivers[SLIM_MAX_SUPPORTED_CORES];

static int slim_register_debugfs(struct slim_plat *main);
static void slim_unregister_debugfs(struct slim_plat *info);

static irqreturn_t slim_interrupt_handler(int irq, void *dev_id)
{
	struct slim_driver *driver = dev_id;

	if (driver->interrupt)
		driver->interrupt(driver);
	else
		return IRQ_NONE;

	return IRQ_HANDLED;
}

static int slim_create_core(int coreid, struct slim_plat *info)
{
	struct slim_core *core = &slim_cores[coreid];

	core->base_address = info->base_address;
	core->size = info->size;
	core->imem_offset = info->platform_data->imem_offset;
	core->imem_size = info->platform_data->imem_size;

	core->dmem_offset = info->platform_data->dmem_offset;
	core->dmem_size = info->platform_data->dmem_size;

	core->peripheral_offset = info->platform_data->pers_offset;

	core->register_offset = info->platform_data->regs_offset;

	core->irq = info->irq;
	core->iomap = ioremap(info->base_address, info->size);

	printk("%x remapped from %x\n", (unsigned int)core->iomap, (unsigned int)info->base_address);

	if (!core->iomap)
		return -ENOMEM;

	if (request_irq
	    (info->irq, slim_interrupt_handler, IRQF_DISABLED, "SLIM",
	     info->driver)) {
		printk("%s: Couldn't get interrupt %d\n", __FUNCTION__,
		       info->irq);
		goto err1;
	}

	if (!
	    (info->module =
	     info->driver->create_driver(info->driver,
					 info->platform_data->version)))
		goto err2;

	slim_register_debugfs(info);

	//We have to do this in create_driver, cause it zero's memory we might need to set up
	//We need a function which loads the data, then slim_boot_core should just hti the run switch
	//slim_boot_core(info->module);

	return 0;

      err2:
	free_irq(info->irq, info->driver);

      err1:
	iounmap(core->iomap);

	return -ENOMEM;
}

void slim_core_go(struct slim_module *module)
{
	struct slim_core *core = &slim_cores[module->core];

	printk("Run register: %x\n", (unsigned int)&SLIM_CORE(core)->Run);

	SLIM_CORE(core)->Run = 0xffffffff;	//SLIM_RUN_ENABLE;

	printk("Run register: %x\n", SLIM_CORE(core)->Run);
}

static int slim_delete_core(struct slim_plat *info)
{
	struct slim_core *core = &slim_cores[info->core];

	slim_unregister_debugfs(info);

	info->driver->delete_driver(info->driver);

	info->module = NULL;
	slim_reset_core(info->core);

	free_irq(info->irq, info->driver);
	iounmap(core->iomap);

	core->iomap = NULL;
	info->driver = NULL;
	info->module = NULL;

    return 0; // what is correct return value?
}

int slim_register_driver(struct slim_driver *driver)
{
	int n;

	for (n = 0; n < SLIM_MAX_SUPPORTED_CORES; n++)
		if (!drivers[n].base_address && !drivers[n].driver) {
			drivers[n].driver = driver;
			drivers[n].core = n;
			return 0;
		} else if (drivers[n].base_address && drivers[n].platform_data) {
			if (!strcmp
			    (drivers[n].platform_data->name, driver->name)) {
				drivers[n].driver = driver;
				drivers[n].core = n;
				return slim_create_core(n, &drivers[n]);
			}
		}

	return -ENODEV;
}

void slim_unregister_driver(struct slim_driver *driver)
{
	int n;

	for (n = 0; n < SLIM_MAX_SUPPORTED_CORES; n++)
		if (drivers[n].driver == driver)
			slim_delete_core(&drivers[n]);

}

/* 
   DebugFS Interface
*/

//struct slim_core_module *main;

static ssize_t slim_debugfs_read_file(struct file *file, char __user * user_buf,
				      size_t count, loff_t * ppos)
{
	int ainfo = (int)(file->private_data) & (SLIM_MAX_SUPPORTED_CORES - 1);
	int atype = (int)(file->private_data) >> 16;
	struct slim_plat *info = &drivers[ainfo];
	int acore = info->core;
	char *ptr = info->dbuf;
	struct slim_core *core = &slim_cores[acore];

	//struct slim_core_module *main = debug->main;

	switch (atype) {
	case DEBUG_ELF:
		return simple_read_from_buffer(user_buf, count, ppos, core->elf,
					       slim_elf_write_size());

	case DEBUG_CORE:
		slim_dump_core(info->module);
		return simple_read_from_buffer(user_buf, count, ppos,
					       core->elf_core,
					       slim_elf_write_size());

	case DEBUG_INFO:
		ptr = slim_debug_core(acore, ptr, "");
		if (info->driver->debug)
			ptr = info->driver->debug(info->driver, ptr, "");
		//if (main->debug) ptr = main->debug(main, ptr, "");
		break;

	case DEBUG_REGS:
		ptr = slim_debug_regs(acore, ptr, "");
		break;

	case DEBUG_MEMORY:
		ptr = slim_debug_core_memory(acore, ptr, "");
		break;

	default:
		ptr += sprintf(ptr, "Not yet implemented\n");
		break;
	}

	return simple_read_from_buffer(user_buf, count, ppos, info->dbuf,
				       ptr - info->dbuf);
}

static int slim_debugfs_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
	file->private_data = inode->i_private;
    #else /* STLinux 2.2 kernel */
    file->private_data = inode->u.generic_ip;
    #endif
	return 0;
}

static struct file_operations slim_debugfs_fops = {
	.read = slim_debugfs_read_file,
	.open = slim_debugfs_open,
};

#define DEBUG_FILE(dir, name, enum) \
  info->slim_debugfs_dentry[enum] = debugfs_create_file(name, 0444, dir, (void*)(info->core | ( enum << 16)) , &slim_debugfs_fops)

static int slim_register_debugfs(struct slim_plat *info)
{

	info->slim_debugfs_dir = debugfs_create_dir("slim", NULL);

	DEBUG_FILE(info->slim_debugfs_dir, "info", DEBUG_INFO);
	DEBUG_FILE(info->slim_debugfs_dir, "mem", DEBUG_MEMORY);
	DEBUG_FILE(info->slim_debugfs_dir, "regs", DEBUG_REGS);
	DEBUG_FILE(info->slim_debugfs_dir, "slim.elf", DEBUG_ELF);
	DEBUG_FILE(info->slim_debugfs_dir, "core.elf", DEBUG_CORE);

	return 0;
}

static void slim_unregister_debugfs(struct slim_plat *info)
{
	debugfs_remove(info->slim_debugfs_dentry[DEBUG_INFO]);
	debugfs_remove(info->slim_debugfs_dentry[DEBUG_REGS]);
	debugfs_remove(info->slim_debugfs_dentry[DEBUG_MEMORY]);
	debugfs_remove(info->slim_debugfs_dentry[DEBUG_ELF]);
	debugfs_remove(info->slim_debugfs_dentry[DEBUG_CORE]);
	debugfs_remove(info->slim_debugfs_dir);
}

/*
 *  GDB Debug interfaces
 */
#define SLIM_MAX_MINORS 10
#define SLIM_MAJOR      29

static int slim_debug_open(struct inode *inode, struct file *file)
{
	//void *debugdeg;

	//debugdev = dvbdev_find_device (iminor(inode));
	return 0;
}

static int slim_debug_ioctl(struct inode *inode, struct file *file,
			    unsigned int cmd, unsigned long parg)
{
	int ret = 0;

	switch (cmd) {
	case SLIM_READ_MEM:
		ret = 0;
	case SLIM_WRITE_MEM:
		ret = 0;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static struct file_operations slim_debug_fops = {
	.owner = THIS_MODULE,
	.open = slim_debug_open,
	.ioctl = slim_debug_ioctl
};

static struct cdev slim_debug_cdev = {
	.kobj = {.name = "Slim",},
	.owner = THIS_MODULE,
};

int slim_register_gdb_debug(void)
{
	int retval;
	dev_t dev = MKDEV(SLIM_MAJOR, 0);

	if ((retval =
	     register_chrdev_region(dev, SLIM_MAX_MINORS, "Slim debug")) != 0) {
		printk("%s: unable to get major %d\n", __FUNCTION__,
		       SLIM_MAJOR);
		return retval;
	}

	cdev_init(&slim_debug_cdev, &slim_debug_fops);
	if ((retval = cdev_add(&slim_debug_cdev, dev, SLIM_MAX_MINORS)) != 0) {
		printk("%s: unable to get major %d\n", __FUNCTION__,
		       SLIM_MAJOR);
		goto error;
	}

	return 0;

      error:
	cdev_del(&slim_debug_cdev);
	unregister_chrdev_region(dev, SLIM_MAX_MINORS);
	return retval;
}

/*
 *  Platform structures / procedures
 */
extern struct slim_core_module main_module;
static struct platform_device *slim_device_data;

static int slim_probe(struct device *dev)
{
	struct plat_slim *platform_data;
	unsigned int base, size, irq, n;

	slim_device_data = to_platform_device(dev);
	slim_device = dev;

	if (!slim_device_data || !slim_device_data->name
	    || !slim_device_data->dev.platform_data) {
		printk(KERN_ERR
		       "%s: Device probe failed.  Check your kernel SoC config!!\n",
		       __FUNCTION__);

		return -ENODEV;
	}

	platform_data = (struct plat_slim *)slim_device_data->dev.platform_data;

	base =
	    platform_get_resource(slim_device_data, IORESOURCE_MEM, 0)->start;
	size =
	    (((unsigned
	       int)(platform_get_resource(slim_device_data, IORESOURCE_MEM,
					  0)->end)) - base) + 1;
	irq = platform_get_resource(slim_device_data, IORESOURCE_IRQ, 0)->start;

	for (n = 0; n < SLIM_MAX_SUPPORTED_CORES; n++)
		if (drivers[n].driver && !drivers[n].base_address) {
			if (!strcmp
			    (drivers[n].driver->name, platform_data->name)) {
				drivers[n].base_address = base;
				drivers[n].size = size;
				drivers[n].irq = irq;
				drivers[n].platform_data = platform_data;

				return slim_create_core(n, &drivers[n]);
			}
		} else if (!drivers[n].base_address && !drivers[n].driver) {
			drivers[n].base_address = base;
			drivers[n].size = size;
			drivers[n].irq = irq;
			drivers[n].platform_data = platform_data;
			drivers[n].core = n;
		}

	return 0;
}

static int slim_remove(struct device *dev)
{
	int n;

	for (n = 0; n < SLIM_MAX_SUPPORTED_CORES; n++)
		if (drivers[n].driver)
			slim_delete_core(&drivers[n]);

	return 0;
}

/* What ever the driver if fdma or slim, this should
   always be the same */
static struct device_driver slim_driver = {
	.name = "tkdma",
	.bus = &platform_bus_type,
	.probe = slim_probe,
	.remove = slim_remove,
};

static __init int slim_init(void)
{
	return driver_register(&slim_driver);
}

static void slim_cleanup(void)
{
	driver_unregister(&slim_driver);
}

MODULE_DESCRIPTION("Slim Driver");
MODULE_AUTHOR("STMicroelectronics Limited");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

module_init(slim_init);
module_exit(slim_cleanup);

EXPORT_SYMBOL(slim_register_driver);
EXPORT_SYMBOL(slim_unregister_driver);
EXPORT_SYMBOL(slim_core_go);
