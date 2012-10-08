#include <stdio.h>

#define LEN 100
char buffer[LEN];

void main(void)
{
    FILE* fp = fopen("/proc/proc_test/hello", "r");
    if(fp == NULL)
    {
        printf("open error\n");
        return;
    }
    fwrite("13081198", sizeof(char), 8, fp);
    
    fread((void*)buffer, sizeof(char), LEN, fp);
    printf("%s", buffer);
}
