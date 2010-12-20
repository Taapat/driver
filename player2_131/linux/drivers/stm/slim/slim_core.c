/*
 *  SLIM Core Generic driver.
 *
 * Copyright (C) 2006 STMicroelectronics Limited
 * Author: Peter Bennett <peter.bennett@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */

/* This handles the core specific functions */

#include <linux/module.h>
#include <linux/stm/slim.h>
#include <linux/delay.h>

#include <linux/stm/slim.h>
#include "slim_core.h"
#include "slim_elf.h"

static int stop_core(struct slim_core *core, int force);

struct slim_core slim_cores[SLIM_MAX_SUPPORTED_CORES];

static char *dump_pool_memory(struct slim_memory_pool *pool, char *buffer,
			      char *line_indent)
{
	char *ptr = buffer;
	int n;

	for (n = 0; n < pool->size / 4; n++) {
		if (!(n % 8))
			ptr +=
			    sprintf(ptr, "\n%s   0x%04x: ", line_indent,
				    pool->address + n);
		ptr += sprintf(ptr, "0x%08x  ", ((int *)pool->pointer)[n]);
	}

	ptr += sprintf(ptr, "\n");

	return ptr;
}

char *slim_dump_circular_buffer(struct slim_module *module, int descriptor,
				char *buffer, char *line_indent)
{
	int base = (descriptor & 0x1ff) << 3;
	int size = ((descriptor >> 9) & 0x3f) << 2;
	int error = descriptor & 0x1000 ? 1 : 0;
	int wp = (descriptor >> 16) & 0xff;
	int rp = (descriptor >> 24) & 0xff;
	int n;
	struct slim_core *core = &slim_cores[module->core];

	int *pointer = &SLIM_DMEM(core, base);	//&core->slimcore->dmem[base];

	char *ptr = buffer;

	ptr +=
	    sprintf(ptr,
		    "%s -- 0x%08x [Base = 0x%04x, Size = 0x%04x, Wp = 0x%02x, Rp = 0x%02x] %s --",
		    line_indent, descriptor, base, size, wp, rp,
		    error ? "[E]" : "");

	for (n = 0; n < size; n++) {
		if (!(n % 8))
			ptr +=
			    sprintf(ptr, "\n%s    0x%04x: ", line_indent,
				    base + n);
		ptr += sprintf(ptr, "0x%08x  ", pointer[n]);
	}

	ptr += sprintf(ptr, "\n");

	return ptr;
}

char *slim_dump_pool(struct slim_memory_pool *pool, char *buffer,
		     char *line_id1, char *line_id2)
{
	char *ptr = buffer;
	ptr +=
	    sprintf(ptr, "%s     Size: %04u Bytes   %s\n", line_id1, pool->size,
		    line_id2);
	ptr += sprintf(ptr, "%s  Address: 0x%04x\n", line_id1, pool->address);
	ptr +=
	    sprintf(ptr, "%s  Pointer: 0x%08x\n", line_id1, (int)pool->pointer);
	return ptr;
}

static char *dump_pool(struct slim_memory_pool *pool, char *buffer,
		       char *line_indent)
{
	char *ptr = buffer;
	char line_id[64];

	while (pool) {
		//ptr += sprintf(ptr,"%s  --- %s ---\n",line_indent, pool->module->name);
		sprintf(line_id, "[%s]", pool->module->name);

		ptr = slim_dump_pool(pool, ptr, line_indent, line_id);
		ptr = dump_pool_memory(pool, ptr, line_indent);

		pool = pool->next;
	};

	return ptr;
}

