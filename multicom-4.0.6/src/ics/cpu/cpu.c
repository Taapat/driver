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

#include <bsp/_bsp.h>

/*
 * Routines to manage the logical to BSP translation of cpu names and CPU numbers
 * as well as SH4 specific code to load and reset the CPUs via BSP lookup tables
 *
 * The reset code is a direct copy of that found in the sh4 toolkit slaveboot.c
 * example code
 */

#if defined(__sh__) ||  defined(__arm__)

/*
 * Set the boot address of a CPU *without* resetting the CPU.
 */
static
ICS_ERROR setBootAddress (ICS_UINT index, ICS_OFFSET startAddress)
{
  ICS_OFFSET bootRegORVal;

  if (bsp_sys_boot_registers[index].address == NULL)
    return -1;

  bootRegORVal = (startAddress << bsp_sys_boot_registers[index].leftshift) & bsp_sys_boot_registers[index].mask;
  if (bootRegORVal != ((startAddress << bsp_sys_boot_registers[index].leftshift)))
  {
    /* Boot address was invalid - would not fit in register without losing information */
    ICS_EPRINTF(ICS_DBG_LOAD, "Invalid sys boot info: startAddr 0x%x lsh %d mask 0x%x != 0x%x\n",
		startAddress, 
		bsp_sys_boot_registers[index].leftshift, 
		bsp_sys_boot_registers[index].mask,
		bootRegORVal);
    
    return ICS_INVALID_ARGUMENT;
  }
  
  ICS_PRINTF(ICS_DBG_LOAD, "writing to boot register %p = 0x%x\n",
	     bsp_sys_boot_registers[index].address,
	     (*(bsp_sys_boot_registers[index].address) & ~(bsp_sys_boot_registers[index].mask)) | bootRegORVal);
  
  *(bsp_sys_boot_registers[index].address) =
    (*(bsp_sys_boot_registers[index].address) & ~(bsp_sys_boot_registers[index].mask)) | bootRegORVal;

  ICS_PRINTF(ICS_DBG_LOAD, "boot register %p = 0x%x\n",
	     bsp_sys_boot_registers[index].address,
	     *(bsp_sys_boot_registers[index].address));

  return ICS_SUCCESS;
}

/* 
 * Companion reset/boot sequence
 *
 * This code is based on slaveboot.c code from the sh4 toolkit examples tree
 */
static
ICS_ERROR slaveBoot (ICS_UINT index, ICS_OFFSET startAddress)
{
  ICS_ERROR res;

  /* Unset the CPU's "allow boot" - needed for STi7141 - older SoCs don't mind */
  if (bsp_sys_boot_enable[index].address) 
  {
    ICS_PRINTF(ICS_DBG_LOAD, "clearing boot enable %p (0x%x): mask 0x%x\n",
	       (void *)bsp_sys_boot_enable[index].address,
	       *(bsp_sys_boot_enable[index].address),
	       bsp_sys_boot_enable[index].mask);
    *(bsp_sys_boot_enable[index].address) &= bsp_sys_boot_enable[index].mask;
  }

  /* Bypass CPU core reset out signals */
  {
    unsigned int i;
    for (i = 0; i < bsp_sys_reset_bypass_count; i++)
    {
      if (bsp_sys_reset_bypass[i].address)
      {
	ICS_PRINTF(ICS_DBG_LOAD, "mask & set reset bypass %p (0x%x): value 0x%x mask 0x%x\n",
		   (void *)bsp_sys_reset_bypass[i].address,
		   *(bsp_sys_reset_bypass[i].address),
		   bsp_sys_reset_bypass[i].value,
		   bsp_sys_reset_bypass[i].mask);
		   
        *(bsp_sys_reset_bypass[i].address) = bsp_sys_reset_bypass[i].value | 
	  ((*(bsp_sys_reset_bypass[i].address)) & bsp_sys_reset_bypass[i].mask);
      }
    }
  }

  /* Configure the boot address */
  res = setBootAddress(index, startAddress);
  if (res != ICS_SUCCESS)
  {
    /* Failed to set boot address to startAddress */
    return res;
  }

  /* Assert and de-assert reset */
  if (bsp_sys_reset_registers[index].value)
  {
    ICS_PRINTF(ICS_DBG_LOAD, "toggle AH sys reset register %p (0x%x) value 0x%x mask 0x%x\n",
	       bsp_sys_reset_registers[index].address,
	       *(bsp_sys_reset_registers[index].address),
	       bsp_sys_reset_registers[index].value,
	       bsp_sys_reset_registers[index].mask);
    
    /* Reset is active high */
    *(bsp_sys_reset_registers[index].address) |= bsp_sys_reset_registers[index].value;
    *(bsp_sys_reset_registers[index].address) &= bsp_sys_reset_registers[index].mask;
  }
  else
  {
    ICS_PRINTF(ICS_DBG_LOAD, "toggle AL sys reset register %p (0x%x) mask : 0x%x\n",
	       bsp_sys_reset_registers[index].address,
	       *(bsp_sys_reset_registers[index].address),
	       bsp_sys_reset_registers[index].mask);

    /* Reset is active low - just use mask and ~mask */
    *(bsp_sys_reset_registers[index].address) &= bsp_sys_reset_registers[index].mask;
    *(bsp_sys_reset_registers[index].address) |= ~bsp_sys_reset_registers[index].mask;   
  }

  /* Set the CPU's "allow boot"
   * This must be *after* the reset cycle on sti7141 - older SoCs don't mind.
   */
  if (bsp_sys_boot_enable[index].address)
  {
    ICS_PRINTF(ICS_DBG_LOAD, "setting boot enable %p (0x%x): value 0x%x mask 0x%x\n",
	       (void *)bsp_sys_boot_enable[index].address,
	       *(bsp_sys_boot_enable[index].address),
	       bsp_sys_boot_enable[index].value,
	       bsp_sys_boot_enable[index].mask);
	       
    *(bsp_sys_boot_enable[index].address) = bsp_sys_boot_enable[index].value |
      ((*(bsp_sys_boot_enable[index].address)) & bsp_sys_boot_enable[index].mask);
  }
  
  return ICS_SUCCESS;
}

