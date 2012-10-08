obj-m := tasklet.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNELBUILD) M=$(shell pwd) modules

clean:
	@rm -rf *.o *.mod.c .*.cmd *.markers *.order
	@rm -rf .tmp_versions *.symvers
clean_all: clean
	@rm -rf *.ko

