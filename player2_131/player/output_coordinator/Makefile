ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

output_coordinator-objs := output_coordinator_base.o

obj-y += $(output_coordinator-objs)
