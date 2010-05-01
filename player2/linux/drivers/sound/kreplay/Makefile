CCFLAGSY += -Wno-uninitialized

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m := kreplay.o kreplay-fdma.o

kreplay-fdma-y := ksound-streaming.o ksound-streaming_demo.o
