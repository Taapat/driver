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

ifdef OCTAGON1008
CCFLAGSY+=-DOCTAGON1008
endif

ifdef FLASH_UFS910
CCFLAGSY += -DUFS910
endif

ifdef UFS910
CCFLAGSY += -DUFS910
endif

ifdef FLASH_UFS910
CCFLAGSY += -DFLASH_UFS910
endif

ifdef CUBEREVO
CCFLAGSY += -DCUBEREVO
endif
ifdef CUBEREVO_MINI
CCFLAGSY += -DCUBEREVO_MINI
endif
ifdef CUBEREVO_MINI2
CCFLAGSY += -DCUBEREVO_MINI2
endif
ifdef CUBEREVO_250HD
CCFLAGSY += -DCUBEREVO_250HD
endif
ifdef CUBEREVO_2000HD
CCFLAGSY += -DCUBEREVO_2000HD
endif
ifdef CUBEREVO_9500HD
CCFLAGSY += -DCUBEREVO_9500HD
endif
ifdef CUBEREVO_MINI_FTA
CCFLAGSY += -DCUBEREVO_MINI_FTA
endif
ifdef TF7700
CCFLAGSY += -DTF7700
endif
ifdef HL101
CCFLAGSY += -DHL101
endif
ifdef VIP1_V2
CCFLAGSY += -DVIP1_V2
endif
ifdef VIP2_V1
CCFLAGSY += -DVIP2_V1
endif
ifdef UFS922
CCFLAGSY+=-DUFS922
endif
ifdef UFS912
CCFLAGSY+=-DUFS912
endif
ifdef SPARK
CCFLAGSY+=-DSPARK
endif
ifdef FORTIS_HDBOX
CCFLAGSY += -DFORTIS_HDBOX
endif
ifdef HOMECAST5101
CCFLAGSY += -DHOMECAST5101
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
#obj-y	+= player2/
obj-y	+= boxtype/
obj-y	+= simu_button/
obj-y	+= e2_proc/
obj-y	+= frontends/
obj-y	+= frontcontroller/
obj-y	+= pti/
#obj-y	+= pti_np/
obj-y	+= compcache/

ifdef STM22
obj-y	+= logfs/
endif

#obj-y	+= proc_register/

ifndef VIP2_V1
ifndef SPARK
obj-y	+= cic/
endif
endif

ifndef HOMECAST5101
ifndef FORTIS_HDBOX
ifndef UFS922
ifndef TF7700
obj-y	+= button/
obj-y	+= led/
endif
endif
endif
endif

ifdef UFS922
obj-y	+= ufs922_fan/
endif

ifdef HOMECAST5101
obj-y	+= button_hs5101/
obj-y	+= player2/linux/drivers/media/dvb/stm/dvb
endif

ifdef UFS912
#obj-y	+= cec/
obj-y	+= clk/
endif

ifdef SPARK
obj-y   += clk/
endif

endif
