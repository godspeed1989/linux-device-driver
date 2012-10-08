#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main(int argc,char **argv)
{
    char wr_buffer[20] = {'\0'};
    char rd_buffer[20] = {'\0'};

    strcpy(wr_buffer, "yanlin");

    int fd = open("/dev/rwbuf",O_RDWR|O_CREAT|O_TRUNC);
    printf("fd=%d\n",fd);
    if(fd < 0)
    {
        perror("/dev/rwbuf");
        return (-1);
    }

    int state = write(fd, wr_buffer, 6);
    printf("write %d chars\n", state);
    state = read(fd, rd_buffer, 6);
    printf("read %d chars : \"%s\"\n", state, rd_buffer);
    
    close(fd);
    return 0;
}

