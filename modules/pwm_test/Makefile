ifneq ($(KERNELRELEASE),)
obj-m += pwm_test.o
else
KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

endif
