EXTRA_CFLAGS += -I$(TREE_ROOT)/linux/include/linux/stm/ 
EXTRA_CFLAGS += -I$(TREE_ROOT)/linux/drivers/media/video/stm/
EXTRA_CFLAGS += -I$(TREE_ROOT)/linux/drivers/sound/pseudocard
EXTRA_CFLAGS += -I$(CONFIG_STGFB_PATH)/include 
EXTRA_CFLAGS += -DDEBUG_DVB -D__TDT__

stmsysfs-objs:= player_interface.o \
		sysfs_module.o \
		player_sysfs.o

obj-m += stmsysfs.o
