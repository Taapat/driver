ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else

EXTRA_CXXFLAGS := -fno-rtti -fno-exceptions

ifeq ($(CONFIG_STM_FMDSW),y)
EXTRA_CFLAGS +=  -DUSE_FMD
FMDSW_SRC := $(SRC_TOPDIR)/STMCommon/fmdsw.cpp
endif

CORESOURCEFILES := $(addprefix /Linux/stm/coredisplay/, coredisplay.c devfbOS.c hdmidev.c hdmiedid.c hdmikthread.c hdmisysfs.c)
GENINITSRCS := $(addprefix /Generic/,DisplayDevice.cpp DisplayPlane.cpp MetaDataQueue.cpp Output.cpp)

#Generic Gamma
STM_GAMMA := $(addprefix $(SRC_TOPDIR)/Gamma/,                               \
			GenericGammaDevice.cpp                                 \
			GenericGammaOutput.cpp                                 \
			GammaMixer.cpp                                         \
			GammaCompositorPlane.cpp                               \
			GammaCompositorGDP.cpp                                 \
			GammaCompositorCursor.cpp                              \
			GammaCompositorVideoPlane.cpp                          \
			GammaVideoPlug.cpp)

STM_COMMON := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmawg.cpp stmdenc.cpp     \
			stmfsynth.cpp stmvtg.cpp stmblitter.cpp                \
			stmteletext.cpp stmhdmi.cpp stmiframemanager.cpp)

# Classes for SoCs containing the BDispII IP
STM_BDISP := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmbdisp.cpp stmbdispaq.cpp \
			stmbdispoutput.cpp stmvirtplane.cpp )

# Classes for SoCs containing the HDFormatter/TVOut style video output stage.
STM_HDF_TVOUT = $(addprefix $(SRC_TOPDIR)/STMCommon/,                          \
			stmsdvtg.cpp                                           \
			stmhdfhdmi.cpp                                         \
			stmhdfawg.cpp                                          \
			stmfdvo.cpp                                            \
			stmtvoutteletext.cpp)

STM_HDMI_DMA_IFRAME = $(addprefix $(SRC_TOPDIR)/STMCommon/, stmdmaiframes.cpp)

ALLSRCS := $(CORESOURCEFILES) $(FMDSW_SRC) $(GENINITSRCS) $(STM_COMMON) $(STM_GAMMA)


#STx710x specific
STX710XSRCS = $(STM_BDISP) $(STM_HDMI_DMA_IFRAME)

STX710XSRCS += $(addprefix $(SRC_TOPDIR)/STMCommon/, stmdmaiframes.cpp)

STX710XSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, GammaDisplayFilter.cpp        \
			VDPFilter.cpp GammaCompositorDISP.cpp GammaBlitter.cpp \
			GDPBDispOutput.cpp)

STX710XSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100device.cpp     \
			stb7100AWGAnalog.cpp stb7100analogueouts.cpp           \
			stb7100dvo.cpp stb7100hdmi.cpp stb7100denc.cpp         \
			stb7100vtg.cpp stb7100mixer.cpp                        \
			stb710xgdp.cpp stb710xcursor.cpp stb7109dei.cpp)

stx710x-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STX710XSRCS))
stx710x-display-objs := $(patsubst %.c,%.o,$(stx710x-display-objs))

obj-m += stmcore-display-stx7100.o
obj-m += stmcore-display-stx7109c3.o

stmcore-display-stx7100-objs   := $(stx710x-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/stx7100.o
stmcore-display-stx7109c3-objs := $(stx710x-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/stx7109c3.o

EXTRA_CFLAGS    := -I$(DRIVER_TOPDIR)/stgfb/stmfb -I$(DRIVER_TOPDIR)/include/stmfb -I$(DRIVER_TOPDIR)/stgfb/stmfb/Linux/include
EXTRA_CXXFLAGS := -fno-rtti -fno-exceptions

#ifdef __TDT__
#EXTRA_CFLAGS += -DDEBUG
#endif

# C++ build magic
EXTRA_CFLAGS += -DINSERT_EXTRA_CXXFLAGS_HERE
mould_cxx_cflags = $(subst -ffreestanding,,\
		   $(subst -Wstrict-prototypes,,\
		   $(subst -Wno-pointer-sign,,\
		   $(subst -Wdeclaration-after-statement,,\
		   $(subst -Werror-implicit-function-declaration,,\
		   $(subst -DINSERT_EXTRA_CXXFLAGS_HERE,$(EXTRA_CXXFLAGS),\
		   $(1)))))))


quiet_cmd_cc_o_cpp = CC $(quiet_modtab) $@

cmd_cc_o_cpp = $(call mould_cxx_cflags,$(cmd_cc_o_c))

define rule_cc_o_cpp
        $(call echo-cmd,checksrc) $(cmd_checksrc)                         \
        $(call echo-cmd,cc_o_cpp)                                         \
        $(cmd_cc_o_cpp);                                                  \
        $(cmd_modversions)                                                \
        scripts/basic/fixdep $(depfile) $@ '$(call make-cmd,cc_o_cpp)' > $(@D)/.$(@F).tmp;  \
        rm -f $(depfile);                                                 \
        mv -f $(@D)/.$(@F).tmp $(@D)/.$(@F).cmd
endef

%.o: %.cpp FORCE
	$(call cmd,force_checksrc)
	$(call if_changed_rule,cc_o_cpp)
endif
#obj-y += Linux/
FBSOURCEFILES := $(addprefix $(SRC_TOPDIR)/Linux/video/, \
					stmfbops.c stmfbioctl.c stmfbsysfs.c \
					stmfbvar.c stmfboutconfig.c stmfb.c)

obj-m += stmfb.o

stmfb-objs := $(patsubst %.c,%.o,$(FBSOURCEFILES))
stmfb-stb7100-objs := $(stmfb-objs)
