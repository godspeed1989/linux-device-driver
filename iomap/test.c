#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "iomap.h"

#define  IO_BASE_ADDR          0x000a0000
#define  PAGE_SIZE                   4096
#define  NR_PAGES                       5
#define  MAP_LEN     (NR_PAGES*PAGE_SIZE)

void hex_display(char *p, int lines)
{
	int i, j;
	#define  PER_LINE 17
	printf("-------------\n");
	for(i=0; i<lines; i++)
	{
		printf("%08x: ", i*PER_LINE);
		for(j=0; j<PER_LINE; j++)
		{
			unsigned short content = (unsigned char)p[i*PER_LINE+j];
			if(content > 0xf)  printf("%2x ", content);
			else               printf("0%x ", content);
		}
		printf("\n");
	}
}

int main()
{
	int i, fd;
	char buf[MAP_LEN];
	void *ptr;
	struct Iomap dev;
	if(getpagesize() != PAGE_SIZE)
	{
		printf("page size not match.\n");
		return -1;
	}
	/* open char dev */
	fd = open("/dev/iomap0", O_RDWR);
	if(fd<0)
	{
		perror("open err");
		return -1;
	}
	/* setup */
	dev.base = IO_BASE_ADDR;
	dev.size = MAP_LEN;
	if(ioctl(fd, IOMAP_SET, &dev))
	{
		perror("ioctl err");
		goto exit;
	}
	/* mmap */
	ptr = mmap(NULL, MAP_LEN, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(!ptr)
	{
		perror("mmap err");
		goto exit;
	}
	/* write tests */
	lseek(fd, 0, SEEK_SET);
	for(i=0; i<MAP_LEN; i++)
		*((char*)ptr+i) = 0xa5;
	hex_display((char*)ptr, 10);
	/* write */
	lseek(fd, 0, SEEK_SET);
	strcpy(buf, "01234567890");
	write(fd, buf, strlen(buf));
	/* read */
	lseek(fd, 0, SEEK_SET);
	read(fd, buf, MAP_LEN);
	hex_display(buf, 10);
	
	munmap(ptr, MAP_LEN);
exit:
	close(fd);
	return 0;
}

