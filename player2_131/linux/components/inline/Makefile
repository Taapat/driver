ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

inline-objs :=  osinline.o \
		osdev_user.o \
		osdev_device.o \
		sh4cache.o 

lib-y += $(inline-objs)
