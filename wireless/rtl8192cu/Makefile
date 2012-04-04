EXTRA_CFLAGS += -O1 
#EXTRA_CFLAGS += -O3
#EXTRA_CFLAGS += -Wall 
#EXTRA_CFLAGS += -Wextra 
#EXTRA_CFLAGS += -Werror
#EXTRA_CFLAGS += -pedantic
#EXTRA_CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes 
EXTRA_CFLAGS += -Wno-unused-variable -Wno-unused-value -Wno-unused-label -Wno-unused-parameter -Wno-uninitialized
EXTRA_CFLAGS += -Wno-unused -Wno-unused-function
EXTRA_CFLAGS += -I$(src)/include

CONFIG_BUILT_IN                 =       n

CONFIG_RTL8712			=	n
CONFIG_RTL8192C			=       y

CONFIG_USB_HCI			=	y
CONFIG_SDIO_HCI			= 	n
CONFIG_MP_INCLUDED		=	n

CONFIG_PLATFORM_I386_PC	        =	y

CONFIG_PLATFORM_ARM_S3C2K4		= 	n
CONFIG_PLATFORM_ARM_PXA2XX		= 	n
CONFIG_PLATFORM_ARM_S3C6K4		= 	n
CONFIG_PLATFORM_MIPS_RMI	= 	n
CONFIG_PLATFORM_RTD2880B	=	n
CONFIG_PLATFORM_MIPS_AR9132	= 	n
CONFIG_PLATFORM_MT53XX			=	n
CONFIG_PLATFORM_RTK_DMP			=	n

CONFIG_DRVEXT_MODULE	        =       n

export TOPDIR := $(PWD)


ifeq ($(CONFIG_RTL8712), y)

RTL871X = rtl8712

ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME = 8712s
endif
ifeq ($(CONFIG_USB_HCI), y)
MODULE_NAME = 8712u
endif

endif

ifeq ($(CONFIG_RTL8192C), y)

RTL871X = rtl8192c

ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME = 8192cs
endif
ifeq ($(CONFIG_USB_HCI), y)
MODULE_NAME = 8192cu
endif

endif


ifeq ($(CONFIG_SDIO_HCI), y)

 
_OS_INTFS_FILES := os_intf/osdep_service.o \
                    os_intf/linux/os_intfs.o \
                    os_intf/osdep_sdio_intf.o \
		    		os_intf/linux/sdio_intf.o \

_HAL_INTFS_FILES := hal/$(RTL871X)/hal_init.o \
		    hal/$(RTL871X)/sdio_halinit.o \
		    hal/$(RTL871X)/sdio_ops.o \
		    hal/$(RTL871X)/sdio_ops_linux.o    	

endif


ifeq ($(CONFIG_USB_HCI), y)
 
ifeq ($(CONFIG_BUILT_IN), y)
$(shell cp $(src)/autoconf_$(RTL871X)_usb_linux.h $(src)/include/autoconf.h)
else
$(shell cp $(PWD)/autoconf_$(RTL871X)_usb_linux.h $(PWD)/include/autoconf.h)
endif


PWD := $(shell pwd)


