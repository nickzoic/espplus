#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "usbdrv.h"

#define DEBUG 1
#include "debug.h"

void delay(uint16_t ms) {
    uint16_t delay_count = F_CPU / 17500;
    volatile uint16_t i;
    while (ms != 0) {
	for (i=0; i != delay_count; i++);
	ms--;
    }
}

int main() {
    DEBUG_INIT();

    DDRB |= (1 << DDB1);
    while (1) {
	DEBUG_STR("hello");
        PORTB |= (1 << PORTB1);
        delay(100);
        PORTB &= ~(1 << PORTB1);
        delay(100);
        PORTB |= (1 << PORTB1);
        delay(100);
        PORTB &= ~(1 << PORTB1);
	DEBUG_STR(" world\n");
        delay(1000);
    }
}
