#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "test.h"

#define NR_FORKS 3 /* number of child tasks */
#define NR_FORKS_MAX 100 /* some maximum number of child */
#define MAX_ITER 10000

/*
 * the loop can be at most 4 level
 * each level iterates from 1 to MAX_ITER
 */
static void wrr_loop(void)
{
	struct sched_param param;
	int ret;
	unsigned int i, j, k, l, v;
	pid_t pid = getpid();

	/* setscheduler to WRR scheduler */
	param.sched_priority = 0;
	ret = sched_setscheduler(pid, SCHED_WRR, &param);
	printf("[%u] set sched to WRR returns %d\n", pid, ret);
	if (ret < 0)
		printf("error: %s\n", strerror(errno));

	/* iterate */
	for (i = 0; i < MAX_ITER; i++) {
		printf("%d\n", i);
		for (j = 0; j < 1; j++) {
			for (k = 0; k < MAX_ITER; k++) {
				v = 0;
				for (l = 0; l < MAX_ITER; l++)
					v++;
			}
		}
	}
}

int main(void)
{
	wrr_loop();

	return 0;
}
