#*******************************************************************************
# Copyright (C) 2010   STMicroelectronics. All Rights Reserved.
# This file is part of the latest release of the Multicom4 project. This release 
# is fully functional and provides all of the original MME functionality.This 
# release  is now considered stable and ready for integration with other software 
# components.

 # Multicom4 is a free software; you can redistribute it and/or modify it under the 
# terms of the GNU General Public License as published by the Free Software Foundation 
# version 2.

# Multicom4 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
# See the GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along with Multicom4; 
# see the file COPYING.  If not, write to the Free Software Foundation, 59 Temple Place - 
# Suite 330, Boston, MA 02111-1307, USA.

# Written by Multicom team at STMicroelectronics in November 2010.  
# Contact multicom.support@st.com. 
#*******************************************************************************

#*******************************************************************************
#
# ST40 Linux userspace Multicom4 specific makefile.
#
#*******************************************************************************
#*******************************************************************************

LIBDIR = lib/$(OS)/$(CPU)$(MULTILIB)
OBJDIR = obj/$(OS)/$(CPU)$(MULTILIB)

################################################################################

ics_OBJS = 							\
	$(OBJDIR)/ics/linux/ics_user_lib$(OBJSFX).$(O) 	\
	$(OBJDIR)/ics/debug/debug$(OBJSFX).$(O) 		\

mme_OBJS = 							\
	$(OBJDIR)/mme/linux/mme_user_lib$(OBJSFX).$(O) 	\
	$(OBJDIR)/mme/debug/mme_debug$(OBJSFX).$(O) 		\
	$(OBJDIR)/ics/linux/linux_user_task$(OBJSFX).$(O) 	\
  

################################################################################

CCUSERFLAGS += \
	-Iinclude    \
	-Isrc/ics/include \
	-Isrc/mme/include \

################################################################################
# Library names

ALLLIBS = mme ics

################################################################################
# Main targets

all : libs

################################################################################
# Libraries

-include makelib.inc

# Compile both a '.a' and '.so' library
# 
$(foreach lib,$(ALLLIBS),$(eval $(call LIB_template,$(lib))))

$(foreach lib,$(ALLLIBS),$(eval $(call LIBSO_template,$(lib))))

libs : $(LIBS) $(LIBS_SO)

################################################################################

$(OBJDIR)/%$(OBJSFX).$(O): src/%.c
	-@$(MKTREE) $(call DOSCMD,$(dir $@))
	$(CCBUILD) $<

################################################################################

clean:
	-$(RM) $(call DOSCMD, $(OBJS) $(LIBS) $(LIBS_SO))
	-$(RM) $(call DOSCMD, $(patsubst %.$(O),%.d,$(OBJS)))

################################################################################

-include $(patsubst %.$(O),%.d,$(OBJS))

################################################################################
