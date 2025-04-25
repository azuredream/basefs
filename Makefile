# Minimal Kernel Module Makefile
obj-m += basefs.o

# List all C objects that form the "basefs" module
basefs-objs := basefs.o super.o inode.o file.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
