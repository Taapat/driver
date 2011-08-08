/*
 *   lnb_core.c - lnb power control driver
 */

#include "lnb_core.h"

enum
{
	A8293,
	LNB24,
	LNB_PIO,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
static const struct i2c_device_id lnb_id[] = {
        { "a8293", A8293 },
        { "lnb24", LNB24 },
        { "pio", LNB_PIO },
        { }
};
MODULE_DEVICE_TABLE(i2c, lnb_id);
#endif

static int devType;
static char *type = "a8293";
short debug = 0;

/*
 * Addresses to scan
 */
static unsigned short normal_i2c[] = {
	0x08, /* A8293 */
	I2C_CLIENT_END
};
I2C_CLIENT_INSMOD;

static struct i2c_driver lnb_i2c_driver;
static struct i2c_client *lnb_client = NULL;

/*
 * i2c probe
 */

static int lnb_newprobe(struct i2c_client *client, const struct i2c_device_id *id)
{
	if (lnb_client) {
		dprintk("[LNB]: failure, client already registered\n");
		return -ENODEV;
	}

	dprintk("[LNB]: chip found @ 0x%x\n", client->addr);

	switch(devType)
	{
	case A8293:   a8293_init(client);   break;
	case LNB24:   lnb24_init(client);   break;
	case LNB_PIO: lnb_pio_init();		break;
	default: return -ENODEV;
	}
	lnb_client = client;
	return 0;
}

static int lnb_remove(struct i2c_client *client)
{
	lnb_client = NULL;
	dprintk("[LNB]: remove\n");
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)

static int lnb_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *client;
	int err;

	dprintk("[LNB]: attach\n");

	if (!(client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
		dprintk("[LNB]: attach nomem 1\n");
		return -ENOMEM;
	}

	/*
		NB: this code only works for a single client
	*/
	client->adapter = adap;
	client->addr = addr;
	client->driver = &lnb_i2c_driver;
	strlcpy(client->name, "LNB", I2C_NAME_SIZE);

	if ((err = lnb_newprobe(client, NULL))) {
		kfree(client);
		return err;
	}

	dprintk("[LNB]: attach final\n");
	i2c_attach_client(client);
	dprintk("[LNB]: attach final ok\n");

	return 0;
}

static int lnb_probe(struct i2c_adapter *adap)
{
	int ret = 0;

	dprintk("[LNB]: probe\n");
	ret = i2c_probe(adap, &addr_data, lnb_attach);
	dprintk("[LNB]: probe end %d\n",ret);

	return ret;
}

static int lnb_detach(struct i2c_client *client)
{
	int err = lnb_remove(client);
	i2c_detach_client(client);
	if (client)
		kfree(client);

	return err;
}

#endif /* < 2.6.30 */

/*
 * devfs fops
 */

static int lnb_command_ioctl(struct i2c_client *client, unsigned int cmd, void *arg)
{
	int err = 0;

	dprintk("[LNB]: %s (%x)\n", __func__, cmd);

	if (!client && (devType != LNB_PIO))
		return -1;

	switch(devType)
	{
	case A8293:   err = a8293_command(client, cmd, arg);   	break;
	case LNB24:   err = lnb24_command(client, cmd, arg);    break;
	case LNB_PIO: err = lnb_pio_command(cmd, arg); 			break;
	}
	return err;
}

int lnb_command_kernel(unsigned int cmd, void *arg)
{
	int err = 0;
	struct i2c_client *client = lnb_client;

	dprintk("[LNB]: %s (%x)\n", __func__, cmd);

	if (!client && (devType != LNB_PIO))
		return -1;

	switch(devType)
	{
	case A8293:   err = a8293_command_kernel(client, cmd, arg);	break;
	case LNB24:   err = lnb24_command_kernel(client, cmd, arg);	break;
	case LNB_PIO: err = lnb_pio_command_kernel(cmd, arg); 		break;
	}
	return err;
}

EXPORT_SYMBOL(lnb_command_kernel); // export to use in other drivers

static int lnb_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	dprintk("[LNB]: IOCTL\n");
	return lnb_command_ioctl(lnb_client, cmd, (void *) arg);
}

