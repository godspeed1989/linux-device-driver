obj-m := kmmap.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all: km_test
	make -C $(KERNELBUILD) M=$(shell pwd) modules

km_test: km_test.c
	$(CC) $< -o $@

clean:
	@rm -rf *.o *.mod.c .*.cmd *.markers *.order .tmp_versions *.symvers
clean_all: clean
	@rm -rf *.ko km_test

