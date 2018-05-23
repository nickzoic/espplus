#include "usb.h"
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

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
    UCSR1B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);
}

void uart_send(uint8_t *message, uint16_t size) {
	for (uint16_t i = 0; i < size; i++) {
		loop_until_bit_is_set(UCSR1A, UDRE1);
		UDR1 = message[i];
	}
}

volatile uint8_t ring_buff[1024] = { 0 };
volatile uint16_t ring_head = 0;
volatile uint16_t ring_tail = 0;

ISR(USART1_RX_vect)
{
    ring_buff[ring_head] = UDR1;
    ring_head = (ring_head + 1) % sizeof(ring_buff);
}

void uart_task() {
    uint8_t message[64];
    int i = 0;
    cli();
    while (i < sizeof(message) && ring_head != ring_tail) {
	message[i++] = ring_buff[ring_tail];
	ring_tail = (ring_tail + 1) % sizeof(ring_buff);
    }
    sei();
    if (i) {
        usb_send(USB_CHANNEL_HID, message, i);
        //usb_send(USB_CHANNEL_CDC, message, i);
    }
}

void usb_recv(uint8_t channel, uint8_t *message, int16_t size) {
    uart_send(message, size);
    //usb_send(channel, message, size);
}

int main(void) {
    usb_init(usb_recv);
    uart_init();

    sei();

//esp_reset();
    
    for(;;) {
	    usb_task();
	    uart_task();
    }
}