static int pool_getused(int acore, enum slim_memory_type type)
{
	struct slim_core *core = &slim_cores[acore];
	struct slim_memory_pool *pool = NULL;

	switch (type) {
	case SLIM_POOL_TEXT:
		pool = core->text_pool;
		break;
	case SLIM_POOL_DATA_SMALL:
		pool = core->small_pool;
		break;
	case SLIM_POOL_DATA_BIG:
		{
			int big = 0;
			pool = core->big_pool;
			if (pool) {
				while (pool->next)
					pool = pool->next;
				big =
				    (core->dmem_size - 3) - (pool->address * 4);
			}

			return big;
		}
	case SLIM_POOL_CIRCULAR_BUFFERS:
		{
			int buffer = 0;

			pool = core->buffer_pool;
			if (pool) {
				while (pool->next)
					pool = pool->next;
				buffer =
				    (127 * 4) - (pool->address * 4 +
						 pool->size);
			}

			return buffer;
		}
		break;

	default:
		return 0;
	};

	if (!pool)
		return 0;

	while (pool->next)
		pool = pool->next;

	return pool->address * 4 + pool->size;
}

static int pool_getsize(int acore, enum slim_memory_type type)
{
	struct slim_core *core = &slim_cores[acore];

	switch (type) {
	case SLIM_POOL_TEXT:
		return core->imem_size;
	case SLIM_POOL_DATA_SMALL:
		return 127 * 4;
	case SLIM_POOL_DATA_BIG:
	case SLIM_POOL_CIRCULAR_BUFFERS:
		return core->dmem_size - 127 * 4;
	default:
		return -1;
	};

	return -1;
}

char *slim_debug_core_memory(unsigned int acore, char *buffer,
			     char *line_indent)
{
	char *ptr = buffer;

	struct slim_core *core = &slim_cores[acore];
	int used, free, used2;

	used = pool_getused(acore, SLIM_POOL_TEXT);
	free = pool_getsize(acore, SLIM_POOL_TEXT) - used;
	ptr +=
	    sprintf(ptr,
		    "%s\n--- Text Pool   - Used: %04u  Free: %04u Bytes -------\n",
		    line_indent, used, free);
	ptr = dump_pool(core->text_pool, ptr, line_indent);

	used = pool_getused(acore, SLIM_POOL_DATA_SMALL);
	free = pool_getsize(acore, SLIM_POOL_DATA_SMALL) - used;
	ptr +=
	    sprintf(ptr,
		    "\n%s--- Small Pool  - Used: %04u  Free: %04u Bytes -------\n",
		    line_indent, used, free);
	ptr = dump_pool(core->small_pool, ptr, line_indent);

	used = pool_getused(acore, SLIM_POOL_DATA_BIG);
	used2 = pool_getused(acore, SLIM_POOL_CIRCULAR_BUFFERS);
	free = pool_getsize(acore, SLIM_POOL_CIRCULAR_BUFFERS) - used - used2;
	ptr +=
	    sprintf(ptr,
		    "\n%s--- Big Pool    - Used: %04u  Free: %04u Bytes -------\n",
		    line_indent, used, free);
	ptr = dump_pool(core->big_pool, ptr, line_indent);

	ptr +=
	    sprintf(ptr,
		    "\n%s--- Buffer Pool - Used: %04u  Free: %04u Bytes -------\n",
		    line_indent, used2, free);
	ptr = dump_pool(core->buffer_pool, ptr, line_indent);

	ptr += sprintf(ptr, "\n");

	return ptr;
}

