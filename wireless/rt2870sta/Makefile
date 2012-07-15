ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

RT28xx_MODE               = STA
CHIPSET                   = 2870
OSABL                     = NO

HAS_WPA_SUPPLICANT        = y
HAS_NATIVE_WPA_SUPPLICANT_SUPPORT=y
HAS_DOT11_N_SUPPORT       = y
HAS_DOT11N_DRAFT3_SUPPORT = y
HAS_STATS_COUNT           = y
HAS_RESOURCE_PRE_ALLOC    = y


EXTRA_CFLAGS += -I$(src)/include 
EXTRA_CFLAGS += -Wall -O2 -Wundef -Wstrict-prototypes
EXTRA_CFLAGS += -Wno-trigraphs -Wdeclaration-after-statement 
EXTRA_CFLAGS += -Wno-pointer-sign -fno-strict-aliasing -fno-common 
EXTRA_CFLAGS += -fomit-frame-pointer -ffreestanding -m4-nofpu

WFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT  -DLINUX -Wall -Wstrict-prototypes -Wno-trigraphs 
WFLAGS += -DSYSTEM_LOG_SUPPORT -DLED_CONTROL_SUPPORT
WFLAGS += -DCONFIG_STA_SUPPORT -DDBG

ifeq ($(HAS_RESOURCE_PRE_ALLOC),y)
WFLAGS += -DRESOURCE_PRE_ALLOC
endif

ifeq ($(HAS_WPA_SUPPLICANT),y)
WFLAGS += -DWPA_SUPPLICANT_SUPPORT
ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DNATIVE_WPA_SUPPLICANT_SUPPORT
endif
endif

ifeq ($(HAS_DOT11_N_SUPPORT),y)
WFLAGS += -DDOT11_N_SUPPORT
ifeq ($(HAS_DOT11N_DRAFT3_SUPPORT),y)
WFLAGS += -DDOT11N_DRAFT3
endif
endif

ifeq ($(HAS_STATS_COUNT),y)
WFLAGS += -DSTATS_COUNT_SUPPORT
endif

ifeq ($(OSABL),YES)
WFLAGS += -DOS_ABL_SUPPORT -DEXPORT_SYMTAB
endif

ifeq ($(HAS_CLIENT_WDS_SUPPORT),y)
WFLAGS += -DCLIENT_WDS
endif

ifeq ($(CHIPSET),2870)
WFLAGS +=-DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT2870 -DRT28xx -DRTMP_TIMER_TASK_SUPPORT -DA_BAND_SUPPORT
CHIPSET_DAT = 2870
endif

EXTRA_CFLAGS += -o $(WFLAGS) 

MOD_NAME = rt$(CHIPSET)sta
DAT_PATH = /etc/Wireless/RT2870STA
DAT_FILE_NAME = RT2870STA.dat

rt$(CHIPSET)sta-objs := \
		common/crypt_md5.o\
		common/crypt_sha2.o\
		common/crypt_hmac.o\
		common/crypt_aes.o\
		common/crypt_arc4.o\
		common/mlme.o\
		common/cmm_wep.o\
		common/action.o\
		common/cmm_data.o\
		common/rtmp_init.o\
		common/cmm_tkip.o\
		common/cmm_aes.o\
		common/cmm_sync.o\
		common/eeprom.o\
		common/cmm_sanity.o\
		common/cmm_info.o\
		common/cmm_cfg.o\
		common/cmm_wpa.o\
		common/dfs.o\
		common/spectrum.o\
		common/rtmp_timer.o\
		common/rt_channel.o\
		common/cmm_profile.o\
		common/cmm_asic.o\
		common/cmm_cmd.o\
		sta/assoc.o\
		sta/auth.o\
		sta/auth_rsp.o\
		sta/sync.o\
		sta/sanity.o\
		sta/rtmp_data.o\
		sta/connect.o\
		sta/wpa.o\
		sta/ags.o\
		sta/sta_cfg.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
		common/rtmp_init_inf.o\
		os/linux/rt_profile.o\
		os/linux/sta_ioctl.o\
		os/linux/rt_linux.o\
		os/linux/rt_main_dev.o
endif

ifeq ($(HAS_DOT11_N_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
		common/ba_action.o
endif

rt$(CHIPSET)sta-objs += \
		common/cmm_mac_usb.o\
		common/rtusb_io.o\
		common/rtusb_bulk.o\
		common/rtusb_data.o\
		common/cmm_data_usb.o\
		common/ee_prom.o\
		common/rtmp_mcu.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
		common/rtusb_dev_id.o\
		os/linux/rt_usb.o\
		os/linux/rt_usb_util.o\
		os/linux/usb_main_dev.o
endif

obj-m := $(MOD_NAME).o
