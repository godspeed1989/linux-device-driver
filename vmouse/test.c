#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	int i, fd;
	int x, y;
	char buffer[10];
	fd = open("/sys/devices/platform/vms/coordinates", O_RDWR);
	if(fd < 0)
	{
		perror("can't open vmouse file\n");
		exit(-1);
	}
	i = 0;
	while(i<10)
	{
		x = random()%200;
		y = random()%200;
		sprintf(buffer, "%d %d %d", x, y, 0);
		write(fd, buffer, strlen(buffer));
		fsync(fd);
		sleep(1);
		i++;
	}
	close(fd);
	return 0;
}
