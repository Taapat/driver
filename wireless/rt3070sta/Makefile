ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

RT28xx_MODE               = STA
CHIPSET                   = 3070
OSABL                     = NO

HAS_WPA_SUPPLICANT	  = y
HAS_NATIVE_WPA_SUPPLICANT_SUPPORT=y
HAS_DOT11_N_SUPPORT       = y
HAS_DOT11N_DRAFT3_SUPPORT = y
HAS_KTHREAD_SUPPORT       = y


EXTRA_CFLAGS += -I$(src)/include 
EXTRA_CFLAGS += -Wall -O2 -Wundef -Wstrict-prototypes
EXTRA_CFLAGS += -Wno-trigraphs -Wdeclaration-after-statement 
EXTRA_CFLAGS += -Wno-pointer-sign -fno-strict-aliasing -fno-common 
EXTRA_CFLAGS += -fomit-frame-pointer -ffreestanding -m4-nofpu

WFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT  -DLINUX -Wall -Wstrict-prototypes -Wno-trigraphs 
WFLAGS += -DSYSTEM_LOG_SUPPORT  -DRT28xx_MODE=$(RT28xx_MODE) -DCHIPSET=$(CHIPSET) -DRESOURCE_PRE_ALLOC
WFLAGS += -DST

ifeq ($(CHIPSET),2070)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRT2070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET),3070)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET),3370)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT33xx -DRT3070 -DRT3370 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(HAS_RESOURCE_BOOT_ALLOC),y)
WFLAGS += -DRESOURCE_BOOT_ALLOC
endif

ifeq ($(HAS_KTHREAD_SUPPORT),y)
WFLAGS += -DKTHREAD_SUPPORT
endif

ifeq ($(RT28xx_MODE),STA)
WFLAGS += -DCONFIG_STA_SUPPORT -DDBG

ifeq ($(HAS_XLINK),y)
WFLAGS += -DXLINK_SUPPORT
endif


ifeq ($(HAS_WPA_SUPPLICANT),y)
WFLAGS += -DWPA_SUPPLICANT_SUPPORT
ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DNATIVE_WPA_SUPPLICANT_SUPPORT -DCREDENTIAL_STORE -DPROFILE_STORE
endif
endif

ifeq ($(OSABL),YES)
HAS_OSABL_FUNC_SUPPORT=y
HAS_OSABL_OS_PCI_SUPPORT=y
HAS_OSABL_OS_USB_SUPPORT=y
HAS_OSABL_OS_RBUS_SUPPORT=n
HAS_OSABL_OS_AP_SUPPORT=y
HAS_OSABL_OS_STA_SUPPORT=y
endif


ifeq ($(HAS_ATE),y)
WFLAGS += -DRALINK_ATE
WFLAGS += -DCONFIG_RT2880_ATE_CMD_NEW
ifeq ($(HAS_NEW_TXCONT),y)
WFLAGS += -DNEW_TXCONT
endif
ifeq ($(HAS_QA_SUPPORT),y)
WFLAGS += -DRALINK_QA
endif
endif


ifeq ($(HAS_SNMP_SUPPORT),y)
WFLAGS += -DSNMP_SUPPORT
endif

ifeq ($(HAS_QOS_DLS_SUPPORT),y)
WFLAGS += -DQOS_DLS_SUPPORT
endif

ifeq ($(HAS_DOT11_N_SUPPORT),y)
WFLAGS += -DDOT11_N_SUPPORT
ifeq ($(HAS_DOT11N_DRAFT3_SUPPORT),y)
WFLAGS += -DDOT11N_DRAFT3
endif
endif


ifeq ($(HAS_CS_SUPPORT),y)
WFLAGS += -DCARRIER_DETECTION_SUPPORT
endif

ifeq ($(HAS_STATS_COUNT),y)
WFLAGS += -DSTATS_COUNT_SUPPORT
endif

