#*******************************************************************************
# Copyright (C) 2010   STMicroelectronics. All Rights Reserved.
# This file is part of the latest release of the Multicom4 project. This release 
# is fully functional and provides all of the original MME functionality.This 
# release  is now considered stable and ready for integration with other software 
# components.

 # Multicom4 is a free software; you can redistribute it and/or modify it under the 
# terms of the GNU General Public License as published by the Free Software Foundation 
# version 2.

# Multicom4 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
# See the GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along with Multicom4; 
# see the file COPYING.  If not, write to the Free Software Foundation, 59 Temple Place - 
# Suite 330, Boston, MA 02111-1307, USA.

# Written by Multicom team at STMicroelectronics in November 2010.  
# Contact multicom.support@st.com. 
#*******************************************************************************

#
# Linux specific Makefile for ICS subsystem module
#
#

CCFLAGSY    += -D__LINUX__ -D__SH4__ -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -DMULTICOM_GPL
EXTRA_CFLAGS += -I$(DRIVER_TOPDIR)/include/multicom 
EXTRA_CFLAGS += -I$(DRIVER_TOPDIR)/multicom/src/include 
EXTRA_CFLAGS += -I$(DRIVER_TOPDIR)/multicom/src/mme/include 
EXTRA_CFLAGS += -I$(DRIVER_TOPDIR)/multicom/src/embx/include 
EXTRA_CFLAGS += -I$(DRIVER_TOPDIR)/multicom/src/ics/include
CCFLAGSY    += -fno-common -ffreestanding -fno-omit-frame-pointer -fno-optimize-sibling-calls
CCFLAGSY    += -fno-strict-aliasing -fno-stack-protector
CCFLAGSY    += -Wall
CCFLAGSY    += -Wundef -Wstrict-prototypes -Wno-trigraphs 
CCFLAGSY    += -Wdeclaration-after-statement -Wno-pointer-sign
CCFLAGSY    += -O2

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

#
# Inside Linux kernel tree
#

obj-m   := ics.o ics_user.o

# ICS specific CFLAGS
EXTRA_CFLAGS += -I$(obj)/../../include -I$(obj)/../../include/bsp -I$(obj)/include  $(DEBUG_CFLAGS) $(MULTICOM_EXTRA_CFLAGS)

# Allow a non local BSP source directory.
# PATH specified must be relative to this directory though
BSP_SRCDIR ?= ../bsp

# Assume defualt cpu as st40 .. then change it based in SOC type.
defualt-cpu	:=st40

ifdef CONFIG_CPU_SUBTYPE_STX7105
bsp-prefix := $(BSP_SRCDIR)/stx7105
endif

ifdef CONFIG_CPU_SUBTYPE_STX7108
bsp-prefix := $(BSP_SRCDIR)/stx7108
bsp-core   := host
endif

ifdef CONFIG_CPU_SUBTYPE_STX7111
bsp-prefix := $(BSP_SRCDIR)/stx7111
endif

ifdef CONFIG_CPU_SUBTYPE_STX7141
bsp-prefix := $(BSP_SRCDIR)/stx7141
bsp-core   := estb
endif

ifdef CONFIG_CPU_SUBTYPE_STX7200
bsp-prefix := $(BSP_SRCDIR)/stx7200
endif

ifdef CONFIG_CPU_SUBTYPE_STX5206
bsp-prefix := $(BSP_SRCDIR)/stx5289
endif

ifdef CONFIG_CPU_SUBTYPE_STX5289
bsp-prefix := $(BSP_SRCDIR)/stx5289
endif

ifdef CONFIG_CPU_SUBTYPE_FLI7510
bsp-prefix := $(BSP_SRCDIR)/fli7510
endif

ifdef CONFIG_CPU_SUBTYPE_STXH205
bsp-prefix := $(BSP_SRCDIR)/stxh205
endif

ifdef CONFIG_MACH_STM_STIH415
bsp-prefix := $(BSP_SRCDIR)/stxh415
defualt-cpu	:=cortexa9
endif

ifdef CONFIG_MACH_STM_FLI7610
bsp-prefix := $(BSP_SRCDIR)/fli7610
defualt-cpu	:=cortexa9
endif

ics-objs := \
	admin/admin.o \
	admin/admin_client.o \
	channel/channel.o \
	connect/connect.o \
	cpu/cpu.o \
	debug/debug.o \
	debug/debug_mem.o \
	debug/debug_mmap.o \
	dyn/dyn.o \
	dyn/dyn_client.o \
	elf/elf.o \
	event/event.o \
	heap/heap.o \
	init/ics_init.o \
	linux/ics_module.o \
	linux/ics_module_procfs.o \
	linux/linux.o \
	linux/linux_debug.o \
	linux/linux_event.o \
	linux/linux_task.o \
	load/load.o \
	mailbox/mailbox.o \
	msg/msg_cancel.o \
	msg/msg_recv.o \
	msg/msg_send.o \
	nsrv/nsrv.o \
	nsrv/nsrv_client.o \
	port/port.o \
	region/region.o \
	shm/shm_debug.o \
	shm/shm_init.o \
	shm/shm_channel_alloc.o \
	shm/shm_channel_open.o \
	shm/shm_channel_recv.o \
	shm/shm_channel_send.o \
	shm/shm_connect.o \
	shm/shm_stats.o \
	shm/shm_watchdog.o \
	stats/stats.o \
	transport/transport.o \
	watchdog/watchdog.o \
	$(bsp-prefix)/cores.o \
	$(bsp-prefix)/$(defualt-cpu)/$(bsp-core)/name.o \
	$(bsp-prefix)/$(defualt-cpu)/reset.o \
	$(bsp-prefix)/$(defualt-cpu)/$(bsp-core)/mbox.o \

ics_user-objs := \
	linux/ics_user_module.o
