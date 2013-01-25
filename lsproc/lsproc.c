#include "lsproc.h"
#include "cmd.h"

static struct file_operations lsproc_fops =
{
	.owner	=	THIS_MODULE,
	.open	=	lsproc_open,
	.read	=	lsproc_read,
	.compat_ioctl	=	lsproc_ioctl,
};

int __init lsproc_init(void)
{
	int retn;
	MSG();
	retn = register_chrdev(DEV_NUM, MOD_NAME, &lsproc_fops);
	if(retn)
	{
		DPRINTF( MOD_NAME" register error\n");
		return -EIO;
	}
	return 0;
}
module_init(lsproc_init);

void __exit lsproc_exit(void)
{
	MSG();
	unregister_chrdev(DEV_NUM, MOD_NAME);
}
module_exit(lsproc_exit);

/**
 * file operations impliment
 */

static int lsproc_open(struct inode *inode, struct file *filep)
{
	MSG();
	DPRINTF("opener pid=%d [%s]\n", current->pid, current->comm);
	return 0;
}

static ssize_t lsproc_read(struct file *filep,
                           char *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static void proc_tree(unsigned long ptr)
{
	struct list_head *ele;
	struct task_struct *task;
	MSG();
	list_for_each(ele, &current->tasks)
	{
		task = list_entry(ele, struct task_struct, tasks);
		printk("%d %s\n", task->pid, task->comm);
	}
}

static void proc_tree_grp(unsigned long ptr)
{
	struct list_head *ele;
	struct task_struct *task;
	MSG();
	list_for_each(ele, &current->tasks)
	{
		task = list_entry(ele, struct task_struct, tasks);
		printk("%d %s\n", task->pid, task->comm);
	}
}

static void proc_memstat(unsigned long pid)
{
	struct task_struct *task;
	MSG();
	task = pid_task(find_vpid(pid), PIDTYPE_PID);
}

static void proc_detail(unsigned long pid)
{
	MSG();
}

static long lsproc_ioctl(struct file *filep,
                         unsigned int cmd, unsigned long arg)
{
	MSG();
	switch(cmd)
	{
		case PROC_TREE:
			proc_tree(arg);
			break;
		case THREAD_GRP:
			proc_tree_grp(arg);
			break;
		case MEM_STAT:
			proc_memstat(arg);
			break;
		case PROC_DETAIL:
			proc_detail(arg);
			break;
		default:
			break;
	}
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yanlin-1203121857");