/* Currently we implement a very simple memory allocation algorithm */
struct slim_memory_pool *slim_allocate_memory(struct slim_module *module,
					      enum slim_memory_type type,
					      unsigned int size)
{
	int i;
	int free_pool = -1;
	struct slim_memory_pool *pool, *next;

	struct slim_core *core = &slim_cores[module->core];

	if (module->core >= SLIM_MAX_SUPPORTED_CORES)
		return NULL;

	for (i = 0; i < SLIM_MAX_NUMBER_POOLS; i++)
		if (!core->used_pool[i])
			free_pool = i;

	if (free_pool == -1) {
		printk(KERN_WARNING "%s: Couldn't find a free memory pool\n",
		       __FUNCTION__);
		return (struct slim_memory_pool *)NULL;
	}

	pool = &core->pools[free_pool];
	core->used_pool[free_pool] = 1;

	pool->core = module->core;
	pool->next = NULL;
	pool->size = size;
	pool->module = module;
	pool->pool = type;

	switch (type) {
	case SLIM_POOL_TEXT:	/* Allocate text segments from 0x0 onwards always leave 
				   two instructions between modules to avoid hazards */
		if (!core->text_pool) {
			core->text_pool = pool;

			pool->pointer =
			    (void *)(core->ioaddr + core->imem_offset);
			pool->address = 0;
		} else {
			next = core->text_pool;
			while (next->next)
				next = next->next;

			next->next = pool;
			pool->pointer =
			    (void *)(next->pointer + next->size + 8);
			pool->address = next->address + (next->size / 4) + 2;
		}

		break;
	case SLIM_POOL_DATA_SMALL:	/* Small pool is allocated from 0x0 to the 127th word */
		if (!core->small_pool) {
			core->small_pool = pool;

			pool->pointer =
			    (void *)(core->ioaddr + core->dmem_offset);
			pool->address = 0;
		} else {
			next = core->small_pool;
			while (next->next)
				next = next->next;

			next->next = pool;
			pool->pointer = (void *)(next->pointer + next->size);
			pool->address = next->address + (next->size / 4);
		}

		break;
	case SLIM_POOL_DATA_BIG:	/* The big pool is allocated like a stack,
					   highest address's to lowest */
		if (!core->big_pool) {
			core->big_pool = pool;

			pool->pointer =
			    (void *)(core->ioaddr + core->dmem_offset +
				     core->dmem_size - size);
			pool->address = (core->dmem_size / 4) - (size / 4);
		} else {
			next = core->big_pool;
			while (next->next)
				next = next->next;

			next->next = pool;
			pool->pointer = (void *)(next->pointer - size);
			pool->address = next->address - (size / 4);
		}

		break;
	case SLIM_POOL_CIRCULAR_BUFFERS:	/* Circular buffers have to be allocated 
						   within the first 16K so we allocate these
						   directly after the 127 words */

		if (!core->buffer_pool) {
			core->buffer_pool = pool;

			pool->pointer =
			    (void *)(core->ioaddr + core->dmem_offset +
				     (128 * 4));
			pool->address = 128;
		} else {
			next = core->buffer_pool;
			while (next->next)
				next = next->next;

			next->next = pool;
			pool->pointer = (void *)(next->pointer + next->size);
			pool->address = next->address + (next->size / 4);
		}

		break;

	default:
		printk(KERN_WARNING "%s: Invalid SLIM memory type requested\n",
		       __FUNCTION__);
		return NULL;
	};

	return pool;
}

void slim_free_memory(struct slim_memory_pool *pool)
{
	struct slim_core *core = &slim_cores[pool->core];
	struct slim_memory_pool *spool, **apool;

	switch (pool->pool) {
	case SLIM_POOL_TEXT:
		apool = &core->text_pool;
		break;
	case SLIM_POOL_DATA_SMALL:
		apool = &core->small_pool;
		break;
	case SLIM_POOL_DATA_BIG:
		apool = &core->big_pool;
		break;
	case SLIM_POOL_CIRCULAR_BUFFERS:
		apool = &core->buffer_pool;
		break;
	default:
		printk(KERN_ERR "%s: Memory corruption undefined pool\n",
		       __FUNCTION__);
		return;
	};

	spool = *apool;

	if (!spool) {
		printk(KERN_ERR
		       "%s: Trying to free pool that isn't associated with the correct pool\n",
		       __FUNCTION__);
		return;
	}

	if (spool == pool)
		*apool = pool->next;
	else {
		while (spool && spool->next != pool)
			spool = spool->next;

		if (!spool) {
			printk(KERN_ERR
			       "%s: Trying to free pool that isn't associated with the correct pool\n",
			       __FUNCTION__);
			return;
		}

		spool->next = pool->next;
	}

	core->used_pool[pool - core->pools] = 0;
}

