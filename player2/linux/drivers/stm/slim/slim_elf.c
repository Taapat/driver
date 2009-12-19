/*
 *  ELF support for the SLIM core.
 *
 * Copyright (C) 2006 STMicroelectronics Limited
 * Author: Peter Bennett <peter.bennett@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */

#include <linux/string.h>
#include <linux/device.h>

#include <linux/elf.h>
#include "slim_elf.h"

/* asm-slim-elf.h */

/* Relocation Types */
#define R_SLIM_8      1
#define R_SLIM_12     2
#define R_SLIM_LDC_16 3
#define R_SLIM_JAB_16 4

/* end asm-slim-elf.h */

#define ELF_GET_NAME(x,y) & x [sechdr_##x[hdr_##x ->e_shstrndx].sh_offset + y]
#define ELF_DEFINE(x) Elf32_Ehdr *hdr_##x = (Elf32_Ehdr*)x; Elf32_Shdr *sechdr_##x = (Elf32_Shdr*)&x[hdr_##x->e_shoff]

static Elf32_Shdr *elf_find_section(const char *data, const char *name)
{
	ELF_DEFINE(data);
	unsigned int i;

	for (i = 1; i < hdr_data->e_shnum; i++)
		if (strcmp(ELF_GET_NAME(data, sechdr_data[i].sh_name), name) ==
		    0)
			return &sechdr_data[i];

	return (Elf32_Shdr *) 0;
}

static Elf32_Sym *elf_find_symbol(const char *data, const char *name)
{
	Elf32_Shdr *symbols = elf_find_section(data, ".symtab");
	Elf32_Shdr *strtab = elf_find_section(data, ".strtab");
	Elf32_Sym *sym = (Elf32_Sym *) & data[symbols->sh_offset];
	unsigned int i;

	for (i = 1; i < symbols->sh_size / sizeof(*sym); i++)
		if (sym[i].st_shndx != SHN_UNDEF)
			if (strcmp
			    (&data[sym[i].st_name + strtab->sh_offset],
			     name) == 0)
				return &sym[i];

	return (Elf32_Sym *) 0;
}

static int elf_resolve_symbols(const char *data, const char *rdata)
{
	ELF_DEFINE(rdata);
	Elf32_Shdr *symbols = elf_find_section(data, ".symtab");
	Elf32_Shdr *strtab = elf_find_section(data, ".strtab");
	Elf32_Sym *sym = (Elf32_Sym *) & data[symbols->sh_offset];
	unsigned int i;

	int result = 0;

	for (i = 1; i < symbols->sh_size / sizeof(*sym); i++) {
		const char *name = &data[sym[i].st_name + strtab->sh_offset];
		if (sym[i].st_shndx == SHN_UNDEF && *name) {
			//printk("%s %x\n",&data[sym[i].st_name + strtab->sh_offset],sym[i].st_shndx);
			Elf32_Sym *resolved = elf_find_symbol(rdata, name);
			if (resolved) {
				//printf("found symbol %s @ %x+%x %d\n",name,resolved->st_value,sechdr_rdata[resolved->st_shndx].sh_addr,resolved->st_shndx);
				sym[i].st_value =
				    resolved->st_value +
				    sechdr_rdata[resolved->st_shndx].sh_addr;
			} else
				result = -1;
		}
	}

	return result;
}

