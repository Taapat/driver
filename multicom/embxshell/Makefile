ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else
obj-m           += embxshell.o
embxshell-objs	+= embx_buffer.o embx_cache.o embx_connect.o embx_debug.o embx_deinit.o embx_handle.o \
		   embx_init.o embx_linux.o embx_module.o embx_object.o embx_open.o \
		   embx_port.o embx_sendrec.o embx_support.o embx_tune.o \
		   embx_transport.o

EXTRA_CFLAGS    := -D__LINUX__ -D__SH4__ -D__KERNEL__ -DMULTICOM_GPL -I$(DRIVER_TOPDIR)/include/multicom
endif

