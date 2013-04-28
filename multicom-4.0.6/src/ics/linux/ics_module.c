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

#include "_ics_module.h"

#include <linux/proc_fs.h>
#include <linux/ctype.h>

/* Not yet defined in bpa2.h */
extern void bpa2_memory (struct bpa2_part *part, unsigned long *base, unsigned long *size);

static uint  cpu_num;
static ulong cpu_mask;

/*
 * Tuneable parameters and their default values
 */
static int   debug_flags = 0;
static int   debug_chan  = ICS_DBG_STDOUT|ICS_DBG_LOG;
static int   init        = 1;
static int   connect     = 0;
static int   watchdog    = 0;

static char *bpa2_part   = "bigphysarea";	/* Default use "bigphysarea" partition */

#if defined(__KERNEL__) && defined(MODULE)

MODULE_DESCRIPTION("ICS: Inter CPU System");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("GPL");

module_param(cpu_num, uint, 0);
MODULE_PARM_DESC(cpu_num, "ICS CPU number");

module_param(cpu_mask, ulong, 0);
MODULE_PARM_DESC(cpu_mask, "Bitmask of ICS CPU configuration");

module_param(debug_flags, int, 0);
MODULE_PARM_DESC(debug_flags, "Bitmask of ICS debugging flags");

module_param(debug_chan, int, 0);
MODULE_PARM_DESC(debug_chan, "Bitmask of ICS debugging channels");

module_param(init, int, S_IRUGO);
MODULE_PARM_DESC(init, "Do not initialize ICS if set to 0");

module_param(connect, int, S_IRUGO);
MODULE_PARM_DESC (connect, "Connect to all CPUs on init");

module_param(watchdog, int, S_IRUGO);
MODULE_PARM_DESC (watchdog, "Enable CPU watchdog handlers");

module_param(bpa2_part, charp, S_IRUGO);
MODULE_PARM_DESC (bpa2_part, "Set BPA2 partition name for ICS contig memory allocations");

/*
 * Support for declaring ICS regions on module load
 * The syntax is
 *    
 *     regions = region[,region]
 *     region = <bpa2name>:<cacheflag>|<phys base>:<size>:<cacheflag>
 * 
 * Where bpa2name is the ASCII name of the bpa2 partition e.g. LMI_VID or LMI_SYS
 * Otherwise the physical base address and size of a region can be specified directly
 * cacheflag: can be 1/0 for CACHED / UNCACHED region mapping.
 *
 * Once configured these regions will be mapped into all CPUs as CACHED / UNCACHED 
 * translations
 */
static char *     regions = NULL;
module_param     (regions, charp, S_IRUGO);
MODULE_PARM_DESC (regions, "Memory region string of the form <bpa2name>:<cacheflag>|<base>:<size>:<cacheflag>[,<bpa2name>:<cacheflag>|<base>:<size>:<cacheflag>]");

/*
 * Support for declaring ICS companion firmware on module load
 * The syntax is
 *
 *      firmware = companion[,companion]
 *      companion = <cpu>:<filename>
 *
 *   where <cpu> can either be the logical cpu number (integer)
 *   or the cpu core name e.g. audio0 or video0
 
 *   <filename> must be an ASCII filename of an ELF file located in /lib/firmware
 */

static char *     firmware = NULL;
module_param     (firmware, charp, S_IRUGO);
MODULE_PARM_DESC (firmware, "Coprocessor firmware string in the form <cpu>:<filename>[,<cpu>:<filename>]");


/*
 * Export all the ICS module symbols
 */
EXPORT_SYMBOL(ics_cpu_init);

EXPORT_SYMBOL(ics_cpu_version);
EXPORT_SYMBOL(ics_cpu_self);
EXPORT_SYMBOL(ics_cpu_name);
EXPORT_SYMBOL(ics_cpu_type);
EXPORT_SYMBOL(ics_cpu_mask);
EXPORT_SYMBOL(ics_cpu_index);
EXPORT_SYMBOL(ics_cpu_lookup);

EXPORT_SYMBOL(ics_cpu_start);
EXPORT_SYMBOL(ics_cpu_reset);

