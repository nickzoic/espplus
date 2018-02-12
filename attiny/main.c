#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>
#include "usbdrv.h"

#define DEBUG 1
#include "debug.h"

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
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
	DEBUG_STR("hi");
        PORTB |= (1 << PORTB1);
        usbPoll();
	_delay_ms(500);
        PORTB &= ~(1 << PORTB1);
        /*if (usbInterruptIsReady()) {
	    DEBUG_STR("i");
	    usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));	
	}*/
    }
}
