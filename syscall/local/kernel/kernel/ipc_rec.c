/*
 *  Copyright (C) 2016 Columbia University
 *
 *  Author: W4118 Staff <w4118@lists.cs.columbia.edu>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, version 2 of the
 *  License.
 *
 *  OS w4118 fall 2016 IPC stats recording functionality.
 */
#include <linux/ipc_rec.h>
#include <linux/slab.h>
#include <linux/rcupdate.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/stddef.h>
#include <linux/binder_custom.h>
#include <linux/sched.h>
#include <linux/spinlock_types.h>


DEFINE_MUTEX(ipc_rec_lock);
DEFINE_SPINLOCK(ip_rec_spinlock);

void binder_trans_notify(int from_proc, int to_proc, int data_size)
{
  /* The following steps for each are:
   * ---------------------------------
   * 1) Iterate through the global list to see if the PID is there
   * 2) If PID is there, then grab the binder_stats associated with it
   *    and update it
   * 3) check to see if other process is in peer list. if not, add it
   *
   */

  struct list_head *pos;
  struct binder_peer_list *tmp;
  struct task_struct *t;
  char commtmp[16];
  bool found;
  struct bp_pid_list *entry;
  pid_t to_proc2 = to_proc;
  pid_t from_proc2 = from_proc;


  /* do everything for the from_proc) */
  entry = find_bp_pid_list((pid_t)from_proc);
  if (entry != NULL) {  
    entry->stats.nr_trans++;
    entry->stats.bytes += data_size;


    /* now check the peer list to see if the othe peer is already there */
    t = find_task_by_vpid((pid_t)to_proc);
    get_task_comm(commtmp, t);
    found  = false;
    spin_lock(&ip_rec_spinlock);
    list_for_each(pos, &entry->peer_list) {
      tmp = list_entry(pos, struct binder_peer_list, list);
      if (tmp->peer.pid == to_proc2) {
	found = true;
	break;
      }
    }
    spin_unlock(&ip_rec_spinlock);
    if (found == false) {
      spin_lock(&ip_rec_spinlock);
      tmp = (struct binder_peer_list*)kmalloc(sizeof(struct binder_peer_list*), GFP_KERNEL);
      if (!tmp) {
	printk(KERN_ERR "ERROR: not enough memory to add new peer %d for pid %d. %d\n", to_proc, from_proc, -ENOMEM);
	return;
      }

      strncpy(tmp->peer.comm, commtmp, 16);
      tmp->peer.pid = (pid_t)to_proc;
      tmp->peer.uid = t->cred->uid;

      list_add(&(tmp->list),&entry->peer_list);
      spin_unlock(&ip_rec_spinlock);
    }
  }
  


  /* Do everything for the to_proc*/
  entry = find_bp_pid_list((pid_t)to_proc);
  if (entry != NULL) {
    entry->stats.nr_trans++;
    entry->stats.bytes += data_size;


    /* now check the peer list to see if the othe peer is already there */
    t = find_task_by_vpid((pid_t)from_proc);
    get_task_comm(commtmp, t);
    found  = false;
    spin_lock(&ip_rec_spinlock);
    list_for_each(pos, &entry->peer_list) {
      tmp = list_entry(pos, struct binder_peer_list, list);
      if (tmp->peer.pid == from_proc2) {
	found = true;
	break;
      }
    }
    spin_unlock(&ip_rec_spinlock);
    if (found == false) {
      spin_lock(&ip_rec_spinlock);
      tmp = (struct binder_peer_list*)kmalloc(sizeof(struct binder_peer_list*), GFP_KERNEL);
      if (!tmp) {
	printk(KERN_ERR "ERROR: not enough memory to add new peer %d for pid %d. %d\n", to_proc, from_proc, -ENOMEM);
	return;
      }

      strncpy(tmp->peer.comm, commtmp, 16);
      tmp->peer.pid = (pid_t)from_proc;
      tmp->peer.uid = t->cred->uid;

      list_add(&(tmp->list),&entry->peer_list);
      spin_unlock(&ip_rec_spinlock);
    }
  }
  

  return;

  
  
  
}
