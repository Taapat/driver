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

#include <ics.h>	/* External defines and prototypes */

#if defined(__os21__) || defined(__KERNEL__)
#include "_ics_sys.h"	/* Internal defines and prototypes */
#endif

/* 
 * Set default flags and logging
 */
#ifdef ICS_DEBUG
#define _DEBUG_FLAGS		ICS_DBG_ERR
#define _DEBUG_CHAN		ICS_DBG_STDERR|ICS_DBG_LOG
#else
#define _DEBUG_FLAGS		0
#define _DEBUG_CHAN		ICS_DBG_LOG
#endif

/* Global debug level flag
 * Can be defined on the build command line or via the ics_debug_flags() call
 */
#if defined(ICS_DEBUG_FLAGS)
ICS_DBG_FLAGS _ics_debug_flags = ICS_DEBUG_FLAGS;
#else
ICS_DBG_FLAGS _ics_debug_flags = _DEBUG_FLAGS;
#endif

/* Global debug channel
 * Can be defined on the build command line or via the ics_debug_chan() call
 */
#if defined(ICS_DEBUG_CHAN)
ICS_DBG_CHAN  _ics_debug_chan = ICS_DEBUG_CHAN;
#else
ICS_DBG_CHAN  _ics_debug_chan = _DEBUG_CHAN;
#endif

/* 
 * Return a string representing the supplied error code 
 */
const ICS_CHAR *ics_err_str (ICS_ERROR err)
{
  switch (err)
  {
  case ICS_SUCCESS:		return "ICS_SUCCESS"; break;
  case ICS_NOT_INITIALIZED:	return "ICS_NOT_INITIALIZED"; break;
  case ICS_ALREADY_INITIALIZED: return "ICS_ALREADY_INITIALIZED"; break;
  case ICS_ENOMEM:		return "ICS_ENOMEM"; break;
  case ICS_INVALID_ARGUMENT:	return "ICS_INVALID_ARGUMENT"; break;
  case ICS_HANDLE_INVALID:	return "ICS_HANDLE_INVALID"; break;

  case ICS_SYSTEM_INTERRUPT:	return "ICS_SYSTEM_INTERRUPT"; break;
  case ICS_SYSTEM_ERROR:	return "ICS_SYSTEM_ERROR"; break;
  case ICS_SYSTEM_TIMEOUT:	return "ICS_SYSTEM_TIMEOUT"; break;

  case ICS_NOT_CONNECTED:	return "ICS_NOT_CONNECTED"; break;
  case ICS_CONNECTION_REFUSED:	return "ICS_CONNECTION_REFUSED"; break;
    
  case ICS_FULL:		return "ICS_FULL"; break;
  case ICS_EMPTY:		return "ICS_EMPTY"; break;

  case ICS_PORT_CLOSED:		return "ICS_PORT_CLOSED"; break;

  case ICS_NAME_NOT_FOUND:	return "ICS_NAME_NOT_FOUND"; break;
  case ICS_NAME_IN_USE:		return "ICS_NAME_IN_USE"; break;
  }

  return "Unknown ICS error code";
}

/* Set local cpu debug flags */
void ics_debug_flags (ICS_UINT flags)
{
  _ics_debug_flags = flags;

  return;
}

/* Set local cpu debug channel */
void ics_debug_chan (ICS_UINT flags)
{
  _ics_debug_chan = flags;

  return;
}

#if defined(__os21__) || defined(__KERNEL__)

/* Dump out the debug log for a given cpu */
ICS_ERROR ICS_debug_dump (ICS_UINT cpuNum)
{
  ICS_ERROR      err;
  ICS_DBG_CHAN   old;

  /* Save the current debug channel flags and then
   * direct new output only to stderr. This prevents
   * us logging the dump output into our own cyclic log
   */
  old = _ics_debug_chan;
  _ics_debug_chan = ICS_DBG_STDERR;

  /* Get the transport to dump the log */
  err = ics_transport_debug_dump(cpuNum);

  /* Add an extra newline to distinguish the end of the output */
  _ICS_OS_PRINTF("\n");
  
  /* Restore the original debug channel flags */
  _ics_debug_chan = old;
    
  return err;
}

/* Copy out the debug log for a given cpu */
ICS_ERROR ICS_debug_copy (ICS_UINT cpuNum, ICS_CHAR *buf, ICS_SIZE bufSize, ICS_INT *bytesp)
{
  ICS_ERROR      err;
  
  /* Get the transport to copy the log */
  err = ics_transport_debug_copy(cpuNum, buf, bufSize, bytesp);

  return err;
}

/*
 * All ICS debug printf calls and also 'user' printf calls come through here
 *
 * The ICS_DEBUG_MSG() macro checks the ics_debug_chan has ICS_DBG_LOG set
 * before calling this function
 */
void ics_debug_msg (ICS_CHAR *msg, ICS_UINT len)
{
  ics_transport_debug_msg(msg, len);
}

/* Leave a spare char for '\0' termination */
#define BUF_SIZE (_ICS_DEBUG_LINE_SIZE-1)

void ICS_debug_printf (const char *fmt, ...)
{
  int     size = 0;
  char    buf[BUF_SIZE+1];

  va_list args;

  va_start(args, fmt);
  size = vsnprintf(buf, BUF_SIZE, fmt, args);
  va_end(args);
  
  /* Always terminate with a newline (in case of truncation) */
  buf[BUF_SIZE-1] = '\n';
  buf[BUF_SIZE]   = '\0';

  /* Log to console as directed */
  if (_ics_debug_chan & (ICS_DBG_STDERR|ICS_DBG_STDOUT))
    _ICS_OS_PRINTF("%s", buf);
  
  /* Always log to cyclic buffer */
  ics_debug_msg(buf, size);
}

#endif /* __os21__ || __KERNEL__ */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
