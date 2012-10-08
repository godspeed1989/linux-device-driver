obj-m := vms.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all: test vms

vms:
	make -C $(KERNELBUILD) M=$(shell pwd) modules

test: test.c
	$(CC) -o $@ $<

clean:
	@rm -rf *.o *.mod.c .*.cmd *.markers *.order
	@rm -rf .tmp_versions *.symvers
clean_all: clean
	@rm -rf *.ko

