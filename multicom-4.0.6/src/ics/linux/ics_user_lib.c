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

/*
 * Linux Userspace ICS library
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#include <ics.h>

#include <ics/ics_ioctl.h>

#include "_ics_os.h"		/* OS wrappers */
#include "_ics_util.h"		/* ICS misc macros */
#include "_ics_debug.h"

#define ICS_DEV_NAME	"/dev/ics"

/* Need to emulate this for the _ics_debug.h macros */
#define ics_debug_printf(FMT, FN, LINE, ...)	do { _ICS_OS_PRINTF("%04d:%s:%d ", getpid(), FN, LINE); \
				       		     _ICS_OS_PRINTF(FMT, ## __VA_ARGS__); } while (0)

#define DEVICE_CLOSED ((int)-1)

static int deviceFd = DEVICE_CLOSED;

static _ICS_OS_MUTEX apiLock;

ICS_ERROR ics_user_init (void)
{
        ICS_ERROR err;

        if (deviceFd == DEVICE_CLOSED) {
	        deviceFd = open(ICS_DEV_NAME, O_RDWR);
		if (deviceFd < 0) {
			fprintf(stderr, "Failed to open %s - errno %d\n", ICS_DEV_NAME, errno);
			
		        err =  ICS_NOT_INITIALIZED;
                        goto exit;
                }
		
        } else {
		err = ICS_ALREADY_INITIALIZED;
                goto exit;
        }
	
        if (!_ICS_OS_MUTEX_INIT(&apiLock)) {
                err = ICS_ENOMEM;
                goto exit;
        }

        err = ICS_SUCCESS;

exit:
	return err;
}

ICS_ERROR ics_user_term (void)
{
        int res;

        if (deviceFd == DEVICE_CLOSED) {
	        return ICS_NOT_INITIALIZED;
	}

        res = close(deviceFd);
        if (res != 0) {
	        return ICS_SYSTEM_ERROR;
        }
        deviceFd = DEVICE_CLOSED;

        _ICS_OS_MUTEX_DESTROY(&apiLock);

	return ICS_SUCCESS;
}

ICS_ERROR ics_load_elf_file (const ICS_CHAR *fname, ICS_UINT flags, ICS_OFFSET *entryAddrp)
{
	int res;
	ICS_ERROR err;

	ics_load_elf_file_t load;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return ICS_NOT_INITIALIZED;
        }
	
        if (NULL == fname || NULL == entryAddrp || strlen(fname) > ICS_LOAD_MAX_NAME) {
	  return ICS_INVALID_ARGUMENT;
        }
	
	load.fname = fname;
	load.fnameLen = strlen(fname);
	load.flags = flags;
	
	res = ioctl(deviceFd, ICS_IOC_LOADELFFILE, &load);
	err = (res == -EFAULT) ? ICS_INVALID_ARGUMENT : load.err;
	
	if (err == ICS_SUCCESS)
	  *entryAddrp = load.entryAddr;

	return err;
}

ICS_ERROR ics_cpu_start (ICS_OFFSET entryAddr, ICS_UINT cpuNum, ICS_UINT flags)
{
	int res;
	ICS_ERROR err;

	ics_cpu_start_t start;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return ICS_NOT_INITIALIZED;
        }
	
	start.entryAddr = entryAddr;
	start.cpuNum    = cpuNum;
	start.flags     = flags;

	res = ioctl(deviceFd, ICS_IOC_CPUSTART, &start);
	err = (res == -EFAULT) ? ICS_INVALID_ARGUMENT : start.err;

	return err;
}

ICS_ERROR ics_cpu_reset (ICS_UINT cpuNum, ICS_UINT flags)
{
	int res;
	ICS_ERROR err;

	ics_cpu_reset_t reset;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return ICS_NOT_INITIALIZED;
        }
	
	reset.cpuNum    = cpuNum;
	reset.flags     = flags;

	res = ioctl(deviceFd, ICS_IOC_CPURESET, &reset);
	err = (res == -EFAULT) ? ICS_INVALID_ARGUMENT : reset.err;

	return err;
}

ICS_INT ics_cpu_self (void)
{
	int res;
	ICS_ERROR err;

	ics_cpu_info_t info;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return -1;
 }
	
	res = ioctl(deviceFd, ICS_IOC_CPUINFO, &info);

	if (res != 0 || info.err != ICS_SUCCESS)
		return -1;
	
	return info.cpuNum;
}

ICS_ERROR ics_cpu_init (ICS_UINT cpuNum, ICS_ULONG cpuMask, ICS_UINT flags)
{
	int res;
	ICS_ERROR err;

	ics_cpu_init_t init;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return 0;
  }
	
 init.inittype = 1;
 init.flags    = flags;
 init.cpuNum   = cpuNum;
 init.cpuMask  = cpuMask;

	res = ioctl(deviceFd, ICS_IOC_CPUINIT, &init);

	if (res != 0 || init.err != ICS_SUCCESS)
		return -1;
	
	return ICS_SUCCESS;

}

