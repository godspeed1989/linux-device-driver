#include "lsproc.h"
#include "share.h"
#include <linux/list.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

static struct file_operations lsproc_fops =
{
	.owner	=	THIS_MODULE,
	.open	=	lsproc_open,
	.read	=	lsproc_read,
	.unlocked_ioctl	=	lsproc_ioctl,
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
 * get proccess tree information
 */
static Proc P[MAX_PROC];
static int nr_proc;
static void get_proc_info(void)
{
	int i, j;
	struct list_head *ele, *head, *pos, *tgrp;
	struct task_struct *task, *temp;
	
	head = &current->tasks;
	for(ele = head->next, i=0; ele != head && i<MAX_PROC-1; ele = ele->next, i++)
	{
		task = list_entry(ele, struct task_struct, tasks);
		P[i].pid = task->pid;
		P[i].ppid = task->real_parent->pid;
		P[i].tgid = task->tgid;
		strcpy(P[i].comm, task->comm);
		tgrp = &task->thread_group;
		for(pos = tgrp->next, j=0; pos != tgrp; pos = pos->next, j++)
		{
			temp = list_entry(pos, struct task_struct, thread_group);
			P[i].tgrp[j].pid = temp->pid;
			strcpy(P[i].tgrp[j].comm, temp->comm);
		}
		P[i].nr_tgrp = j;
	}
	P[i].comm[0] = '\0';
	nr_proc = i;
}
static void get_proc_tree(unsigned long ptr)
{
	MSG();
	get_proc_info();
	DPRINTF("%d proccess(es)\n", nr_proc);
	copy_to_user((void*)ptr, (void*)P, nr_proc*sizeof(Proc));
}

/**
 * get proccess memory layout infomation
 */
static mm_info mminfo;
static void proc_memstat(unsigned long pid)
{
	struct task_struct *task;
	MSG();
	task = pid_task(find_vpid(pid), PIDTYPE_PID);
	if(task == NULL)
		task = current;
	mminfo.start_code = task->mm->start_code;
	mminfo.end_code = task->mm->end_code;
	mminfo.start_data = task->mm->start_data;
	mminfo.end_data = task->mm->end_data;
	mminfo.start_brk = task->mm->start_brk;
	mminfo.brk = task->mm->brk;
	mminfo.start_stack = task->mm->start_stack;
	mminfo.arg_start = task->mm->arg_start;
	mminfo.arg_end = task->mm->arg_end;
	mminfo.env_start = task->mm->env_start;
	mminfo.env_end = task->mm->env_end;
}

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
	MSG();
	copy_to_user(buf, &mminfo, sizeof(mminfo));
	return 0;
}

static long lsproc_ioctl(struct file *filep,
                         unsigned int cmd, unsigned long arg)
{
	MSG();
	switch(cmd)
	{
		case PROC_TREE: case PROC_TGRP:
			get_proc_tree(arg);
			break;
		case PROC_MEM_STAT: case PROC_DETAIL:
			proc_memstat(arg);
			break;
		default:
			break;
	}
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yanlin-1203121857");

