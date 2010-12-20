CCFLAGSY := -Wno-uninitialized

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m := ksound.o ktone.o

ksound-y := ksound-core.o