static int lnb_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int lnb_close (struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations lnb_fops = {
	.owner 		= THIS_MODULE,
	.ioctl 		= lnb_ioctl,
	.open  		= lnb_open,
	.release  	= lnb_close
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static int lnb_detect(struct i2c_client *client, int kind, const char **info)
#else
static int lnb_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
#endif
{
	const char *name = "";
    if (kind == 0) {
    	name = "a8293";
    	kind = A8293;
    }

	if (kind < 0) { /* detection and identification */
		if (!type || !strcmp("a8293", type))
			kind = A8293;
		else if (!strcmp("lnb24", type))
			kind = LNB24;
		else if(!strcmp("pio", type))
			kind = LNB_PIO;
		else
			return -ENODEV;
	}
	switch (kind) {
	case A8293:    name = "a8293";   break;
	case LNB24:    name = "lnb24";  break;
	case LNB_PIO:  name = "lnp-pio";  break;
	default: return -ENODEV;
	}
	devType = kind;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	*info = name;
#else
	strlcpy(info->type, name, I2C_NAME_SIZE);
#endif
	return 0;
}

/*
 * i2c
 */

static struct i2c_driver lnb_i2c_driver = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	.driver = {
		.owner = THIS_MODULE,
		.name 	= "lnb_driver",
	},
	.id = I2C_DRIVERID_LNB,
	.attach_adapter = lnb_probe,
	.detach_client 	= lnb_detach,
#else
	.class = I2C_CLASS_TV_DIGITAL,
	.driver = {
		.owner = THIS_MODULE,
		.name  = "lnb_driver", /* 2.6.30 requires name without spaces */
	},
	.probe 			= lnb_newprobe,
	.detect 		= lnb_detect,
	.remove 		= lnb_remove,
	.id_table 		= lnb_id,
	.address_data 	= &addr_data,
#endif
	.command 		= lnb_command_ioctl
};

/*
 * module init/exit
 */

int __init lnb_init(void)
{
	int res, err;
	const char *name;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	err = lnb_detect(NULL, -1, &name);
#else
	struct i2c_board_info info;
	err = lnb_detect(NULL, -1, &info);
	name = info.type;
#endif

	if (err) {
		printk("[LNB]: Error detecting lnb type!\n");
		return err;
	}

	dprintk("[LNB] lnb power control handling, type: %s\n", name);

	if(devType == LNB_PIO) {
		if ((res = lnb_pio_init())) {
			dprintk("[LNB] init lnb pio failed\n");
			return res;
		}
	} else {
		if ((res = i2c_add_driver(&lnb_i2c_driver))) {
			dprintk("[LNB] i2c add driver failed\n");
			return res;
		}

		if (!lnb_client) {
			printk(KERN_ERR "[LNB] no client found\n");
			i2c_del_driver(&lnb_i2c_driver);
			return -EIO;
		}
	}

	if (register_chrdev(LNB_MAJOR,"LNB",&lnb_fops)<0) {
		printk(KERN_ERR "[LNB] unable to register device\n");
		if(devType == LNB_PIO)
			lnb_pio_exit();
		else
			i2c_del_driver(&lnb_i2c_driver);
		return -EIO;
	}

	return 0;
}

void __exit lnb_exit(void)
{
	unregister_chrdev(LNB_MAJOR,"LNB");
	i2c_del_driver(&lnb_i2c_driver);
}

module_init(lnb_init);
module_exit(lnb_exit);

module_param(type,charp,0);
MODULE_PARM_DESC(type, "device type (a8293, lnb24, pio)");

module_param(debug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(debug, "Debug Output 0=disabled >0=enabled(debuglevel)");

MODULE_AUTHOR("Spider-Team");
MODULE_DESCRIPTION("Multiplatform lnb power control driver");
MODULE_LICENSE("GPL");


