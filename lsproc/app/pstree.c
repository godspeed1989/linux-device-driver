#include "../share.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define DEV_PATH "/dev/lsproc"
#define INDENT "    "

static Proc P[MAX_PROC];
void print_pstree(int fd);
void print_pstgrp(int fd);

int main(int argc, const char *argv[])
{
	int fd;
	fd = open(DEV_PATH, O_RDWR|O_SYNC);
	if(fd<0)
	{
		perror(DEV_PATH" open error");
		return -1;
	}
	
	//print_pstree(fd);
	print_pstgrp(fd);
	close(fd);
	return 0;
}

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
		printf("└─%d [%s]\n", P[idx].pid, P[idx].comm);
	else
		printf("├─%d [%s]\n", P[idx].pid, P[idx].comm);
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
	ioctl(fd, PROC_TREE, P);
	memset(ignore, 0, sizeof(char)*MAX_PROC);
	for(i=0; i<MAX_PROC && P[i].comm[0] != '\0'; i++)
	{
		if(!ignore[i])
		{
			ignore[i] = 1;
			printf("├─%d [%s] %d\n", P[i].tgid, P[i].comm, P[i].tid);
			for(j=i+1; j<MAX_PROC && P[j].comm[0] != '\0'; j++)
			{
				if(!ignore[j] && P[i].tgid == P[j].tgid)
				{
					ignore[j] = 1;
					printf("├─%d [%s] %d\n", P[i].tgid, P[i].comm, P[i].tid);
				}
			}
			printf("\n");
		}
	}
}

