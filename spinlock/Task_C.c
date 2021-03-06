#include "spinlock.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>

int main()
{
    int* shared_mem;
    int shmid = shmget((key_t)1234, sizeof(int)*6, 0666 | IPC_CREAT);
    shared_mem = (int*)shmat(shmid, (void*)0, 0);
    
    //sleep(1);
    while(1)
    {
        spin_lock(&shared_mem[5]);
        spin_lock(&shared_mem[2]);
        printf("Task C is running.\n");
        spin_unlock(&shared_mem[2]);
    }

    return 0;
}

