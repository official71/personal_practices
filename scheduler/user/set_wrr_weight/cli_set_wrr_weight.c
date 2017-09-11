#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SC_SET_WRR_WEIGHT 245

int main(int argc, char **argv)
{
	int weight;
	char *tmp, *arg1;

	if (argc != 2) {
		printf("Invalid arguments\n");
		return 0;
	}

	arg1 = *(argv+1);
	weight = strtol(arg1, &tmp, 10);

	if (syscall(SC_SET_WRR_WEIGHT, weight) < 0) {
		fprintf(stderr, "Error syscall %d with weight %d\n",
			SC_SET_WRR_WEIGHT, weight);
		return 0;
	}

	printf("Successfully set WRR weight %d\n", weight);

	return 0;
}
