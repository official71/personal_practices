/* file.c: sensorfs file implementation
 *
 * W4118 Homework 6
 */

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sensorfs.h>
#include <linux/uaccess.h>

#include "internal.h"

//TODO: Probably need a few function implemented here
//and to fill out the following structs appropriately.

static int sensorfs_file_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	if (!filp->private_data)
		return -ENOENT;

	return 0;
}

static int sensorfs_file_release(struct inode *inode, struct file *filp)
{
	return 1;
}

static ssize_t sensorfs_file_read(struct file *filp, char *buf,
				  size_t count, loff_t *offset)
{
	char *content = (char *)filp->private_data;
	int size;
	loff_t off = *offset;

	if (!content || !count)
		return 0;
	size = strlen(content);
	if (!size)
		return 0;
	if (off > size)
		return 0;
	if (off + count > size)
		count = size - off;

	//lock
	if (copy_to_user(buf, content + off, count))
		return -EFAULT;

	*offset = off + count;

	return count;
}

static int parent_inode_ino(struct sensorfs_dir_entry *sde)
{
	struct sensorfs_dir_entry *parent_sde;
	struct inode *parent_inode;

	parent_sde = sde->parent;
	if (!parent_sde)
		return 0;

	parent_inode = parent_sde->inode;
	if (!parent_inode)
		return 0;

	return parent_inode->i_ino;
}

static int sensorfs_dir_readdir_sde(struct sensorfs_dir_entry *sde,
				    struct file *filp, void *dirent,
				    filldir_t filldir)
{
	unsigned int ino;
	int i;
	struct inode *inode = file_inode(filp);
	int ret = 0;

	ino = inode->i_ino;
	i = filp->f_pos;
	switch (i) {
	case 0:
		if (filldir(dirent, ".", 1, i, ino, DT_DIR) < 0)
			goto out;
		i++;
		filp->f_pos++;
		/* fall through */
	case 1:
		if (filldir(dirent, "..", 2, i, parent_inode_ino(sde),
			    DT_DIR) < 0)
			goto out;
		i++;
		filp->f_pos++;
		/* fall through */
	default:
		//lock
		sde = sde->first_child;
		i -= 2;
		for (;;) {
			if (!sde) {
				ret = 1;
				//unlock
				goto out;
			}
			if (!i)
				break;
			sde = sde->next;
			i--;
		}

		do {
			struct sensorfs_dir_entry *next;

			atomic_inc(&sde->count);
			//unlock
			if (filldir(dirent, sde->name, sde->namelen,
				    filp->f_pos, sde->inode->i_ino,
				    sde->mode >> 12) < 0) {
				atomic_dec(&sde->count);
				goto out;
			}
			//lock
			filp->f_pos++;
			next = sde->next;
			atomic_dec(&sde->count);
			sde = next;
		} while (sde);
		//unlock
	}
	ret = 1;
out:
	return ret;
}

static int sensorfs_dir_readdir(struct file *filp, void *dirent,
				filldir_t filldir)
{
	struct inode *inode = file_inode(filp);
	struct sensorfs_inode *sinode = inode_to_sensorfs_inode(inode);

	return sensorfs_dir_readdir_sde(sinode->sde, filp, dirent, filldir);
}

const struct file_operations sensorfs_file_operations = {
	.read		= sensorfs_file_read,
	.write		= NULL,
	.open		= sensorfs_file_open,
	.release	= sensorfs_file_release,
};

const struct inode_operations sensorfs_file_inode_operations = {
	.create		= NULL,
};

const struct file_operations sensorfs_dir_operations = {
	.readdir	= sensorfs_dir_readdir,
};
