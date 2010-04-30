
CCFLAGSY :=-I./linux/components/inline         \
	-I./components/include              \
	-I./components/infrastructure

CCFLAGSY+=-D__SH4__       

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
	ccflags-y += $(CCFLAGSY)
else
	CFLAGS += $(CCFLAGSY)
endif

obj-y += infrastructure/


