#ifndef __SHARE_H__
#define __SHARE_H__

#ifndef MODULE
#  include <sys/types.h>
#  define TASK_COMM_LEN 16
#else
#  include <linux/sched.h>
#endif

typedef struct Proc
{
	pid_t pid, ppid, tgid, tid;
	int nr_tgrp;
	char comm[TASK_COMM_LEN];
}Proc;

#define MAX_PROC 1024

#define PROC_TREE		0x00aa0000
#define PROC_MEM_STAT	0x00cc0000
#define PROC_DETAIL		0x00dd0000


#endif

