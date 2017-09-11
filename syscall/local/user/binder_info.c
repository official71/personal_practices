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
#include "binder_info.h"

#define BINDER_REC 244 /* binder_rec syscall number */
#define BINDER_STATS 245 /* binder_stats syscall number */


int main(int argc, char **argv)
{
	struct binder_stats stats;
	struct binder_peer *peer;
	int s = sizeof(struct binder_peer);
	char buf[4096];
	size_t size;
	int mode = 0;
	char* tmp;
	pid_t pid;
	int i;

	char* arg1 = *(argv+1);
	char* arg2 = *(argv+2);

	if (argc != 3) {
		printf("Invalid arguments\n");
		return 0;
	}

	if (!strcmp(arg1, "start")) {
		mode = 1;
	} else if (!strcmp(arg1, "stop")) {
		mode = 2;
	} else if (!strcmp(arg1, "print")) {
		mode = 3;
	}

	if (!mode) {
		printf("Invalid arg %s\n", arg1);
		return 0;
	}

	pid = (pid_t)strtol(arg2, &tmp, 10);
	if (pid <= 0) {
		printf("Invalid pid %d\n", pid);
		return 0;
	}

	switch (mode) {
	case 1: /* start */
		if (0 > syscall(BINDER_REC, pid, 1)) {
			fprintf(stderr, "Error processing pid %u: %s\n", 
				pid, strerror(errno));
		}
		break;
	case 2: /* stop */
		if (0 > syscall(BINDER_REC, pid, 0)) {
			fprintf(stderr, "Error processing pid %u: %s\n", 
				pid, strerror(errno));
		}
		break;
	case 3: /* print */
		memset(buf, 0, sizeof(buf));
		memset(&stats, 0, sizeof(stats));
		size = 4096;

		if (0 > syscall(BINDER_STATS, pid, &stats, (void*)buf, &size)) {
			fprintf(stderr, "Error processing pid %u: %s\n", 
				pid, strerror(errno));
		} else {
			printf("%s (%u):\t%u bytes\t%u transactions\n",
			       stats.comm, pid, stats.bytes, stats.nr_trans);

			for (peer = (struct binder_peer*)buf, i = 0; 
			    i + s <= size;
			    peer++, i += s) {
				printf("\t\t%s\t%u\t%u\n", peer->comm,
				       peer->pid, peer->uid);
			}
		}
		break;
	}

	return 0;
}
