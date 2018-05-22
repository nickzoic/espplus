#include "usb.h"

void usb_recv(uint8_t channel, uint8_t *message, int16_t size) {
    usb_send(channel, message, size);
}

int main(void) {
    usb_init(usb_recv);
    
    for(;;) {
	    usb_task();
    }
}
