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
#include "light.h"


int main(int argc, char **argv)
{
	int mode = 0;
	char* tmp;
	int p, q, ret, nr;
	struct event_requirements req;

	char* arg1 = *(argv+1);
	char* arg2 = *(argv+2);
	char* arg3;

	if (argc < 3 || argc > 4) {
		printf("Invalid arguments\n");
		return 0;
	}

	if (!strcmp(arg1, "c")) {
		mode = 1;
	} else if (!strcmp(arg1, "w")) {
		mode = 2;
	} else if (!strcmp(arg1, "d")) {
		mode = 3;
	}

	if (!mode) {
		printf("Invalid arg %s\n", arg1);
		return 0;
	}

	p = (int)strtol(arg2, &tmp, 10);
	if (p <= 0) {
		printf("Invalid p %d\n", p);
		return 0;
	}

	if (mode == 1) {
		if (argc != 4) {
			printf("Invalid arguments\n");
			return 0;
		}

		arg3 = *(argv+3);
		q = (int)strtol(arg3, &tmp, 10);
		if (q <= 0) {
			printf("Invalid q %d\n", q);
			return 0;
		}
	}


	switch (mode) {
	case 1: /* create */
		nr = __NR_light_evt_create;
		req.req_intensity = p;
		req.frequency = q;
		ret = syscall(nr, &req);
		printf("syscall %d returns %d\n", nr, ret);
		if (0 > ret) {
			fprintf(stderr, "Error processing %d: %s\n", 
				nr, strerror(errno));
		}
		break;
	case 2: /* wait */
		nr = __NR_light_evt_wait;
		ret = syscall(nr, p);
		printf("syscall %d returns %d\n", nr, ret);
		if (0 > ret) {
			fprintf(stderr, "Error processing %d: %s\n", 
				nr, strerror(errno));
		}
		break;
	case 3: /* destroy */
		nr = __NR_light_evt_destroy;
		ret = syscall(nr, p);
		printf("syscall %d returns %d\n", nr, ret);
		if (0 > ret) {
			fprintf(stderr, "Error processing %d: %s\n", 
				nr, strerror(errno));
		}
		break;
	}

	return 0;
}
