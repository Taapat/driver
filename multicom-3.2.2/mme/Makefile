ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else
obj-m           += mme_host.o
mme_host-objs   += ExecutionLoop.o LocalTransformer.o LookupTable.o Manager.o RemoteTransformer.o mme_debug.o \
                   mme_host_release.o MMEMessageQueue.o mme_tune.o mme_linux_module.o

#EXTRA_CFLAGS   := -DDEBUG
EXTRA_CFLAGS    += -D__TDT__ -D__LINUX__ -D__SH4__ -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -DMULTICOM_GPL -I$(DRIVER_TOPDIR)/include/multicom
endif
