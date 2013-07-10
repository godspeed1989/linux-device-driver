#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const char * ifname = "insane";

int main(int argc, char* argv[])
{
	int sock;
	struct ifreq req;

	sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (sock < 0)
	{
		fprintf(stderr, "socket: %s\n", strerror(errno));
		return -1;
	}

	/* check that the insane interface exists */
	strncpy(req.ifr_name, ifname, IFNAMSIZ);
	if( ioctl(sock, SIOCGIFFLAGS, &req) < 0 )
	{
		fprintf(stderr, "%s: %s\n", ifname, strerror(errno));
		if (errno == ENODEV)
			fprintf(stderr, "(did you load the module?)\n");
		return 1;
	}

	/* bind socket to insane interface */
	strncpy(req.ifr_ifrn.ifrn_name, ifname, IFNAMSIZ);
	if( setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, \
		&req, sizeof(req)) < 0 )
	{
		fprintf(stderr, "bind failed: %s \n", strerror(errno));
		return 2;
	}	

	close(sock);
	return 0;
}
