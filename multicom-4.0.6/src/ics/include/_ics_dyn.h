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

#ifndef _ICS_DYN_SYS_H
#define _ICS_DYN_SYS_H

#ifdef __sh__
/* The sh linker prefixes symbol names with an '_' */
#define _ICS_DYN_SYM_PREFIX "_"
#else
#define _ICS_DYN_SYM_PREFIX ""
#endif

/* These are the names of the functions which will be called
 * (if present) as each module is loaded or unloaded
 */
#define _ICS_DYN_INIT_NAME  _ICS_DYN_SYM_PREFIX "module_init"
#define _ICS_DYN_TERM_NAME  _ICS_DYN_SYM_PREFIX "module_term"

typedef struct ics_dyn_mod
{
  ICS_CHAR	 name[_ICS_MAX_PATHNAME_LEN+1];	/* Name of module (for debugging purposes) */

  void		*handle;			/* Relocation system handle */
  ICS_DYN  	 parent;			/* Module's parent handle */
  
} ics_dyn_mod_t;

/* Exported private APIs */
ICS_ERROR ics_dyn_load_image (ICS_CHAR *name, ICS_OFFSET paddr, ICS_SIZE imageSize, ICS_UINT flags,
			      ICS_DYN handle, ICS_DYN *handlep);
ICS_ERROR ics_dyn_unload (ICS_DYN handle);

#endif /* _ICS_DYN_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

