#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SC_GET_WRR_INFO 244
#define SC_GET_CPU 168
#define MAX_CPUS 8 /* upper bound of the number of CPUs */
struct wrr_info {
	int num_cpus;
	int nr_running[MAX_CPUS];
	int total_weight[MAX_CPUS];
};

#define SLEEP_TIME 1000000 /* one second */

static void monitor_wrr_info(void)
{
	struct wrr_info info;
	int ret, i, err = 0;

	while (1) {
		ret = syscall(SC_GET_WRR_INFO, &info);

		if (ret < 0) {
			fprintf(stderr, "Error syscall %d: %s\n",
				SC_GET_WRR_INFO, strerror(errno));
			if (++err >= 100)
				break;

			goto next;
		}

		printf("\nNumber of CPUs: %d\n", ret);
		for (i = 0; i < ret; i++)
			printf("CPU %d: nr running %d, total weight %d\n",
			       i, info.nr_running[i], info.total_weight[i]);

next:
		if (usleep(SLEEP_TIME) < 0)
			break;
		if (usleep(SLEEP_TIME) < 0)
			break;
	}
}

int main(void)
{
	monitor_wrr_info();

	return 0;
}