void *slim_get_symbol_ptr(struct slim_module *module, const char *name)
{
	int value, offset;
	int core = module->core;

	if ((slim_elf_get_symbol(slim_cores[core].elf, name, &value, &offset))
	    == -1)
		return 0;

	return (void *)&SLIM_DMEM(&slim_cores[core], value / 4);
}

unsigned int slim_get_symbol(struct slim_module *module, const char *name)
{
	int value, offset;
	int core = module->core;

	if ((slim_elf_get_symbol(slim_cores[core].elf, name, &value, &offset))
	    == -1)
		return -1;

	return value / 4;
}

void *slim_get_peripheral(struct slim_module *module, int address)
{
	struct slim_core *core = &slim_cores[module->core];
	//int offset = address | 0xfffff000;

	return (void *)&SLIM_PERS(core, address);
	//(void*)&SLIM_PERS(core,((core->peripheral_offset - core->dmem_offset) / 4 )+ offset);
	//&core->slimcore->pers[((core->peripheral_offset - core->dmem_offset) / 4) + offset];
}

int slim_elf_load(struct slim_module *module)
{
	int res = -1;
	int text_size, small_size, big_size, size;
	struct slim_core *core = &slim_cores[module->core];

	if (!module)
		return -EINVAL;
	//  if (core->init) 
	//    return -EINVAL;
	if (!module->firmware)
		return -EINVAL;
	if (module->firmware_data == NULL && module->firmware == NULL)
		return -EINVAL;
	//if (!text_pool || !small_pool || !big_pool) 
	//  return -EINVAL;

	/* If we've been asked to load some firmware let's do it */
	if (!module->firmware_data)
		if ((res =
		     slim_request_firmware(module->firmware,
					   &module->firmware_data)) < 0)
			return res;

	/* Now we have the elf file, get some size info about the sections */
	slim_elf_get_sizes(module->firmware_data, &text_size, &small_size,
			   &big_size);

	/* Allocate memory as necessary */
	if (text_size)
		if (!
		    (module->text_pool =
		     slim_allocate_memory(module, SLIM_POOL_TEXT, text_size)))
			goto err1;

	if (small_size)
		if (!
		    (module->small_pool =
		     slim_allocate_memory(module, SLIM_POOL_DATA_SMALL,
					  small_size)))
			goto err2;

	if (big_size)
		if (!
		    (module->big_pool =
		     slim_allocate_memory(module, SLIM_POOL_DATA_BIG,
					  big_size)))
			goto err3;

	if (slim_elf_resolve(module->firmware_data, core->elf,
			     module->text_pool ? module->text_pool->address : 0,
			     module->small_pool ? module->small_pool->
			     address : 0,
			     module->big_pool ? module->big_pool->
			     address : 0)) {
		printk(KERN_ERR "%s: Couldn't resolve module %s\n",
		       __FUNCTION__, module->name);
		goto err4;
	}

	module->next = NULL;

	if (!core->elf) {
		core->elf = (char *)kmalloc(size =
					    slim_elf_write_size(), GFP_KERNEL);
		core->elf_core = (char *)kmalloc(size, GFP_KERNEL);
		memset(core->elf, 0, size);
		module->mainmodule = 0xfead;
		core->priv = module;
	} else {
		struct slim_module *next = core->priv;

		module->mainmodule = 0;

		while (next && next->next)
			next = next->next;
		next->next = module;
	}

	slim_elf_write_out(core->elf, module->firmware_data);

	return 0;

      err4:
	if (module->big_pool)
		slim_free_memory(module->big_pool);

      err3:
	if (module->small_pool)
		slim_free_memory(module->small_pool);

      err2:
	if (module->text_pool)
		slim_free_memory(module->text_pool);

      err1:
	if (module->firmware && module->firmware_data)
		kfree(module->firmware_data);

	printk(KERN_ERR "%s: we got an error here\n", __FUNCTION__);

	return res;
}

