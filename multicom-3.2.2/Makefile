CCFLAGSY    += -D__LINUX__ -D__SH4__ -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -DMULTICOM_GPL -I$(DRIVER_TOPDIR)/include/multicom
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-y	+= embxshell/
obj-y	+= embxmailbox/
obj-y	+= embxshm/
obj-y	+= mme/
