#ifndef __SHARE_H__
#define __SHARE_H__

#ifndef MODULE
#  include <sys/types.h>
#  define TASK_COMM_LEN 16
#else
#  include <linux/sched.h>
#endif

#define MAX_PROC 1024
#define TGRP_MAX 32

typedef struct t_info
{
	pid_t pid;
	char comm[TASK_COMM_LEN];
}t_info;

typedef struct Proc
{
	pid_t pid, ppid, tgid;
	unsigned int nr_tgrp;
	t_info tgrp[TGRP_MAX];
	char comm[TASK_COMM_LEN];
}Proc;


#define PROC_TREE		0x00aa0000
#define PROC_MEM_STAT	0x00cc0000
#define PROC_DETAIL		0x00dd0000


#endif

