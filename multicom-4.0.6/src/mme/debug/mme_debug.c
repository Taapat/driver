/**************************************************************
 * Copyright (C) 2010   STMicroelectronics. All Rights Reserved.
 * This file is part of the latest release of the Multicom4 project. This release 
 * is fully functional and provides all of the original MME functionality.This 
 * release  is now considered stable and ready for integration with other software 
 * components.

 * Multicom4 is a free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License as published by the Free Software Foundation 
 * version 2.

 * Multicom4 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along with Multicom4; 
 * see the file COPYING.  If not, write to the Free Software Foundation, 59 Temple Place - 
 * Suite 330, Boston, MA 02111-1307, USA.

 * Written by Multicom team at STMicroelectronics in November 2010.  
 * Contact multicom.support@st.com. 
**************************************************************/

/* 
 * 
 */ 

#include <mme.h>	/* External defines and prototypes */

/* Global debug level flag
 * Can be defined on the build command line or via the MME_debug_level() call
 */
#if defined(MME_DEBUG_FLAGS)
MME_DBG_FLAGS mme_debug_flags = MME_DEBUG_FLAGS;
#else
MME_DBG_FLAGS mme_debug_flags = 0;
#endif

/* 
 * Display an English representation of the supplied error code 
 */
const char *MME_ErrorStr (MME_ERROR res)
{
  switch (res)
  {
  case MME_SUCCESS:			return "MME_SUCCESS"; break;
  case MME_DRIVER_NOT_INITIALIZED:	return "MME_DRIVER_NOT_INITIALIZED"; break;
  case MME_DRIVER_ALREADY_INITIALIZED:	return "MME_DRIVER_ALREADY_INITIALIZED"; break;
  case MME_NOMEM:			return "MME_NOMEM"; break;
  case MME_INVALID_TRANSPORT:		return "MME_INVALID_TRANSPORT"; break;
  case MME_INVALID_HANDLE:		return "MME_INVALID_HANDLE"; break;
  case MME_INVALID_ARGUMENT:		return "MME_INVALID_ARGUMENT"; break;

  case MME_UNKNOWN_TRANSFORMER:		return "MME_UNKNOWN_TRANSFORMER"; break;
  case MME_TRANSFORMER_NOT_RESPONDING:	return "MME_TRANSFORMER_NOT_RESPONDING"; break;

  case MME_HANDLES_STILL_OPEN:		return "MME_HANDLES_STILL_OPEN"; break;
  case MME_COMMAND_STILL_EXECUTING:	return "MME_COMMAND_STILL_EXECUTING"; break;

  case MME_COMMAND_ABORTED:		return "MME_COMMAND_ABORTED"; break;

  case MME_DATA_UNDERFLOW:		return "MME_DATA_UNDERFLOW"; break;
  case MME_DATA_OVERFLOW:		return "MME_DATA_OVERFLOW"; break;

  case MME_TRANSFORM_DEFERRED:		return "MME_TRANSFORM_DEFERRED"; break;
    
  case MME_SYSTEM_INTERRUPT:		return "MME_SYSTEM_INTERRUPT"; break;
  case MME_INTERNAL_ERROR:		return "MME_INTERNAL_ERROR"; break;

  case MME_ICS_ERROR:			return "MME_ICS_ERROR"; break;
    
  case MME_NOT_IMPLEMENTED:		return "MME_NOT_IMPLEMENTED"; break;
    
    /* MME4 API Extension */
  case MME_COMMAND_TIMEOUT:		return "MME_COMMAND_TIMEOUT"; break;
  }
  
  return "Unknown MME error code";
}

MME_ERROR MME_DebugFlags (MME_DBG_FLAGS flags)
{
  mme_debug_flags = flags;

  return MME_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
