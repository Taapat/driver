
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

silencegen-objs:= silencegen_module.o silencegen_transformer.o

obj-m += silencegen.o
