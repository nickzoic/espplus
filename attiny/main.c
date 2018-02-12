#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>
#include "usbdrv.h"

#define DEBUG 1
#include "debug.h"

const char usbDescriptorDevice[] PROGMEM = {  // USB device descriptor
  0x12,  // sizeof(usbDescriptorDevice): length of descriptor in bytes
  USBDESCR_DEVICE,        // descriptor type
  0x10, 0x02,             // USB version supported == 2.1
  USB_CFG_DEVICE_CLASS,
  USB_CFG_DEVICE_SUBCLASS,
  0,                      // protocol
  8,                      // max packet size
  // the following two casts affect the first byte of the constant only, but
  // that's sufficient to avoid a warning with the default values.
  (char)USB_CFG_VENDOR_ID,
  (char)USB_CFG_DEVICE_ID,
  USB_CFG_DEVICE_VERSION,
  1,  // manufacturer string index
  2,  // product string index
  3,  // serial number string index
  1,  // number of configurations
};

const int usbDescriptorStringManufacturer[] PROGMEM = {
    USB_STRING_DESCRIPTOR_HEADER(10),
    'H', 'a', 'l', 'l', 'o', 'W', 'e', 'r', 'l', 'd'
};

const int usbDescriptorStringProduct[] PROGMEM = {
    USB_STRING_DESCRIPTOR_HEADER(10),
    'H', 'i', 'l', 'l', 'o', 'W', 'u', 'r', 'l', 'd'
};

const int usbDescriptorStringSerialNumber[] PROGMEM = {
    USB_STRING_DESCRIPTOR_HEADER(10),
    'H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'
};

/// START CARGO CULT FROM sowbug/weblight

#define USB_BOS_DESCRIPTOR_TYPE (15)
#define MS_OS_20_DESCRIPTOR_LENGTH (0x1e)
#define WL_REQUEST_WINUSB (0xfc)
#define WL_REQUEST_WEBUSB (0xfe)

const uchar BOS_DESCRIPTOR[] PROGMEM = {
  // BOS descriptor header
  0x05, 0x0F, 0x39, 0x00, 0x02,

  // WebUSB Platform Capability descriptor
  0x18,  // Descriptor size (24 bytes)
  0x10,  // Descriptor type (Device Capability)
  0x05,  // Capability type (Platform)
  0x00,  // Reserved

  // WebUSB Platform Capability ID (3408b638-09a9-47a0-8bfd-a0768815b665)
  0x38, 0xB6, 0x08, 0x34,
  0xA9, 0x09,
  0xA0, 0x47,
  0x8B, 0xFD,
  0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,

  0x00, 0x01,         // WebUSB version 1.0
  WL_REQUEST_WEBUSB,  // Vendor-assigned WebUSB request code
  0x01,               // Landing page: https://sowbug.github.io/webusb

  // Microsoft OS 2.0 Platform Capability Descriptor
  // Thanks http://janaxelson.com/files/ms_os_20_descriptors.c
  0x1C,  // Descriptor size (28 bytes)
  0x10,  // Descriptor type (Device Capability)
  0x05,  // Capability type (Platform)
  0x00,  // Reserved

  // MS OS 2.0 Platform Capability ID (D8DD60DF-4589-4CC7-9CD2-659D9E648A9F)
  0xDF, 0x60, 0xDD, 0xD8,
  0x89, 0x45,
  0xC7, 0x4C,
  0x9C, 0xD2,
  0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,

  0x00, 0x00, 0x03, 0x06,    // Windows version (8.1) (0x06030000)
  MS_OS_20_DESCRIPTOR_LENGTH, 0x00,
  WL_REQUEST_WINUSB,         // Vendor-assigned bMS_VendorCode
  0x00                       // Doesn’t support alternate enumeration
};


#define WINUSB_REQUEST_DESCRIPTOR (0x07)
const uchar MS_OS_20_DESCRIPTOR_SET[MS_OS_20_DESCRIPTOR_LENGTH] PROGMEM = {
  // Microsoft OS 2.0 descriptor set header (table 10)
  0x0A, 0x00,  // Descriptor size (10 bytes)
  0x00, 0x00,  // MS OS 2.0 descriptor set header
  0x00, 0x00, 0x03, 0x06,  // Windows version (8.1) (0x06030000)
  MS_OS_20_DESCRIPTOR_LENGTH, 0x00,  // Size, MS OS 2.0 descriptor set

  // Microsoft OS 2.0 compatible ID descriptor (table 13)
  0x14, 0x00,  // wLength
  0x03, 0x00,  // MS_OS_20_FEATURE_COMPATIBLE_ID
  'W',  'I',  'N',  'U',  'S',  'B',  0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#define WEBUSB_REQUEST_GET_ALLOWED_ORIGINS (0x01)
#define WEBUSB_REQUEST_GET_URL (0x02)

/// END CARGO CULT (MOSTLY)

usbMsgLen_t usbFunctionDescriptor(usbRequest_t *rq)
{
    DEBUG_BYTES(rq, sizeof(usbRequest_t));
    switch (rq->wValue.bytes[1]) {
	case USBDESCR_STRING:
	    switch (rq->wValue.bytes[0]) {
                case 1:
		    usbMsgPtr = (usbMsgPtr_t)(usbDescriptorStringManufacturer);
		    return sizeof(usbDescriptorStringManufacturer);
                case 2:
		    usbMsgPtr = (usbMsgPtr_t)(usbDescriptorStringProduct);
		    return sizeof(usbDescriptorStringProduct);
                case 3:
		    usbMsgPtr = (usbMsgPtr_t)(usbDescriptorStringSerialNumber);
		    return sizeof(usbDescriptorStringSerialNumber);
	    }
	    break;
	case USB_BOS_DESCRIPTOR_TYPE:
	    usbMsgPtr = (usbMsgPtr_t)(BOS_DESCRIPTOR);
	    return sizeof(BOS_DESCRIPTOR);
    }
    return 0;
}

const uchar webusb_url[] PROGMEM = { 19, 3, 0, 'e', 's', 'p', 'p', 'l', 'u', 's', '.', 'z', 'o', 'i', 'c', '.', 'o', 'r', 'g' };

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    usbRequest_t *rq = (void *)data;
    switch (rq->bRequest) {
	case WL_REQUEST_WEBUSB: 
	    switch (rq->wIndex.word) {
	        case WEBUSB_REQUEST_GET_ALLOWED_ORIGINS:
		case WEBUSB_REQUEST_GET_URL:
	            usbMsgPtr = webusb_url;
	            return sizeof(webusb_url);
	    }
	case WL_REQUEST_WINUSB:
	    switch (rq->wIndex.word) {
		case WINUSB_REQUEST_DESCRIPTOR:
		    usbMsgPtr = MS_OS_20_DESCRIPTOR_SET;
		    return sizeof(MS_OS_20_DESCRIPTOR_SET);
	    }
    }
		
    return 0;   /* default for not implemented requests: return no data back to host */
}

int main() {

    wdt_enable(WDTO_1S);

    DEBUG_INIT();
    DEBUG_STR("hello!\n");

    usbInit();
    usbDeviceDisconnect();
    _delay_ms(500);
    usbDeviceConnect();
    sei();

    DDRB |= (1 << DDB1);
    while (1) {
	wdt_reset();
        PORTB |= (1 << PORTB1);
        usbPoll();
        PORTB &= ~(1 << PORTB1);
        /*if (usbInterruptIsReady()) {
	    DEBUG_STR("i");
	    usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));	
	}*/
    }
}
