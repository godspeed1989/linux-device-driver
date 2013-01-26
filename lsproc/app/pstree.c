#include "../share.h"
#include <stdio.h>
#include <fcntl.h>

#define DEV_PATH "/dev/lsproc"
#define INDENT "    "

static Proc P[MAX_PROC];
void print_pstree(int fd);

int main(int argc, const char *argv[])
{
	int fd;
	fd = open(DEV_PATH, O_RDWR|O_SYNC);
	if(fd<0)
	{
		perror(DEV_PATH" open error");
		return -1;
	}
	
	print_pstree(fd);
	close(fd);
	return 0;
}

static void print_tree(int idx, int depth)
{
	int i, j;
	short children[MAX_PROC];
	for(i=0; i<depth; i++)
		printf("│"INDENT);

	printf("├─%d %s %d\n", P[idx].pid, P[idx].comm, P[idx].ppid);
	j = 0;
	for(i=0; i<MAX_PROC && P[i].comm[0] != '\0'; i++)
	{
		if(P[i].ppid == P[idx].pid && P[i].pid != P[idx].pid)
			children[j++] = i;
	}
	for(i=0; i<j; i++)
	{
		if(i != j-1)
			print_tree(children[i], depth+1);
		else
		{
			print_tree(children[i], depth+1);
		}
	}
}

void print_pstree(int fd)
{
	int i;
	ioctl(fd, PROC_TREE, P);
	/* find idle process 0 */
	for(i=0; i<MAX_PROC && P[i].comm[0] != '\0'; i++)
		if(P[i].pid == 0) break;
	if(P[i].comm[0] != '\0')
		print_tree(i, 0);
	else
		printf("error: can't find idle process 0\n");
}

