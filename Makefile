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
ifdef TF7700
CFLAGS+=-DTF7700
endif
ifdef HL101
CFLAGS+=-DHL101
endif
ifdef UFS922
CFLAGS+=-DUFS922
endif
ifdef FORTIS_HDBOX
CFLAGS+=-DFORTIS_HDBOX
endif
obj-y	:= avs/ 
obj-y	+= multicom/
obj-y	+= stgfb/
obj-y	+= player2/
obj-y	+= boxtype/
obj-y	+= simu_button/
obj-y	+= e2_proc/
obj-y	+= frontends/
obj-y	+= cic/
obj-y	+= pti/
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
#obj-y   += zd1211/
endif
ifdef HL101
obj-y   += proton/
endif
