#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "light.h"

#define DELAY 10000

int make_child(int event, pid_t cpid) {
	int ret = 0;
	pid_t pid;
	if (cpid < 0) {
		printf("failed to make!");
		exit(-1);
	} else if (cpid > 0) { 
		pid = cpid;
		while(1) {
			cpid = wait(0);
			if (cpid < 0) {
				printf("error wait: %s\n", strerror(errno));
				exit(-1);
			}
			if (cpid == pid)
				break;
		}
	} else {
		while(1) {
		ret = syscall(__NR_light_evt_wait, event);
		break;
		}

	printf("%d detected a <val> intensity event\n", event);
	syscall(__NR_light_evt_destroy, event);	
	}
	return ret;
}

int kill_child(int event) {
	return syscall(__NR_light_evt_destroy, event);
}

int main (void)
{
        /* Write your test program here */
        struct event_requirements low = {50, 3};
        struct event_requirements med = {500, 5};
        struct event_requirements hig = {20000, 3};

	int ev_1 = syscall(__NR_light_evt_create, &low);
	if (ev_1 <= 0) {
		printf("error low_syscall: %s\n", strerror(errno));
	} 

	int ev_2 = syscall(__NR_light_evt_create, &med);
        if (ev_2 <= 0) {
                printf("error med_syscall: %s\n", strerror(errno));
        } 

	int ev_3 = syscall(__NR_light_evt_create, &hig);
        if (ev_3 <= 0) {
                printf("error hi_syscall: %s\n", strerror(errno));
        }

	int j;
	for (j = 0; j < 1; ++j) {
		pid_t pid = fork();
		if (pid == 0) {
		make_child(ev_1, pid);
		make_child(ev_2, pid);
		make_child(ev_3, pid);
		}
	else if (pid > 0) {
		sleep(DELAY);
		kill_child(ev_1);
		kill_child(ev_2);
		kill_child(ev_3);
		printf("\nTimed out!\n");
	}
	}
		
        return 0;
}
