// usb.h : generic interface for USB library.
#include <stdint.h>

#define USB_CHANNEL_CDC (1)
#define USB_CHANNEL_HID (2)

typedef void (usb_recv_t)(uint8_t channel, uint8_t *message, int16_t size);

void usb_init(usb_recv_t *usb_recv);

void usb_send(uint8_t channel, uint8_t *message, int16_t size);

void usb_task(void);
