# common build includes for the tuxbox
# change CDKPREFIX and CVSROOT 
# according to your setup if
# necessary

# directory containing the cdk
CDKDIR:=$(HOME)/tuxbox/target
# directory containing the cvs sources
CVSDIR:=$(HOME)/tuxbox/cvs

#######################################
ARCH=sh
CROSS_COMPILE=sh4-linux-
#######################################
PATH:=$(CDKDIR)/cdk/bin:$(PATH)
CDKROOT:=$(CDKDIR)/cdkroot
MAKE=/usr/bin/make
#######################################
KERNEL_LOCATION=$(CVSDIR)/cdk/linux
INSTALL_MOD_PATH=$(CDKDIR)/cdkroot
#######################################

# we need to get rid of the ".." at the end to avoid having to rebuild
# everything in case we switch directories :S
DRIVER_TOPDIR:=$(shell $(DRIVER_TOPDIR)/unify_path $(DRIVER_TOPDIR))

export PATH
export CVSROOT
export MAKE
export ARCH
export CROSS_COMPILE
export KERNEL_LOCATION
export INSTALL_MOD_PATH
export DRIVER_TOPDIR

# set KBUILD_VERBOSE to 1 to get all the dirty details

all:
	@$(MAKE) -C $(KERNEL_LOCATION) M=$(DRIVER_TOPDIR) KBUILD_VERBOSE=0 modules

install: all
	@$(MAKE) -C $(KERNEL_LOCATION) M=$(DRIVER_TOPDIR) KBUILD_VERBOSE=0 modules_install

clean:
	@$(MAKE) -C $(KERNEL_LOCATION) M=$(shell pwd) KBUILD_VERBOSE=0 clean

# Spider-Team:
# not clean config file, we use it for other modules
# init config file must be done when run make.sh
#
#$(DRIVER_TOPDIR)/.config:
#	@echo export CONFIG_ZD1211REV_B=y 	> $(CONFIGFILE); \
#	echo export CONFIG_ZD1211=n		>> $(CONFIGFILE);

# for CDK compatibility, there is no useable distclean from here
distclean:	clean

.PHONY:	clean
