#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#define NUM 5*32*8

int main(void)
{
	int fd, i;
	unsigned int read_buf[10];/*�����û��ռ仺����*/
	fd = open("/dev/mydriver", O_RDWR | O_NONBLOCK);/*�Զ�д�ͷ�������ʽ���豸*/
	if (fd!=-1)
	{
		printf("venderID    deviceID    busno    deviceno    funcno    basereg\n");
		for(i=0; i<NUM; i++)
		{
			read(fd, read_buf, 4*6);/*���豸�ļ���ȡ4���������ݣ�24���ֽڵ�������*/
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

