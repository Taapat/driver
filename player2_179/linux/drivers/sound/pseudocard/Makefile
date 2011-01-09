# workaround for stm24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb

pseudocard-objs := pseudo_mixer.o

EXTRA_CFLAGS+=-I$(CONFIG_STGFB_PATH)/include

obj-m := pseudocard.o
