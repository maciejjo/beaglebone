hcsr04:
	gcc hcsr04.c -o hcsr04 -lprussdrv
	pasm -b hcsr04.p
	dtc -O dtb -o HCSR04-PRU-00A0.dtbo -b 0 -@ HCSR04-PRU.dts

clean:
	rm hcsr04 hcsr04.bin HCSR04-PRU-00A0.dtbo
