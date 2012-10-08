#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "mysyscall.h"

int main()
{
	syscall(G_NUM);
	return 0;
}
