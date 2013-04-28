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

#include <mme.h>	/* External defines and prototypes */

#include "_mme_sys.h"	/* Internal defines and prototypes */

#include <mme/mme_ioctl.h>	/* External ioctl interface */

#include "_mme_user.h"		/* Internal definitions */

#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#if defined(__KERNEL__) && defined(MODULE)

MODULE_DESCRIPTION("MME: Multi Media Encoder - Userspace Driver");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("GPL");

static int major = 0 /*MME_MAJOR_NUM */;	/* Can be set to 0 for dynamic */
module_param     (major, int, 0);
MODULE_PARM_DESC (major, "MME Major device number");

/* Forward declarations */
static int mme_user_open (struct inode* inode, struct file* filp);
static int mme_user_release (struct inode *inode, struct file *filp);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int mme_user_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg);
#else
static long mme_user_ioctl (struct file *filp, unsigned int command, unsigned long arg);
#endif /*  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) */
static int mme_user_mmap (struct file *filp, struct vm_area_struct *vma);

static struct file_operations mme_user_ops = {
	.owner =   THIS_MODULE,
	.open  =   mme_user_open,
	.release = mme_user_release,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl =   mme_user_ioctl,
#else
	.unlocked_ioctl =   mme_user_ioctl,
#endif /*  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) */
	.mmap =    mme_user_mmap
};


static struct cdev          mme_cdev;
static dev_t                mme_devid;
static struct class        *mme_class;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static struct class_device *mme_class_dev;
#else
static struct device       *mme_class_dev;
#endif

/* ==========================================================================
 * 
 * Called by the kernel when the module is loaded
 *
 * ========================================================================== 
 */