/* We don't remove symbols from the elf which we might need to do */
void slim_elf_unload(struct slim_module *module)
{
	struct slim_core *core = &slim_cores[module->core];
	struct slim_module *next;
	int n;

	if (module->big_pool)
		slim_free_memory(module->big_pool);
	if (module->small_pool)
		slim_free_memory(module->small_pool);
	if (module->text_pool)
		slim_free_memory(module->text_pool);

	if (module->firmware && module->firmware_data) {
		kfree(module->firmware_data);
		module->firmware_data = NULL;
	}

	if (module->mainmodule) {
		kfree(core->elf);
		kfree(core->elf_core);

		core->elf = NULL;
		core->elf_core = NULL;

		core->text_pool = NULL;
		core->small_pool = NULL;
		core->big_pool = NULL;

		for (n = 0; n < SLIM_MAX_NUMBER_POOLS; n++)
			core->used_pool[n] = 0;

		core->priv = NULL;
	} else {
		next = core->priv;
		while (next && next->next != module)
			next = next->next;

		if (!next)
			printk(KERN_WARNING
			       "%s: Tried to remove a module which isn't in the list!\n",
			       __FUNCTION__);
		else
			next->next = module->next;

		/* Redo the elf file */
		memset(core->elf, 0, slim_elf_write_size());

		next = core->priv;
		while (next) {
			slim_elf_write_out(core->elf, next->firmware_data);
			next = next->next;
		}

	}

	module->big_pool = NULL;
	module->small_pool = NULL;
	module->text_pool = NULL;

}

unsigned int slim_create_handler(struct slim_module *module,
				 unsigned long function, unsigned long ptr)
{
	return (function) | (ptr << 16);
}

unsigned int slim_create_buffer(struct slim_memory_pool *cbuffer)
{
	int base = cbuffer->address * 4;
	int size = cbuffer->size;

	if (base & 0x1f)
		return 0;	// Buffers must be aligned to 32 bytes
	if (size & 0xf)
		return 0;	// and be a multiple of 16 bytes in size

	if (base > 0x3fe0)
		return 0;	// the base must exist within the first 16k - 32 bytes
	if (size > 0x3f0)
		return 0;	// the maximum size is 1K - 16 bytes

	return (((size >> 4) & 0x3f) << 9) | ((base >> 5) & 0x1ff);
}

static void copy_data(int *dst, struct slim_memory_pool *pool, char *data)
{
	int *ptr;
	int n;

	ptr = (int *)data;
	if (pool)
		for (n = 0; n < pool->size / 4; n++)
			dst[n + pool->address] = ptr[n];

}

static struct slim_memory_pool *get_pool(struct slim_module *module,
					 enum slim_memory_type type)
{
	switch (type) {
	case SLIM_POOL_TEXT:
		return module->text_pool;

	case SLIM_POOL_DATA_SMALL:
		return module->small_pool;

	case SLIM_POOL_DATA_BIG:
	case SLIM_POOL_CIRCULAR_BUFFERS:
		return module->big_pool;

	default:
		return NULL;
	};
}

char *slim_module_symbol(struct slim_module *module, int value,
			 enum slim_memory_type type, int *offset)
{
	//really unsure about this .priv field
	//struct slim_core *core = &slim_cores[module->core].priv;
	struct slim_core *core = &slim_cores[module->core];

	struct slim_memory_pool *pool = NULL;
	char *sec = NULL;

	switch (type) {
	case SLIM_POOL_TEXT:
		sec = ".text";
		pool = core->text_pool;
		break;

	case SLIM_POOL_DATA_SMALL:
		sec = ".small_pool";
		pool = core->small_pool;
		break;

	case SLIM_POOL_DATA_BIG:
	case SLIM_POOL_CIRCULAR_BUFFERS:
		sec = ".big_pool";
		pool = core->big_pool;
		break;

	default:
		return NULL;
	};

