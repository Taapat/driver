ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

RT28xx_MODE               = STA
CHIPSET                   = 5370
OSABL                     = NO

HAS_ATE                   = y
HAS_QA_SUPPORT            = y
HAS_DOT11_N_SUPPORT       = y
HAS_LED_CONTROL_SUPPORT   = y

EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -Wall -O2 -Wundef -Wstrict-prototypes
EXTRA_CFLAGS += -Wno-trigraphs -Wdeclaration-after-statement 
EXTRA_CFLAGS += -Wno-pointer-sign -fno-strict-aliasing -fno-common 
EXTRA_CFLAGS += -fomit-frame-pointer -ffreestanding -m4-nofpu

WFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT  -DLINUX -Wall -Wstrict-prototypes -Wno-trigraphs 
WFLAGS += -DSYSTEM_LOG_SUPPORT  -DRT28xx_MODE=$(RT28xx_MODE) -DCHIPSET=$(CHIPSET) -DRESOURCE_PRE_ALLOC -DST -D__KERNEL__

ifeq ($(RT28xx_MODE),STA)
WFLAGS += -DCONFIG_STA_SUPPORT -DDBG
endif

ifeq ($(HAS_ATE),y)
WFLAGS += -DRALINK_ATE
WFLAGS += -DCONFIG_RT2880_ATE_CMD_NEW
ifeq ($(HAS_QA_SUPPORT),y)
WFLAGS += -DRALINK_QA
endif
endif

ifeq ($(HAS_DOT11_N_SUPPORT),y)
WFLAGS += -DDOT11_N_SUPPORT
endif

ifeq ($(HAS_LED_CONTROL_SUPPORT),y)
WFLAGS += -DLED_CONTROL_SUPPORT
endif

ifeq ($(CHIPSET),2860)
WFLAGS +=-DRTMP_MAC_PCI -DRTMP_PCI_SUPPORT -DRT2860 -DRT28xx -DA_BAND_SUPPORT
CHIPSET_DAT = 2860
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_SOFTWARE_SUPPORT
endif
endif

ifeq ($(CHIPSET),3090)
WFLAGS +=-DRTMP_MAC_PCI -DRT30xx -DRT3090  -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),2870)
WFLAGS +=-DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT2870 -DRT28xx -DRTMP_TIMER_TASK_SUPPORT -DA_BAND_SUPPORT
CHIPSET_DAT = 2870
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_SOFTWARE_SUPPORT
endif

endif

ifeq ($(CHIPSET),2070)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRT2070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET),3070)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET),2880)
WFLAGS += -DRT2880 -DRT28xx -DRTMP_MAC_PCI -DCONFIG_RALINK_RT2880_MP -DRTMP_RBUS_SUPPORT -DMERGE_ARCH_TEAM -DA_BAND_SUPPORT -DCONFIG_SWMCU_SUPPORT
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_HARDWARE_SUPPORT -DDFS_FCC_BW40_FIX -DDFS_DEBUG
endif
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
endif

ifeq ($(CHIPSET),3572)
WFLAGS +=-DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT2870 -DRT28xx -DRT30xx -DRT35xx -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_SOFTWARE_SUPPORT
endif
endif

ifeq ($(CHIPSET),3062)
WFLAGS +=-DRTMP_MAC_PCI -DRT2860 -DRT28xx -DRT30xx -DRT35xx -DRT3062 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),3562)
WFLAGS +=-DRTMP_MAC_PCI -DRT2860 -DRT28xx -DRT30xx -DRT35xx -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_HARDWARE_SUPPORT  -DDFS_DEBUG 
endif

CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),3593)
WFLAGS +=-DRTMP_MAC_PCI -DDOT11N_SS3_SUPPORT -DNEW_RATE_ADAPT_SUPPORT -DRT3593 -DRT28xx -DRT30xx -DRT35xx -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DRTMP_FREQ_CALIBRATION_SUPPORT -DNEW_MBSSID_MODE -DSPECIFIC_BCN_BUF_SUPPORT
ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_HARDWARE_SUPPORT  -DDFS_DEBUG
endif

CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),3390)
WFLAGS +=-DRTMP_MAC_PCI -DRT30xx -DRT33xx -DRT3090 -DRT3390 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),3370)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT33xx -DRT3070 -DRT3370 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET),5390)
WFLAGS +=-DRTMP_MAC_PCI -DRT30xx -DRT33xx -DRT3090 -DRT3390 -DRT5390 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DSPECIFIC_BCN_BUF_SUPPORT -DRTMP_FREQ_CALIBRATION_SUPPORT -DRTMP_INTERNAL_TX_ALC -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifeq ($(CHIPSET),5370)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT33xx -DRT3070 -DRT3370 -DRT5370 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DhSPECIFIC_BCN_BUF_SUPPORT -DRTMP_FREQ_CALIBRATION_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
endif

ifeq ($(CHIPSET), 3052)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_RBUS_SUPPORT -DRT3052 -DRT305x -DRTMP_RF_RW_SUPPORT -DCONFIG_SWMCU_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
CHIPSET_DAT = 2870
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
endif

ifeq ($(CHIPSET), 3352)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_RBUS_SUPPORT -DRT3352 -DRT305x -DRTMP_RF_RW_SUPPORT -DSPECIFIC_BCN_BUF_SUPPORT -DVCORECAL_SUPPORT -DCONFIG_SWMCU_SUPPORT -DRTMP_INTERNAL_TX_ALC
CHIPSET_DAT = 2860
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
endif

ifeq ($(CHIPSET), 5350)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_RBUS_SUPPORT -DRT5350 -DRT305x -DRT3050 -DRT3350 -DRTMP_RF_RW_SUPPORT -DSPECIFIC_BCN_BUF_SUPPORT -DVCORECAL_SUPPORT -DCONFIG_SWMCU_SUPPORT -DRTMP_INTERNAL_TX_ALC -DRTMP_FREQ_CALIBRATION_SUPPORT
CHIPSET_DAT = 2860
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
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

ifeq ($(HAS_DOT11_N_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
		common/ba_action.o
endif

ifeq ($(HAS_LED_CONTROL_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
		common/rt_led.o
endif

#ifdef RT5350
ifeq ($(CHIPSET),5350)
rt$(CHIPSET)sta-objs += \
		common/cmm_mac_pci.o\
		common/cmm_data_pci.o\
		common/ee_flash.o\
		common/rtmp_swmcu.o\
		common/rt_rf.o\
		chips/rt305x.o\
		chips/rt5350.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
		os/linux/rt_pci_rbus.o\
		os/linux/rt_proc.o\
		os/linux/rt_rbus_pci_util.o\
		os/linux/rt_rbus_pci_drv.o\
		os/linux/rbus_main_dev.o
endif
endif
#endif // RT5350 //



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


#ifdef RT5370
ifeq ($(CHIPSET),5370)
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
		os/linux/rt_usb.o\
		common/frq_cal.o\
        chips/rt3070.o\
        chips/rt30xx.o\
        chips/rt33xx.o\
        chips/rt3370.o\
		chips/rt5390.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
        common/rtusb_dev_id.o\
        os/linux/rt_usb_util.o\
        os/linux/usb_main_dev.o
endif
endif
#endif // RT5370 //

ifeq ($(CHIPSET),PCI)
rt$(CHIPSET)sta-objs += \
		common/cmm_mac_pci.o\
		common/cmm_data_pci.o\
		os/linux/rt_rbus_pci_drv.o\
		common/rtmp_mcu.o\
		common/ee_prom.o\
		common/ee_efuse.o\
		common/rt_rf.o\
		chips/rt28xx.o\
		chips/rt30xx.o\
		chips/rt35xx.o\
		chips/rt3090.o\
		chips/rt33xx.o\
		chips/rt3390.o\
		chips/rt5390.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
		os/linux/rt_pci_rbus.o\
		os/linux/rt_rbus_pci_util.o\
		os/linux/pci_main_dev.o
endif
endif

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

obj-m := $(MOD_NAME).o




