#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

void *pp(void *data)
{
	for(;;)
	{
		printf("I'm %s ", __FUNCTION__);
		printf("pid = %d tid =  %ld\n", getpid(), syscall(SYS_gettid));
		sleep(2);
	}
}

int main()
{
	pthread_t pid1, pid2;
	pthread_create(&pid1, NULL, pp, NULL);
	pthread_create(&pid2, NULL, pp, NULL);
	for(;;)
	{
		printf("I'm %s ", __FUNCTION__);
		printf("pid = %d tid =  %ld\n", getpid(), syscall(SYS_gettid));
		sleep(1);
	}
	return 0;
}

