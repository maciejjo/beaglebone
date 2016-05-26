ifneq ($(KERNELRELEASE),)
obj-m += tb6612.o
else
KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

endif
