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
# Linux specific Makefile
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

obj-m  += ics/ mme/ embx/ embxmailbox/

