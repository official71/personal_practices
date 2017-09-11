/*
 * binder_info.h
 *
 * Data structures of binder peer and statistics should follow
 * those defined in /include/linux/binder_custom.h
 *
 */

#include <linux/types.h>

struct binder_peer { 
	uid_t uid; 
	pid_t pid; 
	char comm[16]; 
};

struct binder_stats { 
	char comm[16]; 
	unsigned int nr_trans; 
	unsigned int bytes;
};

