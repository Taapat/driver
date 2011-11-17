ifeq ($(KERNELRELEASE),)
    DRIVER_TOPDIR:=$(shell pwd)/..
    include $(DRIVER_TOPDIR)/kernel.make
else
    vfd-objs += vfd_core.o
    vfd-objs += trace.o
    vfd-objs += ywi2c_soft.o

    obj-m += vfd.o

    ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
        ccflags-y += $(CCFLAGSY)
    else
        CFLAGS += $(CCFLAGSY)
    endif

endif