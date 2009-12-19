ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else
obj-m           += embxshm.o
embxshm-objs	+= embxshm_connection.o embxshm_death.o embxshm_heap.o \
		   embxshm_init.o embxshm_localport.o embxshm_locks.o \
		   embxshm_lowlevel.o embxshm_mailboxFactory.o embxshm_module.o \
		   embxshm_remoteport.o embxshm_transport.o


#EXTRA_CFLAGS	:= -DDEBUG
EXTRA_CFLAGS    := -D__LINUX__ -D__SH4__ -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -DMULTICOM_GPL -I$(DRIVER_TOPDIR)/include/multicom
endif

