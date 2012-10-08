/*----------------------------------------------*/
#define LOCK    1
#define UNLOCK  0
/*----------------------------------------------*/
#define spin_lock(p)    \
    __asm__ __volatile__(   \
        "1: xor %%ecx , %%ecx \n\t"    \
        "xor %%eax , %%eax \n\t"    \
        "incl %%ecx \n\t"   \
        "lock cmpxchgl %%ecx , %0 \n\t"  \
        "jne 1b \n\t"	\
    	:"+m"(*p)::"%eax","%ecx"    \
    );
/*----------------------------------------------*/
#define spin_unlock(p)    \
    __asm__ __volatile__(   \
        "xor %%eax , %%eax \n\t"  \
        "movl %%eax , %0 \n\t"  \
        :"+m"(*p)::"%eax"   \
    );

