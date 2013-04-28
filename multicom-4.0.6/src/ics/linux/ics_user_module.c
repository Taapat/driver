/**************************************************************
 * Copyright (C) 2010   STMicroelectronics. All Rights Reserved.
 * This file is part of the latest release of the Multicom4 project. This release 
 * is fully functional and provides all of the original MME functionality.This 
 * release  is now considered stable and ready for integration with other software 
 * components.

 * Multicom4 is a free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License as published by the Free Software Foundation 
 * version 2.

 * Multicom4 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along with Multicom4; 
 * see the file COPYING.  If not, write to the Free Software Foundation, 59 Temple Place - 
 * Suite 330, Boston, MA 02111-1307, USA.

 * Written by Multicom team at STMicroelectronics in November 2010.  
 * Contact multicom.support@st.com. 
**************************************************************/

/* 
 * 
 */ 

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

#include <ics/ics_ioctl.h>	/* External ioctl interface */

#include "_ics_user.h"		/* Internal definitions */

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>

#if defined(__KERNEL__) && defined(MODULE)

MODULE_DESCRIPTION("ICS: Inter CPU System - user module");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("GPL");

static int major = ICS_MAJOR_NUM; 			/* Can be set to 0 for dynamic */
module_param     (major, int, 0);
MODULE_PARM_DESC (major, "ICS user module major device number");

/* Forward declarations */
static int ics_user_open (struct inode* inode, struct file* filp);
static int ics_user_release (struct inode *inode, struct file *filp);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int ics_user_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg);
#else
static long ics_user_ioctl (struct file *filp, unsigned int command, unsigned long arg);
#endif /*  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) */

static struct file_operations ics_user_ops = {
	.owner =   THIS_MODULE,
	.open  =   ics_user_open,
	.release = ics_user_release,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl =   ics_user_ioctl,
#else
 .unlocked_ioctl = ics_user_ioctl,
#endif /*  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) */
};


static struct cdev          ics_cdev;
static dev_t                ics_devid;
static struct class        *ics_class;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static struct class_device *ics_class_dev;
#else
static struct device       *ics_class_dev;
#endif

/* ==========================================================================
 * 
 * Called by the kernel when the module is loaded
 *
 * ========================================================================== 
 */

