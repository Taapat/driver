ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else
obj-m           += embxmailbox.o
embxmailbox-objs	+= embx_mailbox.o
#EXTRA_CFLAGS	:= -DDEBUG 
EXTRA_CFLAGS    := -D__LINUX__ -D__SH4__ -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -DMULTICOM_GPL -I$(DRIVER_TOPDIR)/include/multicom
endif