static unsigned int elf_apply_relocations(char *data)
{
	ELF_DEFINE(data);
	Elf32_Shdr *srel = elf_find_section(data, ".rel.text");
	Elf32_Rel *rel = (Elf32_Rel *) & data[srel->sh_offset];
	Elf32_Sym *sym =
	    (Elf32_Sym *) & data[elf_find_section(data, ".symtab")->sh_offset];
	Elf32_Shdr *text = elf_find_section(data, ".text");
	unsigned int i;
	char *op;

	for (i = 0; i < srel->sh_size / sizeof(*rel); i++) {
		signed int *inst =
		    (signed int *)&data[text->sh_offset + rel[i].r_offset];
		signed int offset =
		    sechdr_data[sym[ELF32_R_SYM(rel[i].r_info)].st_shndx].
		    sh_addr;
		signed int resolve =
		    sym[ELF32_R_SYM(rel[i].r_info)].st_value / 4;
		signed int value = 0xdeadbeed;
		signed int new_value = 0xdeadbeef;

		switch (ELF32_R_TYPE(rel[i].r_info)) {
		case R_SLIM_8:
			op = "R_SLIM_8";
			{
				signed char a = (*inst & 0xff);
				value = a;
				new_value = value + offset + resolve;

				if (new_value > 127 || new_value < -128)
					return EREL_OUT_RANGE;

				*inst =
				    (*inst & 0x00ffff00) | (new_value & 0xff);
			}
			break;
		case R_SLIM_12:
			op = "R_SLIM_12";
			{
				unsigned int a = (*inst & 0xfff);
				value = a;
				new_value = value + offset + resolve;

				if (new_value > 4097 || new_value < 0)
					return EREL_OUT_RANGE;

				*inst =
				    (*inst & 0x00fff000) | (new_value & 0xfff);
			}
			break;
		case R_SLIM_LDC_16:
			op = "R_SLIM_LDC_16";
			{
				signed short int a = (*inst & 0xffff);
				value = a;
				new_value = value + offset + resolve;

				if (new_value > 32767 || new_value < -32768)
					return EREL_OUT_RANGE;

				*inst =
				    (*inst & 0x00ff0000) | (new_value & 0xffff);
			}
			break;
		case R_SLIM_JAB_16:
			op = "R_SLIM_JAB_16";
			{
				unsigned int a =
				    (*inst & 0xf) | ((*inst >> 4) & 0xfff0);
				value = a;
				new_value = value + offset + resolve;

				if (new_value > 65536 || new_value < 0)
					return EREL_OUT_RANGE;

				*inst =
				    (*inst & 0x00f000f0) | (new_value & 0xf) |
				    ((new_value << 4) & 0xfff00);
			}
			break;

		default:
			//printk("unhandled relocation %d\n",ELF32_R_TYPE(rel[i].r_info));
			return EREL_UNKNOWN;
		}

		//printf("offset: 0x%04x type:%16s\t %02d %02d section:%12s %04x+%04x+%04x = %04x\n",rel[i].r_offset,op, ELF32_R_SYM(rel[i].r_info), sym[ELF32_R_SYM(rel[i].r_info)].st_shndx, ELF_GET_NAME(data,sechdr_data[sym[ELF32_R_SYM(rel[i].r_info)].st_shndx].sh_name),value,offset,resolve,new_value);
	}

	return 0;
}

static int elf_check_slim(char *data)
{
	Elf32_Ehdr *hdr = (Elf32_Ehdr *) data;

	if (memcmp(ELFMAG, hdr->e_ident, sizeof(ELFMAG) - 1))
		return -1;

	if (hdr->e_machine != EM_SLIM) {
		printk(KERN_WARNING "%s: Elf architecture is not 'SLIM'\n",
		       __FUNCTION__);
		return -2;
	}

	return 0;
}

#define MAX_SYM 128
#define MAX_STRTAB 4096

/* The output of the elf file, everything pre-allocated to make life easy */
struct slim_elf {
	Elf32_Ehdr hdr;		/* Header */
	Elf32_Phdr phdr[2];	/* Program Header 0 = text, 1 = data */
	Elf32_Shdr shdr[8];

	char shstrtab[128];

	Elf32_Sym sym[MAX_SYM];

	char strtab[MAX_STRTAB];

	char text[16 * 1024];
	char data[16 * 1024];

	int regs[16];
	int core_regs[32];
};

#define OFFSET(x,y) ((char*)(&(x->y)) - (char*)(x))

static void elf_write_header(Elf32_Shdr * shdr,
			     int name,
			     int type,
			     int flags,
			     int addr,
			     int offset, int size, int l, int i, int ad, int e)
{
	shdr->sh_name = name;
	shdr->sh_type = type;
	shdr->sh_flags = flags;
	shdr->sh_addr = addr;
	shdr->sh_offset = offset;
	shdr->sh_size = size;
	shdr->sh_link = l;
	shdr->sh_info = i;
	shdr->sh_addralign = ad;
	shdr->sh_entsize = e;
}

int slim_elf_write_size(void)
{
	return sizeof(struct slim_elf);
}

