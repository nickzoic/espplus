#include "usb.h"
#include <stdio.h>
#include <avr/io.h>

#define BAUD 115200
#include <util/setbaud.h>

void uart_init() {
    UBRR1H = UBRRH_VALUE;
    UBRR1L = UBRRL_VALUE;
#if USE_2X
    UCSR1A |= (1 << U2X1);
#else
    UCSR1A &= ~(1 << U2X1);
#endif
    UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
    UCSR1B = _BV(RXEN1) | _BV(TXEN1);
}

void uart_send(uint8_t *message, uint16_t size) {
	for (uint16_t i = 0; i < size; i++) {
		loop_until_bit_is_set(UCSR1A, UDRE1);
		UDR1 = message[i];
	}
}

void uart_task() {
    uint8_t message[16];
    int i = 0;
    while (i < sizeof(message) && (UCSR1A & _BV(RXC1))) {
	message[i++] = UDR1;
    }
    usb_send(USB_CHANNEL_CDC, message, i);
}

void usb_recv(uint8_t channel, uint8_t *message, int16_t size) {
    uart_send(message, size);
    //usb_send(channel, message, size);
}

int main(void) {
    usb_init(usb_recv);
    uart_init();

//esp_reset();
    
    for(;;) {
	    usb_task();
	    uart_task();
    }
}