EXPORT_SYMBOL(ics_err_str);
EXPORT_SYMBOL(ics_debug_flags);
EXPORT_SYMBOL(ics_debug_chan);
EXPORT_SYMBOL(ICS_debug_dump);
EXPORT_SYMBOL(ICS_debug_copy);

EXPORT_SYMBOL(ics_heap_create);
EXPORT_SYMBOL(ics_heap_destroy);
EXPORT_SYMBOL(ics_heap_pbase);
EXPORT_SYMBOL(ics_heap_base);
EXPORT_SYMBOL(ics_heap_size);
EXPORT_SYMBOL(ics_heap_alloc);
EXPORT_SYMBOL(ics_heap_free);

EXPORT_SYMBOL(ics_load_elf_file);
EXPORT_SYMBOL(ics_load_elf_image);

EXPORT_SYMBOL(ICS_channel_alloc);
EXPORT_SYMBOL(ICS_channel_free);
EXPORT_SYMBOL(ICS_channel_open);
EXPORT_SYMBOL(ICS_channel_close);
EXPORT_SYMBOL(ICS_channel_send);
EXPORT_SYMBOL(ICS_channel_recv);
EXPORT_SYMBOL(ICS_channel_release);
EXPORT_SYMBOL(ICS_channel_unblock);

EXPORT_SYMBOL(ICS_cpu_init);
EXPORT_SYMBOL(ICS_cpu_info);
EXPORT_SYMBOL(ICS_cpu_connect);
EXPORT_SYMBOL(ICS_cpu_disconnect);
EXPORT_SYMBOL(ICS_cpu_term);
EXPORT_SYMBOL(ICS_cpu_version);

EXPORT_SYMBOL(ICS_dyn_load_image);
EXPORT_SYMBOL(ICS_dyn_load_file);
EXPORT_SYMBOL(ICS_dyn_unload);

EXPORT_SYMBOL(ICS_msg_send);
EXPORT_SYMBOL(ICS_msg_recv);
EXPORT_SYMBOL(ICS_msg_post);
EXPORT_SYMBOL(ICS_msg_test);
EXPORT_SYMBOL(ICS_msg_wait);
EXPORT_SYMBOL(ICS_msg_cancel);

EXPORT_SYMBOL(ICS_nsrv_add);
EXPORT_SYMBOL(ICS_nsrv_remove);
EXPORT_SYMBOL(ICS_nsrv_lookup);

EXPORT_SYMBOL(ICS_port_alloc);
EXPORT_SYMBOL(ICS_port_free);
EXPORT_SYMBOL(ICS_port_lookup);
EXPORT_SYMBOL(ICS_port_cpu);

EXPORT_SYMBOL(ICS_region_add);
EXPORT_SYMBOL(ICS_region_remove);
EXPORT_SYMBOL(ICS_region_virt2phys);
EXPORT_SYMBOL(ICS_region_phys2virt);

EXPORT_SYMBOL(ICS_stats_add);
EXPORT_SYMBOL(ICS_stats_remove);
EXPORT_SYMBOL(ICS_stats_update);
EXPORT_SYMBOL(ICS_stats_sample);

EXPORT_SYMBOL(ICS_watchdog_add);
EXPORT_SYMBOL(ICS_watchdog_remove);
EXPORT_SYMBOL(ICS_watchdog_reprime);


/*
 * Internal APIs used by MME 
 */
EXPORT_SYMBOL(ICS_debug_printf);	/* Used in os_abstraction.h */
EXPORT_SYMBOL(ics_debug_printf);
EXPORT_SYMBOL(_ics_debug_flags);	/* referenced in ics_mq.h */

EXPORT_SYMBOL(ics_os_virt2phys);
EXPORT_SYMBOL(ics_os_task_create);
EXPORT_SYMBOL(ics_os_task_destroy);

#ifdef ICS_DEBUG_MEM
EXPORT_SYMBOL(_ics_debug_malloc);	/* Needed by MME when debugging malloc */
EXPORT_SYMBOL(_ics_debug_zalloc);
EXPORT_SYMBOL(_ics_debug_free);
#endif

/*
 * Internal APIs used by EMBX mailbox
 */