/* To use this function get allocate a buffer with size returned from
   slim_elf_write_size, then memset it to 0. Run the main firmware
   through first, then any extra modules you have loaded.  Write this
   out to a file and then use it with GDB, objdump, etc..
*/
int slim_elf_write_out(char *buffer, char *data)
{
	ELF_DEFINE(data);
	struct slim_elf *out = (struct slim_elf *)buffer;
	char shstrtab[] =
	    "\0.symtab\0.strtab\0.shstrtab\0.text\0.data\0.regs\0.core_regs\0";
	int n;
	Elf32_Shdr *text, *small_pool, *large_pool, *symbols, *strtab;
	int number_symbols, number_new_symbols = 0, size_strtab = 0;

#if 0
	printf("e_shentsize %x\n", hdr_data->e_shnum);

	for (n = 0; n < (hdr_data->e_shnum); n++) {
		printf("\nSection %d<%s>\n", n,
		       ELF_GET_NAME(data, sechdr_data[n].sh_name));
		printf("         sh_name: 0x%x\n", sechdr_data[n].sh_name);
		printf("         sh_type: %d\n", sechdr_data[n].sh_type);
		printf("       sh_offset: %d\n", sechdr_data[n].sh_offset);
		printf("        sh_flags: 0x%x\n", sechdr_data[n].sh_flags);
		printf("         sh_link: 0x%x\n", sechdr_data[n].sh_link);
		printf("         sh_info: 0x%x\n", sechdr_data[n].sh_info);
		printf("    sh_addralign: 0x%x\n", sechdr_data[n].sh_addralign);
		printf("      sh_entsize: 0x%x\n", sechdr_data[n].sh_entsize);

	}
#endif

	/* Setup the header */
	memcpy(out->hdr.e_ident, hdr_data->e_ident, EI_NIDENT);
	out->hdr.e_type = ET_EXEC;	//hdr_data->e_type;
	out->hdr.e_machine = hdr_data->e_machine;
	out->hdr.e_version = hdr_data->e_version;
	out->hdr.e_entry = 0;
	out->hdr.e_phoff = OFFSET(out, phdr[0]);
	out->hdr.e_shoff = OFFSET(out, shdr[0]);
	out->hdr.e_flags = 0;
	out->hdr.e_ehsize = sizeof(Elf32_Ehdr);
	out->hdr.e_phentsize = sizeof(Elf32_Phdr);
	out->hdr.e_shentsize = sizeof(Elf32_Shdr);
	out->hdr.e_shnum = 8;
	out->hdr.e_shstrndx = 3;

	/* Setup the section string names */
	memcpy(out->shstrtab, shstrtab, sizeof(shstrtab));

	elf_write_header(&out->shdr[1], sizeof("\0.symtab\0.strtab\0.shstrtab"),	//.text
			 SHT_PROGBITS,
			 SHF_ALLOC | SHF_EXECINSTR,
			 0, OFFSET(out, text), sizeof(out->text), 0, 0, 1, 0);

	elf_write_header(&out->shdr[2], sizeof("\0.symtab\0.strtab\0.shstrtab\0.text"),	//.data
			 SHT_PROGBITS,
			 SHF_ALLOC | SHF_WRITE,
			 0x00000,
			 OFFSET(out, data), sizeof(out->data), 0, 0, 1, 0);

	elf_write_header(&out->shdr[3], sizeof("\0.symtab\0.strtab"),	//.shstrtab
			 SHT_STRTAB,
			 0,
			 0,
			 OFFSET(out, shstrtab), sizeof(shstrtab), 0, 0, 1, 0);

	/* Copy the text section out */
	text = elf_find_section(data, ".text");
	memcpy(&out->text[text->sh_addr * 4], &data[text->sh_offset],
	       text->sh_size);

	/* Copy the small data pool */
	small_pool = elf_find_section(data, "small_pool");
	if (small_pool)
		memcpy(&out->data[small_pool->sh_addr * 4],
		       &data[small_pool->sh_offset], small_pool->sh_size);

	/* And the large data pool */
	large_pool = elf_find_section(data, "large_pool");
	if (large_pool)
		memcpy(&out->data[small_pool->sh_addr * 4],
		       &data[small_pool->sh_offset], large_pool->sh_size);

	/* Record the current number of symbols */
	number_symbols = out->shdr[5].sh_size / sizeof(Elf32_Sym);
	number_new_symbols = 0;

	/* And the current size of the symbol names */
	size_strtab = out->shdr[4].sh_size;

	/* Find the Symbol table, and the string table */
	symbols = elf_find_section(data, ".symtab");
	strtab = elf_find_section(data, ".strtab");

	if (symbols) {
		/* Calculate the number of symbols to add, */
		number_new_symbols = symbols->sh_size / sizeof(Elf32_Sym);

		/* Check we have enough space for everything */
		if ((number_symbols + number_new_symbols) > MAX_SYM) {
			printk(KERN_WARNING
			       "%s: Not enough symbols in elf file\n",
			       __FUNCTION__);
			return -ENOMEM;
		}

		if ((size_strtab + strtab->sh_size) > MAX_STRTAB) {
			printk(KERN_WARNING
			       "%s: Not enough strtab in elf file\n",
			       __FUNCTION__);
			return -ENOMEM;
		}

		/* and add them */
		memcpy(&out->sym[number_symbols], &data[symbols->sh_offset],
		       number_new_symbols * sizeof(Elf32_Sym));

		/* Add the strings */
		memcpy(&out->strtab[size_strtab], &data[strtab->sh_offset],
		       strtab->sh_size);

		/* Go through each of the new symbols and update the pointer to the strings */
		for (n = 0; n < number_new_symbols; n++) {
			int shndx = out->sym[number_symbols + n].st_shndx;
			out->sym[number_symbols + n].st_name += size_strtab;

			if (shndx != SHN_UNDEF && shndx < SHN_LORESERVE) {
				/* Update the symbol's pointer to the new place */
				out->sym[number_symbols + n].st_value +=
				    sechdr_data[shndx].sh_addr * 4;

				/* If it is in the text segment, give it the new .text section id */
				if (shndx == (text - sechdr_data))
					out->sym[number_symbols + n].st_shndx =
					    1;

				/* And point the small pool and big pool to the data segment */
				if (small_pool)
					if (shndx == (small_pool - sechdr_data))
						out->sym[number_symbols +
							 n].st_shndx = 2;

				if (large_pool)
					if (shndx == (large_pool - sechdr_data))
						out->sym[number_symbols +
							 n].st_shndx = 2;

				//printf("%d %d %d\n",shndx,text-sechdr_data,((text - sechdr_data) / sizeof(Elf32_Shdr)),((sechdr_data - small_pool) / sizeof(Elf32_Shdr)),((sechdr_data - large_pool) / sizeof(Elf32_Shdr)));

				/* Symbols in this elf section might not be defined because we have them in another section
				   therefore scrub 'em out */
			} else if (shndx == SHN_UNDEF)
				out->sym[number_symbols + n].st_name = 0;

			//printf("%d %d\n",out->sym[number_symbols+n].st_value);
		}

		/* Update the new sizes of symbol and string tables */
		size_strtab += strtab->sh_size;
		number_symbols += number_new_symbols;
		//printf("number syms %x\n",number_new_symbols);
	}

	/* Now write / update the symbol table and string table */
	elf_write_header(&out->shdr[4], sizeof("\0.symtab"),	//.strtab
			 SHT_STRTAB, 0, 0, OFFSET(out, strtab), size_strtab,	//sizeof(strtab),
			 0, 0, 1, 0);

	elf_write_header(&out->shdr[5], 1,	//.symtab
			 SHT_SYMTAB, 0, 0, OFFSET(out, sym[0]), number_symbols * sizeof(Elf32_Sym), 4,	// Link to section 4
			 0x14,	// Info ????? not sure
			 0x4,	// alignment
			 0x10);	// entsize (sizeof(Elf32_Sym))

	elf_write_header(&out->shdr[6], sizeof("\0.symtab\0.strtab\0.shstrtab\0.text\0.data"), SHT_NOTE, 0,	//SHF_ALLOC | SHF_WRITE,
			 0x00000,
			 OFFSET(out, regs), sizeof(out->regs), 0, 0, 1, 0);

	elf_write_header(&out->shdr[7], sizeof("\0.symtab\0.strtab\0.shstrtab\0.text\0.data\0.regs"), SHT_NOTE, 0,	//SHF_ALLOC | SHF_WRITE,
			 0x00000,
			 OFFSET(out, core_regs),
			 sizeof(out->core_regs), 0, 0, 1, 0);

	return 0;
	//printk("%d %d %d\n",out,OFFSET(out,shstrtab),sizeof(out->shstrtab));
}

