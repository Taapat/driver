TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CFLAGS += -I$(TREE_ROOT)/linux/include/linux/stm/ 
CFLAGS += -I$(TREE_ROOT)/linux/drivers/media/video/stm/ 
CFLAGS += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core 
CFLAGS += -I$(CONFIG_STGFB_PATH)/include 
CFLAGS += -DDEBUG_DVB

pti-objs:= pti_main.o

obj-m += pti.o
