#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>


#define BUF_LEN 1024

int here[]={0,50,75,110,134,150,200,300,600,1200,1800,2400,4800,9600,19200,
38400,57600,115200,230400};
int speed[]={B0,B50,B75,B110,B134,B150,B200,B300,B600,B1200,B1800,B2400,
B4800,B9600,B19200,B38400,B57600,B115200,B230400};
char inbuf[BUF_LEN]={0};
char outbuf[BUF_LEN]={0};
int term_flag=0;
int set_speed(int, int);
int set_parity(int, int, int, int);
void sig_handler(int);

void sig_handler(int sig){
	if(SIGINT == sig)
		term_flag=1;
	}
int set_speed(int fd, int spd){
	int i,status;
	struct termios opt;
	if(tcgetattr(fd,&opt)!=0)
		return -1;
	for(i=0;i<sizeof(here)/sizeof(int);i++)
		if(here[i] == spd)
			break;
	if(i == (sizeof(here)/sizeof(int)))
			return -1;
	tcflush(fd, TCIOFLUSH);
	cfsetispeed(&opt, speed[i]);
	cfsetospeed(&opt, speed[i]);
	status=tcsetattr(fd, TCSANOW, &opt);
	if(status!=0)
		return -1;
	tcflush(fd,TCIOFLUSH);
	return 0;
	}
/**
 * On success, return 0; else, return -1.
 */
int set_parity(int fd, int databits,int stopbits,int parity){
	struct termios options; 
	if(tcgetattr( fd,&options) != 0)
		return -1; 
	/*I just need Raw mode.*/ 
	options.c_cflag &= ~CSIZE; /* Mask the character size bits */
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag  &= ~OPOST;
	switch (databits)
	{   
	case 7:		
		options.c_cflag |= CS7; 
		break;
	case 8:     
		options.c_cflag |= CS8;
		break;   
	default:    
		return -1;  
	}

	switch (parity) 
	{   
	case 'n':
	case 'N':    
		options.c_cflag &= ~PARENB;
		options.c_iflag &= ~INPCK;
		break;  
	case 'o':   
	case 'O':     
		options.c_cflag |= (PARODD | PARENB);
		options.c_iflag |= INPCK;  
		break;  
	case 'e':  
	case 'E':   
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK;
		break;
	case 'S': 
	case 's': 
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;  
	default:   
		return -1;  
	}  

	switch (stopbits)
	{   
	case 1:    
		options.c_cflag &= ~CSTOPB;  
		break;  
	case 2:    
		options.c_cflag |= CSTOPB;  
		break;
	default:    
		 return -1; 
	} 

	if (parity != 'n')   
		options.c_iflag |= INPCK; 
	tcflush(fd,TCIFLUSH);
	options.c_cc[VTIME] = 10 ;
	options.c_cc[VMIN] = 0;
	if (tcsetattr(fd,TCSANOW,&options) != 0)   
		return -1;  
	return 0;  
	}

int main(int argc, char** argv){
	int i, opt, com=1;
	ssize_t nread, nwrite;
	int speed = 19200;
	int fd;
	int databits=8, stopbits=1, parity='N';

	while((opt = getopt(argc, argv, "c:b:d:s:p:")) != -1) {
		switch(opt)
		{
		case 'c':
			com = atoi(optarg);
			break;
		case 'b':
			speed = atoi(optarg);
			break;
		case 'd':
			databits = atoi(optarg);
			break;
		case 's':
			stopbits = atoi(optarg);
			break;
		case 'p':
			parity = optarg[0];	/*This is special, we want a char.*/
			break;
		case ':':
			fprintf(stderr,"Option %c needs a value!\n",opt);
			break;
		case '?':
			fprintf(stderr,"Unknow options!\n");
			break;
		}/*switch*/
	}/*while*/
	printf("Initializing port...");
	if(com==1)
		fd = open("/dev/ttyS0",O_RDWR | O_NOCTTY | O_NDELAY);
	else if(com==2)
		fd = open("/dev/ttyS1",O_RDWR | O_NOCTTY | O_NDELAY);
	else{
		fprintf(stderr,"Specified Serial Port invalid!\n");
		return -1;
		}
	if(fd==-1){
		perror("Can't open file");
		return -1;
		}
	fcntl(fd, F_SETFL, FNDELAY);/*If no data, return immediately.*/
	if(-1==set_speed(fd, speed)){
		fprintf(stderr,"Specified Speed invalid!\n");
		return -1;
		}
	if(-1==set_parity(fd, databits, stopbits, parity)){
		fprintf(stderr,"Specified Bits invalid or unknown error occurs!\n");
		return -1;
		}
	printf("\nOK!\n");
	(void)signal(SIGINT,sig_handler);
	
	while(1){
		printf(">>");
		fflush(stdout);
		fflush(stdin);
		nwrite = read(STDIN_FILENO, outbuf, BUF_LEN);
		while(write(fd, outbuf, nwrite-1)!=nwrite-1);
		tcflush(fd,TCOFLUSH);
		printf("Transmit %d bytes:\n", nwrite-1);
		for(i=0;i<nwrite-1;i++)
			printf("%2x ",(unsigned char)outbuf[i]);
		printf("\n---------------------\n");
		fflush(stdout);
		fflush(stdin);
		while((nread = read(fd, inbuf, BUF_LEN))>0){
			/*inbuf[nread] = '\xff'; */
			printf("Received %d bytes:\n", nread);
			for(i=0;i<nread;i++)
				printf("%2x ",(unsigned char)inbuf[i]);
			printf("\n---------------------\n");
			}
		if(term_flag==1)break;
		tcflush(fd,TCIFLUSH);	
		}
	close(fd);
	return 0;
	}


