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
 * Private header file for ICS4 implementation
 *
 */ 

#ifndef _EMBX_SYS_H
#define _EMBX_SYS_H

/* 
 * Global variables
 */
extern EMBX_BOOL                 embx_initialised;

/* Convert an ICS_ERROR code into and EMBX_ERROR code */
_ICS_OS_INLINE_FUNC_PREFIX
EMBX_ERROR EMBX_ERROR_CODE (ICS_ERROR err)
{
  switch (err)
  {
  case ICS_SUCCESS:
    return EMBX_SUCCESS;
    
  case ICS_NOT_INITIALIZED:
    return EMBX_DRIVER_NOT_INITIALIZED;

  case ICS_ALREADY_INITIALIZED:
    return EMBX_ALREADY_INITIALIZED;

  case ICS_ENOMEM:
    return EMBX_NOMEM;

  case ICS_INVALID_ARGUMENT:
    return EMBX_INVALID_ARGUMENT;

  case ICS_HANDLE_INVALID:
    return EMBX_INVALID_ARGUMENT;

  case ICS_SYSTEM_INTERRUPT:
    return EMBX_SYSTEM_INTERRUPT;

  case ICS_SYSTEM_ERROR:
    return EMBX_SYSTEM_ERROR;

  case ICS_SYSTEM_TIMEOUT:
    return EMBX_SYSTEM_TIMEOUT;

  case ICS_NOT_CONNECTED:
    return EMBX_TRANSPORT_CLOSED;
    
  case ICS_CONNECTION_REFUSED:
    return EMBX_CONNECTION_REFUSED;

  case ICS_PORT_CLOSED:
    return EMBX_PORT_CLOSED;

  default:
    break;
  }

  /* No direct mapping for these error codes */
  return ICS_SYSTEM_ERROR;
}

#endif /* _EMBX_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
