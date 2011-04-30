TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/stm/ 
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/media/video/stm/ 
CCFLAGSY += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core 
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/include 
CCFLAGSY += -DDEBUG_DVB

ifeq ($(CONFIG_PLAYER_131),y)
CCFLAGSY+= -DPLAYER_131
else ifeq ($(CONFIG_PLAYER_179),y)
CCFLAGSY+= -DPLAYER_179
else ifeq ($(CONFIG_PLAYER_191),y)
CCFLAGSY+= -DPLAYER_191
endif

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

pti-objs:= pti_main.o

obj-m += pti.o
