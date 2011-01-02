#include "bpamem.h"
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/bpa2.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/ctype.h>
#include <linux/mm.h>
#include <asm/semaphore.h>
#include <linux/major.h>


#define MAX_BPA_ALLOCS 32

typedef struct
{
	int used;
	struct bpa2_part *part;
	unsigned long phys_addr, size;
} bpamem_dev_private;

bpamem_dev_private bpamem_dev[MAX_BPA_ALLOCS];
struct semaphore sem_find_dev;

unsigned int find_next_device(void)
{
	unsigned int i, dev = -1;
	
	down(&sem_find_dev);
	for(i = 0; i < MAX_BPA_ALLOCS; i++)
		if(bpamem_dev[i].used == 0)
		{
			dev = i;
			break;
		}
	if(dev != -1)
		bpamem_dev[dev].used = 1; 
	up(&sem_find_dev);
	return dev;
}

int bpamemio_allocmem(BPAMemAllocMemData *in_data)
{
	BPAMemAllocMemData data;
	unsigned int dev;
	
	// reserve a device
	dev = find_next_device();
	if(dev == -1)	// nothing left
	{
		DEBUG_PRINTK("memory already allocated\n");
		return -1;
	}
	
	if(copy_from_user(&data, in_data, sizeof(BPAMemAllocMemData)))
	{
		bpamem_dev[dev].used = 0;
		return -1;
	}
		
	DEBUG_PRINTK("data.bpa_part = %s\n", data.bpa_part);
	
	data.device_num = dev;
	if(data.mem_size % PAGE_SIZE != 0) 
	      data.mem_size = (data.mem_size / PAGE_SIZE + 1) * PAGE_SIZE;
	
	bpamem_dev[dev].part = bpa2_find_part(data.bpa_part);
	if(!bpamem_dev[dev].part)
	{
		DEBUG_PRINTK("bpa part %s not found\n", data.bpa_part);
		bpamem_dev[dev].used = 0;
		return -1;
	}
		
	bpamem_dev[dev].phys_addr = bpa2_alloc_pages(bpamem_dev[dev].part, data.mem_size / PAGE_SIZE, 1, GFP_KERNEL);
	if(!bpamem_dev[dev].phys_addr)
	{
		DEBUG_PRINTK("could not alloc bpa mem\n");
		bpamem_dev[dev].used = 0;
		return -ENOMEM;
	}
		
	bpamem_dev[dev].size = data.mem_size;
	
	if(copy_to_user(in_data, &data, sizeof(BPAMemAllocMemData)))
	{
		bpa2_free_pages(bpamem_dev[dev].part, bpamem_dev[dev].phys_addr);
		bpamem_dev[dev].phys_addr = 0;
		bpamem_dev[dev].used = 0;
		return -1;
	}
	
	return 0;
}

int bpamemio_deallocmem(unsigned int dev)
{
	if(bpamem_dev[dev].used)
	{
		bpa2_free_pages(bpamem_dev[dev].part, bpamem_dev[dev].phys_addr);
		bpamem_dev[dev].phys_addr = 0;
		bpamem_dev[dev].used = 0;
	}
	
	return 0;
}

static int bpamem_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_num, unsigned long ioctl_param)
{
	unsigned int dev;
	dev = MINOR(inode->i_rdev);
	switch (ioctl_num) 
	{
		case BPAMEMIO_ALLOCMEM:   return bpamemio_allocmem((BPAMemAllocMemData *)ioctl_param);

		case BPAMEMIO_FREEMEM:    return bpamemio_deallocmem(dev);
	};
	return -1;
}

static int bpamem_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int bpamem_release (struct inode *inode, struct file *filp)
{
	return 0;
}


static int bpamem_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long start;
	unsigned int dev;
	dev = MINOR(file->f_dentry->d_inode->i_rdev);

	
	if(bpamem_dev[dev].used && bpamem_dev[dev].phys_addr == 0)
	{
		DEBUG_PRINTK("no bpa mem allocated but mmap requested for dev %d\n", dev);
		return -1;	// Not allocated
	}
	
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_IO | VM_RESERVED | VM_DONTEXPAND;

  	start = bpamem_dev[dev].phys_addr + (vma->vm_pgoff << PAGE_SHIFT);

  	if(io_remap_pfn_range(vma, vma->vm_start, start >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot))
  	{
  		DEBUG_PRINTK("remap_pfn_range failed\n");
		return -EIO;

  	}
  	
  	return 0;  	
}

static struct file_operations bpamem_devops = 
{
	.owner   	= THIS_MODULE,
	.open		= bpamem_open,
	.release	= bpamem_release,
	.ioctl		= bpamem_ioctl,
	.mmap		= bpamem_mmap,
};

static int __init bpamem_init(void)
{
	int ret;
	unsigned int i;
	for(i = 0; i < MAX_BPA_ALLOCS; i++)
	{
		bpamem_dev[i].used = 0;
		bpamem_dev[i].phys_addr = 0;
	}
		
	ret = register_chrdev(BPAMEM_MAJOR, "BPAMem", &bpamem_devops);
	
	sema_init(&sem_find_dev, 1);
	
	DEBUG_PRINTK("BPAMem driver register result = %d", ret);
	return ret;
}

static void __exit bpamem_exit(void)
{
	int ret, i;
	for(i = 0; i < MAX_BPA_ALLOCS; i++)
		bpamemio_deallocmem(i);
		
#if 0 // [schischu] function does not return an value for stlinux23
	ret = unregister_chrdev(BPAMEM_MAJOR, "BPAMem");
	if (ret < 0)
		DEBUG_PRINTK("error in unregister_chrdev: %d\n", ret);
#else
	unregister_chrdev(BPAMEM_MAJOR, "BPAMem");
#endif
}

module_init(bpamem_init);
module_exit(bpamem_exit);

MODULE_DESCRIPTION("Alloc bpa2 mem from user space");
MODULE_AUTHOR("Team Ducktales");
MODULE_LICENSE("GPL");
