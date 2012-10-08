obj-m := simp_blk0.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNELBUILD) M=$(shell pwd) modules

clean:
	@rm -rf *.o *.mod.c .*.cmd *.markers *.order .tmp_versions *.symvers
clean_all: clean
	@rm -rf *.ko