int __init mme_user_init (void)
{
	int result;

	if (major) {
		/* Static major number allocation */
		mme_devid = MKDEV(major, 0);
		result =  register_chrdev_region(mme_devid, MME_DEV_COUNT, MME_DEV_NAME);
	}
	else {
		/* Dynamic major number allocation */
		result = alloc_chrdev_region(&mme_devid, 0, MME_DEV_COUNT, MME_DEV_NAME);
	}
	
	if (result) {
		printk(KERN_ERR "mme: register_chrdev failed : %d\n", result);
		goto err_register;
	}
	
	cdev_init(&mme_cdev, &mme_user_ops);
	mme_cdev.owner = THIS_MODULE;
	result = cdev_add(&mme_cdev, mme_devid, MME_DEV_COUNT);

	if (result) {
		printk(KERN_ERR "mme: cdev_add failed : %d\n", result);
		goto err_cdev_add;
	}
	
	/* It appears we have to create a class in order for udev to work */
	mme_class = class_create(THIS_MODULE, MME_DEV_NAME);
	if((result = IS_ERR(mme_class))) {
		printk(KERN_ERR "mme: class_create failed : %d\n", result);
		goto err_class_create;
	}
	
	/* class_device_create() causes the /dev/mme file to appear when using udev
	 * however it is a GPL only function.
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
	mme_class_dev = class_device_create(mme_class, NULL, mme_devid, NULL, MME_DEV_NAME);
#else
	mme_class_dev = device_create(mme_class, NULL, mme_devid, NULL, MME_DEV_NAME);
#endif
	if((result = IS_ERR(mme_class_dev)))
	{
		printk(KERN_ERR "mme: class_device_create failed : %d\n", result);
		goto err_class_device_create;
	}

	return result;

err_class_device_create:
	class_destroy(mme_class);

err_class_create:
	cdev_del(&mme_cdev);

err_cdev_add:
	unregister_chrdev_region(mme_devid, MME_DEV_COUNT);

err_register:

	return result;
}

/* ==========================================================================
 *
 * Called by the kernel when the module is unloaded
 *
 * ========================================================================== 
 */
void __exit mme_user_exit (void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
	class_device_unregister(mme_class_dev);
#else
	device_unregister(mme_class_dev);
#endif
	class_destroy(mme_class);
	cdev_del(&mme_cdev);

	unregister_chrdev_region(mme_devid, MME_DEV_COUNT);
}

module_init(mme_user_init);
module_exit(mme_user_exit);

/* ==========================================================================
 *
 * Called by the kernel when the device is opened by an app (the mme user lib)
 *
 * ========================================================================== 
 */
static
int mme_user_open (struct inode* inode, struct file* filp) 
{
	mme_user_t *instance;

	_ICS_OS_ZALLOC(instance, sizeof(*instance));
	if (instance == NULL) {
		goto nomem;
	}

	filp->private_data = instance;

	(void) _ICS_OS_MUTEX_INIT(&instance->ulock);

	INIT_LIST_HEAD(&instance->allocatedBufs);

	return 0;

nomem:
	return -ENOTTY;
}

/* ==========================================================================
 *
 * Called by the kernel when the device is closed by an app (the mme user lib)
 *
 * ========================================================================== 
 */
static int
mme_user_release (struct inode *inode, struct file *filp) 
{
	mme_user_t *instance = (mme_user_t *) filp->private_data;
	
	MME_ASSERT(instance);

	/* Terminate all transformers */
	mme_user_transformer_release(instance);

	/* Release all allocated buffers */
	mme_user_buffer_release(instance);

	MME_ASSERT(list_empty(&instance->allocatedBufs));

	_ICS_OS_MUTEX_DESTROY(&instance->ulock);

	_ICS_OS_FREE(instance);

	return 0;
}

/* ==========================================================================
 * 
 * Called by the kernel when an ioctl system call is made
 *
 * ========================================================================== 
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static
int mme_user_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
#else 
static long mme_user_ioctl (struct file *filp, unsigned int command, unsigned long arg)
#endif /*  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) */
{
	mme_user_t *instance = (mme_user_t *) filp->private_data;

	int res = -ENOTTY;

	MME_ASSERT(instance);

	switch (command)
	{
	case MME_IOC_ABORTCOMMAND:
		res = mme_user_abort_command(instance, (void *) arg);
		break;

	case MME_IOC_SENDCOMMAND:
		res = mme_user_send_command(instance, (void *) arg);
		break;

	case MME_IOC_WAITCOMMAND:
		res = mme_user_wait_command(instance, (void *) arg);
		break;

	case MME_IOC_GETCAPABILITY:
		res = mme_user_get_capability(instance, (void *) arg);
		break;

	case MME_IOC_INITTRANSFORMER:
		res = mme_user_init_transformer(instance, (void *) arg);
		break;

	case MME_IOC_TERMTRANSFORMER:
		res = mme_user_term_transformer(instance, (void *) arg);
		break;

	case MME_IOC_HELPTASK:
		res = mme_user_help_task(instance, (void *) arg);
		break;

	case MME_IOC_ALLOCBUFFER:
		res = mme_user_alloc_buffer(instance, (void *) arg);
		break;

	case MME_IOC_FREEBUFFER:
		res = mme_user_free_buffer(instance, (void *) arg);
		break;

	case MME_IOC_ISTFRREG:
		res = mme_user_is_transformer_registered(instance, (void *) arg);
		break;

	case MME_IOC_SETTUNEABLE:
		res = mme_user_set_tuneable(instance, (void *) arg);
		break;

	case MME_IOC_GETTUNEABLE:
		res = mme_user_get_tuneable(instance, (void *) arg);
		break;

	case MME_IOC_MMEINIT:
		res = mme_user_mmeinit(instance, (void *) arg);
		break;

	case MME_IOC_MMETERM:
		res = mme_user_mmeterm(instance, (void *) arg);
		break;

	default:
		break;
	}

	return res;
}

/* ==========================================================================
 * 
 * mme_user_mmap()
 * Called via mmap sys call from the mme user library
 *
 * ========================================================================== 
 */
static 
int mme_user_mmap (struct file *filp, struct vm_area_struct *vma)
{
	int res;
	mme_user_t  *instance = (mme_user_t *) filp->private_data;

	unsigned long size    = vma->vm_end-vma->vm_start;
	unsigned long offset  = vma->vm_pgoff * PAGE_SIZE;
	
	mme_user_buf_t *allocBuf;
	
	MME_ASSERT(instance);

	MME_PRINTF(MME_DBG_BUFFER, "offset %lx size %ld\n", offset, size);

	_ICS_OS_MUTEX_TAKE(&instance->ulock);

	/* Lookup the supplied address in the allocated buffer list
	 * This has two effects;
	 *   a) It means we can stop the user from arbitarily mapping any bit of physical memory
	 *   b) We can find out the CACHED flags in order to create the correct mapping
	 */
	list_for_each_entry(allocBuf, &instance->allocatedBufs, list)
	{
		if (allocBuf->offset == offset && allocBuf->size == size)
			/* Found it! */
			break;
	}

	_ICS_OS_MUTEX_RELEASE(&instance->ulock);
	
	if (allocBuf == NULL) {
		res = -EINVAL;
		goto exit;
	}

	/* Set the VM_IO flag to signify the buffer is mmapped
	 * from an allocated data buffer
	 */
	vma->vm_flags |= VM_RESERVED | VM_IO;

	/* Query the underlying MME_DataBuffer_t cache attributes */
	if (allocBuf->mflags & ICS_UNCACHED) {
		/* Mark the physical area as uncached */
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	}

	if (remap_pfn_range(vma, vma->vm_start, offset>>PAGE_SHIFT, size, vma->vm_page_prot) < 0) {
		MME_EPRINTF(MME_DBG_BUFFER,
			    "Failed remap_page_range - start 0x%08x, phys 0x%08x, size %d\n", 
			    (int)(vma->vm_start), (int)offset, (int)size);
		res = -EAGAIN;
		goto exit;
	}

	MME_PRINTF(MME_DBG_BUFFER, "Mapped virt 0x%08x len %d to phys 0x%08x (%s)\n", 
		   (int)vma->vm_start, (int)size, (int)offset,
		   (allocBuf->mflags & ICS_UNCACHED) ? "UNCACHED" : "CACHED");

	res = 0;

exit:
	return res;
}

#endif /* defined(__KERNEL__) && defined(MODULE) */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
