obj-m += mod_mtier.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	-sudo rmmod mod_mtier
	sudo insmod mod_mtier.ko
