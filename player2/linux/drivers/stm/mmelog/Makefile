CONFIG_STM_SLIM = m
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif
mmelog-objs += acc_mme_wrapper.o
obj-m += mmelog.o