int slim_elf_resolve(char *data, char *rdata, int text, int small_pool,
		     int large_pool)
{
	int res;
	Elf32_Shdr *stext, *ssmall_pool, *sbig_pool;

	if ((res = elf_check_slim(data)) < 0)
		return res;

	stext = elf_find_section(data, ".text");
	ssmall_pool = elf_find_section(data, "small_pool");
	sbig_pool = elf_find_section(data, "big_pool");

	if (!stext)
		return ENO_TEXT;

	stext->sh_addr = text;
	if (ssmall_pool)
		ssmall_pool->sh_addr = small_pool;
	if (sbig_pool)
		sbig_pool->sh_addr = large_pool;

	if (rdata)
		if (elf_resolve_symbols(data, rdata) < 0)
			return ESYM_UNRESOLVED;

	if ((res = elf_apply_relocations(data)) < 0)
		return res;

	return 0;
}

int slim_elf_get_sizes(char *data, int *text, int *small_pool, int *big_pool)
{
	Elf32_Shdr *stext = elf_find_section(data, ".text");
	Elf32_Shdr *ssmall_pool = elf_find_section(data, "small_pool");
	Elf32_Shdr *sbig_pool = elf_find_section(data, "big_pool");

	if (stext)
		*text = stext->sh_size;
	else
		*text = 0;
	if (ssmall_pool)
		*small_pool = ssmall_pool->sh_size;
	else
		*small_pool = 0;
	if (sbig_pool)
		*big_pool = sbig_pool->sh_size;
	else
		*big_pool = 0;

	return 0;
}

