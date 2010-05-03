ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

strelay-objs += st_relay.o

ifdef CONFIG_RELAY
obj-m += strelay.o
endif
