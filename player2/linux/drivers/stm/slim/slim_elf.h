/*
 *  ELF support for the SLIM core.
 *
 * Copyright (C) 2006 STMicroelectronics Limited
 * Author: Peter Bennett <peter.bennett@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */


#ifndef _SLIM_ELF_H
#define _SLIM_ELF_H

int slim_elf_resolve(char *elf, char *relf, int text, int small_pool,
		     int large_pool);
int slim_elf_write_out(char *elf_out, char *elf_in);
int slim_elf_write_size(void);
int slim_elf_get_sizes(char *elf, int *text, int *small_pool, int *big_pool);
int slim_elf_get_offsets(char *elf, int *text, int *data, int *small_pool,
			 int *big_pool);
int slim_elf_get_symbol(char *elf, const char *name, int *value, int *offset);
//char *slim_elf_find_symbol(char *elf, const char *name, int value, int *offset);
char *slim_elf_search_symbol(char *elf, const char *name, int value,
			     int *offset);

#define ENOT_ELF  -1
#define ENOT_SLIM -2
#define EREL_OUT_RANGE -3
#define EREL_UNKNOWN -4
#define ENO_TEXT -5
#define ESYM_UNRESOLVED -6

/* Slim architecture id */
#define EM_SLIM 0x51cd

#endif
