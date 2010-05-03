# CONFIG_STM_SLIM is not set
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

st-slim-objs += slim_elf.o slim_core.o slim_driver.o
obj-$(CONFIG_STM_SLIM) += st-slim.o

