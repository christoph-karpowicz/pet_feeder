main:
	avr-gcc -g -Os -mmcu=atmega16 -c main.c
	avr-gcc -g -Os -mmcu=atmega16 -c i2c.c
	avr-gcc -g -mmcu=atmega16 -o main.elf main.o i2c.o
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avrdude -c usbasp -p m16 -P usb -U flash:w:main.hex