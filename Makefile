main:
	avr-gcc -g -Os -mmcu=atmega16 -c main.c
	avr-gcc -g -Os -mmcu=atmega16 -c lib/init.c
	avr-gcc -g -Os -mmcu=atmega16 -c lib/led.c
	avr-gcc -g -Os -mmcu=atmega16 -c lib/servo.c
	avr-gcc -g -Os -mmcu=atmega16 -c lib/button.c
	avr-gcc -g -Os -mmcu=atmega16 -c lib/i2c.c
	avr-gcc -g -Os -mmcu=atmega16 -c lib/display.c
	avr-gcc -g -mmcu=atmega16 -o main.elf main.o init.o led.o servo.o button.o i2c.o display.o
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avrdude -c usbasp -p m16 -P usb -U flash:w:main.hex