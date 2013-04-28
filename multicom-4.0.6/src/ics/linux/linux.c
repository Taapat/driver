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

/* Configured in ics_module.c */
struct bpa2_part *ics_bpa2_part;

/*
 * Physically contiguous memory allocation
 *
 * This will use the BPA2 partition allocator if the ics_bpa2_part partition
 * has been initialised during module init. Otherwise it falls back to the
 * older BPA2 API; bigphysarea
 * The BPA2 partition allocator returns a physical address which needs to be
 * mapped into kernel address space. This is always done with a CACHED mapping.
 *
 * Debug wrappers for these are defined in _ics_debug.h
 */
void *_ics_os_contig_alloc (ICS_SIZE size, ICS_SIZE align)
{
    if (ics_bpa2_part) {
	unsigned long addr;
	unsigned long npages;
	void *ptr;

	npages = ((size-1)/PAGE_SIZE)+1;
	addr = bpa2_alloc_pages(ics_bpa2_part, npages, ((align-1)/PAGE_SIZE)+1, GFP_KERNEL);
	
	if (!addr)
	    return NULL;

	/* Always map memory CACHED */
	ptr = ioremap_cache(addr, npages*PAGE_SIZE);
	
	if (ptr == NULL)
	    ICS_EPRINTF(ICS_DBG_INIT, "Failed to map contig memory @ %lx size %d\n", addr, size);

	return ptr;
    }
    else
	return (void *) bigphysarea_alloc_pages(((size-1)/PAGE_SIZE)+1, ((align-1)/PAGE_SIZE)+1, GFP_KERNEL);
}

/*
 * Free off a contiguous memory allocation
 *
 * For BPA2 partition allocated memory we also need to iounmap() it
 * as well as converting it back to a physical address so it can be freed
 *
 */
#if defined (__arm__)
extern void set_iounmap_nonlazy(void);
#endif
void _ics_os_contig_free (ICS_VOID *ptr)
{
    if (ics_bpa2_part) {
	ICS_ERROR     err;
	ICS_OFFSET    addr;
	ICS_MEM_FLAGS mflags;

	/* First need to convert vaddr back to phys */
	err = _ICS_OS_VIRT2PHYS(ptr, &addr, &mflags);

#if defined (__arm__)
 /* Call before iounmap(), if you want vm_area_struct's to be freed immediately.  */
 /* Check kernel file vmalloc.c, if it exports the symbol "set_iounmap_nonlazy". */
#ifndef DISABLE_IOUNMAP_NONLAZY
 set_iounmap_nonlazy();
#endif
#endif

	/* Now unmap vaddr space */
	iounmap(ptr);

	if (err == ICS_SUCCESS)
	    bpa2_free_pages(ics_bpa2_part, addr);
	else
	    ICS_EPRINTF(ICS_DBG_INIT, "Failed to free contig memory @ %p : %d\n", ptr, err);
    }
    else
	bigphysarea_free_pages((void *)ptr);
}

/* 
 * Translate a kernel virtual address to a physical one 
 *
 * Returns either ICS_SUCCESS if the translation succeeds
 * Returns ICS_HANDLE_INVALID otherwise
 */
