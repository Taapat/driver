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

#ifndef _ICS_ELF_H
#define _ICS_ELF_H

ICS_ERROR ics_elf_check_magic (ICS_CHAR *hdr);
ICS_ERROR ics_elf_check_hdr (ICS_CHAR *image);

ICS_OFFSET ics_elf_entry (ICS_CHAR *image);
ICS_ERROR ics_elf_load_size (ICS_CHAR *image, ICS_OFFSET *basep, ICS_SIZE *sizep, ICS_SIZE *alignp, ICS_OFFSET *bootp);
ICS_ERROR ics_elf_load_image (ICS_CHAR *image, ICS_VOID *mem, ICS_SIZE memSize, ICS_OFFSET baseAddr);

#endif /* _ICS_ELF_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

