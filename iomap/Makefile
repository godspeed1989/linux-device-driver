obj-m := iomap.o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all: mod test

mod: rs.c share.h
	make -C $(KERNELBUILD) M=$(shell pwd) modules

test: test.c share.h
	$(CC) -o $@ $<

clean:
	@rm -rf test *.o *.mod.c .*.cmd *.markers *.order .tmp_versions *.symvers
clean_all: clean
	@rm -rf *.ko

