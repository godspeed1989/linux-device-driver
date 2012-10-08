#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
unsigned int globalvar;
int main()
{
	int fd;
	fd = open("/tmp/bh", O_RDWR, S_IRUSR|S_IWUSR);
	if(fd != -1)
	{
		while(1)
		{
			read(fd, &globalvar, sizeof(int));
			printf("The globalvar is %d\n", globalvar);
			if(globalvar == 0)
			{
				close(fd);
				break;
			}
		}
	}
	else
	{
		printf("Device open failed\n");
	}
	return 0;
}