EXPORT_SYMBOL(ics_mailbox_init);
EXPORT_SYMBOL(ics_mailbox_term);
EXPORT_SYMBOL(ics_mailbox_interrupt_clear);
EXPORT_SYMBOL(ics_mailbox_interrupt_disable);
EXPORT_SYMBOL(ics_mailbox_interrupt_enable);
EXPORT_SYMBOL(ics_mailbox_interrupt_raise);
EXPORT_SYMBOL(ics_mailbox_update_interrupt_handler);
EXPORT_SYMBOL(ics_mailbox_status_get);
EXPORT_SYMBOL(ics_mailbox_status_set);
EXPORT_SYMBOL(ics_mailbox_status_mask);
EXPORT_SYMBOL(ics_mailbox_register);
EXPORT_SYMBOL(ics_mailbox_deregister);
EXPORT_SYMBOL(ics_mailbox_alloc);
EXPORT_SYMBOL(ics_mailbox_free);
EXPORT_SYMBOL(ics_mailbox_find);

static struct {
	ICS_ULONG  base;
	ICS_ULONG  size;
	ICS_REGION rgnCached;
	ICS_REGION rgnUncached;
	ICS_ULONG cacheflag;
} region[_ICS_MAX_REGIONS];

static ICS_UINT numRegions;

static struct {
	ICS_UINT    cpuNum;
	const char *filename;
} load[_ICS_MAX_CPUS];

static ICS_UINT numCpus;

static int parseNumber (const char *str, ICS_ULONG *num)
{
	int   base;
	char *endp;

	/* lazy evaluation will prevent us illegally accessing str[1] */
	base = (str[0] == '0' && str[1] == 'x' ? str += 2, 16 : 10);
	*num = simple_strtoul(str, &endp, base);

	return ('\0' != *str && '\0' == *endp ? 0 : -EINVAL);
}

/* Parse a comma separated list of region declarations
   
 * Each element can be of the form <bpa2name>:<cacheflag>|<base>:<size>:<cacheflag>
 *
 */
static int parseRegionString (char *str)
{
	char *reg;
	
	if (!str || !*str)
		return -EINVAL;
	
	while ((reg = strsep(&str, ",")) != NULL) {
		ICS_OFFSET base;
		ICS_ULONG  size;
		ICS_ULONG  cacheflag=0xCF;

		char *token;

		/* Attempt to parse as <base>:<size>:<cacheflag> */
		token = strsep(&reg, ":");
		if (!token || !*token) {
			return -EINVAL;
		}
		
		/* Only call memparse if we have a numeric value to parse.
		 * Otherwise if bpa2name begins with a value modifier
		 * (g,G,m,M,k,K) it will be incorrectly consumed
		 * (i.e. Consider gfx-memory)
		 */
		if (isdigit( *token ))
			base = memparse(token, &token);

		if (!*token) {
			/* Success, now parse size */
			token = strsep(&reg, ":");
			if (!token || !*token) 
				/* No size parameter */
				return -EINVAL;

			size = memparse(token, &token);
			if (*token)
				/* Invalid size parameter */
				return -EINVAL;
		}
		else {
			/* Assume its a bpa2name */
			struct bpa2_part *part;
			
			/* Lookup the BPA2 partition */
			part = bpa2_find_part(token);
			if (part == NULL) {
				printk("Failed to find BPA2 region '%s'\n", token);
				return -EINVAL;
			}
			
			/* Query the base and size of partition */
			bpa2_memory(part, &base, &size);
			
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
			/* Convert to a physical address */
			base = virt_to_phys((void *)base);
#endif
		}
		
#if 0
		printk("region[%d] base 0x%lx size 0x%lx\n",
		       numRegions, base, size);
#endif

		/* Now parse cacheflag */
			token = strsep(&reg, ":");
			if (!token || !*token) {
     	/* No cache flag parameter */
#if defined(__sh__)
      cacheflag = 2;   /* Set defaults to cached and uncahced */
#elif defined(__arm__)
     cacheflag  = 1;   /* Set defaults to cached */
#endif 	/* __sh__ */		
   }
   else { 
     /* read the user supplied cache flag */
     if (isdigit( *token ))
			    cacheflag = memparse(token, &token);
   } 

#if 0
		printk("region[%d] base 0x%lx size 0x%lx, cacheflag=%d\n",
		       numRegions, base, size, cacheflag);
#endif
		/* Fill out region table */
		region[numRegions].base = base;
		region[numRegions].size = size;
		region[numRegions].cacheflag = cacheflag;
		
		numRegions++;
	}

	return 0;
}

