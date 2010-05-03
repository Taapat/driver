ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else

EXTRA_CXXFLAGS := -fno-rtti -fno-exceptions

ifeq ($(CONFIG_STM_FMDSW),y)
EXTRA_CFLAGS +=  -DUSE_FMD
FMDSW_SRC := $(SRC_TOPDIR)/STMCommon/fmdsw.cpp
endif

# Common stmcoredisplay Linux specific module files 
CORESOURCEFILES := $(addprefix $(SRC_TOPDIR)/Linux/stm/coredisplay/, devfbOS.c \
			hdmisysfs.c hdmidev.c hdmiedid.c hdmikthread.c         \
			coredisplay.c)
# Base class files and C API implementation
GENINITSRCS := $(addprefix $(SRC_TOPDIR)/Generic/,                             \
			DisplayDevice.cpp                                      \
			DisplayPlane.cpp                                       \
			Output.cpp                                             \
			MetaDataQueue.cpp)

# Classes common to all ST SoCs regardless of the display architecture
STM_COMMON := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmawg.cpp stmdenc.cpp     \
			stmfsynth.cpp stmvtg.cpp stmblitter.cpp                \
			stmteletext.cpp stmhdmi.cpp stmiframemanager.cpp)

# Classes required for all SoCs containing Gamma based hardware video
# composition
STM_GAMMA := $(addprefix $(SRC_TOPDIR)/Gamma/,                                 \
			GenericGammaDevice.cpp                                 \
			GenericGammaOutput.cpp                                 \
			GammaMixer.cpp                                         \
			GammaCompositorPlane.cpp                               \
			GammaCompositorGDP.cpp                                 \
			GammaCompositorCursor.cpp                              \
			GammaCompositorVideoPlane.cpp                          \
			GammaVideoPlug.cpp)

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

ifeq ($(CONFIG_CPU_SUBTYPE_STX7100),y)
export CONFIG_CPU_SUBTYPE_STB7100=y
endif

ifeq ($(CONFIG_CPU_SUBTYPE_STB7100),y)
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

ifeq ($(CONFIG_SH_ST_MB602),y)
obj-m += stmcore-display-sti5202.o
stmcore-display-sti5202-objs   := $(stx710x-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/sti5202.o
else
obj-m += stmcore-display-stx7100.o
obj-m += stmcore-display-stx7109c2.o
obj-m += stmcore-display-stx7109c3.o

stmcore-display-stx7100-objs   := $(stx710x-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/stx7100.o
stmcore-display-stx7109c2-objs := $(stx710x-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/stx7109c2.o
stmcore-display-stx7109c3-objs := $(stx710x-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/stx7109c3.o
endif

endif

ifeq ($(CONFIG_CPU_SUBTYPE_STX7200),y)

STI7200SRCS = $(STM_HDF_TVOUT) $(STM_HDMI_DMA_IFRAME) $(STM_BDISP)

STI7200SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, VDPFilter.cpp GDPBDispOutput.cpp MainVideoPipe.cpp)

# We reuse some bits from 710x for 7200 as well
STI7200SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100denc.cpp stb7109dei.cpp)
STI7200SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/, sti7200device.cpp     \
			sti7200remoteoutput.cpp sti7200hdmi.cpp                \
			sti7200denc.cpp sti7200gdp.cpp sti7200mixer.cpp        \
			sti7200cursor.cpp sti7200hdfdvo.cpp                    \
			sti7200flexvp.cpp sti7200xvp.cpp                       \
			sti7200iqi.cpp)

STI7200C1SRCS = $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/,                      \
			sti7200cut1devicefactory.cpp                           \
			sti7200cut1remotedevice.cpp                            \
			sti7200cut1bdisp.cpp                                   \
			sti7200cut1localdevice.cpp                             \
			sti7200cut1localauxoutput.cpp                          \
			sti7200localmainoutput.cpp                             \
			sti7200cut1mainvideopipe.cpp)

STI7200C2SRCS = $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/,                      \
			sti7200cut2devicefactory.cpp                           \
			sti7200cut2remotedevice.cpp                            \
			sti7200cut2bdisp.cpp                                   \
			sti7200cut2localdevice.cpp                             \
			sti7200cut2localauxoutput.cpp                          \
			sti7200cut2localmainoutput.cpp                         \
			sti7200cut2mainvideopipe.cpp)

STI7200C2SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7111/, sti7111hdfoutput.cpp)

sti7200-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STI7200SRCS))
sti7200-display-objs := $(patsubst %.c,%.o,$(sti7200-display-objs))

