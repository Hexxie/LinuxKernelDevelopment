obj-m := chardev.o

KDIR := /usr/src/linux-headers-4.13.0-37-generic

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf *.o *.ko *.mod *.symvers *.order


