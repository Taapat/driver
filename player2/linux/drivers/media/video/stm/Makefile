CONFIG_STM_HAVANA_V4L2=m
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/include/stmfb
CONFIG_STGFB_LOCATION=$(CONFIG_STGFB_PATH)

EXTRA_CFLAGS=-I$(CONFIG_STGFB_LOCATION) -I$(CONFIG_STGFB_LOCATION)/include

obj-$(CONFIG_STM_HAVANA_V4L2)+= stm_v4l2.o

stm_v4l2-objs += stm_driver.o \
                 stmvout_device.o \
                 stmvin_device.o \
                 stmvout_driver.o 

##stm7100.o stm_common.o
