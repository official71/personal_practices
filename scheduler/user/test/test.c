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
	int cpu, old_cpu;
	unsigned int i, j, k, l, v;
	pid_t pid = getpid();

	/* initially assigned cpu */
	ret = syscall(SC_GET_CPU, &old_cpu, NULL, NULL);
	if (ret < 0)
		printf("error getcpu for pid %u, %s\n",
		       pid, strerror(errno));
	printf("[%u] cpu %d\n", pid, old_cpu);

	/* setscheduler to WRR scheduler */
	param.sched_priority = 0;
	ret = sched_setscheduler(pid, SCHED_WRR, &param);
	printf("[%u] set sched to WRR returns %d\n", pid, ret);
	if (ret < 0)
		printf("error: %s\n", strerror(errno));

	/* cpu after setsched */
	ret = syscall(SC_GET_CPU, &old_cpu, NULL, NULL);
	if (ret < 0)
		printf("error getcpu for pid %u, %s\n",
		       pid, strerror(errno));
	printf("[%u] cpu after setsched WRR: %d\n", pid, old_cpu);

	/* iterate */
	for (i = 0; i < MAX_ITER; i++) {
		/* check cpu */
		ret = syscall(SC_GET_CPU, &cpu, NULL, NULL);
		if (ret < 0)
			printf("error getcpu for pid %u, %s\n",
				pid, strerror(errno));
		if (cpu != old_cpu) {
			printf("\n[%u] * cpu switch: %d ---> %d\n",
				pid, old_cpu, cpu);
			old_cpu = cpu;
		}

		for (j = 0; j < 1; j++) {
			for (k = 0; k < MAX_ITER; k++) {
				v = 0;
				for (l = 0; l < MAX_ITER; l++)
					v++;
			}
		}
	}
}

static int wrr_loop_fork(void)
{
	int pid = fork();

	if (pid < 0) {
		printf("Error fork: %s\n", strerror(errno));
		return pid;
	}

	if (pid == 0) {
		/* run wrr looping */
		wrr_loop();
		exit(0);
	}

	return pid;
}

int main(int argc, char **argv)
{
	int p, pid, nr_forks;
	char *arg1, *tmp;

	if (argc == 1)
		nr_forks = NR_FORKS;
	else if (argc == 2) {
		arg1 = *(argv+1);
		nr_forks = strtol(arg1, &tmp, 10);
	} else {
		printf("Invalid input arguments\n");
		exit(1);
	}

	/* validate number of child tasks */
	if (nr_forks <= 0 || nr_forks > NR_FORKS_MAX) {
		printf("Invalid number of child tasks: %d\n", nr_forks);
		exit(1);
	}

	for (p = 0; p < nr_forks; p++) {
		pid = wrr_loop_fork();
		if (pid <= 0)
			break;

		printf("Start child task %u\n", (pid_t)pid);
	}

	return 0;
}