#if defined(__arm__)
extern unsigned int                bsp_sys_reset_bypass_count;
extern unsigned int                bsp_sys_boot_registers_count;
extern unsigned int                bsp_sys_reset_registers_count;

void slaveResetRegMap ()
{
				static int ResetRegMapped = ICS_FALSE;
    unsigned int i;

    /* Check if register already mapped.*/
				if (ICS_TRUE == ResetRegMapped)
						return;

    /* Map reset and boot register into kernel space */  
    for (i = 0; i < bsp_sys_reset_bypass_count; i++)
    {
      if (bsp_sys_reset_bypass[i].address)
      {
        /* Must map the physical bsp_sys_reset_bypass register addresses into (uncached) kernel space */
        bsp_sys_reset_bypass[i].address = _ICS_OS_MMAP((unsigned long)bsp_sys_reset_bypass[i].address,_ICS_OS_PAGESIZE, ICS_FALSE);
      }
    }
    for (i = 0; i < bsp_sys_boot_registers_count; i++)
    {
      if (bsp_sys_boot_registers[i].address)
      {
        /* Must map the physical bsp_sys_boot register addresses into (uncached) kernel space */
        bsp_sys_boot_registers[i].address = _ICS_OS_MMAP((unsigned long)bsp_sys_boot_registers[i].address,_ICS_OS_PAGESIZE, ICS_FALSE);
      }
    }
    for (i = 0; i < bsp_sys_reset_registers_count; i++)
    {
      if (bsp_sys_reset_registers[i].address)
      {
        /* Must map the physical bsp_sys_reset register addresses into (uncached) kernel space */
        bsp_sys_reset_registers[i].address = _ICS_OS_MMAP((unsigned long)bsp_sys_reset_registers[i].address,_ICS_OS_PAGESIZE, ICS_FALSE);
      }
    }
    /* Set register mapped flag.*/
				ResetRegMapped = ICS_TRUE;
}
#endif /* __arm__*/


static
void slaveReset (ICS_UINT index)
{
  /* Bypass CPU core reset out signals */
  {
    unsigned int i;
    for (i = 0; i < bsp_sys_reset_bypass_count; i++)
    {
      if (bsp_sys_reset_bypass[i].address)
      {
	ICS_PRINTF(ICS_DBG_LOAD, "mask & set reset bypass %p (0x%x): value 0x%x mask 0x%x\n",
		   (void *)bsp_sys_reset_bypass[i].address,
		   *(bsp_sys_reset_bypass[i].address),
		   bsp_sys_reset_bypass[i].value,
		   bsp_sys_reset_bypass[i].mask);

        *(bsp_sys_reset_bypass[i].address) = bsp_sys_reset_bypass[i].value | 
	  ((*(bsp_sys_reset_bypass[i].address)) & bsp_sys_reset_bypass[i].mask);
      }
    }
  }

  /* Assert and hold reset */
  if (bsp_sys_reset_registers[index].value)
  {
    ICS_PRINTF(ICS_DBG_LOAD, "toggle AH sys reset register %p value 0x%x mask 0x%x\n",
	       bsp_sys_reset_registers[index].address,
	       bsp_sys_reset_registers[index].value,
	       bsp_sys_reset_registers[index].mask);
    
    /* Reset is active high - just use value */
    *(bsp_sys_reset_registers[index].address) |= bsp_sys_reset_registers[index].value;
  }
  else
  {
    ICS_PRINTF(ICS_DBG_LOAD, "toggle AL sys reset register %p mask : 0x%x\n",
	       bsp_sys_reset_registers[index].address,
	       bsp_sys_reset_registers[index].mask);

    /* Reset is active low - just use mask */
    *(bsp_sys_reset_registers[index].address) &= bsp_sys_reset_registers[index].mask;
  }
}

