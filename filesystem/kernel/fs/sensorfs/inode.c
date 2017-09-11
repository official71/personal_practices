/* inode.c: sensorfs inode implementations
 *
 * W4118 Homework 6
 */

#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/backing-dev.h>
#include <linux/sensorfs.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/idr.h>
#include <linux/sensor_types.h>
#include <linux/uaccess.h>
#include "internal.h"

#define SENSORFS_DEFAULT_MODE	0444
#define SENSORFS_DYNAMIC_FIRST 0xF0000000U
#define MAX_LINE 128

static const struct super_operations sensorfs_ops;
static const struct dentry_operations sensorfs_dentry_operations;
static struct dentry *sensorfs_lookup(struct inode *dir, struct dentry *dentry,
	unsigned int flags);
static const struct inode_operations sensorfs_dir_inode_operations = {
	.create		= NULL,
	.lookup		= sensorfs_lookup,
};


static DEFINE_IDA(sensorfs_inum_ida);
static DEFINE_SPINLOCK(sensorfs_inum_lock);
DEFINE_SPINLOCK(sensorfs_biglock); //TODO: USE ME!
static struct kmem_cache *sensorfs_inode_cache;

static struct sensorfs_dir_entry sensorfs_root = {
	.namelen = 0,
	.name = NULL,
	//TODO: Will need to finish this up
	.size = 0,
	.mode = S_IFDIR | S_IRUGO,
	.count = ATOMIC_INIT(1),
	.sensorfs_iops = &sensorfs_dir_inode_operations,
	.sensorfs_fops = &sensorfs_dir_operations,
	.parent = &sensorfs_root,
};

int sensorfs_alloc_inum(unsigned int *inum)
{
	unsigned int i;
	int error;

retry:
	if (!ida_pre_get(&sensorfs_inum_ida, GFP_KERNEL))
		return -ENOMEM;

	spin_lock_irq(&sensorfs_inum_lock);
	error = ida_get_new(&sensorfs_inum_ida, &i);
	spin_unlock_irq(&sensorfs_inum_lock);
	if (error == -EAGAIN)
		goto retry;
	else if (error)
		return error;

	if (i > UINT_MAX - SENSORFS_DYNAMIC_FIRST) {
		spin_lock_irq(&sensorfs_inum_lock);
		ida_remove(&sensorfs_inum_ida, i);
		spin_unlock_irq(&sensorfs_inum_lock);
		return -ENOSPC;
	}
	*inum = SENSORFS_DYNAMIC_FIRST + i;
	return 0;
}

struct sensorfs_inode *inode_to_sensorfs_inode(struct inode *inode)
{
	return container_of(inode, struct sensorfs_inode, vfs_inode);
}

struct inode *sensorfs_get_inode(struct super_block *sb,
	const struct inode *dir,
	struct sensorfs_dir_entry *de)
{
	//TODO: Complete the implementation
	//You will also have to use this function somewhere.
	//This function should construct an inode representing
	//a given sensorfs_dir_entry (de) under a parent directory
	//Should also handle the situation when dir is NULL meaning
	//return the root directory entry's inode
	struct inode *inode = new_inode(sb);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode_init_owner(inode, dir, de->mode);
		inode_to_sensorfs_inode(inode)->sde = de;
		inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;

		inode->i_op = de->sensorfs_iops;
		inode->i_fop = de->sensorfs_fops;
		inode->i_private = (void *)de->contents;

		if (de->mode)
			inode->i_mode = de->mode;
		if (de->size)
			inode->i_size = de->size;
	}
	de->inode = inode;

	return inode;
}

static int sensorfs_delete_dentry(const struct dentry *dentry)
{
	return 1;
}

static struct sensorfs_dir_entry *sensorfs_sde_lookup(
	struct sensorfs_dir_entry *parent,
	const char *name, int len)
{
	//TODO: Implement
	//Given a parent, and a name of a file, returns the
	//sensorfs_dir_entry corresponding to that name
	struct sensorfs_dir_entry *sde, *found = NULL;

	spin_lock_irq(&sensorfs_biglock);

	for (sde = parent->first_child; sde; sde = sde->next) {
		if (sde->namelen != len)
			continue;
		if (!memcmp(sde->name, name, len)) {
			found = sde;
			break;
		}
	}

	spin_unlock_irq(&sensorfs_biglock);

	return found;
}

