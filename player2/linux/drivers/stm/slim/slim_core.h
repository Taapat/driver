/*
 *  SLIM Core Generic driver.
 *
 * Copyright (C) 2006 STMicroelectronics Limited
 * Author: Peter Bennett <peter.bennett@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */

/* Responsibilities:
      Managing the SLIM core
      * SLIM memory allocation
      * Loading the elf file
      * Resolving Symbols
      * Loading instruction memory
      * Booting the core
      
*/

#ifndef _SLIM_CORE_H
#define _SLIM_CORE_H

#define SLIM_RUN_STOP    (0<<0)
#define SLIM_RUN_ENABLE  (1<<0)
#define SLIM_RUN_STOPPED (1<<1)

#define SLIM_CLOCK_STOP   (1<<0)
#define SLIM_CLOCK_STEP   (1<<1)
#define SLIM_CLOCK_RESET  (1<<2)

#define SLIM_DIV 2

#if SLIM_DIV==2
#define SLIM_PER_OFFSET 0xC00
#else
#define SLIM_PER_OFFSET 0x800
#endif

#define SLIM_PERS_STBUS_SYNC    (0xFE2 - SLIM_PER_OFFSET)
#define SLIM_STBUS_SYNC_DISABLE (1<<0)
#define SLIM_PERS_MBOX          (0xFF0 - SLIM_PER_OFFSET)
#define SLIM_PERS_COUNTER       (0x380)
//0xFE5 - SLIM_PER_OFFSET)

#if 0
struct slim_core_map {
	struct {
		unsigned int ID;
		unsigned int VCR;

		volatile unsigned int Run;
		volatile unsigned int ClockGate;
		volatile unsigned int IFPipe;
		volatile unsigned int ID1Pipe;
		volatile unsigned int ID2Pipe;
		volatile unsigned int EXPipe;

		volatile unsigned int PC;

		volatile unsigned int ID1OpA;
		volatile unsigned int ID1OpB;
		volatile unsigned int ID2OpA;
		volatile unsigned int ID2OpB;

		volatile unsigned int ALUResult;
		volatile unsigned int IFInstShadow;

		volatile unsigned int JumpFlag;
		volatile unsigned int EXDmemShadow;

		volatile unsigned int Rpt;

		unsigned int r2[(0x1000 / SLIM_DIV) - 18];	// 9
	} core;

	volatile int regs[0x1000 / SLIM_DIV];

	volatile int dmem[0x800 / SLIM_DIV];

	volatile int pers[0x800 / SLIM_DIV];

	volatile int imem[0x1000 / SLIM_DIV];
};
#endif

struct internal_slim_core {
	unsigned int ID;
	unsigned int VCR;

	volatile unsigned int Run;
	volatile unsigned int ClockGate;
	volatile unsigned int IFPipe;
	volatile unsigned int ID1Pipe;
	volatile unsigned int ID2Pipe;
	volatile unsigned int EXPipe;

	volatile unsigned int PC;

	volatile unsigned int ID1OpA;
	volatile unsigned int ID1OpB;
	volatile unsigned int ID2OpA;
	volatile unsigned int ID2OpB;

	volatile unsigned int ALUResult;
	volatile unsigned int IFInstShadow;

	volatile unsigned int JumpFlag;
	volatile unsigned int EXDmemShadow;

	volatile unsigned int Rpt;

};

#define SLIM_IMEM(slim_core,word_offset) (((unsigned int*)((((unsigned int)(slim_core)->iomap))+(slim_core)->imem_offset))[word_offset])
#define SLIM_DMEM(slim_core,word_offset) (((unsigned int*)((((unsigned int)(slim_core)->iomap))+(slim_core)->dmem_offset))[word_offset])
#define SLIM_PERS(slim_core,word_offset) (((unsigned int*)((((unsigned int)(slim_core)->iomap))+(slim_core)->peripheral_offset))[word_offset])
#define SLIM_CORE(slim_core)                  ((struct internal_slim_core*)((slim_core)->iomap))
#define SLIM_REGS(slim_core,word_offset) (((unsigned int*)(((unsigned int)(slim_core)->iomap)+(slim_core)->register_offset))[word_offset])

struct slim_core {
	char init;

	char used_pool[SLIM_MAX_NUMBER_POOLS];
	struct slim_memory_pool pools[SLIM_MAX_NUMBER_POOLS];

	struct slim_memory_pool *text_pool;
	struct slim_memory_pool *small_pool;
	struct slim_memory_pool *big_pool;
	struct slim_memory_pool *buffer_pool;

	unsigned long base_address;

	union {
		void *iomap;
		//    struct slim_core_map *slimcore;
		unsigned long ioaddr;
	};

	int irq;
	unsigned long imem_offset;
	unsigned long dmem_offset;
	unsigned long peripheral_offset;
	unsigned long register_offset;

	unsigned long size;
	unsigned long imem_size;
	unsigned long dmem_size;

	unsigned long freq;

	char *elf;
	char *elf_core;

	void *priv;
};

extern struct slim_core slim_cores[SLIM_MAX_SUPPORTED_CORES];

int slim_request_firmware(char *name, char **firmware_data);
int slim_elf_load(struct slim_module *module);

char *slim_debug_core_memory(unsigned int acore, char *buffer,
			     char *line_indent);
char *slim_dump_pool(struct slim_memory_pool *pool, char *buffer,
		     char *line_indent, char *line_id2);
char *slim_debug_core(unsigned int acore, char *buffer, char *line_indent);
char *slim_debug_regs(unsigned int acore, char *buffer, char *line_indent);
char *slim_dump_circular_buffer(struct slim_module *module, int descriptor,
				char *buffer, char *line_indent);

int slim_boot_core(struct slim_module *module);
int slim_load_module(struct slim_module *module);

void slim_reset_core(int core);

//int slim_delete_core( unsigned int core );

#endif