int __init ics_user_init (void)
{
	int result;

	if (major) {
		/* Static major number allocation */
		ics_devid = MKDEV(major, 0);
		result =  register_chrdev_region(ics_devid, ICS_DEV_COUNT, ICS_DEV_NAME);
	}
	else {
		/* Dynamic major number allocation */
		result = alloc_chrdev_region(&ics_devid, 0, ICS_DEV_COUNT, ICS_DEV_NAME);
	}
	
	if (result) {
		printk(KERN_ERR "ics: register_chrdev failed : %d\n", result);
		goto err_register;
	}
	
	cdev_init(&ics_cdev, &ics_user_ops);
	ics_cdev.owner = THIS_MODULE;
	result = cdev_add(&ics_cdev, ics_devid, ICS_DEV_COUNT);

	if (result) {
		printk(KERN_ERR "ics: cdev_add failed : %d\n", result);
		goto err_cdev_add;
	}
	
	/* It appears we have to create a class in order for udev to work */
	ics_class = class_create(THIS_MODULE, ICS_DEV_NAME);
	if((result = IS_ERR(ics_class))) {
		printk(KERN_ERR "ics: class_create failed : %d\n", result);
		goto err_class_create;
	}
	
	/* class_device_create() causes the /dev/ics file to appear when using udev
	 * however it is a GPL only function.
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
	ics_class_dev = class_device_create(ics_class, NULL, ics_devid, NULL, ICS_DEV_NAME);
#else
	ics_class_dev = device_create(ics_class, NULL, ics_devid, NULL, ICS_DEV_NAME);
#endif
	if((result = IS_ERR(ics_class_dev)))
	{
		printk(KERN_ERR "ics: class_device_create failed : %d\n", result);
		goto err_class_device_create;
	}

	return result;

err_class_device_create:
	class_destroy(ics_class);

err_class_create:
	cdev_del(&ics_cdev);

err_cdev_add:
	unregister_chrdev_region(ics_devid, ICS_DEV_COUNT);

err_register:
	
	return result;
}

/* ==========================================================================
 *
 * Called by the kernel when the module is unloaded
 *
 * ========================================================================== 
 */
void __exit ics_user_exit (void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
	class_device_unregister(ics_class_dev);
#else
	device_unregister(ics_class_dev);
#endif
	class_destroy(ics_class);
	cdev_del(&ics_cdev);

	unregister_chrdev_region(ics_devid, ICS_DEV_COUNT);
}

module_init(ics_user_init);
module_exit(ics_user_exit);

/* ==========================================================================
 *
 * Called by the kernel when the device is opened by an app (the ics user lib)
 *
 * ========================================================================== 
 */
static
int ics_user_open (struct inode* inode, struct file* filp) 
{
	ics_user_t *instance;
	
	_ICS_OS_ZALLOC(instance, sizeof(*instance));
	if (instance == NULL) {
		goto nomem;
	}
	
	filp->private_data = instance;

	(void) _ICS_OS_MUTEX_INIT(&instance->ulock);
	
	return 0;

nomem:
	return -ENOTTY;
}

/* ==========================================================================
 *
 * Called by the kernel when the device is closed by an app (the ics user lib)
 *
 * ========================================================================== 
 */
static int
ics_user_release (struct inode *inode, struct file *filp) 
{
	ics_user_t *instance = (ics_user_t *) filp->private_data;
	
	ICS_ASSERT(instance);

	_ICS_OS_MUTEX_DESTROY(&instance->ulock);
	
	_ICS_OS_FREE(instance);

	return 0;
}

int ics_user_load_elf_file (ics_user_t *instance, void *arg)
{
	ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;
	
	ics_load_elf_file_t   *load = (ics_load_elf_file_t *) arg;
	ics_load_elf_file_t    local;
	
	char                   fname[ICS_LOAD_MAX_NAME+1];
	
	/* copy the load parameters and name */
	if (copy_from_user(&local, load, sizeof(local)) ||
	    copy_from_user(fname, local.fname, min(local.fnameLen, ICS_LOAD_MAX_NAME))) {
		res = -EFAULT;
		goto errorExit;
	}
	fname[local.fnameLen] = '\0';
	
	err = ics_load_elf_file(fname, local.flags, &local.entryAddr);
	
	if (err == ICS_SUCCESS) {
		/* Pass back entry address to caller */
		res = put_user(local.entryAddr, &load->entryAddr);
	}

errorExit:
        (void) put_user(err, &(load->err));
	
	return res;
}

int ics_user_cpu_start (ics_user_t *instance, void *arg)
{
	ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;

	ics_cpu_start_t       *start = (ics_cpu_start_t *) arg;
	ics_cpu_start_t        local;
	
	/* copy the start parameters */
	if (copy_from_user(&local, start, sizeof(local))) {
		res = -EFAULT;
		goto errorExit;
	}
	
	err = ics_cpu_start(local.entryAddr, local.cpuNum, local.flags);

	return put_user(err, &(start->err));

errorExit:
        (void) put_user(err, &(start->err));
			
	return res;
}

int ics_user_cpu_reset (ics_user_t *instance, void *arg)
{
	ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;

	ics_cpu_reset_t       *reset = (ics_cpu_reset_t *) arg;
	ics_cpu_reset_t        local;

	/* copy the reset parameters */
	if (copy_from_user(&local, reset, sizeof(local))) {
		res = -EFAULT;
		goto errorExit;
	}
	
	err = ics_cpu_reset(local.cpuNum, local.flags);

	return put_user(err, &(reset->err));

errorExit:
        (void) put_user(err, &(reset->err));
			
	return res;
}

int ics_user_cpu_init(ics_user_t *instance,void * arg)
{

	ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;

	ics_cpu_init_t         *init = (ics_cpu_init_t *) arg;
	ics_cpu_init_t          localinit;

	/* copy the bsp parameters */
	if (copy_from_user(&localinit, init, sizeof(localinit))) {
		res = -EFAULT;
		goto errorExit;
	}

 if(localinit.inittype == 0 )	
   err = ICS_cpu_init(localinit.flags);
 else
   err = ics_cpu_init(localinit.cpuNum, localinit.cpuMask, localinit.flags);

	return put_user(err, &(init->err));

errorExit:
 (void) put_user(err, &(init->err));
			
	return res;
}

int ics_user_cpu_term(ics_user_t *instance,void * arg)
{

	ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;

	ics_cpu_term_t         *term = (ics_cpu_term_t *) arg;
	ics_cpu_term_t          localterm;

	/* copy the bsp parameters */
	if (copy_from_user(&localterm, term, sizeof(localterm))) {
		res = -EFAULT;
		goto errorExit;
	}
	
 ICS_cpu_term(localterm.flags);
 err = ICS_SUCCESS;

	return put_user(err, &(term->err));

errorExit:
 (void) put_user(err, &(term->err));
			
	return res;
}

int ics_user_region_add(ics_user_t *instance,void * arg)
{
	ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;

	ics_user_region_t         *region = (ics_user_region_t *) arg;
	ics_user_region_t          localregion;

	/* copy the region parameters */
	if (copy_from_user(&localregion, region, sizeof(localregion))) {
		res = -EFAULT;
		goto errorExit;
	}
 
 /* Add region */
 err = ICS_region_add(localregion.map,localregion.paddr,localregion.size,localregion.mflags,localregion.cpuMask,&localregion.region);

 /*Copy output parmater from ICS_region_add */
	res = copy_to_user(region,&localregion,sizeof(localregion));
	if (res != 0)
			goto errorExit;

	return put_user(err, &(region->err));

errorExit:
 (void) put_user(err, &(region->err));
			
	return res;
}

int ics_user_region_remove(ics_user_t *instance,void * arg)
{
 ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;

	ics_user_region_t         *region = (ics_user_region_t *) arg;
	ics_user_region_t          localregion;

	/* copy the region parameters */
	if (copy_from_user(&localregion, region, sizeof(localregion))) {
		res = -EFAULT;
		goto errorExit;
	}
 
 /* Remove region */
 err = ICS_region_remove(localregion.region,localregion.mflags);

	return put_user(err, &(region->err));

errorExit:
 (void) put_user(err, &(region->err));
			
	return res;

}

int ics_user_cpu_info (ics_user_t *instance, void *arg)
{
	ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;

	ics_cpu_info_t        *info = (ics_cpu_info_t *) arg;
	
	ICS_INT                cpuNum;
	ICS_ULONG              cpuMask;
	
	cpuNum = ics_cpu_self();
	if (cpuNum < 0) {
		err = ICS_NOT_INITIALIZED;
		goto errorExit;
	}

	cpuMask = ics_cpu_mask();
	
	res = put_user(cpuNum, &(info->cpuNum));
	if (res != 0)
		goto errorExit;
		
	res = put_user(cpuMask, &(info->cpuMask));	
	if (res != 0)
		goto errorExit;

	err = ICS_SUCCESS;

        return put_user(err, &(info->err));

errorExit:
        (void) put_user(err, &(info->err));
			
	return res;
}

int ics_user_cpu_bsp (ics_user_t *instance, void *arg)
{
	ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;

	ics_cpu_bsp_t         *bsp = (ics_cpu_bsp_t *) arg;
	ics_cpu_bsp_t          local;

	/* copy the bsp parameters */
	if (copy_from_user(&local, bsp, sizeof(local))) {
		res = -EFAULT;
		goto errorExit;
	}
	
	if (local.name) {
		const char *name = ics_cpu_name(local.cpuNum);
		
		if (name == NULL) {
			err = ICS_INVALID_ARGUMENT;
			goto errorExit;
		}
			
		res = copy_to_user(local.name, name, strlen(name)+1);
		if (res != 0)
			goto errorExit;
	}

	if (local.type) {
		const char *type = ics_cpu_type(local.cpuNum);
		
		if (type == NULL) {
			err = ICS_INVALID_ARGUMENT;
			goto errorExit;
		}
			
		res = copy_to_user(local.type, type, strlen(type)+1);
		if (res != 0)
			goto errorExit;
	}

	err = ICS_SUCCESS;

	return put_user(err, &(bsp->err));

errorExit:
        (void) put_user(err, &(bsp->err));
			
	return res;
}

int ics_user_cpu_lookup (ics_user_t *instance, void *arg)
{
	ICS_ERROR              err = ICS_INVALID_ARGUMENT;
	int                    res = -ENODEV;

	ics_cpu_bsp_t         *bsp = (ics_cpu_bsp_t *) arg;
	ics_cpu_bsp_t          local;

	/* copy the bsp parameters */
	if (copy_from_user(&local, bsp, sizeof(local))) {
		res = -EFAULT;
		goto errorExit;
	}
	
	if (local.name) {
		 int cpuNum = ics_cpu_lookup(local.name);
		
		res = copy_to_user(&(bsp->cpuNum),&cpuNum,sizeof(cpuNum));
		if (res != 0)
			goto errorExit;
	}

	err = ICS_SUCCESS;

	return put_user(err, &(bsp->err));

errorExit:
        (void) put_user(err, &(bsp->err));
			
	return res;
}

/* ==========================================================================
 * 
 * Called by the kernel when an ioctl system call is made
 *
 * ========================================================================== 
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static
int ics_user_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
#else
static long ics_user_ioctl (struct file *filp, unsigned int command, unsigned long arg)
#endif /*  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) */
{
	ics_user_t *instance = (ics_user_t *) filp->private_data;

	int res = -ENOTTY;

	ICS_ASSERT(instance);

	switch (command)
	{
	case ICS_IOC_LOADELFFILE:
		res = ics_user_load_elf_file(instance, (void *) arg);
		break;

	case ICS_IOC_CPUSTART:
		res = ics_user_cpu_start(instance, (void *) arg);
		break;

	case ICS_IOC_CPURESET:
		res = ics_user_cpu_reset(instance, (void *) arg);
		break;

	case ICS_IOC_CPUINFO:
		res = ics_user_cpu_info(instance, (void *) arg);
		break;

	case ICS_IOC_CPUBSP:
		res = ics_user_cpu_bsp(instance, (void *) arg);
		break;

	case ICS_IOC_CPULOOKUP:
		res = ics_user_cpu_lookup(instance, (void *) arg);
		break;

	case ICS_IOC_CPUINIT:
		res = ics_user_cpu_init(instance, (void *) arg);
		break;

	case ICS_IOC_CPUTERM:
		res = ics_user_cpu_term(instance, (void *) arg);
		break;

	case ICS_IOC_REGIONADD:
		res = ics_user_region_add(instance, (void *) arg);
		break;

	case ICS_IOC_REGIONREMOVE:
		res = ics_user_region_remove(instance, (void *) arg);
		break;

	default:
		break;
	}

	return res;
}

#endif /* defined(__KERNEL__) && defined(MODULE) */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
