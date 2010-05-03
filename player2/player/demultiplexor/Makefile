ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

demultiplexor-objs := 	demultiplexor_base.o \
			demultiplexor_ts.o

obj-y += $(demultiplexor-objs)

