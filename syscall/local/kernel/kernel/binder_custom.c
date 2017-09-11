/*
 * binder_custom.c
 *
 * Implementation of syscalls:
 * - binder_rec
 * - binder_stats
 *
 */
#include <linux/unistd.h>
#include <linux/stddef.h>
#include <linux/sched.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/spinlock_types.h>
#include <linux/binder_custom.h>
#include <linux/syscalls.h>


DEFINE_SPINLOCK(spinlock);
LIST_HEAD(global_binder_pid_list);

/*
 * Find the entry for recording the binder stastics and peers
 * for input pid in the global binder list
 */
struct bp_pid_list* find_bp_pid_list(pid_t pid)
{
	struct list_head *pos;
	struct bp_pid_list *tmp;
	struct bp_pid_list *found = NULL;
	
	spin_lock(&spinlock);

	/* initialize list at first access */
	if (!global_binder_pid_list.next) 
		INIT_LIST_HEAD(&global_binder_pid_list);
	
	list_for_each (pos, &global_binder_pid_list) {
		tmp = list_entry(pos, struct bp_pid_list, list);
		if (tmp && tmp->pid == pid) {
			found = tmp;
			break;
		}
	}

	spin_unlock(&spinlock);
	return found;
}

/*
 * Add entry for new recorded pid in the global binder list
 */
void add_bp_pid_list(struct bp_pid_list *new)
{
	spin_lock(&spinlock);
	list_add(&(new->list), &global_binder_pid_list);
	spin_unlock(&spinlock);

	return;
}

/*
 * Delete entry from global biner list
 */
void del_bp_pid_list(struct bp_pid_list *del)
{
	struct list_head *pos,*n;
	struct binder_peer_list *entry;

	spin_lock(&spinlock);
	list_for_each_safe(pos, n, &del->peer_list) {
		list_del(pos);
		entry = list_entry(pos, struct binder_peer_list, list);
		kfree(entry);
	}
	list_del(&del->list);
	kfree(del);
	spin_unlock(&spinlock);

	return;
}

/*
 * Syscall: sys_binder_rec(pid_t pid, int state)
 * Start/stop recording binder info for pid
 * Start when state is 1, stop when state is 0
 */
SYSCALL_DEFINE2(binder_rec, pid_t, pid, int, state)
{
	int retval = 0;
	struct task_struct *p;
	struct bp_pid_list *bp_pid;

	if (state != 0 && state != 1) {
		retval = -EINVAL;
		goto out;
	}

	if (!pid) {
		retval = -EINVAL;
		goto out;
	}

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	rcu_read_unlock();	
	if (!p) {
		retval = -ESRCH;
		goto out;
	}

	bp_pid = find_bp_pid_list(pid);
	if (!bp_pid) {
		if (!state){
			retval = -EINVAL;
			goto out;
		}
			

		//add entry for pid
		bp_pid = (struct bp_pid_list*)kmalloc(sizeof(struct bp_pid_list),
						      GFP_KERNEL);
		if (!bp_pid) {
			retval = -ENOMEM;
			goto out;
		}

		bp_pid->pid = pid;

		get_task_comm(bp_pid->stats.comm, p);
		bp_pid->stats.nr_trans = 0;
		bp_pid->stats.bytes = 0;
		INIT_LIST_HEAD(&bp_pid->peer_list);

		add_bp_pid_list(bp_pid);
	} else {
		if (state)
			goto out;

		//delete entry for pid
		del_bp_pid_list(bp_pid);
	}

out:

	return retval;
}

/*
 * Syscall: sys_binder_stats(pid_t pid, struct binder_stats *stats,
 *                           void *buf, size_t *size)
 * Output the binde statistics of pid into stats
 *        the info of binder peers into buf with size bytes written
 * Return the number of peers recorded with transactions with pid
 */
SYSCALL_DEFINE4(binder_stats, pid_t, pid, struct binder_stats *, stats,
		void *, buf, size_t *, size)
{
	int retval = 0;
        struct task_struct *p;
        struct bp_pid_list *bp_pid;
	struct list_head *pos;
	struct binder_peer_list *tmp;
	struct binder_peer *cur_peer;
	size_t cur_size;

        if (!pid || !stats || !buf || !size) {
                retval = -EINVAL;
                goto out;
	}

	if (*size < sizeof(struct binder_peer)) {
		retval = -EINVAL;
		goto out;
	}
				        
	
	bp_pid = find_bp_pid_list(pid);
	if (!bp_pid) { /* no entry for this pid */
		*size = 0;
		retval = -EINVAL;
		goto out;
	}

	rcu_read_lock(); 
	p = find_task_by_vpid(pid); 
	rcu_read_unlock(); 
	if (!p) { 
		retval = -ESRCH; 
		goto out;
	}
	
								        


	spin_lock(&spinlock);
	if (copy_to_user(stats, &bp_pid->stats, sizeof(struct binder_stats))) {
		retval = -EFAULT;
		goto unlock_out;
	}

	cur_peer = (struct binder_peer*)buf;
	cur_size = 0;
	list_for_each (pos, &bp_pid->peer_list) {
		tmp = list_entry(pos, struct binder_peer_list, list);
		if (!tmp) /* should not happen, null peer in peer_list */ {
			retval = -EFAULT;
			goto unlock_out;
		}

		retval++; /* increase the number of peers */

		/* try copy binder_peer info to user space */
		if ((void*)cur_peer + sizeof(struct binder_peer) > buf + *size)
			continue;
			
		if (copy_to_user(cur_peer, &tmp->peer, 
				 sizeof(struct binder_peer))) {
			retval = -EFAULT;
			goto unlock_out;
		}
		cur_peer++;
		cur_size += sizeof(struct binder_peer);
	}

	*size = cur_size;
unlock_out: spin_unlock(&spinlock);
out:
	return retval;
}
