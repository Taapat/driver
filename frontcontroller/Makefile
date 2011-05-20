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

ifdef UFS912
obj-y	+= micom/
endif

ifdef SPARK
obj-y   += aotom/
endif

ifdef SPARK2
obj-y   += aotom/
endif

ifdef OCTAGON1008
obj-y	+= nuvoton/
endif

ifdef ATEVIO7500
obj-y	+= nuvoton/
endif
