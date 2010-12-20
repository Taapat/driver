ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

buffer-objs := 	buffer_individual_generic.o \
		buffer_manager_generic.o \
		buffer_pool_generic.o

obj-y += $(buffer-objs)

