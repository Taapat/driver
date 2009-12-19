ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else
obj-m           += avs.o
avs-objs	+= ak4705.o avs_core.o stv6412.o fake_avs.o

EXTRA_CFLAGS	:= -DDEBUG
#EXTRA_CFLAGS    := -I$(DRIVER_TOPDIR)/include -I$(DRIVER_TOPDIR)/dvb/drivers/media/dvb
endif

