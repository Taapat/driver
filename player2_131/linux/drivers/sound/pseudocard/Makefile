CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/include/stmfb
CCFLAGSY +=-I$(CONFIG_STGFB_PATH)

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

pseudocard-objs := pseudo_mixer.o
obj-m := pseudocard.o
