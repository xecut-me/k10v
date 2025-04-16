build:
	avr-gcc -mmcu=atmega328p -DF_CPU=8000000UL -Os -o main.elf main.c

objcopy: build
	avr-objcopy -O ihex -R .eeprom main.elf main.hex

upload: objcopy
	avrdude -p m328p -c stk500v1 -U flash:w:main.hex:i -P /dev/ttyACM1

all: build objcopy upload

.PHONY: all build objcopy upload
