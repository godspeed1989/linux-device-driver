#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#define NUM 5*32*8

int main(void)
{
	int fd, i;
	unsigned int read_buf[10];/*建立用户空间缓冲区*/
	fd = open("/dev/mydriver", O_RDWR | O_NONBLOCK);/*以读写和非阻塞方式打开设备*/
	if (fd!=-1)
	{
		printf("venderID    deviceID    busno    deviceno    funcno    basereg\n");
		for(i=0; i<NUM; i++)
		{
			read(fd, read_buf, 4*6);/*从设备文件读取4个整型数据，24个字节到缓冲区*/
			if(read_buf[0] && (read_buf[0]&0xffff) != 0xffff)
			{
				printf("  0x%x       0x%x       0x%x       0x%x       0x%x       0x%x\n", \
				read_buf[0],read_buf[1],read_buf[2],read_buf[3],read_buf[4],read_buf[5]);
			}
		}
		close(fd);
	}
	else
	{
		printf ("Device open failure\n");
	}
	return 0;
}

