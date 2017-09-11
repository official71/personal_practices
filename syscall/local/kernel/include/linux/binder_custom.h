#ifndef BINDER_CUSTOM_H
#define BINDER_CUSTOM_H
/*
 *
 * We are creating the following format to store the pid, and
 * their corresponding peers lists
 *
 * --------------------------------------------------
 * |     global list of PIDs and their peer list     |
 * |            (instance if bp_pid_list)            |
 * --------------------------------------------------
 * | PID 1 | Peer list (instance of binder_peer_list |
 * --------------------------------------------------
 * | PID 2 | Peer List (instance of binder_peer_list |
 * ---------------------------------------------------
 * | PID 3 | Peer List                               |
 * ---------------------------------------------------
 * | PID 4 | Peer List                               |                                                                                                                                                                                     
 * --------------------------------------------------- 
 *  etc, etc
 * 
 * Each Peer List is going to contain a list of 
 * binder_peers
 * 
 * Each "PID X" contains both the pid and an instance of
 * binder_stats for it
 *
 */

#include <linux/types.h>
#include <linux/list.h>


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

struct binder_peer_list {
  struct binder_peer peer;
  struct list_head list;
};

struct bp_pid_list {
  pid_t pid;
  struct binder_stats stats;
  struct list_head peer_list;
  struct list_head list;
};


/* functions */
struct bp_pid_list* find_bp_pid_list(pid_t pid);
void add_bp_pid_list(struct bp_pid_list *new);
void del_bp_pid_list(struct bp_pid_list *del);


#endif
