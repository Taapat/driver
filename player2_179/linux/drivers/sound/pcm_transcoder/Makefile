# workaround for stm24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

pcm_transcoder-objs:= pcm_transcoder_module.o pcm_transcoder_transformer.o

obj-m += pcm_transcoder.o