ICS_ERROR ics_cpu_start (ICS_OFFSET entryAddr, ICS_UINT cpuNum, ICS_UINT flags)
{
  ICS_ERROR res;
  ICS_UINT validFlags = 0;
  
  int index;
  
  if (cpuNum >= _ICS_MAX_CPUS || entryAddr == ICS_BAD_OFFSET || (flags & ~validFlags))
    return ICS_INVALID_ARGUMENT;
  
  /* Translate cpuNum into a BSP table index */
  index = ics_cpu_index(cpuNum);
  
  if (index < 0)
    return ICS_NOT_INITIALIZED;

  ICS_PRINTF(ICS_DBG_LOAD, "Starting cpu %d index %d entryAddr 0x%x\n",
	     cpuNum, index, entryAddr);

  res = slaveBoot(index, entryAddr);

  return res;
}

ICS_ERROR ics_cpu_reset (ICS_UINT cpuNum, ICS_UINT flags)
{
  ICS_UINT validFlags = 0;

  int index;

  if (cpuNum >= _ICS_MAX_CPUS || (flags & ~validFlags))
    return ICS_INVALID_ARGUMENT;

  /* Translate cpuNum into a BSP table index */
  index = ics_cpu_index(cpuNum);

  if (index < 0)
    return ICS_NOT_INITIALIZED;

  ICS_PRINTF(ICS_DBG_LOAD, "resetting cpu %d index %d\n",
	     cpuNum, index);

#if defined(__arm__)
  /*  Map the reset registers physical addresses into (uncached) kernel space */
  slaveResetRegMap();
#endif /* __arm__ */
  /* Reset the corresponding CPU */
  slaveReset(index);

  return ICS_SUCCESS;
}

#endif /* __st40__ */

/* Find a CPU in the BSP table and return its logical cpuNum */
int ics_cpu_lookup (const ICS_CHAR *cpuName)
{ 
  int i;

  /* Search BSP CPU table for a match */
  for (i = 0; i < bsp_cpu_count; i++)
  {
    if (!strcmp(cpuName, bsp_cpus[i].name))
    {
      /* Match found - return logical CPU number */
      return bsp_cpus[i].num;
    }
  }
  
  /* No match */
  return -1;
}

/* Return local CPU's logical number */
ICS_INT ics_cpu_self (void)
{
  return ics_cpu_lookup(bsp_cpu_name);
}

/* Return a bitmask of each logical CPU present */
ICS_ULONG ics_cpu_mask (void)
{
  int i;
  ICS_ULONG cpuMask = 0;

  for (i = 0; i < bsp_cpu_count; i++)
  {
    /* -ve num tells us to skip this one */
    if (bsp_cpus[i].num >= 0)
      cpuMask |= (1UL << bsp_cpus[i].num);
  }
  
  return cpuMask;
}

/* Return the CPU name of a given cpuNum */
const char *ics_cpu_name (ICS_UINT cpuNum)
{
  int i;

  if (cpuNum >= _ICS_MAX_CPUS)
    return "invalid";

  for (i = 0; i < bsp_cpu_count; i++)
  {
    if (bsp_cpus[i].num == cpuNum)
      return bsp_cpus[i].name;
  }

  return "unknown";
}

/* Return the CPU type of a given cpuNum */
const char *ics_cpu_type (ICS_UINT cpuNum)
{
  int i;

  if (cpuNum >= _ICS_MAX_CPUS)
    return "invalid";

  for (i = 0; i < bsp_cpu_count; i++)
  {
    if (bsp_cpus[i].num == cpuNum)
      return bsp_cpus[i].type;
  }
  
  return "unknown";

}

/* Return the BSP index of a given cpuNum */
int ics_cpu_index (ICS_UINT cpuNum)
{
  int i;

  if (cpuNum >= _ICS_MAX_CPUS)
    return -1;

  for (i = 0; i < bsp_cpu_count; i++)
  {
    if (bsp_cpus[i].num == cpuNum)
      return i;
  }

  return -1;
}

/*
 * Return a pointer to the ICS version string
 *
 * This string takes the form: 
 *
 *   {major number}.{minor number}.{patch number} : [text]
 *
 * That is, a major, minor and release number, separated by
 * decimal points, and optionally followed by a colon and a printable text string.
 */
const ICS_CHAR *ics_cpu_version (void)
{
  static const ICS_CHAR * version_string = __ICS_VERSION__" ("__TIME__" "__DATE__")";
  
  return version_string;
}


/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

