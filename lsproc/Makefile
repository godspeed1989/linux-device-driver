MOD=lsproc

obj-m := $(MOD).o

KERNELBUILD := /lib/modules/$(shell uname -r)/build

all:
	make -C ./app
	make -C $(KERNELBUILD) M=$(shell pwd) modules

insmod:
	sudo insmod $(MOD).ko

rmmod:
	sudo rmmod $(MOD)

clean:
	@rm -rf *.o *.mod.c .*.cmd *.markers *.order .tmp_versions *.symvers
clean_all: clean
	@rm -rf *.ko

