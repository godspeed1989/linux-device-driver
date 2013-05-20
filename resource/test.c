#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "share.h"

#define MAP_LEN		(NR_PAGES * getpagesize())

#define DEVICE_NAME	"/proc/my_resource"

int main()
{
	int i;
	int fd, ret;
	void *ptr;
	/* open proc device */
	fd = open(DEVICE_NAME, O_RDONLY);
	if(fd == -1)
	{
		printf(DEVICE_NAME " open error\n");
		ret = -1;
		goto error;
	}
	/* mmap to a new area */
	ptr = mmap(NULL, MAP_LEN, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(ptr == NULL)
	{
		perror("mmap error");
		ret = -1;
		goto exit;
	}
	/* test read/write function */
	for(i=0; i<NR_PAGES; i++)
	{
		char * p = ptr + i*getpagesize() + rand()%1000;
		strcpy(p, DEVICE_NAME);
		if(strcmp(p, DEVICE_NAME))
			printf("read/write test error at %d\n", i);
	}
	printf("Total %d MB occupied memory\n", MAP_LEN>>20);
	munmap(ptr, MAP_LEN);
exit:
	close(fd);
error:
	return ret;
}