ifeq ($(CONFIG_PLATFORM_I386_PC), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
SUBARCH := $(shell uname -m | sed -e s/i.86/i386/)
ARCH ?= $(SUBARCH)
CROSS_COMPILE ?=
KVER  := $(shell uname -r)
KSRC := /lib/modules/$(KVER)/build
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/
INSTALL_PREFIX :=
endif

ifeq ($(CONFIG_PLATFORM_ARM_PXA2XX), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
ARCH := arm
CROSS_COMPILE := arm-none-linux-gnueabi-
KVER  := 2.6.34.1
KSRC ?= /usr/src/linux-2.6.34.1
endif

ifeq ($(CONFIG_PLATFORM_ARM_S3C2K4), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
ARCH := arm
CROSS_COMPILE := arm-linux-
KVER  := 2.6.24.7_$(ARCH)
KSRC := /usr/src/kernels/linux-$(KVER)
endif

ifeq ($(CONFIG_PLATFORM_ARM_S3C6K4), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
ARCH := arm
CROSS_COMPILE := arm-none-linux-gnueabi-
KVER  := 2.6.34.1
KSRC ?= /usr/src/linux-2.6.34.1
endif

ifeq ($(CONFIG_PLATFORM_RTD2880B), y)
EXTRA_CFLAGS += -DCONFIG_BIG_ENDIAN -DCONFIG_PLATFORM_RTD2880B
ARCH:=
CROSS_COMPILE:=
KVER:=
KSRC:=
endif

ifeq ($(CONFIG_PLATFORM_MIPS_RMI), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
ARCH:=mips
CROSS_COMPILE:=mipsisa32r2-uclibc-
KVER:= 
KSRC:= /root/work/kernel_realtek
endif

ifeq ($(CONFIG_PLATFORM_MIPS_PLM), y)
EXTRA_CFLAGS += -DCONFIG_BIG_ENDIAN
ARCH:=mips
CROSS_COMPILE:=mipsisa32r2-uclibc-
KVER:= 
KSRC:= /root/work/kernel_realtek
endif

ifeq ($(CONFIG_PLATFORM_MSTAR389), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN -DCONFIG_PLATFORM_MSTAR389
ARCH:=mips
CROSS_COMPILE:= mips-linux-gnu-
KVER:= 2.6.28.10
KSRC:= /home/mstar/mstar_linux/2.6.28.9/
endif

ifeq ($(CONFIG_PLATFORM_MIPS_AR9132), y)
EXTRA_CFLAGS += -DCONFIG_BIG_ENDIAN
ARCH := mips
CROSS_COMPILE := mips-openwrt-linux-
KSRC := /home/alex/test_openwrt/tmp/linux-2.6.30.9
endif

ifeq ($(CONFIG_PLATFORM_MT53XX), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN -DCONFIG_PLATFORM_MT53XX
ARCH:= arm
CROSS_COMPILE:= arm11_mtk_le-
KVER:= 2.6.27
KSRC?= /proj/mtk00802/BD_Compare/BDP/Dev/BDP_V301/BDP_Linux/linux-2.6.27
endif

ifeq ($(CONFIG_PLATFORM_RTK_DMP), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN -DRTK_DMP_PLATFORM
ARCH:=mips
CROSS_COMPILE:=mipsel-linux-
KVER:= 
KSRC ?= /root/Desktop/SVN/linux-2.6.12
endif

 
_OS_INTFS_FILES :=	os_dep/osdep_service.o \
			os_dep/linux/os_intfs.o \
			os_dep/linux/usb_intf.o \
			os_dep/linux/ioctl_linux.o \
			os_dep/linux/xmit_linux.o \
			os_dep/linux/mlme_linux.o \
			os_dep/linux/recv_linux.o \


_HAL_INTFS_FILES :=	hal/hal_init.o \
                        hal/rtl8192c_d_hal_init.o \
			hal/$(RTL871X)/$(RTL871X)_phycfg.o \
			hal/$(RTL871X)/$(RTL871X)_rf6052.o \
			hal/$(RTL871X)/$(RTL871X)_dm.o \
			hal/$(RTL871X)/$(RTL871X)_rxdesc.o \
			hal/$(RTL871X)/usb/usb_ops_linux.o \
			hal/$(RTL871X)/usb/usb_halinit.o \
			hal/$(RTL871X)/usb/Hal8192CUHWImg.o \
			hal/$(RTL871X)/usb/rtl$(MODULE_NAME)_xmit.o \
			hal/$(RTL871X)/usb/rtl$(MODULE_NAME)_recv.o \
			hal/$(RTL871X)/usb/$(RTL871X)_cmd.o \
		
endif


ifneq ($(KERNELRELEASE),)


rtk_core :=	core/rtw_cmd.o \
		core/rtw_security.o \
		core/rtw_debug.o \
		core/rtw_io.o \
		core/rtw_ioctl_query.o \
		core/rtw_ioctl_set.o \
		core/ieee80211.o \
		core/rtw_mlme.o \
		core/rtw_mlme_ext.o \
		core/rtw_wlan_util.o \
		core/rtw_pwrctrl.o \
		core/rtw_rf.o \
		core/rtw_recv.o \
		core/rtw_sta_mgt.o \
		core/rtw_xmit.o \

ifeq ($(CONFIG_BUILT_IN), y)

obj-y += rtk_core.o

obj-y += $(MODULE_NAME).o

#obj-y := $(MODULE_NAME).o

else

obj-m := $(MODULE_NAME).o

$(MODULE_NAME)-y += $(rtk_core)

endif

									
$(MODULE_NAME)-y += core/efuse/rtl8712_efuse.o

$(MODULE_NAME)-y += core/led/$(RTL871X)_led.o

$(MODULE_NAME)-y += $(_HAL_INTFS_FILES)

$(MODULE_NAME)-y += $(_OS_INTFS_FILES)


$(MODULE_NAME)-$(CONFIG_MP_INCLUDED) += mp/rtl871x_mp.o \
					mp/rtl871x_mp_ioctl.o \
					ioctl/rtl871x_ioctl_rtl.o

else

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(PWD)  modules

install:
	install -p -m 644 $(MODULE_NAME).ko  $(MODDESTDIR)
	/sbin/depmod -a ${KVER}

uninstall:
	rm -f $(MODDESTDIR)/$(MODULE_NAME).ko
	/sbin/depmod -a ${KVER}
	
	
config_r:
	@echo "make config"
	/bin/bash script/Configure script/config.in
	
.PHONY: modules clean

clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm .tmp_versions -fr ; rm Module.symvers -fr
	rm -fr Module.markers ; rm -fr modules.order
	cd core/efuse ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd core/led ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd core ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd hal/$(RTL871X)/usb ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd hal/$(RTL871X)/pcie ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd hal/$(RTL871X) ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd os_dep/linux ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd os_dep ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 

endif