	while (pool) {
		int addr = pool->address * 4;

		if (addr >= value && (addr + pool->size) < value) {
			if (pool == get_pool(pool->module, type)) {
				//value -= addr;

				return slim_elf_search_symbol(pool->module->
							      firmware_data,
							      sec, value,
							      offset);
			}
		}

		pool = pool->next;
	}

	return NULL;
}

char *slim_symbol(char *buf, struct slim_module *module, int value,
		  enum slim_memory_type type)
{
	//  struct slim_module *next = slim_cores[module->core].priv;

	//slim_module_symbol(next, value,
    return buf; // todo totally rubbish but then pete hasn't implemented it!
}

#if 0
char *temo(char *buf, struct slim_module *module, int all, int value,
	   enum slim_memory_type type)
{
	char *sec = NULL;
	char small_pool[] = ".small_pool";
	unsigned int offset = 0xffffffff;
	char *name = NULL;
	struct slim_module *next = NULL;
	struct slim_module *mod = NULL;

	do {

		if (all)
			next = slim_cores[module->core].priv;
		else
			next = module;

		switch (type) {
		case SLIM_POOL_TEXT:
			sec = ".text";
			pool = module->text_pool;
			break;

		case SLIM_POOL_DATA:
		case SLIM_POOL_DATA_SMALL:
		case SLIM_POOL_DATA_BIG:
		case SLIM_POOL_CIRCULAR_BUFFERS:
			if (sec == small_pool) {
				pool = module->big_pool;
				sec = ".big_pool";
			} else {
				pool = module->small_pool;
				sec = small_pool;
			}
			break;
		};

		while (module) {
			char *name2;
			unsigned int offset2 = 0xffffffff;

			name2 =
			    elf_search_symbol(module->firmware_data, sec, value,
					      &offset2);

			offset2 += pool->address * 4;

			printk(KERN_WARNING "\n%s: mod:%s %x offset:%x\n",
			       __FUNCTION__, module->name, name2, offset2);

			if (name2 && offset2 < offset) {
				name = name2;
				offset = offset2;
				mod = module;
			}

			if (all)
				module = module->next;
			else
				module = NULL;
		};

	} while (sec == small_pool);

	if (name && mod) {
		if (offset)
			sprintf(buf, "%s + 0x%x [%s]", name, offset, mod->name);
		else
			sprintf(buf, "%s [%s]", name, mod->name);

		return buf;
	}

	return NULL;
}
#endif

char *slim_debug_core(unsigned int acore, char *buffer, char *line_indent)
{
	char *ptr = buffer;
	struct slim_core *core = &slim_cores[acore];
	//  struct slim_core_map *slimcore = core->slimcore; 
	int PC = SLIM_CORE(core)->PC * 4;
	//int offset = 0;
	//struct slim_module *main = core->priv;
	//char *name = elf_search_symbol(main->firmware_data, ".text", PC, &offset);

	//char fname[128];

	//char *buf;		// = temo2(main,PC,SLIM_POOL_TEXT,&offset);

	//ptr += sprintf(ptr,"%s      ID: 0x%08x\n",line_indent,slimcore->core.ID);
	//ptr += sprintf(ptr,"%s     VCR: 0x%08x\n",line_indent,slimcore->core.VCR);
	ptr +=
	    sprintf(ptr, "%s  Frequency: %03lu MHz\n", line_indent, core->freq);
	ptr +=
	    sprintf(ptr, "%s     Repeat: 0x%08x\n", line_indent,
		    SLIM_CORE(core)->Rpt);
	ptr +=
	    sprintf(ptr, "%s        Run: 0x%08x\n", line_indent,
		    SLIM_CORE(core)->Run);
	ptr += sprintf(ptr, "%s         PC: 0x%08x []\n", line_indent, PC);

#if 0
	ptr +=
	    sprintf(ptr, "%s  IFPipe: 0x%08x\n", line_indent,
		    SLIM_CORE(core)->IFPipe);
	ptr +=
	    sprintf(ptr, "%s ID1Pipe: 0x%08x\n", line_indent,
		    SLIM_CORE(core)->ID1Pipe);
	ptr +=
	    sprintf(ptr, "%s ID2Pipe: 0x%08x\n", line_indent,
		    SLIM_CORE(core)->ID2Pipe);
	ptr +=
	    sprintf(ptr, "%s  EXPipe: 0x%08x\n", line_indent,
		    SLIM_CORE(core)->EXPipe);
#endif

#if 0				// Need to stop the slim core to read the registers
	for (n = 1; n < 15; n++) {
		if (!((n - 1) % 4))
			ptr += sprintf(ptr, "\n%s  ", line_indent);
		ptr += sprintf(ptr, "Reg %02d: 0x%08x  ", n, slimcore->regs[n]);
	}
#endif

	ptr += sprintf(ptr, "\n\n");

	return ptr;
}

