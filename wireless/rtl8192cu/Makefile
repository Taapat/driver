ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

EXTRA_CFLAGS += -O2 -DCONFIG_LITTLE_ENDIAN
EXTRA_CFLAGS += -Wno-unused-variable -Wno-unused-value -Wno-unused-label -Wno-unused-parameter -Wno-uninitialized
EXTRA_CFLAGS += -Wno-unused -Wno-unused-function
EXTRA_CFLAGS += -I$(src)/include

CONFIG_AUTOCFG_CP               = n
CONFIG_RTL8712					= n
CONFIG_RTL8192C					= y
CONFIG_USB_HCI					= y
CONFIG_SDIO_HCI					= n
CONFIG_MP_INCLUDED				= n
CONFIG_POWER_SAVING				= n
CONFIG_USB_AUTOSUSPEND			= n
CONFIG_HW_PWRP_DETECTION		= n
CONFIG_RTL8192CU_REDEFINE_1X1 	= n
CONFIG_DRVEXT_MODULE			= n

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
 
ifeq ($(CONFIG_POWER_SAVING), y)
EXTRA_CFLAGS += -DCONFIG_POWER_SAVING
endif

ifeq ($(CONFIG_USB_AUTOSUSPEND), y)
EXTRA_CFLAGS += -DCONFIG_USB_AUTOSUSPEND
endif

ifeq ($(CONFIG_HW_PWRP_DETECTION), y)
EXTRA_CFLAGS += -DCONFIG_HW_PWRP_DETECTION
endif

ifeq ($(CONFIG_RTL8192CU_REDEFINE_1X1), y)
EXTRA_CFLAGS += -DRTL8192C_RECONFIG_TO_1T1R
endif 

EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN

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

RTK_CORE :=	core/rtw_cmd.o \
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


$(MODULE_NAME)-y += $(RTK_CORE)							
$(MODULE_NAME)-y += core/efuse/rtl8712_efuse.o
$(MODULE_NAME)-y += core/led/$(RTL871X)_led.o
$(MODULE_NAME)-y += $(_HAL_INTFS_FILES)
$(MODULE_NAME)-y += $(_OS_INTFS_FILES)

$(MODULE_NAME)-$(CONFIG_MP_INCLUDED) += mp/rtl871x_mp.o \
					mp/rtl871x_mp_ioctl.o \
					ioctl/rtl871x_ioctl_rtl.o

obj-m := $(MODULE_NAME).o

