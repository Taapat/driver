CONFIG_STM_HAVANA_V4L2=m
TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CCFLAGSY+=-I$(DRIVER_TOPDIR)/stgfb/stmfb/Linux/include	\
	-I$(DRIVER_TOPDIR)/include							\
	-I$(DRIVER_TOPDIR)/include/player2					\
	-I$(TREE_ROOT)/linux/components/inline				\
	-I$(TREE_ROOT)/components/include					\
	-I$(TREE_ROOT)/linux/include						\
	-I$(TREE_ROOT)/linux/include/linux/dvb				\
	-I$(TREE_ROOT)/linux/include/linux/stm				\
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/dvb		\
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/allocator \
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/h264_preprocessor \
	-I$(TREE_ROOT)/linux/drivers/sound/pseudocard				 \
	-I$(TREE_ROOT)/linux/drivers/stm/mmelog						\
	-I$(TREE_ROOT)/linux/drivers/stm/monitor					\
	-I$(TREE_ROOT)/linux/drivers/media/sysfs/stm

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-$(CONFIG_STM_HAVANA_V4L2)+= stm_v4l2.o

stm_v4l2-objs += stm_driver.o \
                 stmvout_device.o \
                 stmvin_device.o \
                 stmvout_driver.o 

##stm7100.o stm_common.o
