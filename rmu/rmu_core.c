/*
 *   rmu_core.c - rf modulator core driver
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "event.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <asm/string.h>

#include "rmu_core.h"
#include "rmu74055.h"

#define IOCTL_SET_CHANNEL           0
#define IOCTL_SET_TESTMODE          1
#define IOCTL_SET_SOUNDENABLE       2    
#define IOCTL_SET_SOUNDSUBCARRIER   3
#define IOCTL_SET_FINETUNE          4
#define IOCTL_SET_STANDBY           5

/*
 * Addresses to scan
 */

static unsigned short normal_i2c[] = {0xca>>1, 0xcb>>1,  I2C_CLIENT_END};
I2C_CLIENT_INSMOD;

static struct i2c_driver rmu_i2c_driver;
static struct i2c_client client_template = {
	.name = "RF MODULATOR",
	.flags = 0,
	.driver = &rmu_i2c_driver
};

char data[10];

/*
 * i2c probe
 */

static struct i2c_client *rmu_client = NULL;

static int rmu_newprobe(struct i2c_client *client, const struct i2c_device_id *id)
{
	if (rmu_client) {
		dprintk("[RMU]: failure, client already registered\n");
		return -ENODEV;
	}

	dprintk("[RMU]: chip found @ 0x%x\n", client->addr);

	rmu74055_init(client);
	rmu_client = client;
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static int rmu_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *client;

	dprintk("[RMU]: attach\n");

	if (client_registered > 0) {
		dprintk("[RMU]: attach failed\n");
		return -1;
	}

	client_registered++;

	dprintk("[RMU]: chip found @ 0x%x\n",addr);

	if (!(client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
		dprintk("[RMU]: attach nomem 1\n");
		return -ENOMEM;
	}

	/*
		NB: this code only works for a single client
	*/
	client_template.adapter = adap;
	client_template.addr = addr;
	memcpy(client, &client_template, sizeof(struct i2c_client));

	dprintk("[RMU]: attach final\n");
	i2c_attach_client(client);
	dprintk("[RMU]: attach final ok\n");

	return 0;
}


static int rmu_probe(struct i2c_adapter *adap)
{
	int ret = 0;
	
	dprintk("[RMU]: probe\n");
	ret = i2c_probe(adap, &addr_data, rmu_attach);
	dprintk("[RMU]: probe end %d\n",ret);

	return ret;
}


static int rmu_detach(struct i2c_client *client)
{
	dprintk("[RMU]: detach\n");
	i2c_detach_client(client);
	if (client)
		kfree(client);

	return 0;
}
#endif
/*
 * devfs fops
 */

static int rmu_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	int err = 0;

	dprintk("[RMU]: command\n");
	err = rmu74055_command(client, cmd, arg);
	return err;
}

static int rmu_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	dprintk("[RMU]: IOCTL %d\n", cmd);
	return rmu_command(&client_template, cmd, (void *) arg);
}

static struct file_operations rmu_fops = {
	owner:		THIS_MODULE,
	ioctl:		rmu_ioctl
};


/*
 * i2c
 */

static struct i2c_driver rmu_i2c_driver = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	.driver = {
		.owner		= THIS_MODULE,
		.name		= "rf modulator driver",
	},
	.id			= I2C_DRIVERID_RMU,
	.attach_adapter	= &rmu_probe,
	.detach_client	= &rmu_detach,
#else
	.class = I2C_CLASS_TV_DIGITAL,
	.driver = {
		.owner = THIS_MODULE,
		.name = "rf_modulator", /* 2.6.30 requires name without spaces */
	},
	.probe = rmu_newprobe,
//	.detect = rmu_detect,
//	.remove = rmu_remove,
//	.id_table = rmu_id,
	.address_data = &addr_data,
#endif
	.command		= &rmu_command
};

/*
 * module init/exit
 */

static struct miscdevice rmu_dev = {
	.minor = RMU_MINOR,
	.name = "rfmod",
	.fops = &rmu_fops
};

int __init rmu_init(void)
{
	printk("[RMU]: i2c driver init\n");
	int res;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	if ((res = i2c_add_driver(&rmu_i2c_driver))) {
		dprintk("[RMU]: i2c add driver failed\n");
		return res;
	}

	if (!client_registered){
		printk(KERN_ERR "rmu: no client found\n");
		i2c_del_driver(&rmu_i2c_driver);
		return -EIO;
	}

	
	rmu74055_init(&client_template);

#else /* >= 2.6.30 */
	if ((res = i2c_add_driver(&rmu_i2c_driver)))
		return res;

#endif /* >= 2.6.30 */

	if (misc_register(&rmu_dev)<0){
		printk(KERN_ERR "rmu: unable to register device\n");
		return -EIO;
        }
	
	return 0;
}

void __exit rmu_exit(void)
{
	i2c_del_driver(&rmu_i2c_driver);
	misc_deregister(&rmu_dev);

}

module_init(rmu_init);
module_exit(rmu_exit);

MODULE_AUTHOR("none");
MODULE_DESCRIPTION("ABIPBOX rf modulator driver");
MODULE_LICENSE("GPL");

