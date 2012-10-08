#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "iomap.h"

#define  IO_BASE_ADDR          0xcd000000
#define  PAGE_SIZE                4096
#define  NR_PAGES                    5
#define  MAP_LEN   (NR_PAGES*PAGE_SIZE)

void display(char *p, int lines)
{
	int i, j;
	#define  PER_LINE 17
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

void main()
{
	int i, fd;
	char buf[MAP_LEN];
	void *ptr;
	struct Iomap dev;
	/* open char dev */
	fd = open("/dev/iomap0", O_RDWR);
	if(!fd) {
		perror("open err");
		return;
	}
	/* setup */
	dev.base = IO_BASE_ADDR;
	dev.size = MAP_LEN;
	if(ioctl(fd, IOMAP_SET, &dev))
	{
		perror("ioctl err");
		goto exit;
	}
	/* read */
	read(fd, buf, MAP_LEN);
	//display(buf, 40);
	/* write */
	strcpy(buf, "000000000000000000");
	write(fd, buf, MAP_LEN);
	/* mmap */
	ptr = mmap(NULL, MAP_LEN, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(!ptr)
	{
		perror("mmap err");
		goto exit;
	}
	display((char*)ptr, 10);
	for(i=0; i<MAP_LEN/3; i++)
		*((char*)ptr+i) = '0';
	printf("-------------\n");
	display((char*)ptr, 10);
	
	munmap(ptr, MAP_LEN);
exit:
	close(fd);
}
