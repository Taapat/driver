ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m += platform.o \
         p2div64.o
         
p2div64-objs:=  divdi3.o \
                udivdi3.o \
                div64.o

