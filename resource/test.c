#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define NR_PAGES	5
#define MAP_LEN		(NR_PAGES * getpagesize())

#define DEVICE_NAME	"/proc/my_resource"

int main()
{
	int fd, ret;
	void *ptr;
	/* open proc device */
	fd = open(DEVICE_NAME, O_RDONLY);
	if(fd == -1)
	{
		printf(DEVICE_NAME " open error\n");
		ret = -1;
		goto exit;
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
	strcpy(ptr, DEVICE_NAME);
	if(strcmp(ptr, DEVICE_NAME))
		printf("read/write test error\n");
exit:
	close(fd);
	return ret;
}

