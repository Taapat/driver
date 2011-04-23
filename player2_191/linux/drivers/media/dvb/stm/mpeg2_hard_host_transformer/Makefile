# workaround for stm24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

EXTRA_CFLAGS  += -I$(CONFIG_KERNEL_PATH)/components/include
EXTRA_CFLAGS  += -I$(DRIVER_TOPDIR)/include/player2

mpeg2hw-objs:= mpeg2_hard_host_transformer.o mpeg2hard.o

obj-$(CONFIG_STM_UNIFY_MPEG2HW) += mpeg2hw.o