/* Parse a comma separated list of firmware files 
 *
 * Each element can be of the form <cpu>:<filename>
 */

static int parseFirmwareString (char *str)
{
	char *firm;
	
	if (!str || !*str)
		return -EINVAL;
	
	while ((firm = strsep(&str, ",")) != NULL) {
		char *token;
		
		ICS_ULONG cpuNum;
		
		token = strsep(&firm, ":");
		if (!token || !*token) {
			/* No cpu specified */
			return -EINVAL;
		}
		
		/* Parse the cpu as a number */
		if (parseNumber(token, &cpuNum)) {
			/* Failed: Assume its a cpuName instead */
			cpuNum = ics_cpu_lookup(token);
			if (cpuNum < 0)
				return -EINVAL;
		}

		/* Don't allow cpuNum == 0 (i.e. us!) */
		if (cpuNum == 0)
			return -EINVAL;
		
	        token = strsep(&firm, ":");
		if (!token || !*token) {
			/* No filename given */
			return -EINVAL;
		}
		
		load[numCpus].cpuNum = cpuNum;
		load[numCpus].filename = token;

#if 0
		printk("load[%d] cpuNum %d (%s) filename '%s'\n",
		       numCpus, load[numCpus].cpuNum, ics_cpu_name(load[numCpus].cpuNum), load[numCpus].filename);
#endif

		numCpus++;
	}
	
	return 0;
}


static int load_firmware (void)
{
	ICS_ERROR err;
	int cpu;

	for (cpu = 0; cpu < numCpus; cpu++) {
		ICS_OFFSET entryAddr;

		assert(load[cpu].filename);
		
		printk("ICS loading firmware '%s' cpu %d (%s)\n",
		       load[cpu].filename, load[cpu].cpuNum, ics_cpu_name(load[cpu].cpuNum));
#if defined(__arm__)
extern void slaveResetRegMap ();
  /*  Map the reset registers physical addresses into (uncached) kernel space */
  slaveResetRegMap();
#endif /* __arm__ */
	
		/* Load the ELF image */
		err = ics_load_elf_file(load[cpu].filename, 0, &entryAddr);
		if (err != ICS_SUCCESS) {
			printk(KERN_ERR "Failed to load '%s' : %s (%d)\n",
			       load[cpu].filename,
			       ics_err_str(err), err);
			return -EINVAL;
		}

		printk("ICS starting cpu %d (%s) entry 0x%lx\n",
		       load[cpu].cpuNum, ics_cpu_name(load[cpu].cpuNum), entryAddr);

		/* Start the loaded code on the specified cpu */
		err = ics_cpu_start(entryAddr, load[cpu].cpuNum, 0);
		if (err != ICS_SUCCESS) {
			printk(KERN_ERR "Failed to start cpu %d entry 0x%lx : %s (%d)\n",
			       load[cpu].cpuNum, entryAddr,
			       ics_err_str(err), err);

			return -EINVAL;
		}
	}
	
	return 0;
}