char *slim_debug_regs(unsigned int acore, char *buffer, char *line_indent)
{
	struct slim_core *core = &slim_cores[acore];
	//struct slim_core_map *slimcore = core->slimcore; 
	int n;
	char *ptr = buffer;

	stop_core(core, 1);

	for (n = 0; n < 15; n++)
		ptr +=
		    sprintf(ptr, "%s  Reg[%02d]: 0x%08x\n", line_indent, n,
			    SLIM_REGS(core, n));

	SLIM_CORE(core)->Run = SLIM_RUN_ENABLE;
	SLIM_CORE(core)->ClockGate = 0;

	return ptr;
}

static void load_module(struct slim_module *module)
{
	struct slim_core *core = &slim_cores[module->core];
	//struct slim_core_map     *slimcore = core->slimcore;
	int otext, odata, osmall_pool, obig_pool;

	slim_elf_get_offsets(module->firmware_data, &otext, &odata,
			     &osmall_pool, &obig_pool);

	if (module->text_pool)
		copy_data((int *)&SLIM_IMEM(core, 0), module->text_pool,
			  &module->firmware_data[otext]);
	if (module->small_pool)
		copy_data((int *)&SLIM_DMEM(core, 0), module->small_pool,
			  &module->firmware_data[osmall_pool]);
	if (module->big_pool)
		copy_data((int *)&SLIM_DMEM(core, 0), module->big_pool,
			  &module->firmware_data[obig_pool]);
}

int slim_boot_core(struct slim_module *module)
{
	struct slim_core *core = &slim_cores[module->core];
	//  struct slim_core_map *slimcore = core->slimcore;
	int n;

	//  printk("SYNC = 0x%08x\n",&SLIM_PERS(core,SLIM_PERS_STBUS_SYNC));
	//  SLIM_PERS(core,SLIM_PERS_STBUS_SYNC) = SLIM_STBUS_SYNC_DISABLE;

	printk("0x%08x 0x%08lx\n", (unsigned int)core->iomap, core->imem_offset);
	printk("IMEM: 0x%08x\n", (unsigned int)&SLIM_IMEM(core, 0));

	SLIM_IMEM(core, 0) = 0xffffffff;
	SLIM_DMEM(core, 0) = 0xffffffff;

	printk("IMEM: 0x%08x DMEM: 0x%08x\n", SLIM_IMEM(core, 0),
	       SLIM_DMEM(core, 0));

	// Init imem so every instruction is a jump to itself
	for (n = 0; n < core->imem_size / 4; n++)
		SLIM_IMEM(core, n) =
		    0x00d00010 | (n & 0xf) | ((n & 0xfff0) << 4);

	// Init dmem to zero
	for (n = 0; n < core->dmem_size / 4; n++)
		SLIM_DMEM(core, n) = 0x0;

	load_module(module);

	//lock irq
	//getnstimeofday(&tv);
	core->freq = SLIM_PERS(core, SLIM_PERS_COUNTER);
	msleep(100);
	//getnstimeofday(&tv2);
	core->freq = (SLIM_PERS(core, SLIM_PERS_COUNTER) - core->freq) / 100000;
	//unlock irq

	//slimcore->core.Run = SLIM_RUN_ENABLE;

	return 0;
}

