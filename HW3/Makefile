PWD := $(shell pwd) 
KVERSION := $(shell uname -r)
KERNEL_DIR = /lib/modules/$(KVERSION)/build
MODULE_NAME = kfetch_mod_312551116
obj-m := $(MODULE_NAME).o

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
load:
	sudo insmod $(MODULE_NAME).ko
	#sudo dmesg
unload:
	sudo rmmod $(MODULE_NAME)
