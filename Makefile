# Tuxbox drivers Makefile
# there are only three targets
#
# make all     - builds all modules
# make install - installs the modules
# make clean   - deletes modules recursively
#
# note that "clean" only works in the current
# directory while "all" and "install" will
# execute from the top dir.

ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)
include $(DRIVER_TOPDIR)/kernel.make
else
CCFLAGSY    += -D__TDT__ -D__LINUX__ -D__SH4__ -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB

CONFIGFILE := $(DRIVER_TOPDIR)/.config

include $(CONFIGFILE)

ifdef HAVANA_P0207_5
CCFLAGSY+=-DHAVANA_P0207_5
endif

ifdef HL101
CCFLAGSY += -DHL101
endif
ifdef SPARK
CCFLAGSY+=-DSPARK
endif
ifdef SPARK7162
CCFLAGSY+=-DSPARK7162
endif
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

export CCFLAGSY

obj-y	:= avs/
obj-y	+= multicom/
obj-y	+= stgfb/
obj-y	+= player2/
obj-y	+= boxtype/
obj-y	+= simu_button/
obj-y	+= e2_proc/
obj-y	+= frontends/
obj-y	+= frontcontroller/
obj-y	+= wireless/

ifneq (,$(findstring pti_np,$(shell ls $(DRIVER_TOPDIR))))
obj-y	+= pti_np/
else
obj-y	+= pti/
endif

obj-y	+= compcache/
obj-y	+= bpamem/


ifdef  HL101
obj-y	+= smartcard/
endif

ifndef SPARK
ifndef SPARK7162
obj-y	+= cic/
endif
endif

ifdef SPARK
obj-y	+= smartcard/
obj-y	+= cpu_frequ/
obj-y	+= cec/
endif

ifdef SPARK7162
obj-y	+= i2c_spi/
obj-y	+= smartcard/
obj-y	+= cec/
endif

endif
