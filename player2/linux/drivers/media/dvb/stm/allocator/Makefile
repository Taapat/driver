stmalloc-objs :=        allocator.o

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

obj-$(CONFIG_STM_UNIFY_STMALLOC) += stmalloc.o

