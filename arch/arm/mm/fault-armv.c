/*
 *  linux/arch/arm/mm/fault-armv.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Modifications for ARM processor (c) 1995-2002 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bitops.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/pagemap.h>

#include <asm/bugs.h>
#include <asm/cacheflush.h>
#include <asm/cachetype.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

static unsigned long shared_pte_mask = L_PTE_MT_BUFFERABLE;

/*
 * We take the easy way out of this problem - we make the
 * PTE uncacheable.  However, we leave the write buffer on.
 *
 * Note that the pte lock held when calling update_mmu_cache must also
 * guard the pte (somewhere else in the same mm) that we modify here.
 * Therefore those configurations which might call adjust_pte (those
 * without CONFIG_CPU_CACHE_VIPT) cannot support split page_table_lock.
 */
static int adjust_pte(struct vm_area_struct *vma, unsigned long address)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte, entry;
	int ret;

	pgd = pgd_offset(vma->vm_mm, address);
	if (pgd_none_or_clear_bad(pgd))
		return 0;
	
    pmd = pmd_offset(pgd, address);	
	if (pgd_none_or_clear_bad(pgd))
		return 0;

	pte = pte_offset_map(pmd, address);
	entry = *pte;

	/*
	 * If this page is present, it's actually being shared.
	 */
	ret = pte_present(entry);

	/*
	 * If this page isn't present, or is already setup to
	 * fault (ie, is old), we can safely ignore any issues.
	 */
	if (ret && (pte_val(entry) & L_PTE_MT_MASK) != shared_pte_mask) {
		unsigned long pfn = pte_pfn(entry);
		flush_cache_page(vma, address, pfn);
		outer_flush_range((pfn << PAGE_SHIFT),
				  (pfn << PAGE_SHIFT) + PAGE_SIZE);
		pte_val(entry) &= ~L_PTE_MT_MASK;
		pte_val(entry) |= shared_pte_mask;
		set_pte_at(vma->vm_mm, address, pte, entry);
		flush_tlb_page(vma, address);
	}
	pte_unmap(pte);
	return ret;
}

static void
make_coherent(struct address_space *mapping, struct vm_area_struct *vma, unsigned long addr, unsigned long pfn)
{
	struct mm_struct *mm = vma->vm_mm;
	struct vm_area_struct *mpnt;
	struct prio_tree_iter iter;
	unsigned long offset;
	pgoff_t pgoff;
	int aliases = 0;

	pgoff = vma->vm_pgoff + ((addr - vma->vm_start) >> PAGE_SHIFT);

	/*
	 * If we have any shared mappings that are in the same mm
	 * space, then we need to handle them specially to maintain
	 * cache coherency.
	 */
	flush_dcache_mmap_lock(mapping);
	vma_prio_tree_foreach(mpnt, &iter, &mapping->i_mmap, pgoff, pgoff) {
		/*
		 * If this VMA is not in our MM, we can ignore it.
		 * Note that we intentionally mask out the VMA
		 * that we are fixing up.
		 */
		if (mpnt->vm_mm != mm || mpnt == vma)
			continue;
		if (!(mpnt->vm_flags & VM_MAYSHARE))
			continue;
		offset = (pgoff - mpnt->vm_pgoff) << PAGE_SHIFT;
		aliases += adjust_pte(mpnt, mpnt->vm_start + offset);
	}
	flush_dcache_mmap_unlock(mapping);
	if (aliases)
		adjust_pte(vma, addr);
	else
		flush_cache_page(vma, addr, pfn);
}

/*
 * Take care of architecture specific things when placing a new PTE into
 * a page table, or changing an existing PTE.  Basically, there are two
 * things that we need to take care of:
 *
 *  1. If PG_dcache_dirty is set for the page, we need to ensure
 *     that any cache entries for the kernels virtual memory
 *     range are written back to the page.
 *  2. If we have multiple shared mappings of the same space in
 *     an object, we need to deal with the cache aliasing issues.
 *
 * Note that the pte lock will be held.
 */
void update_mmu_cache(struct vm_area_struct *vma, unsigned long addr, pte_t pte)
{
	unsigned long pfn = pte_pfn(pte);
	struct address_space *mapping;
	struct page *page;

	if (!pfn_valid(pfn))
		return;

	page = pfn_to_page(pfn);
	mapping = page_mapping(page);
#ifndef CONFIG_SMP
	if (test_and_clear_bit(PG_dcache_dirty, &page->flags))
		__flush_dcache_page(mapping, page);
#endif
	if (mapping) {
		if (cache_is_vivt())
			make_coherent(mapping, vma, addr, pfn);
		else if (vma->vm_flags & VM_EXEC)
			__flush_icache_all();
	}
}

/*
 * Check whether the write buffer has physical address aliasing
 * issues.  If it has, we need to avoid them for the case where
 * we have several shared mappings of the same object in user
 * space.
 */
static int __init check_writebuffer(unsigned long *p1, unsigned long *p2)
{
	register unsigned long zero = 0, one = 1, val;

	local_irq_disable();
	mb();
	*p1 = one;
	mb();
	*p2 = zero;
	mb();
	val = *p1;
	mb();
	local_irq_enable();
	return val != zero;
}

void __init check_writebuffer_bugs(void)
{
	struct page *page;
	const char *reason;
	unsigned long v = 1;

	printk(KERN_INFO "CPU: Testing write buffer coherency: ");

	page = alloc_page(GFP_KERNEL);
	if (page) {
		unsigned long *p1, *p2;
		pgprot_t prot = __pgprot(L_PTE_PRESENT|L_PTE_YOUNG|
					 L_PTE_DIRTY|L_PTE_WRITE|
					 L_PTE_MT_BUFFERABLE);

		p1 = vmap(&page, 1, VM_IOREMAP, prot);
		p2 = vmap(&page, 1, VM_IOREMAP, prot);

		if (p1 && p2) {
			v = check_writebuffer(p1, p2);
			reason = "enabling work-around";
		} else {
			reason = "unable to map memory\n";
		}

		vunmap(p1);
		vunmap(p2);
		put_page(page);
	} else {
		reason = "unable to grab page\n";
	}

	if (v) {
		printk("failed, %s\n", reason);
		shared_pte_mask = L_PTE_MT_UNCACHED;
	} else {
		printk("ok\n");
	}
}
