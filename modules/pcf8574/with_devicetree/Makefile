obj-m += pcf8574.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	dtc -O dtb -o PCF8574-OVERLAY-00A0.dtbo -b 0 -@ pcf8574_overlay.dts
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm PCF8574-OVERLAY-00A0.dtbo
