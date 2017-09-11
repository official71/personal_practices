/*
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define __NR_GET_PAGETABLE_LAYOUT 244
#define __NR_EXPOSE_PAGE_TABLE 245

#define PTRS_PER_PGD 512
#define PTRS_PER_PMD 512
#define PTRS_PER_PTE 512

struct pagetable_layout_info {
      uint32_t pgdir_shift;
      uint32_t pmd_shift;
      uint32_t page_shift;
};

#define pgd_index(addr) (((addr) >> pgdir_shift) & (PTRS_PER_PGD - 1))
#define pgd_offset(pgd, addr) ((pgd) + pgd_index(addr))

#define pmd_index(addr) (((addr) >> pmd_shift) & (PTRS_PER_PMD - 1))
#define pte_index(addr) (((addr) >> page_shift) & (PTRS_PER_PTE - 1))

static void print_layout_info(struct pagetable_layout_info *info)
{
	printf("pgdir shift: %d\n", info->pgdir_shift);
	printf("pmd shift: %d\n", info->pmd_shift);
	printf("page shift: %d\n", info->page_shift);
}

int main(int argc, char **argv)
{
	int ret, verbose, i, pid;
	struct pagetable_layout_info info;
	uint32_t pgdir_shift, pmd_shift, page_shift;
	char *arg, *tmp;
	unsigned long va_begin, va_end;
	unsigned long pgd_offset, pmd_offset, pte_offset;
	unsigned long pgd_index;
	unsigned long *fake_pgd, *fake_pmds, *page_table_addr;
	unsigned long pgd_mmsize, pmd_mmsize, pte_mmsize, page_size;

	/*
	 * parse input arguments
	 */
	verbose = 0;
	i = 1;
	if (argc == 5) {
		arg = *(argv+1);
		if (strcmp(arg, "-v")) {
			printf("Invalid argument: %s\n", arg);
			return -EINVAL;
		}
		verbose = 1;
		i++;
	} else if (argc != 4) {
		printf("Invalid arguments\n");
		return -EINVAL;
	}

	pid = (int)strtol(*(argv+i), &tmp, 10);
	if (pid <= 0 && pid != -1) {
		printf("Invalid pid %d\n", pid);
		return -EINVAL;
	}

	i++;
	va_begin = (unsigned long)strtol(*(argv+i), &tmp, 16);
	i++;
	va_end = (unsigned long)strtol(*(argv+i), &tmp, 16);

	printf("vm_inspector arguments: verbose %d, pid %u, va_begin %lx, va_end %lx\n",
	       verbose, pid, va_begin, va_end);

	/*
	 * get pagetable layout
	 */
	ret = syscall(__NR_GET_PAGETABLE_LAYOUT, &info, sizeof(info));
	if (0 > ret) {
		fprintf(stderr, "Error syscall %d: %s\n",
		       __NR_GET_PAGETABLE_LAYOUT, strerror(errno));
		return ret;
	}

	print_layout_info(&info);
	pgdir_shift = info.pgdir_shift;
	pmd_shift = info.pmd_shift;
	page_shift = info.page_shift;

	/*
	 * memory mapping
	 */
	pgd_mmsize = 512 * sizeof(unsigned long);
	fake_pgd = (unsigned long *)malloc(pgd_mmsize);
	if (fake_pgd)
		printf("Failed to malloc for fake pgd\n");
		return -1;
	}

	pmd_mmsize = 128000 * sizeof(char);
	fake_pgd = (unsigned long *)malloc(pmd_mmsize);
	if (fake_pmd) {
		printf("Failed to malloc for fake pmd\n");
		ret = -1;
		goto free_pgd;
	}

	page_size = (1UL << page_shift);
	pte_mmsize = pmd_mmsize * page_size / sizeof(unsigned long);
	page_table_addr = mmap(NULL, pte_mmsize, PROT_READ,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (page_table_addr < 0) {
		printf("Error mmap for page tables: %s\n", strerror(errno));
		ret = -1;
		goto free_pmd;
	}
	printf("memory mapped for fake pgd: %ld, pmd: %ld, pte: %ld\n",
	       pgd_mmsize, pmd_mmsize, pte_mmsize);
/*	pgd_index = (p >> pgdir_shift) & (PTRS_PER_PGD - 1);
	pgd_offset = pgd + pgd_index;
	printf("pgd index: %lx, pgd offset: %lx\n", pgd_index, pgd_offset);
*/
#if 0
	/* call syscall 245 to expose page table */
	ret = syscall(__NR_EXPOSE_PAGE_TABLE, pid, fake_pgd, fake_pmds,
		      page_table_addr, va_begin, va_end);
	if (ret < 0) {
		fprintf(stderr, "Error syscall %d: %s\n",
		       __NR_EXPOSE_PAGE_TABLE, strerror(errno));
		goto unmap_all;
	}

	/* print mapped page table */
	unsigned long va, *fake_pgd_ite, *fake_pmd_ite, *fake_pte;

	va = va_begin & 0xfffff000;
	for (; va < va_end; va += page_size) {
		fake_pgd_ite = pgd_offset(fake_pgd, va);
		if (!(*fake_pgd_ite)) {
			if (verbose) {
				/* print 0 */
			}
			continue;
		}

		fake_pmd_ite = *fake_pgd_ite + pmd_index(va);
		if (!(*fake_pmd_ite)) {
			if (verbose) {
				/* print 0 */
			}
			continue;
		}

		fake_pte = *fake_pmd_ite + pte_index(va);
	}

#endif
unmap_all:
	munmap((void*)page_table_addr, pte_mmsize);
free_pmd:
	free(fake_pmd);
free_pgd:
	free(fake_pgd);
	
	return ret;
}
