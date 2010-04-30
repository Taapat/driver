pseudocard-objs := pseudo_mixer.o

CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/include/stmfb
CCFLAGSY:=-I$(CONFIG_STGFB_PATH)

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m := pseudocard.o
