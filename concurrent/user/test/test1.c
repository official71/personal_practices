#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "light.h"


int main (void)
{
	/* Write your test program here */
	int ret;
	pid_t pid;
	struct event_requirements req;
	int event_id;

	ret = fork();
	if (ret < 0) {
		printf("error fork: %s\n", strerror(errno));
		return -1;
	}

	if (ret > 0) {
		pid = ret;
		while(1) {
			ret = wait(0);
			if (ret < 0) {
				printf("error wait: %s\n", strerror(errno));
				return -1;
			}
			if (ret == pid)
				break;
		}
		printf("child %u exits normally\n", pid);
	} else {
		req.req_intensity = 10000;
		req.frequency = 5;
		ret = syscall(__NR_light_evt_create, &req);

		printf("syscall %d returns %d\n", __NR_light_evt_create, ret);
		if (ret < 0) {
			printf("error: %s\n", strerror(errno));
			exit(1);
		}

		event_id = ret;
		ret = syscall(__NR_light_evt_wait, event_id);

		printf("syscall %d returns %d\n", __NR_light_evt_wait, ret);
		if (ret < 0) {
			printf("error: %s\n", strerror(errno));
			exit(1);
		}

		ret = syscall(__NR_light_evt_destroy, event_id);

		printf("syscall %d returns %d\n", __NR_light_evt_destroy, ret);
		if (ret < 0) {
			printf("error: %s\n", strerror(errno));
			exit(1);
		}

		exit(0);
	}

	return 0;
}
