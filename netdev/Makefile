obj-m := insane.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all: module

module:
	make -C $(KERNELBUILD) M=$(shell pwd) modules

clean:
	@rm -rf *.o *.mod.c .*.cmd *.markers *.order .tmp_versions *.symvers 
dist_clean: clean
	@rm -rf *.ko test