ifeq ($(HAS_USB_SUPPORT_SELECTIVE_SUSPEND),y)
WFLAGS += -DUSB_SUPPORT_SELECTIVE_SUSPEND -DCONFIG_PM
endif


ifeq ($(HAS_CFG80211_SUPPORT),y)
WFLAGS += -DRT_CFG80211_SUPPORT
ifeq ($(HAS_RFKILL_HW_SUPPORT),y)
WFLAGS += -DRFKILL_HW_SUPPORT
endif
endif

endif
# endif of ifeq ($(RT28xx_MODE),STA)

EXTRA_CFLAGS += -o $(WFLAGS) 

MOD_NAME = rt$(CHIPSET)sta

#ifdef CONFIG_STA_SUPPORT
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
	common/rtmp_init_inf.o\
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
	os/linux/rt_profile.o\
	chips/rtmp_chip.o\
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
	common/rt_os_util.o\
	os/linux/sta_ioctl.o\
	os/linux/rt_linux.o\
	os/linux/rt_main_dev.o
else
rt$(CHIPSET)sta-objs += \
	os/linux/rt_symb.o
endif

#ifdef DOT11_N_SUPPORT
ifeq ($(HAS_DOT11_N_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
	common/ba_action.o
endif
#endif // DOT11_N_SUPPORT //

#ifdef ETH_CONVERT
ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
rt$(CHIPSET)sta-objs += \
	common/cmm_mat.o \
	common/cmm_mat_iparp.o \
	common/cmm_mat_pppoe.o \
	common/cmm_mat_ipv6.o
endif
#endif // ETH_CONVERT //





ifeq ($(HAS_BLOCK_NET_IF),y)
rt$(CHIPSET)sta-objs += common/netif_block.o
endif

ifeq ($(HAS_QOS_DLS_SUPPORT),y)
rt$(CHIPSET)sta-objs += sta/dls.o
endif



#ifdef LED_CONTROL_SUPPORT
ifeq ($(HAS_LED_CONTROL_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
	common/rt_led.o
endif
#endif // LED_CONTROL_SUPPORT //





#ifdef RT3070
ifeq ($(CHIPSET),2070)
rt$(CHIPSET)sta-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rtmp_mcu.o\
	chips/rt30xx.o\
	common/rt_rf.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	chips/rt3070.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif

ifeq ($(CHIPSET),3070)
rt$(CHIPSET)sta-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rtmp_mcu.o\
	chips/rt30xx.o\
	common/rt_rf.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	chips/rt3070.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif
#endif // RT3070 //

#ifdef RT3370
ifeq ($(CHIPSET),3370)
rt$(CHIPSET)sta-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtusb_bulk.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rtmp_mcu.o\
	common/rt_rf.o\
	chips/rt3070.o\
	chips/rt30xx.o\
	chips/rt33xx.o\
	chips/rt3370.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif
#endif // RT3370 //

ifeq ($(CHIPSET),USB)
rt$(CHIPSET)sta-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rtmp_mcu.o\
	common/rt_rf.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	chips/rt28xx.o\
	chips/rt3070.o\
	chips/rt30xx.o\
	chips/rt33xx.o\
	chips/rt35xx.o\
	chips/rt3370.o\
	chips/rt5390.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif
ifeq ($(HAS_ATE),y)
rt$(CHIPSET)sta-objs += common/rt_ate.o
endif


#ifdef CRDA_SUPPORT
ifeq ($(OSABL),NO)
ifeq ($(HAS_CFG80211_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
	os/linux/cfg80211.o \
	os/linux/cfg80211drv.o
endif
endif

ifeq ($(OSABL),YES)
ifeq ($(HAS_CFG80211_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
	os/linux/cfg80211drv.o
endif
endif
#endif // CRDA_SUPPORT //


#endif // CONFIG_STA_SUPPORT //

obj-m := $(MOD_NAME).o
