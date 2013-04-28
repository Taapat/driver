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

#ifdef CONFIG_PROC_FS

/* 
 * 
 */ 

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

#include "_ics_module.h"

#include <linux/proc_fs.h>

static ICS_STATS_ITEM stats_array[ICS_STATS_MAX_ITEMS];

/* PROCFS stuff */
static struct proc_dir_entry *module_dir, *cpu_dir[_ICS_MAX_CPUS];
static struct proc_dir_entry *debuglog[_ICS_MAX_CPUS], *stats[_ICS_MAX_CPUS];

static
int ics_read_debuglog (char *buf, char **start, off_t offset,
		       int count, int *eof, void *data)
{
	int res;

	ICS_INT  bytes = 0;
	ICS_UINT cpuNum = (ICS_UINT) data;

	/* Extract the debug log into the supplied buffer
	 *
	 * bytes will be updated with the number of bytes copied
	 * and ENOMEM will be returned if there is more data to be returned
	 */
	res = ICS_debug_copy(cpuNum, buf, count, &bytes);

	/* Display an error message in the case where the CPU is not
	 * connected so we can tell the difference between this and no output
	 */
	if ((res != ICS_SUCCESS) && (res != ICS_ENOMEM) && (offset == 0)) {
		bytes = snprintf(buf, count,
				 "Failed to dump cpu %02d log : %s (%d)\n",
				 cpuNum,
				 ics_err_str(res), res);
	}

	/* Option (2) of procfs interface (see proc_file_read() in fs/proc/generic.c) */
	*start = buf;

	*eof = ((res == ICS_ENOMEM) ? 0 : 1);

	return bytes;
}

static int ics_read_stats (char *buf, char **start, off_t offset,
			   int count, int *eof, void *data)
{
	int res, bytes = 0;

	ICS_UINT cpuNum = (ICS_UINT) data;
	ICS_UINT i, nstats;
	ICS_ULONG version;

	res = ICS_cpu_version(cpuNum, &version);
	if (res != ICS_SUCCESS)
		return -EINVAL;
	
	res = ICS_stats_sample(cpuNum, stats_array, &nstats);
	
	if (res != ICS_SUCCESS)
		return -EINVAL;

	bytes += sprintf(buf, "STATISTICS for CPU %02d ICS version 0x%08lx\n", cpuNum, version);
	for (i = 0; i < nstats; i++) {
		ICS_STATS_ITEM *item;
		ICS_STATS_VALUE delta, timeDelta;
		
		item = &stats_array[i];

		/* Calculate deltas */
		delta     = (item->value[0] - item->value[1]);
		timeDelta = (item->timestamp[0] - item->timestamp[1]);
		
		bytes += sprintf(buf+bytes, "%-32s\t", stats_array[i].name);

		if (item->type & ICS_STATS_COUNTER)
			/* Display simple counter */
			bytes += sprintf(buf+bytes, " %lld (%lld)", item->value[0], delta);

		if (item->type & ICS_STATS_RATE)
			/* Display rate per sec */
			bytes += sprintf(buf+bytes, " %ld/s",
					 /* No 64-bit udiv */
					 (1000*(unsigned long)delta)/(unsigned long)timeDelta);
		
		if (item->type & ICS_STATS_PERCENT)
			/* Display as a percentage of time */
			bytes += sprintf(buf+bytes, " %ld%%",
					 /* No 64-bit udiv */
					 (100*(unsigned long)delta)/(unsigned long)timeDelta);

		if (item->type & ICS_STATS_PERCENT100)
			/* Display as 100-percent of time */
			bytes += sprintf(buf+bytes, " %ld%%",
					 /* No 64-bit udiv */
					 100 - ((100*(unsigned long)delta)/(unsigned long)timeDelta));

		bytes += sprintf(buf+bytes, "\n");
	}
				
	return bytes;
}

/* Create the per CPU procfs entries */
int __init ics_create_procfs (ICS_ULONG cpuMask)
{
	int i, res;
	ICS_ULONG mask;
	
	module_dir = proc_mkdir(MODULE_NAME, NULL);
        if(module_dir == NULL) {
                res = -ENOMEM;
                goto out;
        }

	SET_PROCFS_OWNER(module_dir);
	
	for (i = 0, mask = cpuMask; mask; i++, mask >>= 1) {
		if (mask & 1) {
			char procname[64];

			sprintf(procname, "cpu%02d", i);
			
			cpu_dir[i] = proc_mkdir(procname, module_dir);
			if (cpu_dir[i] == NULL) {
				res = -ENOMEM;
				goto remove_cpus;
			}

			SET_PROCFS_OWNER(cpu_dir[i]);

			/*
			 * Create debug log entry
			 */
			debuglog[i] = create_proc_read_entry("log",
							     0444,		/* default mode */
							     cpu_dir[i],	/* parent dir */
							     ics_read_debuglog,
							     (void *)i		/* client data */);
			if (debuglog[i] == NULL) {
				res = -ENOMEM;
				goto remove_files;
			}

			SET_PROCFS_OWNER(debuglog[i]);

			/*
			 * Create stats entry
			 */
			stats[i] = create_proc_read_entry("stats",
							  0444,
							  cpu_dir[i],	/* parent dir */
							  ics_read_stats,
							  (void *)i		/* client data */);
			if (stats[i] == NULL) {
				res = -ENOMEM;
				goto remove_files;
			}
		}
	}

	return 0;

remove_files:
	for (i = 0, mask = cpuMask; mask; i++, mask >>= 1) {
		if ((mask & 1)) {
			if (debuglog[i])
				remove_proc_entry("log", cpu_dir[i]);
			if (stats[i])
				remove_proc_entry("stats", cpu_dir[i]);
		}
	}

remove_cpus:
	for (i = 0, mask = cpuMask; mask; i++, mask >>= 1) {
		if ((mask & 1) && cpu_dir[i]) {
			char procname[64];

			sprintf(procname, "cpu%02d", i);

			remove_proc_entry(procname, module_dir);
		}
	}

	remove_proc_entry(MODULE_NAME, NULL);

out:
	return res;
}

void __exit ics_remove_procfs (ICS_ULONG cpuMask)
{
	int i;
	ICS_ULONG mask;

	/* Remove log/stat files */
	for (i = 0, mask = cpuMask; mask; i++, mask >>= 1) {
		if ((mask & 1)) {
			if (debuglog[i])
				remove_proc_entry("log", cpu_dir[i]);
			if (stats[i])
				remove_proc_entry("stats", cpu_dir[i]);
		}
	}

	/* Remove cpu dirs */
	for (i = 0, mask = cpuMask; mask; i++, mask >>= 1) {
		if ((mask & 1) && cpu_dir[i]) {
			char procname[64];
			
			sprintf(procname, "cpu%02d", i);

			remove_proc_entry(procname, module_dir);
		}
	}
	
	/* Remove module */
	if (module_dir)
		remove_proc_entry(MODULE_NAME, NULL);

	return;
}

#endif /* CONFIG_PROC_FS */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
