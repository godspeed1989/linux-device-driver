#include "spinlock.h"
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#define Time_Wanna_Run 15
int main()
{
    int* shared_mem;
    int shmid;    
    shmid = shmget((key_t)1234, sizeof(int)*6, 0666 | IPC_CREAT);
    shared_mem = (int*)shmat(shmid, (void*)0, 0);

#ifdef DEBUG
    shared_mem[0] = UNLOCK;
    spin_lock(&shared_mem[0]);
    printf("locked -> %d\n", shared_mem[0]);
    spin_unlock(&shared_mem[0]);
    printf("unlocked -> %d\n", shared_mem[0]);
#else
    shared_mem[0] = shared_mem[1] = shared_mem[2] = LOCK;
    shared_mem[3] = shared_mem[4] = shared_mem[5] = LOCK;
    /*execute the Tasks*/
    pid_t pid1, pid2, pid3;    
    if( !(pid1=fork()) )
    {
        execl("./Task_A", "Task_A", NULL);
    }
    if( !(pid2=fork()) )
    {
        execl("./Task_B", "Task_B", NULL);
    }
    if( !(pid3=fork()) )
    {
        execl("./Task_C", "Task_C", NULL);
    }
    sleep(1);//wait for the child to be ready
    /*start the schedule*/
    time_t start = time(NULL);
    int goon = 1;
    int next_state = 0;
    while(goon)
    {
        if(next_state == 1)
        {
            next_state = 2;
            spin_unlock(&shared_mem[0]);
            spin_unlock(&shared_mem[3]);
            sleep(1);
            spin_lock(&shared_mem[0]);
        }
        else if(next_state == 2)
        {
            next_state = 3;
            spin_unlock(&shared_mem[1]);
            spin_unlock(&shared_mem[4]);            
            sleep(1);
            spin_lock(&shared_mem[1]);
        }
        else if(next_state == 3)
        {
            next_state = 0;
            spin_unlock(&shared_mem[2]);
            spin_unlock(&shared_mem[5]);            
            sleep(1);
            spin_lock(&shared_mem[2]);
        }
        else if(next_state == 0)
        {
            next_state = 1;
        }
        else
        {
            printf("Something unusual occur!!!\n");
        }
        if(time(NULL)-start > Time_Wanna_Run)
        {
            goon = 0;
        }
    }

    kill(pid1, SIGABRT);
    kill(pid2, SIGABRT);
    kill(pid3, SIGABRT);    
#endif

    return 0;
}