ICS_ERROR ICS_cpu_init (ICS_UINT flags)
{
	int res;
	ICS_ERROR err;

	ics_cpu_init_t init;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return 0;
  }
	
 init.inittype = 0;
 init.flags    = flags;

	res = ioctl(deviceFd, ICS_IOC_CPUINIT, &init);

	if (res != 0 || init.err != ICS_SUCCESS)
		return -1;
	
	return ICS_SUCCESS;
}


void ICS_cpu_term (ICS_UINT flags)
{
	int res;
	ICS_ERROR err;

	ics_cpu_term_t term;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return;
 }
	
 term.flags =flags;

	res = ioctl(deviceFd, ICS_IOC_CPUTERM, &term);

	if (res != 0 || term.err != ICS_SUCCESS)
		return;
	
	return;
}


ICS_ULONG ics_cpu_mask (void)
{
	int res;
	ICS_ERROR err;

	ics_cpu_info_t info;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return 0;
        }
	
	res = ioctl(deviceFd, ICS_IOC_CPUINFO, &info);

	if (res != 0 || info.err != ICS_SUCCESS)
		return -1;
	
	return info.cpuMask;
}

const char *ics_cpu_name (ICS_UINT cpuNum)
{
	int res;
	ICS_ERROR err;

	ics_cpu_bsp_t bsp;

	static char cname[32];	/* XXXX this is not safe across multiple concurrent calls */

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return NULL;
        }
	
	bsp.cpuNum = cpuNum;
	bsp.name   = cname;
	bsp.type   = NULL;

	res = ioctl(deviceFd, ICS_IOC_CPUBSP, &bsp);
	
	if (res != 0 || bsp.err != ICS_SUCCESS)
		return NULL;

	return cname;
}

const char *ics_cpu_type (ICS_UINT cpuNum)
{
	int res;
	ICS_ERROR err;

	ics_cpu_bsp_t bsp;

	static char ctype[32];	/* XXXX this is not safe across multiple concurrent calls */

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return NULL;
        }
	
	bsp.cpuNum = cpuNum;
	bsp.type   = ctype;
	bsp.name   = NULL;

	res = ioctl(deviceFd, ICS_IOC_CPUBSP, &bsp);
	
	if (res != 0 || bsp.err != ICS_SUCCESS)
		return NULL;

	return ctype;
}

int ics_cpu_lookup (const ICS_CHAR *cpuName)
{
	int res;
	ICS_ERROR err;

	ics_cpu_bsp_t bsp;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return NULL;
        }
	
	bsp.type   = NULL;
	bsp.name   = cpuName;

	res = ioctl(deviceFd, ICS_IOC_CPULOOKUP, &bsp);
	
	if (res != 0 || bsp.err != ICS_SUCCESS)
		return -1;

	return bsp.cpuNum;
}

const ICS_CHAR *ics_cpu_version (void)
{
  static const ICS_CHAR * version_string = __ICS_VERSION__" ("__TIME__" "__DATE__")";
  
  return version_string;
}

ICS_ERROR ICS_region_add (ICS_VOID *map, ICS_OFFSET paddr, ICS_SIZE size, ICS_MEM_FLAGS mflags, ICS_ULONG cpuMask,
			  ICS_REGION *regionp)
{
	int res;
	ICS_ERROR err;

	ics_user_region_t userregion;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return ICS_NOT_INITIALIZED;
        }


	userregion.map      = map;
	userregion.paddr    = paddr; 
	userregion.size     = size;
	userregion.mflags   = mflags;
	userregion.cpuMask  = cpuMask;

	res = ioctl(deviceFd, ICS_IOC_REGIONADD, &userregion);
	err = (res == -EFAULT) ? ICS_INVALID_ARGUMENT : userregion.err;

 *regionp = userregion.region;
	return err;
}

ICS_ERROR ICS_region_remove (ICS_REGION region, ICS_UINT flags)
{
	int res;
	ICS_ERROR err;

	ics_user_region_t userregion;

	if (deviceFd == DEVICE_CLOSED) {
		err = ics_user_init();
		if (err != ICS_SUCCESS)
			return ICS_NOT_INITIALIZED;
        }
	
	userregion.region  =  region;
	userregion.mflags   = flags;

	res = ioctl(deviceFd, ICS_IOC_REGIONREMOVE, &userregion);
	err = (res == -EFAULT) ? ICS_INVALID_ARGUMENT : userregion.err;

	return err;
}
/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
