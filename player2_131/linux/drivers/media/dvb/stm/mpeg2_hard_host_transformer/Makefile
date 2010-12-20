CCFLAGSY  += -I$(CONFIG_KERNEL_PATH)/components/include
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

mpeg2hw-objs:= mpeg2_hard_host_transformer.o mpeg2hard.o

obj-m += mpeg2hw.o
