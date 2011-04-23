# workaround for stm24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

pseudocard-objs := pseudo_mixer.o

CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
EXTRA_CFLAGS+=-I$(CONFIG_STGFB_PATH)/include
EXTRA_CFLAGS+=-I$(DRIVER_TOPDIR)/include/player2

obj-$(CONFIG_STM_UNIFY_PSEUDOCARD) := pseudocard.o