static int ics_probe (struct platform_device *pdev)
{
	int res = 0;
	ICS_ERROR err = ICS_SUCCESS;
	
	ICS_UINT flags = 0;

	printk("ICS init %d debug_flags 0x4%x debug_chan 0x%x connect %d watchdog %d bpa2_part '%s'\n",
	       init, debug_flags, debug_chan, connect, watchdog, bpa2_part);
  
	ics_debug_flags(debug_flags);

	ics_debug_chan(debug_chan);

	if (connect)
		flags |= ICS_INIT_CONNECT_ALL;

	if (watchdog)
		flags |= ICS_INIT_WATCHDOG;

	/* Parse the regions module parameter string */
	res += (NULL == regions ? 0 : parseRegionString(regions));

	/* Parse the firmware module parameter string */
	res += (NULL == firmware ? 0 : parseFirmwareString(firmware));

	/* Load and start any firmware files */
	res += (NULL == firmware ? 0 : load_firmware());

	if (bpa2_part != NULL)
	{
		/* Look up associated BPA2 partition */
		ics_bpa2_part = bpa2_find_part(bpa2_part);
		if (ics_bpa2_part)
			printk("ICS: Using BPA2 partition '%s'\n", bpa2_part);
		else
			printk("ICS: Failed to find BPA2 partition '%s'\n", bpa2_part);
	}
	
	if (res != 0)
		return -EINVAL;

	if (init) {
		int i;

		/* user wants to overload BSP */
		if (cpu_num != 0 || cpu_mask != 0) {
			printk("Calling ics_cpu_init(%d, 0x%lx, 0x%x)\n", cpu_num, cpu_mask, flags);
			err = ics_cpu_init(cpu_num, cpu_mask, flags);
		}
		else {
			printk("Calling ICS_cpu_init(%d, 0x%lx, 0x%x)\n", ics_cpu_self(), ics_cpu_mask(), flags);
			err = ICS_cpu_init(flags);
		}

		if (err != ICS_SUCCESS) {
			printk("ICS Initialisation failed : %s (%d)\n", ics_err_str(err), err);
			return -ENODEV;
		}

		for (i = 0; i < numRegions; i++) {
		   if (region[i].size) {
				    printk("ICS Adding region[%d] 0x%lx 0x%lx cacheflag=0x%x\n", i, region[i].base, region[i].size,region[i].cacheflag);

			   	/* Changes here since, unlike sh, arm support either cahced or uncached maping for same physical location.*/
			   	/* Add a cached translation */
       if((region[i].cacheflag == 2) || (region[i].cacheflag == 1)) {
				      err = ICS_region_add(NULL, region[i].base, region[i].size, ICS_CACHED,
						          ics_cpu_mask(),
						          &region[i].rgnCached);
   				   if (err != ICS_SUCCESS) {
		   			     printk("ICS_region_add(0x%lx, %ld) failed : %s (%d)\n",
				   	            region[i].base, region[i].size,
					               ics_err_str(err), err);
					        goto error;
				      }
       }

			   	/* Add an uncached translation */
       if((region[i].cacheflag == 2) || (region[i].cacheflag == 0)) {
				      err = ICS_region_add(NULL, region[i].base, region[i].size, ICS_UNCACHED,
						           ics_cpu_mask(),
						           &region[i].rgnUncached);
				      if (err != ICS_SUCCESS) {
					        printk("ICS_region_add(0x%lx, %ld) failed : %s (%d)\n",
					            region[i].base, region[i].size,
					            ics_err_str(err), err);
				   	     goto error;
				     }
       }
			 }
 	}
}
	
	/* Create the procfs files and directories */
	ics_create_procfs(ics_cpu_mask());

	return 0;

error:
	ICS_cpu_term(0);

	return -ENODEV;
}

static int __exit ics_remove (struct platform_device *pdev) 
{
	/* Remove the per CPU procfs entries */
	ics_remove_procfs(ics_cpu_mask());

	ICS_cpu_term(0);

	return 0;
}

static struct platform_driver ics_driver = {
	.driver.name  = MODULE_NAME,
	.driver.owner = THIS_MODULE,
	.probe        = ics_probe,
	.remove       = __exit_p(ics_remove),
};

static void ics_release (struct device *dev) {}

struct platform_device ics_pdev = {
	.name           = MODULE_NAME,
	.id             = -1,
	.dev.release    = ics_release,
};


int __init ics_mod_init( void )
{
	platform_driver_register(&ics_driver);

	platform_device_register(&ics_pdev);

	
	return 0;
}

void __exit ics_mod_exit( void )
{
	platform_device_unregister(&ics_pdev);
	
	platform_driver_unregister(&ics_driver);

	return;
}

module_init(ics_mod_init);
module_exit(ics_mod_exit);

#endif /* defined(__KERNEL__) && defined(MODULE) */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
