obj-m := add_syscall1.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all: test_syscall
	make -C $(KERNELBUILD) M=$(shell pwd) modules

test_syscall:
	$(CC) test_syscall.c -o test

clean:
	@rm -rf *.o *.mod.c .*.cmd *.markers *.order .tmp_versions *.symvers
clean_all: clean
	@rm -rf *.ko test

