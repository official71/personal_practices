#include <linux/kernel_stat.h>
#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <linux/mman.h>
#include <linux/swap.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/ksm.h>
#include <linux/rmap.h>
#include <linux/export.h>
#include <linux/delayacct.h>
#include <linux/init.h>
#include <linux/writeback.h>
#include <linux/memcontrol.h>
#include <linux/mmu_notifier.h>
#include <linux/kallsyms.h>
#include <linux/swapops.h>
#include <linux/elf.h>
#include <linux/gfp.h>
#include <linux/migrate.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/rcupdate.h>

#include <linux/io.h>
#include <asm/pgalloc.h>
#include <linux/uaccess.h>
#include <asm/tlb.h>
#include <asm/tlbflush.h>
#include <asm/pgtable.h>

#include "internal.h"

/* Investigate the page table layout
 *
 * The Page table layout varies over different architectures(eg. x86 vs arm).
 * It also changes with the change of system configuration.
 * For example, setting Transparent_hugepage extends the pagesize from 4kb to
 * 64kb.
 * As a result, indexing with Virtual Address in page table waking
 * differs from the case with 4k page size.
 *
 * You need to implement a system call
 * to get the page table layout information of current system.
 * @pgtbl_info : user address to store the related infomation
 * @size : the memory size reserved for pgtbl_info
 */
struct pagetable_layout_info {
	uint32_t pgdir_shift;
	uint32_t pmd_shift;
	uint32_t page_shift;
};

int __get_pagetable_layout(struct pagetable_layout_info __user *pgtbl_info,
				int size)
{
	struct pagetable_layout_info info;

	if (!pgtbl_info)
		return -EINVAL;

	if (size < sizeof(info))
		return -EINVAL;

	info.pgdir_shift = (uint32_t)PGDIR_SHIFT;
	info.pmd_shift = (uint32_t)PMD_SHIFT;
	info.page_shift = (uint32_t)PAGE_SHIFT;

	if (copy_to_user(pgtbl_info, &info, sizeof(struct
						   pagetable_layout_info)))
		return -EFAULT;

	return 0;
}


int copy_pte_to_user(struct mm_struct *src_mm, pte_t *src_pte,
			unsigned long addr, void *begin_vaddr)
{
	unsigned long user_mapped_addr;
	unsigned long phys_addr;
	struct vm_area_struct *vma;

	user_mapped_addr = (addr >> PAGE_SHIFT) / PTRS_PER_PTE * PAGE_SIZE;
	user_mapped_addr += ((unsigned long)begin_vaddr);

	vma = find_vma(src_mm, user_mapped_addr);
	phys_addr = virt_to_phys(src_pte) >> PAGE_SHIFT;

	if (remap_pfn_range(vma, user_mapped_addr, phys_addr, PAGE_SIZE,
							vma->vm_page_prot)){
		pte_unmap(src_pte);
		return -EAGAIN;
	}

	pte_unmap(src_pte);

	return 0;
}


static int copy_pgd_to_user(unsigned long addr, void *src_pte, void *src_pgd)
{
	unsigned long user_mapped_addr;
	unsigned long write_to_addr;
	int retval;

	user_mapped_addr = (addr >> PAGE_SHIFT) / PTRS_PER_PTE * PAGE_SIZE;
	user_mapped_addr += ((unsigned long)src_pte);

	write_to_addr = (addr >> PAGE_SHIFT) / PTRS_PER_PTE * 4;
	write_to_addr += ((unsigned long)src_pgd);

	retval = copy_to_user((void *)write_to_addr, &user_mapped_addr,
			      sizeof(unsigned long));

	if (retval < 0)
		return retval;

	return 0;
}


