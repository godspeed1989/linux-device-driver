#include "share.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
/*http://www.cnblogs.com/noaming1900/archive/2010/10/20/1856797.html*/
int main()
{
	int fd = 0;
	char *map_addr = NULL;
	
	fd = open("/tmp/kmmap",O_RDWR);
	if(fd<0)
	{
		perror("fail to open /tmp/kmmap");
		return -1;
	}
	
	map_addr = mmap(NULL, MY_PAGE_SIZE*MAPPED_PAGES, \
					PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(map_addr == MAP_FAILED)
	{
		perror("fail to mmap");
		return -1;
	}
	printf("mmap success map_addr = %x\n", (unsigned int)map_addr);

	printf("map_addr[0] :%s\n", map_addr);
	printf("map_addr[0+%d] :%s\n", MY_PAGE_SIZE, (map_addr + MY_PAGE_SIZE));
	printf("change the map_addr[0] to 'b7854ad4'.\n");
	strcpy(map_addr, "b7854ad4\n");
	printf("changed map_addr[0] :%s\n", map_addr);
	
	munmap(map_addr, MY_PAGE_SIZE*MAPPED_PAGES);
	return 0;
}

