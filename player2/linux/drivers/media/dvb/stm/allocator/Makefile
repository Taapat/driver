TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CCFLAGSY += -I$(TREE_ROOT)/linux/components/inline         \
	-I$(TREE_ROOT)/components/include              \
	-I$(TREE_ROOT)/components/infrastructure       \
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/allocator \
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/h264_preprocessor \
	-I$(TREE_ROOT)/linux/drivers/stm/st_relay \
	-I$(TREE_ROOT)/linux/drivers/sound/ksound

ifeq ($(CONFIG_CPU_SUBTYPE_STX7200),y)
CCFLAGSY += -DLMI_BASE_ADDRESS=0x19000000 -DLMI_BLOCK_SIZE=0x03000000
else
ifeq ($(CONFIG_CPU_SUBTYPE_STB7100),y)
CCFLAGSY += -DLMI_BASE_ADDRESS=0x10800000 -DLMI_BLOCK_SIZE=0x03800000
else
endif
endif

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

stmalloc-objs :=        allocator.o
obj-m += stmalloc.o

