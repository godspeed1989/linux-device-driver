obj-m := bh.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all: read write 
	make -C $(KERNELBUILD) M=$(shell pwd) modules

read: read.c
	$(CC) $< -o $@
write: write.c
	$(CC) $< -o $@
	
clean:
	@rm -rf *.o *.mod.c .*.cmd *.markers *.order .tmp_versions *.symvers
clean_all: clean
	@rm -rf *.ko

