# CONFIG_STM_TKDMA is not set
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

st-tkdma-objs += tkdma.o

obj-$(CONFIG_STM_TKDMA)      += st-tkdma.o

