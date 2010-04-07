TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CFLAGS += -I$(TREE_ROOT)/linux/include/linux/stm/ 
CFLAGS += -I$(TREE_ROOT)/linux/drivers/media/video/stm/ 
CFLAGS += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core 
CFLAGS += -I$(CONFIG_STGFB_PATH)/include 
CFLAGS += -DDEBUG_DVB

ifdef UFS922
ufs922_cic-objs += ufs922_cic/ufs922_cic.o
obj-m += ufs922_cic.o
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


