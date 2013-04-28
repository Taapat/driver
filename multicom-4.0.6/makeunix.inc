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

UNAME=$(shell uname)

ifeq (,$(UNAME))
# Assume we are running the Evil Windoze (TM)
UNAME=Windows
endif

ifeq (Windows,$(UNAME))

#
# The Evil Windoze (TM)
#
CP         = copy
DATE       = date /t
ECHO        = echo
MKDIR      = mkdir
# mkdir whinges about existing dirs
MKTREE     = mkdir 2>NUL
PWD        = cd
RM         = del 2>NUL /f /q
RMTREE     = rmdir /s /q
DOSCMD     = $(subst /,\,$1)

else

#
# Phew! we are running Unix
#

CP         = cp -f
DATE       = date
ECHO       = echo
MKDIR      = mkdir
MKTREE     = mkdir -p
PWD        = pwd
RM         = rm -f
RMTREE     = rm -rf
DOSCMD     = $1

endif
