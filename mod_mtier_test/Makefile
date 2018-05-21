obj-m += mod_mtier_test.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

debug:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules -DMTIER_DEBUG

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	-sudo rmmod mod_mtier
	sudo insmod mod_mtier.ko