static inline int copy_pmd_to_user(struct mm_struct *dst_mm,
				struct mm_struct *src_mm,
				pud_t *dst_pud, pud_t *src_pud,
				struct vm_area_struct *vma,
				unsigned long addr, unsigned long end)
{
	pmd_t *src_pmd, *dst_pmd;
	unsigned long next;

	dst_pmd = pmd_alloc(dst_mm, dst_pud, addr);
	if (!dst_pmd)
		return -ENOMEM;
	src_pmd = pmd_offset(src_pud, addr);
	do {
		next = pmd_addr_end(addr, end);
		if (pmd_trans_huge(*src_pmd)) {
			int err;

			VM_BUG_ON(next-addr != HPAGE_PMD_SIZE);
			err = copy_huge_pmd(dst_mm, src_mm,
					dst_pmd, src_pmd, addr, vma);
			if (err == -ENOMEM)
				return -ENOMEM;
			if (!err)
				continue;
			/* fall through */
		}
		if (pmd_none_or_clear_bad(src_pmd))
			continue;
	} while (dst_pmd++, src_pmd++, addr = next, addr != end);
	return 0;
}


static struct vm_area_struct *verify_user_mem(struct mm_struct *mm,
					unsigned long addr, unsigned long size)
{
	struct vm_area_struct *vma;

	vma = find_vma(mm, addr);
	if (vma == NULL || (vma->vm_end - addr) < size)
		return NULL;

	return vma;

}

static int copy_ptdata_to_user(struct mm_struct *src_mm,
			struct vm_area_struct *this_vma,
			struct vm_area_struct *user_vma,
			void *user_addr, void *user_pgd)
{
	int retval = 0;
	struct page *page;
	unsigned long next, fst_addr, lst_addr;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	fst_addr = this_vma->vm_start;
	lst_addr = this_vma->vm_end;
	pgd = pgd_offset(src_mm, fst_addr);


	do {
		next = pgd_addr_end(fst_addr, lst_addr);
		if (pgd_none_or_clear_bad(pgd))
			continue;

		pud = pud_offset(pgd, fst_addr);
		if (pud_none_or_clear_bad(pud))
			continue;

		pmd = pmd_offset(pud, fst_addr);
		if (pmd_none(*pmd))
			continue;

		pte = pte_offset_map(pmd, fst_addr);

		retval = copy_pte_to_user(src_mm, pte, fst_addr,
					  (void *)lst_addr);
		if (retval < 0)
			return retval;

		retval = copy_pgd_to_user(fst_addr, user_addr, user_pgd);
		if (retval < 0)
			return retval;

		page = vm_normal_page(this_vma, fst_addr, *pte);
		if (!page)
			return retval;

	} while (pgd++, fst_addr = next, fst_addr != lst_addr);

	return retval;
}
/*
 * Map a target process's Page Table into the current process's address space.
 */

int __expose_page_table(pid_t pid,
		unsigned long fake_pgd,
		unsigned long fake_pmds,
		unsigned long page_table_addr,
		unsigned long begin_vaddr,
		unsigned long end_vaddr)
{
	int retval = 0;
	struct task_struct *p;
	struct mm_struct *mm, *mm_curr;
	struct vm_area_struct *user_vma, *user_pgd, *this_vma;
	unsigned long addr;
	unsigned long tbl_size = (4*4096*4096);
	unsigned long pgd_size = (4*4096);

	if ((unsigned long *)fake_pgd == NULL ||
	    (unsigned long *)fake_pmds == NULL ||
	    (unsigned long *)page_table_addr == NULL)
		return -EINVAL;

	if (pid == -1)
		pid = current->pid;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	rcu_read_unlock();
	if (!p)
		return -ESRCH;

	mm = get_task_mm(p);
	if (!mm)
		return -EINVAL;

	mm_curr = get_task_mm(current);
	if (!mm_curr)
		return -EINVAL;

	user_vma = verify_user_mem(mm, fake_pgd, tbl_size);
	if (!user_vma)
		return -EINVAL;

	user_vma->vm_flags = user_vma->vm_flags & VM_SHARED;

	user_pgd = verify_user_mem(mm_curr, fake_pgd, pgd_size);
	if (!user_pgd)
		return -EINVAL;

	user_pgd->vm_flags = user_pgd->vm_flags & VM_SHARED;
	addr = begin_vaddr;
	this_vma = mm->mmap;

	do {
		retval = copy_ptdata_to_user(mm, this_vma, user_vma,
					     (void *)addr, (void *)fake_pgd);
		if (retval < 0)
			return retval;
		this_vma = this_vma->vm_next;
	} while (this_vma);

	return 0;
}
