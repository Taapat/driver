# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

CONFIG_STM_SLIM = m
mmelog-objs += acc_mme_wrapper.o
obj-m += mmelog.o

