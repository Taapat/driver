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

#include "_ics_sys.h"	/* Internal defines and prototypes */

/* Leave a spare char for '\0' termination */
#define BUF_SIZE (_ICS_DEBUG_LINE_SIZE-1)

/* ICS Debug printf routine
 * Prefixes the output and then displays it on stdout/stderr depending
 * on the setting of ics_debug_chan. Will log only to cyclic buffer if
 * called from Interrupt context
 */
void ics_debug_printf (const char *fmt, const char *fn, int line, ...)
{
  int     size;
  char    buf[BUF_SIZE+1];

  va_list args;

  ICS_BOOL inInterrupt = _ICS_OS_TASK_INTERRUPT();
  
  /* Prefix debug msg with 'TaskName:FnName:Line' */
  size = snprintf(buf, BUF_SIZE, "%s:%s:%d ",
		  (inInterrupt ? "Interrupt" : _ICS_OS_TASK_NAME()),
		  fn, line);
  
  /* Append the printf content */
  va_start(args, line);
  size += vsnprintf(buf+size, BUF_SIZE-size, fmt, args);
  va_end(args);

  /* Always terminate with a newline (in case of truncation) */
  buf[BUF_SIZE-1] = '\n';
  buf[BUF_SIZE]   = '\0';

  if (inInterrupt)
  {
    /* IRQ context - only log the msg into the cyclic buffer */
    ICS_DEBUG_MSG(buf, size);
  }
  else
  {
    /*
     * Log the message as appropriate
     *
     * Only log to the cyclic buffer if ICS_DBG_LOG is set
     */
    if (_ics_debug_chan & ICS_DBG_STDERR)
      printk(KERN_ERR "%s", buf);
    else if (_ics_debug_chan & ICS_DBG_STDOUT)
      printk(KERN_DEBUG "%s", buf);

    /* Log into cyclic buffer (only if ICS_DBG_LOG set) */
    ICS_DEBUG_MSG(buf, size);
  }

  return;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
