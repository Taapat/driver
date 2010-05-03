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
ufs922_cic-objs += ufs922_cic/ufs922_cic.o
obj-m += ufs922_cic.o
else ifdef UFS912
ufs912_cic-objs += ufs912_cic/ufs912_cic.o
obj-m += ufs912_cic.o
else ifdef FORTIS_HDBOX
cimax-objs += cimax/cimax.o
obj-m += cimax.o
else ifdef TF7700
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef HOMECAST5101
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
else ifdef HL101
cimax-objs += cimax/cimax.o
obj-m += cimax.o
else
cimax-objs += cimax/cimax.o
obj-m += cimax.o
endif