static int status(struct slim_core *slimcore)
{
	volatile unsigned long *run =
	    (volatile unsigned long *)&SLIM_CORE(slimcore)->Run;
	return *run;
}

static int stop_core(struct slim_core *slimcore, int force)
{
	int count = 0;

	SLIM_CORE(slimcore)->Run = SLIM_RUN_STOP;

	while (!(status(slimcore) & SLIM_RUN_STOPPED) && count < 10) {
		count++;
		msleep(10);
	}

	if (!(status(slimcore) & SLIM_RUN_STOPPED)) {
		printk(KERN_ERR "%s: SLIM core could not be stopped!!\n",
		       __FUNCTION__);

		if (force) {
			printk(KERN_WARNING "%s: SLIM Forcing STOP ...\n",
			       __FUNCTION__);
			SLIM_CORE(slimcore)->ClockGate = SLIM_CLOCK_STOP;
			SLIM_CORE(slimcore)->Run = SLIM_RUN_STOP;
		} else
			return -1;
	}

	return 0;
}

int slim_load_module(struct slim_module *module)
{
	struct slim_core *core = &slim_cores[module->core];
	//struct slim_core_map *slimcore = core->slimcore;

	//lock irq
	if (stop_core(core, 0))
		return -1;

	load_module(module);

	SLIM_CORE(core)->Run = SLIM_RUN_ENABLE;
	//unlock irq

	return 0;
}

int slim_dump_core(struct slim_module *module)
{
	struct slim_core *core = &slim_cores[module->core];
	//  struct slim_core_map *slimcore = core->slimcore;

	//lock module
	memcpy(core->elf_core, core->elf, slim_elf_write_size());

	//lock irq
	if (stop_core(core, 1))
		return -1;

	if (SLIM_CORE(core)->ClockGate & SLIM_CLOCK_STOP)
		SLIM_CORE(core)->ClockGate = 0;

	printk("RUN: 0x%08x\n", (unsigned int)&SLIM_CORE(core)->Run);
	SLIM_CORE(core)->Run = SLIM_RUN_ENABLE;
	//unlock irq
	//unlock module
    
    return 0;
}

void slim_reset_core(int core)
{
	struct slim_core *score = &slim_cores[core];
	//  struct slim_core_map *slimcore = score->slimcore;

	stop_core(score, 1);

	SLIM_CORE(score)->ClockGate = SLIM_CLOCK_RESET | SLIM_CLOCK_STOP;
	SLIM_CORE(score)->Run = SLIM_RUN_STOP;
	SLIM_CORE(score)->ClockGate = 0;
}

#if 0
int slim_find_symbol(struct slim_module *module,
		     enum slim_memory_type type, struct slim_module **module)
{
}
#endif
EXPORT_SYMBOL(slim_elf_load);
EXPORT_SYMBOL(slim_elf_unload);
EXPORT_SYMBOL(slim_load_module);
EXPORT_SYMBOL(slim_boot_core);
EXPORT_SYMBOL(slim_create_handler);
EXPORT_SYMBOL(slim_create_buffer);
EXPORT_SYMBOL(slim_get_symbol_ptr);
EXPORT_SYMBOL(slim_get_symbol);
EXPORT_SYMBOL(slim_allocate_memory);
EXPORT_SYMBOL(slim_free_memory);
EXPORT_SYMBOL(slim_get_peripheral);
EXPORT_SYMBOL(slim_dump_circular_buffer);
