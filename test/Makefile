CC=avr-gcc
MCU_TARGET=attiny167
F_CPU=16000000
USB_DRV=../v-usb/usbdrv/
CFLAGS=-g -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -DF_CPU=${F_CPU} -mmcu=${MCU_TARGET} -I. -I${USB_DRV} -std=gnu99
USB_LIBS=usbdrv.o usbdrvasm.o

all: main.hex main.lst

usbdrv.o: ${USB_DRV}/usbdrv.c
	${CC} ${CFLAGS} -c $< -o $@

usbdrvasm.o: ${USB_DRV}/usbdrvasm.S
	${CC} ${CFLAGS} -x assembler-with-cpp -c $< -o $@

main.elf: main.o ${USB_LIBS}
	avr-gcc ${CFLAGS} -o $@ $<

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

%.lst: %.elf
	avr-objdump -h -S $< > $@

deploy: main.hex
	sudo micronucleus $<

clean:
	rm -f ${USB_LIBS} main.elf main.o main.hex main.lst
