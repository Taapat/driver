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

#
# Per SoC specification file that uses templates to generate the BSP library targets
# 
# ST231 version

#
# stxh205
#
BSP_SOC   = stxh205
BSP_CORES = audio video
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# stx7105
#
BSP_SOC   = stx7105
BSP_CORES = audio video
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# stx7106
#
BSP_SOC   = stx7106
BSP_CORES = audio video
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# stx7108
#
BSP_SOC   = stx7108
BSP_CORES = audio video gp
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# stx7109
#
BSP_SOC   = stx7109
BSP_CORES = audio video
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# stx7111
#
BSP_SOC   = stx7111
BSP_CORES = audio video
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# stx7141
#
BSP_SOC   = stx7141
BSP_CORES = audio video
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# stx7200
#
BSP_SOC   = stx7200
BSP_CORES = audio0 audio1 video0 video1
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# stx5289
#
BSP_SOC   = stx5289
BSP_CORES = audio video
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# fli7510
#
BSP_SOC   = fli7510
BSP_CORES = audio0 audio1 video
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# stxh415
#
BSP_SOC   = stxh415
BSP_CORES = audio0  audio1 gp video0 video1
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))

#
# fli7610
#
BSP_SOC   = fli7610
BSP_CORES = audio  gp0 gp1 video0 video1
$(foreach core,$(BSP_CORES),$(eval $(call BSPLIB_template,$(BSP_SOC),$(core))))