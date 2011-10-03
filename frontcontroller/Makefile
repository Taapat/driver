TREE_ROOT?=$(DRIVER_TOPDIR)/
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/stm/
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/media/video/stm/
CCFLAGSY += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/include
CCFLAGSY += -DDEBUG_DVB
CCFLAGSY += -I$(TREE_ROOT)/include
CCFLAGSY += -I$(TREE_ROOT)/frontcontroller/include
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

ifdef FLASH_UFS910
obj-y	+= vfd/
endif

ifdef UFS910
obj-y	+= vfd/
endif

ifdef HOMECAST5101
obj-y	+= vfd_hs5101/
endif

ifdef FORTIS_HDBOX
obj-y	+= nuvoton/
endif

ifdef UFS922
obj-y	+= micom/
endif

ifdef TF7700
obj-y	+= tffp/
endif

ifdef HL101
obj-y   += proton/
endif

ifdef VIP1_V2
obj-y   += aotom/
endif

ifdef VIP2_V1
obj-y   += aotom/
endif

ifdef ADB_BOX
obj-y   += adb_box_vfd/
endif

ifdef UFS912
obj-y	+= micom/
endif

ifdef SPARK
obj-y   += aotom/
endif

ifdef SPARK7162
obj-y   += aotom/
endif

ifdef OCTAGON1008
obj-y	+= nuvoton/
endif

ifdef ATEVIO7500
obj-y	+= nuvoton/
endif

ifdef HS7810A
obj-y	+= nuvoton/
endif

ifdef IPBOX9900
obj-y	+= ipbox99xx/
endif

ifdef IPBOX99
obj-y	+= ipbox99xx/
endif

ifdef IPBOX55
obj-y	+= ipbox55/
endif

ifdef CUBEREVO
obj-y	+= ipbox/
else ifdef CUBEREVO_MINI
obj-y	+= ipbox/
else ifdef CUBEREVO_MINI2
obj-y	+= ipbox/
else ifdef CUBEREVO_250HD
obj-y	+= ipbox/
else ifdef CUBEREVO_9500HD
obj-y	+= ipbox/
else ifdef CUBEREVO_2000HD
obj-y	+= ipbox/
else ifdef CUBEREVO_MINI_FTA
obj-y	+= ipbox/
endif

