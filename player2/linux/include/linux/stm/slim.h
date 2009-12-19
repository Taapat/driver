/*
 * A generic driver for the SLIM core.
 *
 * Copyright (C) 2006 STMicroelectronics Limited
 * Author: Peter Bennett <peter.bennett@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */

#ifndef _SLIM_H
#define _SLIM_H

#define SLIM_MAX_SUPPORTED_CORES 2
#define SLIM_MAX_NUMBER_POOLS    64

/* The ID's of Pool's you can allocate SLIM memory from */
enum slim_memory_type {
	SLIM_POOL_FREE = 0,
	SLIM_POOL_TEXT,		/* Instruction memory */
	SLIM_POOL_DATA_SMALL,	/* Variables that can be directly accessed in one instruction from the core.
				   however these are limited (only 128*32 bits) */
	SLIM_POOL_DATA_BIG,	/* Variables that are de-referenced using a register 
				   (which include the internal channel structures). */
	SLIM_POOL_CIRCULAR_BUFFERS,	/* Circular buffers for data buffering */
	SLIM_POOL_DATA
};

/* The structure returned by the allocator */
struct slim_memory_pool {
	int core;		/* The core the pool belongs to */
	enum slim_memory_type pool;	/* The pool the memory belongs to */
	unsigned int size;	/* The number of bytes of memory */
	void *pointer;		/* This CPU's pointer to the memory */
	short int address;	/* Virtual Word offset into SLIM core */

	struct slim_module *module;	/* The module the pool belongs to */
	struct slim_memory_pool *next;	/* The next pool in the list */
};

/* The pool allocation function */
struct slim_memory_pool *slim_allocate_memory(struct slim_module *module,
					      enum slim_memory_type type,
					      unsigned int size);

/* The pool free function */
void slim_free_memory(struct slim_memory_pool *pool);

/* Common structure between core module and sub module */
struct slim_module {
	int core;		        /* Core */
	char *name;		        /* Name of the module */

	char *firmware;         /* Optional file containing the firmware */
	char *firmware_data;    /* Actual firmware data, if firmware is NULL, this must be filled */

	/* Pools associated with the firmware */
	struct slim_memory_pool *text_pool;
	struct slim_memory_pool *small_pool;
	struct slim_memory_pool *big_pool;

	int mainmodule;
	struct slim_module *next;
};

unsigned int slim_get_symbol(struct slim_module *module, const char *name);
void *slim_get_symbol_ptr(struct slim_module *module, const char *name);
void *slim_get_peripheral(struct slim_module *module, int address);

unsigned int slim_create_handler(struct slim_module *module,
				 unsigned long function, unsigned long ptr);
unsigned int slim_create_buffer(struct slim_memory_pool *cbuffer);

int slim_elf_load(struct slim_module *module);
void slim_elf_unload(struct slim_module *module);
int slim_load_module(struct slim_module *module);

struct slim_driver {
	char *name;

	struct slim_module *(*create_driver) (struct slim_driver * driver,
					      int version);
	void (*delete_driver) (struct slim_driver * driver);

	int (*suspend) (struct slim_driver * driver);
	int (*resume) (struct slim_driver * driver);

	int (*interrupt) (struct slim_driver * driver);
	char *(*debug) (struct slim_driver * driver, char *buffer,
			char *line_indent);

	// Private data for the driver
	void *private;
};

int slim_register_driver(struct slim_driver *driver);
void slim_unregister_driver(struct slim_driver *driver);

/*
#define FLEXIDMA_GET_REGISTER _IO('o',1)
#define FLEXIDMA_SET_REGISTER _IO('o',2)
#define FLEXIDMA_RUN
#define FLEXIDMA_STOP
#define FLEXIDMA_STEP
#define FLEXIDMA_BREAK
*/

#define SLIM_READ_MEM   _IO('o',1)
#define SLIM_WRITE_MEM  _IO('o',2)
#define SLIM_GET_SYMBOL _IO('o',3)

struct slim_mailbox {
	unsigned int status;
	unsigned int set;
	unsigned int clear;
	unsigned int mask;
};

struct plat_slim {
	char *name;		/* Name of driver required */
	int version;		/* Version of driver required */

	int imem_offset;	/* Offset to instruction memory */
	int dmem_offset;	/* Offset to data memory */
	int pers_offset;	/* Offset to peripherals */
	int regs_offset;	/* Offset to registers */

	int imem_size;		/* Size of Instruction memory IO (eg. 4 bytes = 1 instruction) */
	int dmem_size;		/* Size of Data Memory in Bytes */
};

#endif
