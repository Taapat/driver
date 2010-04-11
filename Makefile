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
CFLAGS+=-D__TDT__
ifdef UFS910
CFLAGS+=-DUFS910
endif
ifdef CUBEREVO
CFLAGS+=-DCUBEREVO
endif
ifdef CUBEREVO_MINI
CFLAGS+=-DCUBEREVO_MINI
endif
ifdef CUBEREVO_MINI2
CFLAGS+=-DCUBEREVO_MINI2
endif
ifdef CUBEREVO_250HD
CFLAGS+=-DCUBEREVO_250HD
endif
ifdef TF7700
CFLAGS+=-DTF7700
endif
ifdef HL101
CFLAGS+=-DHL101
endif
ifdef VIP2
CFLAGS+=-DVIP2
endif
ifdef UFS922
CFLAGS+=-DUFS922
endif
ifdef FORTIS_HDBOX
CFLAGS+=-DFORTIS_HDBOX
endif
ifdef HOMECAST5101
CFLAGS+=-DHOMECAST5101
endif
obj-y	:= avs/ 
obj-y	+= multicom/
obj-y	+= stgfb/
#obj-y	+= player2/
obj-y	+= boxtype/
obj-y	+= simu_button/
obj-y	+= e2_proc/
obj-y	+= frontends/
obj-y	+= pti/
obj-y	+= compcache/
ifndef VIP2
obj-y	+= cic/
endif
ifndef HOMECAST5101
ifndef FORTIS_HDBOX
ifndef UFS922
ifndef TF7700
obj-y	+= button/
obj-y	+= led/
obj-y	+= vfd/
else
obj-y	+= tffp/
endif
else
obj-y	+= micom/
obj-y	+= ufs922_fan/
endif
else
obj-y	+= nuvoton/
endif
else
obj-y	+= button_hs5101/
obj-y	+= vfd_hs5101/
obj-y	+= player2/linux/drivers/media/dvb/stm/dvb
endif
#obj-y   += zd1211/
endif
# HL101 = argus vip1, opticum 9500hd, truman tm900hd... 
ifdef HL101
obj-y   += proton/
endif
ifdef VIP2
obj-y   += aotom/
endif
