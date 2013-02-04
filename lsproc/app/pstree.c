#include "../share.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_PATH "/dev/lsproc"
#define INDENT "    "

void print_pstree(int fd);
void print_pstgrp(int fd);
void print_psinfo(int fd, pid_t pid);

static void print_help()
{
	printf("Usage: pstree [option] [pid]\n");
	printf("  1: print process tree\n");
	printf("  2: print thread group\n");
	printf("  3: print process pid's memory state and extra info\n");
}

int main(int argc, const char *argv[])
{
	int fd;
	if(argc < 2)
	{
		print_help();
		return 1;
	}
	fd = open(DEV_PATH, O_RDWR|O_SYNC);
	if(fd<0)
	{
		perror(DEV_PATH" open error");
		return -1;
	}
	switch(atoi(argv[1]))
	{
		case 1: print_pstree(fd); break;
		case 2: print_pstgrp(fd); break;
		case 3:
			if(argc < 3)
				print_psinfo(fd, getpid());
			else
				print_psinfo(fd, atoi(argv[2]));
			break;
		default: break;
	}
	close(fd);
	return 0;
}

static Proc P[MAX_PROC];
static char ignore[MAX_PROC] = {0};
static void print_tree(int idx, int depth, int flag)
{
	int i, j;
	short children[MAX_PROC];
	
	for(i=0; i<depth; i++)
		if(!ignore[i])
			printf("│"INDENT);
		else
			printf(" "INDENT);
	
	if(flag)
		printf("└─%d {%s}\n", P[idx].pid, P[idx].comm);
	else
		printf("├─%d {%s}\n", P[idx].pid, P[idx].comm);
	j = 0;
	/* list children proccess(es) */
	for(i=0; i<MAX_PROC && P[i].comm[0] != '\0'; i++)
	{
		if(P[i].ppid == P[idx].pid && P[i].pid != P[idx].pid)
			children[j++] = i;
	}
	for(i=0; i<j; i++)
	{
		if(i == j-1) {
			ignore[depth+1] = 1;
			print_tree(children[i], depth+1, 1);
		}
		else
			print_tree(children[i], depth+1, 0);
	}
	ignore[depth+1] = 0;
}
/**
 * print process tree by pid
 */
void print_pstree(int fd)
{
	int i;
	ioctl(fd, PROC_TREE, P);
	memset(ignore, 0, sizeof(short)*MAX_PROC);
	ignore[0] = 1;
	/* find idle process 0 */
	for(i=0; i<MAX_PROC && P[i].comm[0] != '\0'; i++)
		if(P[i].pid == 0) break;
	if(P[i].comm[0] != '\0')
		print_tree(i, 0, 1);
	else
		printf("error: can't find idle process 0\n");
}

/**
 * print process tree by thread group
 */
void print_pstgrp(int fd)
{
	int i, j;
	ioctl(fd, PROC_TGRP, P);
	for(i=0; i<MAX_PROC && P[i].comm[0] != '\0'; i++)
	{
		if(P[i].nr_tgrp != 0)
		{
			printf("──┬─%d {%s}", P[i].tgid, P[i].comm);
			printf("*%d\n", P[i].nr_tgrp);
			for(j=0; j<P[i].nr_tgrp; j++)
			{
				if(j+1 != P[i].nr_tgrp)
					printf("  ├─");
				else
					printf("  └─");
				printf("%d [%s]\n", P[i].tgrp[j].pid, P[i].tgrp[j].comm);
			}
		}
		else
		{
			printf("────%d {%s}\n", P[i].tgid, P[i].comm);
		}
	}
}

/**
 * print task's memory layout & extra infomation
 */
static mm_info mminfo;
void print_psinfo(int fd, pid_t pid)
{
	int i;
	char *ptr;
	ioctl(fd, PROC_MEM_STAT, pid);
	read(fd, &mminfo, sizeof(mminfo));
	printf("pid = %d\n", mminfo.pid);
	printf("code: %lx\n      %lx\n", mminfo.start_code, mminfo.end_code);
	printf("data: %lx\n      %lx\n", mminfo.start_data, mminfo.end_data);
	printf("brk:  %lx\n      %lx\n", mminfo.start_brk,  mminfo.brk);
	printf("stack:%lx\n", mminfo.start_stack);
	printf("args: %lx\n      %lx\n", mminfo.arg_start, mminfo.arg_end);
	printf("env:  %lx\n      %lx\n", mminfo.env_start, mminfo.env_end);
	if(getpid()==mminfo.pid) //only current process's arg & env can be print
	{
		i = 0, ptr = (char*)mminfo.arg_start;
		while(ptr < (char*)mminfo.arg_end)
		{
			printf("argv[%d] -> %s\n", i++, ptr);
			ptr += strlen(ptr)+1;
		}
		i = 0, ptr = (char*)mminfo.env_start;
		while(ptr < (char*)mminfo.env_end)
		{
			printf("env[%d] -> \"%s\"\n", i++, ptr);
			ptr += strlen(ptr)+1;
		}
	}
}

