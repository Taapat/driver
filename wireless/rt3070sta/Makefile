ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

RT28xx_MODE               = STA
CHIPSET                   = 3070
OSABL                     = NO

HAS_ATE                   = y
HAS_WPA_SUPPLICANT        = y
HAS_DOT11_N_SUPPORT       = y
HAS_DOT11N_DRAFT3_SUPPORT = y
HAS_KTHREAD_SUPPORT       = y

EXTRA_CFLAGS += -I$(src)/include 
EXTRA_CFLAGS += -Wall -O2 -Wundef -Wstrict-prototypes
EXTRA_CFLAGS += -Wno-trigraphs -Wdeclaration-after-statement 
EXTRA_CFLAGS += -Wno-pointer-sign -fno-strict-aliasing -fno-common 
EXTRA_CFLAGS += -fomit-frame-pointer -ffreestanding -m4-nofpu

WFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT -DLINUX -Wall -Wstrict-prototypes -Wno-trigraphs 
WFLAGS += -DSYSTEM_LOG_SUPPORT -DLED_CONTROL_SUPPORT

ifeq ($(CHIPSET),2070)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRT2070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT
endif

ifeq ($(CHIPSET),3070)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT
endif

WFLAGS += -DCONFIG_STA_SUPPORT -DDBG

ifeq ($(HAS_KTHREAD_SUPPORT),y)
WFLAGS += -DKTHREAD_SUPPORT
endif

ifeq ($(HAS_WPA_SUPPLICANT),y)
WFLAGS += -DWPA_SUPPLICANT_SUPPORT
ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DNATIVE_WPA_SUPPLICANT_SUPPORT
endif
endif

ifeq ($(HAS_ATE),y)
WFLAGS += -DRALINK_ATE
ifeq ($(HAS_QA_SUPPORT),y)
WFLAGS += -DRALINK_28xx_QA
endif
endif

ifeq ($(HAS_DOT11_N_SUPPORT),y)
WFLAGS += -DDOT11_N_SUPPORT
ifeq ($(HAS_DOT11N_DRAFT3_SUPPORT),y)
WFLAGS += -DDOT11N_DRAFT3
endif
endif

ifeq ($(OSABL),YES)
WFLAGS += -DOS_ABL_SUPPORT -DEXPORT_SYMTAB
endif

EXTRA_CFLAGS += -o $(WFLAGS) 

MOD_NAME = rt$(CHIPSET)sta

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
rt$(CHIPSET)sta-objs += common/ba_action.o
endif

ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
rt$(CHIPSET)sta-objs += \
		common/cmm_mat.o \
		common/cmm_mat_iparp.o \
		common/cmm_mat_pppoe.o \
		common/cmm_mat_ipv6.o
endif

ifeq ($(HAS_BLOCK_NET_IF),y)
rt$(CHIPSET)sta-objs += common/netif_block.o
endif

ifeq ($(HAS_QOS_DLS_SUPPORT),y)
rt$(CHIPSET)sta-objs += sta/dls.o
endif

ifeq ($(CHIPSET),2070)
rt$(CHIPSET)sta-objs += \
		common/cmm_mac_usb.o\
		common/rtusb_io.o\
		common/rtusb_bulk.o\
		common/rtusb_data.o\
		common/cmm_data_usb.o\
		common/ee_prom.o\
		common/ee_efuse.o\
		common/rtmp_mcu.o\
		chips/rt30xx.o\
		common/rt_rf.o\
		chips/rt3070.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
		common/rtusb_dev_id.o\
		os/linux/rt_usb.o\
		os/linux/rt_usb_util.o\
		os/linux/usb_main_dev.o
endif
endif

ifeq ($(CHIPSET),3070)
rt$(CHIPSET)sta-objs += \
		common/cmm_mac_usb.o\
		common/rtusb_io.o\
		common/rtusb_bulk.o\
		common/rtusb_data.o\
		common/cmm_data_usb.o\
		common/ee_prom.o\
		common/ee_efuse.o\
		common/rtmp_mcu.o\
		chips/rt30xx.o\
		common/rt_rf.o\
		chips/rt3070.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
		common/rtusb_dev_id.o\
		os/linux/rt_usb.o\
		os/linux/rt_usb_util.o\
		os/linux/usb_main_dev.o
endif
endif

ifeq ($(HAS_ATE),y)
rt$(CHIPSET)sta-objs += 	os/linux/rt_ate.o
endif

ifeq ($(OSABL),NO)
ifeq ($(HAS_CFG80211_SUPPORT),y)
rt$(CHIPSET)sta-objs += os/linux/cfg80211.o
endif
endif

obj-m := $(MOD_NAME).o