int slim_elf_get_offsets(char *elf, int *text, int *data, int *small_pool,
			 int *big_pool)
{
	Elf32_Shdr *stext = elf_find_section(elf, ".text");
	Elf32_Shdr *sdata = elf_find_section(elf, ".data");
	Elf32_Shdr *ssmall_pool = elf_find_section(elf, "small_pool");
	Elf32_Shdr *sbig_pool = elf_find_section(elf, "big_pool");

	if (stext)
		*text = stext->sh_offset;
	else
		*text = 0;
	if (sdata)
		*data = sdata->sh_offset;
	else
		*data = 0;
	if (ssmall_pool)
		*small_pool = ssmall_pool->sh_offset;
	else
		*small_pool = 0;
	if (sbig_pool)
		*big_pool = sbig_pool->sh_offset;
	else
		*big_pool = 0;

	return 0;
}

int slim_elf_get_symbol(char *elf, const char *name, int *value, int *offset)
{
	ELF_DEFINE(elf);
	Elf32_Sym *symbol = elf_find_symbol(elf, name);

	if (!symbol)
		return -1;

	*value = symbol->st_value;
	*offset = sechdr_elf[symbol->st_shndx].sh_addr;

	return 0;
}

char *slim_elf_search_symbol(char *elf, const char *name, int value,
			     int *offset)
{
	ELF_DEFINE(elf);
	Elf32_Shdr *sec = elf_find_section(elf, name);
	Elf32_Shdr *symbols = elf_find_section(elf, ".symtab");
	Elf32_Shdr *strtab = elf_find_section(elf, ".strtab");
	Elf32_Sym *sym = (Elf32_Sym *) & elf[symbols->sh_offset];
	Elf32_Sym *sym_found = NULL;
	unsigned int i;

	if (!sec || !symbols || !strtab)
		return NULL;

	for (i = 1; i < symbols->sh_size / sizeof(*sym); i++)
		if (!sym[i].st_info && sym[i].st_shndx == (sec - sechdr_elf)) {
			if (!sym_found)
				sym_found = &sym[i];
			else if (((unsigned)(value - sym[i].st_value)) <
				 ((unsigned)(value - sym_found->st_value)))
				sym_found = &sym[i];
		}

	if (sym_found && offset) {
		*offset = value - sym_found->st_value;
		return &elf[sym_found->st_name + strtab->sh_offset];
	} else {
		return NULL;
	}
}