ICS_ERROR ics_os_virt2phys (ICS_VOID *vaddr, ICS_OFFSET *paddrp, ICS_MEM_FLAGS *mflagsp)
{
    unsigned long addr = (unsigned long) vaddr;

#if defined (__sh__)
    if (addr < P1SEG || ((addr >= VMALLOC_START) && (addr < VMALLOC_END)))
#elif defined (__arm__)
    if (addr < TASK_SIZE || ((addr >= VMALLOC_START) && (addr < VMALLOC_END)))
#endif
    {
	/*
	 * Find the virtual address of either a user page (<P1SEG) or VMALLOC (P3SEG)
	 *
	 * This code is based on vmalloc_to_page() in mm/memory.c
	 */
	struct mm_struct *mm;

	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep, pte;

	/* Must use the correct mm based on whether this is a kernel or a userspace address */
	if (addr >= VMALLOC_START)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31)
	    mm = &init_mm;
#else
	{
	    struct page *page;
	  
	    /* init_mm is no longer exported 
	     * we can do the vmalloc to page conversion but how do we
	     * discover what cache flags that mapping has ?
	     */
	    page = vmalloc_to_page(vaddr);
	    if (page == NULL)
	      return ICS_HANDLE_INVALID;
	    
	    *paddrp = page_to_phys(page) | (addr & (PAGE_SIZE-1));
	    *mflagsp = ICS_CACHED;		/* XXXX assume worst case */
	    
	    return ICS_SUCCESS;
	}
#endif
	else
	    mm = current->mm;

	/* Safety first! */
	if (mm == NULL)
	    return ICS_HANDLE_INVALID;

	spin_lock(&mm->page_table_lock);
	
	pgd = pgd_offset(mm, addr);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
	    goto out;

	pud = pud_offset(pgd, addr);
	if (pud_none(*pud) || pud_bad(*pud))
	    goto out;
	
	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
	    goto out;
	
	ptep = pte_offset_map(pmd, addr);
	if (!ptep) 
	    goto out;

	pte = *ptep;
	if (pte_present(pte)) {
	    pte_unmap(ptep);
	    spin_unlock(&mm->page_table_lock);
	    
	    /* pte_page() macro is broken for SH in linux 2.6.20 and later */
	    *paddrp = page_to_phys(pfn_to_page(pte_pfn(pte))) | (addr & (PAGE_SIZE-1));
	    
	    /* P3 segment pages cannot be looked up with pmb_virt_to_phys()
	     * instead we need to examine the _PAGE_CACHABLE bit in the pte
	     */
#if defined (__sh__)
	    *mflagsp = (pte_val(pte) & _PAGE_CACHABLE) ? ICS_CACHED : ICS_UNCACHED;
#elif defined (__arm__)
    /* Define PTE C-bit mask for arm core. */
#define _PAGE_CACHABLE_MASK	( 0x0F << 2 )
	    *mflagsp = ((pte_val(pte) & _PAGE_CACHABLE_MASK) == L_PTE_MT_UNCACHED) ? ICS_UNCACHED : ICS_CACHED;
#endif
	    return ICS_SUCCESS;
	}
	
	pte_unmap(ptep);
	
    out:
	spin_unlock(&mm->page_table_lock);

	/* Failed to find a pte */
	return ICS_HANDLE_INVALID;
    }
    else
#if defined (__sh__)
#if defined(CONFIG_32BIT)
    {
	unsigned long flags;
	
	/* Try looking for an ioremap() via the PMB */
	if (pmb_virt_to_phys(vaddr, (unsigned long *)paddrp, &flags) == 0)
	{
	  /* Success: generate the returned mem flags */
	  *mflagsp = (flags & PMB_C) ? ICS_CACHED : ICS_UNCACHED;

	  return ICS_SUCCESS;
	}

	/* Failed to find a mapping */
	return ICS_HANDLE_INVALID;
    }
#else
    {
      	/* Assume 29-bit SH4 Linux */
	unsigned long addr = (unsigned long) vaddr;
	
	/* P0/P3 SEGs handled above */
	if (addr < P4SEG)
	{
	  *(paddrp)= PHYSADDR(addr);

	  /* only the P2SEG is uncached */
	  *mflagsp = (PXSEG(addr) == P2SEG) ? ICS_UNCACHED : ICS_CACHED;
	}
	else
	{
	  /* Don't convert P4 SEG addresses */
	  *(paddrp)= addr;
	  *mflagsp = ICS_PHYSICAL;
	}
	
	return ICS_SUCCESS;
    }
#endif /* CONFIG_32BIT */
#elif defined(__arm__)
  {
     *(paddrp)=__virt_to_phys(addr);
	    *mflagsp = ICS_CACHED;		/* XXXX assume worst case */
	    return ICS_SUCCESS;
  }
#endif /* __sh__ */

    /* Not implemented */
    return ICS_HANDLE_INVALID;
}

#if defined(__arm__)
void * _ics_os_mmap(unsigned long phys_addr, size_t size, unsigned int cache)
{
   /* Avoid section mapping -since we can not perform ics_os_virt2phys() on 2MB aligned mapping. */
   if(!((__pfn_to_phys(phys_addr) | size ) & ~PMD_MASK))
      size += PAGE_SIZE; 
   return (cache) ? ioremap_cache((unsigned long)(phys_addr), (size)) : ioremap_nocache((unsigned long)(phys_addr), (size));
}

ICS_ERROR _ics_os_munmap(void * vmem)
{
 /* Call before iounmap(), if you want vm_area_struct's to be freed immediately.  */
 /* Check kernel file vmalloc.c, if it exports the symbol "set_iounmap_nonlazy". */
#ifndef DISABLE_IOUNMAP_NONLAZY
 set_iounmap_nonlazy();
#endif
	/* Now unmap vaddr space */
	iounmap(vmem);

 return ICS_SUCCESS;
}
#endif /* __arm__*/
/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
