/*
 * binder_info.c
 *
 * Start/stop recording binder information, or
 * print information of binder peer and statistics
 * 
 * Usage:
 * binder_info <start|stop|print> pid
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define __NR_GET_PAGETABLE_LAYOUT 244

struct pagetable_layout_info {
      uint32_t pgdir_shift;
      uint32_t pmd_shift;
      uint32_t page_shift;
};

int main(int argc, char **argv)
{
	int ret;
	struct pagetable_layout_info info;

	ret = syscall(__NR_GET_PAGETABLE_LAYOUT, &info, sizeof(info));
	if (0 > ret)
		fprintf(stderr, "Error: %s\n", strerror(errno));

	return 0;
}
