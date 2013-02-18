obj-m := mydriver.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all: module test

module:
	make -C $(KERNELBUILD) M=$(shell pwd) modules

test: test.c
	gcc $< -o $@

clean:
	@rm -rf *.o *.mod.c .*.cmd *.markers *.order .tmp_versions *.symvers 
dist_clean: clean
	@rm -rf *.ko test

