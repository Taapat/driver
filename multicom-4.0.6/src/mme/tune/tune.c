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

#include "_mme_sys.h"	/* Internal defines and prototypes */


/* TIMEOUT SUPPORT: Default transformer management operation timeout value in ms */
#define DEFAULT_TRANSFORMER_TIMEOUT                  5000

/* TIMEOUT SUPPORT: Default transformer command timeout value in ms */
#define DEFAULT_COMMAND_TIMEOUT                      250

/* Default size of the MME_DataBuffer pool */
#define DEFAULT_BUFFER_POOL_SIZE		     (32*1024)

#define HAS_TASK_PRIORITY 1

#ifdef __os21__

#define DEFAULT_MANAGER_TASK_PRIORITY                200
#define DEFAULT_RECEIVER_TASK_PRIORITY               111
/* These priorities have been respaced from Multicom3 to aid the Audio codec subtransformer model */
#define DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST	      55
#define DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL  51
#define DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL        47
#define DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL  43
#define DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST        39

#elif defined __wince__ || defined WIN32

#define DEFAULT_MANAGER_TASK_PRIORITY                THREAD_PRIORITY_HIGHEST
#define DEFAULT_RECEIVER_TASK_PRIORITY               THREAD_PRIORITY_ABOVE_NORMAL
#define DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST      THREAD_PRIORITY_HIGHEST
#define DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL THREAD_PRIORITY_ABOVE_NORMAL
#define DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL       THREAD_PRIORITY_NORMAL
#define DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL THREAD_PRIORITY_BELOW_NORMAL
#define DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST       THREAD_PRIORITY_LOWEST

#elif defined(__linux__) && defined(__KERNEL__)

#define DEFAULT_MANAGER_TASK_PRIORITY                 -(20 + 60)	/* RT */
#define DEFAULT_RECEIVER_TASK_PRIORITY                -(20 + 50)	/* RT */
#define DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST	      -20		/* nice */
#define DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL  -10		/* nice */
#define DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL          0		/* nice */
#define DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL   10		/* nice */
#define DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST         19		/* nice */

#else

#undef  HAS_TASK_PRIORITY
#define HAS_TASK_PRIORITY                            0
#define DEFAULT_MANAGER_TASK_PRIORITY                0
#define DEFAULT_RECEIVER_TASK_PRIORITY               0
#define DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST      0
#define DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL 0
#define DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL       0
#define DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL 0
#define DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST       0


#endif

/*
 * Default tuneable array.
 * Order must match that of the MME_Tuneable_t enum
 */
static MME_UINT tuneables[MME_TUNEABLE_MAX] =
{
  DEFAULT_MANAGER_TASK_PRIORITY,
  DEFAULT_RECEIVER_TASK_PRIORITY,
  DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST,
  DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL,
  DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL,
  DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL,
  DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST,
  DEFAULT_TRANSFORMER_TIMEOUT,
  DEFAULT_BUFFER_POOL_SIZE,
  DEFAULT_COMMAND_TIMEOUT,
};

MME_ERROR MME_ModifyTuneable (MME_Tuneable_t key, MME_UINT value)
{
  switch (key) 
  {
  case MME_TUNEABLE_MANAGER_THREAD_PRIORITY:
  case MME_TUNEABLE_TRANSFORMER_THREAD_PRIORITY:
  case MME_TUNEABLE_EXECUTION_LOOP_HIGHEST_PRIORITY:
  case MME_TUNEABLE_EXECUTION_LOOP_ABOVE_NORMAL_PRIORITY:
  case MME_TUNEABLE_EXECUTION_LOOP_NORMAL_PRIORITY:
  case MME_TUNEABLE_EXECUTION_LOOP_BELOW_NORMAL_PRIORITY:
  case MME_TUNEABLE_EXECUTION_LOOP_LOWEST_PRIORITY:
    if (!HAS_TASK_PRIORITY)
    {
      return MME_INVALID_ARGUMENT;
    }
    break;

  case MME_TUNEABLE_TRANSFORMER_TIMEOUT:
  case MME_TUNEABLE_COMMAND_TIMEOUT:
    break;

  case MME_TUNEABLE_BUFFER_POOL_SIZE:
    break;

  default:
    return MME_INVALID_ARGUMENT;
  }

  tuneables[key] = value;
  return MME_SUCCESS;
}

MME_UINT MME_GetTuneable (MME_Tuneable_t key)
{
  if (key < 0 || key >= MME_TUNEABLE_MAX)
    return MME_INVALID_ARGUMENT;
  
  return tuneables[key];
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
