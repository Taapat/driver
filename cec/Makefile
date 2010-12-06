ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else
obj-m           += cec.o 
cec-objs	+= cec_main.o cec_worker.o cec_opcodes.o cec_internal.o cec_proc.o

#EXTRA_CFLAGS	:= -DDEBUG
EXTRA_CFLAGS    := -I$(DRIVER_TOPDIR)/cec -I$(DRIVER_TOPDIR)/include -I$(DRIVER_TOPDIR)/dvb/drivers/media/dvb
endif

