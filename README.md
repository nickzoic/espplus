# espplus

An attempt to make a friendlier out-of-box experience for MicroPython / ESP32.

See http://nick.zoic.org/art/micropython-webusb/

Currently the interesting code is in 'test', targeting the ATtiny167, specifically the
Digistump Digispark Pro, which I happened to have one of lying around.

It uses V-USB, which is pulled in as a submodule so `git submodule update --init`.
If you're using a Digispark Pro also grab Micronucleus from https://github.com/micronucleus/micronucleus

## Ubuntu Packages

Seems to work well on 17.10 'Artful':

* gcc-avr
* libusb-dev
* libusb-1.0-0

## Build / Flash

If using micronucleus:

    git submodule update --init
    cd attiny
    make
    sudo micronucleus main.hex

## Using

When the device resets it will spend a second in micronucleus waiting to be reprogrammed,
with the LED triple-flickering a few times.
During this time it appears in `lsusb` as `16d0:0753 MCS Digistump DigiSpark`.

Once it reboots it appears in `lsusb` as `1209:adda InterBiometrics`.  The LED should
flash slowly as a watchdog and there'll be some debugging output on pin TX at 38400,N,8,1

It doesn't do anything much yet.


