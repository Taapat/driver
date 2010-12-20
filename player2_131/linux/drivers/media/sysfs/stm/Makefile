TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/media/video/stm/
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/sound/pseudocard
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/include 

CCFLAGSY += -DDEBUG_DVB

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

stmsysfs-objs:= player_interface.o \
		sysfs_module.o \
		player_sysfs.o



obj-m += stmsysfs.o
