CC=avr-gcc
MCU_TARGET=attiny167
F_CPU=16000000
USB_DRV=v-usb/usbdrv
CFLAGS=-g -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -DF_CPU=${F_CPU} -mmcu=${MCU_TARGET} -I. -I${USB_DRV} -std=gnu99
BUILD=./build

all: ${BUILD}/main.hex ${BUILD}/main.lst

${BUILD}/usbdrv.o: ${USB_DRV}/usbdrv.c
	${CC} ${CFLAGS} -c -o $@ $^

${BUILD}/usbdrvasm.o: ${USB_DRV}/usbdrvasm.S
	${CC} ${CFLAGS} -x assembler-with-cpp -c -o $@ $^

${BUILD}/main.elf: ${BUILD}/main.o ${BUILD}/usbdrv.o ${BUILD}/usbdrvasm.o
	${CC} ${CFLAGS} -o $@ $^

${BUILD}/main.o: main.c debug.h
	${CC} ${CFLAGS} -c -o $@ $<

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

%.lst: %.elf
	avr-objdump -h -S $< > $@

deploy: ${BUILD}/main.hex
	micronucleus $<

clean:
	rm -r ${BUILD}

$(shell mkdir -p ${BUILD})
