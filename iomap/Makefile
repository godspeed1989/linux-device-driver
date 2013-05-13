obj-m := iomap.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all: mod test

mod:
	make -C $(KERNELBUILD) M=$(shell pwd) modules

test: test.c
	$(CC) -o $@ $<

clean:
	@rm -rf test *.o *.mod.c .*.cmd *.markers *.order .tmp_versions *.symvers
clean_all: clean
	@rm -rf *.ko

