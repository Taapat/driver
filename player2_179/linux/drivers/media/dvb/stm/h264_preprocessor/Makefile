# workaround for stm24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

sth264pp-objs :=        h264pp.o

obj-m += sth264pp.o