sti7200c1-display-objs := $(patsubst %.cpp,%.o,$(STI7200C1SRCS))
sti7200c1-display-objs := $(patsubst %.c,%.o,$(sti7200c1-display-objs))

sti7200c2-display-objs := $(patsubst %.cpp,%.o,$(STI7200C2SRCS))
sti7200c2-display-objs := $(patsubst %.c,%.o,$(sti7200c2-display-objs))

obj-m += stmcore-display-sti7200c1.o
obj-m += stmcore-display-sti7200c2.o

stmcore-display-sti7200c1-objs := $(sti7200-display-objs) $(sti7200c1-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/sti7200c1.o
stmcore-display-sti7200c2-objs := $(sti7200-display-objs) $(sti7200c2-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/sti7200c2.o

endif

# The display hardware and clocking in 7111 and 7141 is identical enough that
# we can use exactly the same driver objects, we just need a different
# coredriver file to deal with board and HDMI hotplug PIO differences.
ifeq ($(CONFIG_CPU_SUBTYPE_STX7111)$(CONFIG_CPU_SUBTYPE_STX7141),y)

STI7111SRCS = $(STM_HDF_TVOUT) $(STM_HDMI_DMA_IFRAME) $(STM_BDISP)

STI7111SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, VDPFilter.cpp GDPBDispOutput.cpp MainVideoPipe.cpp)

# We reuse some bits from 710x/7200 for 7111 as well
STI7111SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100denc.cpp stb7109dei.cpp)

STI7111SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/, sti7200cursor.cpp     \
			sti7200cut2mainvideopipe.cpp sti7200iqi.cpp)

STI7111SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7111/, sti7111gdp.cpp        \
			sti7111bdisp.cpp sti7111mixer.cpp sti7111dvo.cpp       \
			sti7111hdfoutput.cpp sti7111mainoutput.cpp             \
			sti7111auxoutput.cpp sti7111hdmi.cpp                   \
			sti7111device.cpp sti7111devicecreate.cpp)

sti7111-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STI7111SRCS))
sti7111-display-objs := $(patsubst %.c,%.o,$(sti7111-display-objs))


ifeq ($(CONFIG_CPU_SUBTYPE_STX7111),y)
obj-m += stmcore-display-sti7111.o
stmcore-display-sti7111-objs := $(sti7111-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/sti7111.o
endif

ifeq ($(CONFIG_CPU_SUBTYPE_STX7141),y)
obj-m += stmcore-display-stx7141.o
stmcore-display-stx7141-objs := $(sti7111-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/stx7141.o
endif

endif


# much of this is identical to 7111, the only real difference being an added
# 2nd DVO.
ifeq ($(CONFIG_CPU_SUBTYPE_STX7105),y)

STI7105SRCS = $(STM_HDF_TVOUT) $(STM_HDMI_DMA_IFRAME) $(STM_BDISP)

STI7105SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, VDPFilter.cpp GDPBDispOutput.cpp MainVideoPipe.cpp)

# We reuse some bits from 710x/7200/7111 for 7105 as well
STI7105SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100denc.cpp stb7109dei.cpp)

STI7105SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/, sti7200cursor.cpp     \
			sti7200cut2mainvideopipe.cpp sti7200iqi.cpp)

STI7105SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7111/,                       \
			sti7111gdp.cpp sti7111bdisp.cpp sti7111mixer.cpp       \
			sti7111mainoutput.cpp sti7111auxoutput.cpp             \
			sti7111dvo.cpp sti7111hdfoutput.cpp sti7111hdmi.cpp    \
			sti7111device.cpp)

STI7105SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7105/, sti7105device.cpp sti7105devicecreate.cpp)

sti7105-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STI7105SRCS))
sti7105-display-objs := $(patsubst %.c,%.o,$(sti7105-display-objs))


obj-m += stmcore-display-sti7105.o

stmcore-display-sti7105-objs := $(sti7105-display-objs) $(SRC_TOPDIR)/Linux/stm/coredisplay/sti7105.o

endif

#ifdef __TDT__
EXTRA_CFLAGS    := -I$(DRIVER_TOPDIR)/stgfb/stmfb -I$(DRIVER_TOPDIR)/include/stmfb -I$(DRIVER_TOPDIR)/stgfb/stmfb/Linux/include
EXTRA_CXXFLAGS := -fno-rtti -fno-exceptions

ifdef CUBEREVO_250HD
EXTRA_CFLAGS += -DUSE_EXT_CLK
endif
ifdef CUBEREVO_MINI_FTA
EXTRA_CFLAGS += -DUSE_EXT_CLK
endif

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
