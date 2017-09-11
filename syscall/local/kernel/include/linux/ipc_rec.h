#ifndef _LINUX_IPC_REC_H
#define _LINUX_IPC_REC_H

#include <linux/mutex.h>

extern struct mutex ipc_rec_lock;

/*
 * All sorts of IPC subsystems could have their hooks here, which would be
 * called on every instance of every IPC mechanism.
 *
 * For the Fall 2016 w4118 course we only focus on Binder IPC.
 */

void binder_trans_notify(int from_proc, int to_proc, int data_size);


#endif /* _LINUX_IPC_REC_H */
