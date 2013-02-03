#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

void *pp(void *data)
{
	for(;;)
	{
		printf("I'm test_thread_%s ", (char*)data);
		printf("pid = %d tid =  %ld\n", getpid(), syscall(SYS_gettid));
		sleep(8);
	}
}

int main()
{
	pthread_t pid1, pid2;
	pthread_create(&pid1, NULL, pp, "1");
	pthread_create(&pid2, NULL, pp, "2");
	pthread_create(&pid2, NULL, pp, "3");
	for(;;)
	{
		printf("I'm test_thread_0 ");
		printf("pid = %d tid =  %ld\n", getpid(), syscall(SYS_gettid));
		sleep(8);
	}
	return 0;
}