static void init_once(void *toinit)
{
	struct sensorfs_inode *sinode = (struct sensorfs_inode *)toinit;

	inode_init_once(&sinode->vfs_inode);
}

static void sensorfs_init_nodecache(void)
{
	sensorfs_inode_cache = kmem_cache_create("sensorfs_inode_cache",
		sizeof(struct sensorfs_inode),
		0, (SLAB_RECLAIM_ACCOUNT|
		SLAB_MEM_SPREAD|SLAB_PANIC), init_once);
}

static struct inode *sensorfs_alloc_inode(struct super_block *sb)
{
	struct sensorfs_inode *sinode =
		kmem_cache_alloc(sensorfs_inode_cache, GFP_KERNEL);
	if (!sinode)
		return NULL;

	sinode->sde = NULL;
	return &(sinode->vfs_inode);
}


static void sensorfs_destroy_inode(struct inode *inode)
{
	struct sensorfs_inode *sinode = inode_to_sensorfs_inode(inode);

	kmem_cache_free(sensorfs_inode_cache, sinode);
}

void sensorfs_create_sfile(struct sensorfs_dir_entry *parent, const char *name)
{
	//TODO: Implement
	struct sensorfs_dir_entry *sde = NULL;
	unsigned int len;
	char *str = NULL;

	if (!name)
		return;

	len = strlen(name);
	if (!len)
		return;

	/* trust the input name as this is a read-only fs */
	sde = kzalloc(sizeof(struct sensorfs_dir_entry), GFP_KERNEL);
	if (!sde)
		return;

	str = kzalloc(len, GFP_KERNEL);
	if (!str) {
		kfree(sde);
		return;
	}
	memcpy(str, name, len);

	sde->name = str;
	sde->namelen = len;
	sde->mode = S_IFREG | S_IRUGO;

	sde->parent = parent;
	sde->next = parent->first_child;

	sde->sensorfs_iops = &sensorfs_file_inode_operations;
	sde->sensorfs_fops = &sensorfs_file_operations;
	sde->sb = parent->sb;

	parent->first_child = sde;
}

static struct dentry *sensorfs_lookup(struct inode *dir, struct dentry *dentry,
	unsigned int flags)
{
	//TODO: Implement
	struct sensorfs_dir_entry *sde, *parent;
	struct inode *inode;
	const char *name;
	int len;

	parent = inode_to_sensorfs_inode(dir)->sde;
	if (!parent)
		return ERR_PTR(-ENOENT);

	name = dentry->d_name.name;
	len = dentry->d_name.len;
	if (!name || !len)
		return ERR_PTR(-ENOENT);

	sde = sensorfs_sde_lookup(parent, name, len);
	if (!sde)
		return ERR_PTR(-ENOENT);

	inode = sensorfs_get_inode(dir->i_sb, dir, sde);
	if (!inode)
		return ERR_PTR(-ENOMEM);
	d_set_d_op(dentry, &sensorfs_dentry_operations);
	d_add(dentry, inode);

	return NULL;
}

static loff_t sensorfs_update_sde(struct sensorfs_dir_entry *sde,
				  char *line, int len)
{
	char *temp = sde->contents;
	loff_t size = sde->size;
	int limit = sizeof(sde->contents);
	int i;

	spin_lock_irq(&sensorfs_biglock);
	if (size + len > limit) {
		i = 0;
		while (temp[i] != '\n') {
			i++;
			if (temp[i] == '\0')
				break;
		}
		memmove(&temp[i], line, len - i);
	} else
		memcpy(temp + size, line, len);
	size += len;

	spin_unlock_irq(&sensorfs_biglock);

	return size;
}

