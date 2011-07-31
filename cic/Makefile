TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/stm/
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/media/video/stm/
CCFLAGSY += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/include
CCFLAGSY += -DDEBUG_DVB
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

ifdef UFS922
ufs922_cic-objs += ufs9xx_cic/ufs9xx_cic.o
obj-m += ufs922_cic.o
else ifdef UFS912
ufs912_cic-objs += ufs9xx_cic/ufs9xx_cic.o
obj-m += ufs912_cic.o
else ifdef FORTIS_HDBOX
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef TF7700
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef HOMECAST5101
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef CUBEREVO
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef CUBEREVO_MINI
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef CUBEREVO_MINI2
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef CUBEREVO_250HD
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef CUBEREVO_9500HD
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef CUBEREVO_2000HD
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef CUBEREVO_MINI_FTA
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef HL101
cimax-objs += cimax/cimax.o
obj-m += cimax.o
else ifdef ATEVIO7500
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef HS7810A
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef SPARK
dummy_cic-objs += dummy_cic/dummy_cic.o
obj-m += dummy_cic.o
else ifdef IPBOX9900
obj-m += starci.o
starci-objs:= ipbox9900/starci2win.o
else ifdef IPBOX99
dummy_cic-objs += dummy_cic/dummy_cic.o
obj-m += dummy_cic.o
else ifdef IPBOX55
dummy_cic-objs += dummy_cic/dummy_cic.o
obj-m += dummy_cic.o
else
cimax-objs += cimax/cimax.o
obj-m += cimax.o
endif