int sensorfs_update_info(struct sensor_information *si)
{
	struct sensorfs_dir_entry *sde, *next;
	char gpsbuf[MAX_LINE], lumibuf[MAX_LINE];
	char proxbuf[MAX_LINE], linaccelbuf[MAX_LINE];
	long curr_time = get_seconds();
	int gpslen, lumilen, proxlen, linalen;
	loff_t size;
	char *temp;
	int ret = 0;

	gpslen = sprintf(gpsbuf,
			 "TimeStamp:%ld,MicroLatitude:%d,MicroLongitude:%d\n",
			 curr_time, si->microlatitude, si->microlongitude);
	lumilen = sprintf(lumibuf,
			  "TimeStamp:%ld,Centilux:%d\n",
			  curr_time, si->centilux);
	proxlen = sprintf(proxbuf,
			  "TimeStamp:%ld,Centiproximity:%d\n",
			  curr_time, si->centiproximity);
	linalen = sprintf(linaccelbuf,
			  "TimeStamp:%ld,CentiAccelX:%d,CentiAccelY:%d,CentiAccelZ:%d\n",
			  curr_time, si->centilinearaccelx,
			  si->centilinearaccely, si->centilinearaccelz);

	//lock
	sde = sensorfs_root.first_child;
	while (sde) {
		next = sde->next;
		temp = sde->contents;
		size = sde->size;

		if (!memcmp(sde->name, SENSORFS_LINACCEL_FILENAME,
			    sde->namelen))
			size = sensorfs_update_sde(sde, linaccelbuf, linalen);
		else if (!memcmp(sde->name, SENSORFS_PROX_FILENAME,
				 sde->namelen))
			size = sensorfs_update_sde(sde, proxbuf, proxlen);
		else if (!memcmp(sde->name, SENSORFS_LUMI_FILENAME,
				 sde->namelen))
			size = sensorfs_update_sde(sde, lumibuf, lumilen);
		else if (!memcmp(sde->name, SENSORFS_GPS_FILENAME,
				 sde->namelen))
			size = sensorfs_update_sde(sde, gpsbuf, gpslen);

		if (size < 0) {
			ret = size;
			goto out;
		}

		sde->size = size;
		sde = next;
	}
out:
	//unlock
	return ret;
}

int sensorfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *inode;
	struct sensorfs_dir_entry *sde, *next;

	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_CACHE_SIZE;
	sb->s_blocksize_bits	= PAGE_CACHE_SHIFT;
	sb->s_magic		= SENSORFS_MAGIC; //TODO: FIXME
	sb->s_op		= &sensorfs_ops;
	sb->s_time_gran		= 1;
	sb->s_flags		= MS_RDONLY;

	atomic_inc(&sensorfs_root.count);
	sensorfs_root.sb = sb;
	inode = sensorfs_get_inode(sb, NULL, &sensorfs_root);
	if (!inode) {
		pr_err("sensorfs_fill_super: get root inode failed\n");
		return -ENOMEM;
	}

	sb->s_root = d_make_root(inode);
	if (!sb->s_root) {
		pr_err("sensorfs_fill_super: allocate dentry failed\n");
		return -ENOMEM;
	}

	sde = sensorfs_root.first_child;
	while (sde) {
		next = sde->next;
		if (!sensorfs_get_inode(sb, inode, sde)) {
			pr_err("sensorfs_fill_super: get child inode failed\n");
			return -ENOMEM;
		}
		sde = next;
	}

	return 0;
}

struct dentry *sensorfs_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	//TODO: Probably have to do a little thing here.
	return mount_nodev(fs_type, flags, data, sensorfs_fill_super);
}

static void sensorfs_kill_sb(struct super_block *sb)
{
	kill_litter_super(sb);
}

static int __init init_sensorfs_fs(void)
{
	sensorfs_init_nodecache();
	sensorfs_create_sfile(&sensorfs_root, SENSORFS_GPS_FILENAME);
	sensorfs_create_sfile(&sensorfs_root, SENSORFS_LUMI_FILENAME);
	sensorfs_create_sfile(&sensorfs_root, SENSORFS_PROX_FILENAME);
	sensorfs_create_sfile(&sensorfs_root, SENSORFS_LINACCEL_FILENAME);
	return register_filesystem(&sensorfs_fs_type);
}
module_init(init_sensorfs_fs)

static const struct dentry_operations sensorfs_dentry_operations = {
	.d_delete	   = sensorfs_delete_dentry,
};

struct file_system_type sensorfs_fs_type = {
	.name		= "sensorfs",
	.mount		= sensorfs_mount,
	.kill_sb	= sensorfs_kill_sb,
	.fs_flags	= FS_USERNS_MOUNT,
};

static const struct super_operations sensorfs_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
	.show_options	= generic_show_options,
	.alloc_inode	= sensorfs_alloc_inode,
	.destroy_inode	= sensorfs_destroy_inode,
};

