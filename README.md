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
    sudo micronucleus build/main.hex

## Using

When the device resets it will spend a second in micronucleus waiting to be reprogrammed,
with the LED triple-flickering a few times.
During this time it appears in `lsusb` as `16d0:0753 MCS Digistump DigiSpark`.

Once it reboots it appears in `lsusb` as `1209:adda InterBiometrics`.  The LED should
stop flashing and there'll be some debugging output on pin TX at 38400,N,8,1

The device is set up to send appropriate descriptors for Linux, Windows and Mac to 
recognize it as a WebUSB device.  There's still some work to be done here.
When it works, it pops up a notification to let you know where the website is:

![espplus detected](img/espplus_detected.png)

If you click that notification, it'll take you to
[the espplus site](https://nickzoic.github.io/espplus/) which is just a placeholder
at the moment for a code editor.  With a bit of luck you'll get an "Add Device" button
which will let you associate the site with your device.  Once the device is paired, it
should detect connection and disconnection.

Clicking on the device button will connect to that device and attempt to send it a request:
you can see this in the console debugging output of the device.
It doesn't do anything much yet, but the plan is to drop in a REPL console,
a bit like [WebREPL](https://micropython.org/webrepl/) but over USB.

It will also get a file system tool and a file editor based on [Atom](https://atom.io/)
or maybe even a [Syntax Tree Editor](http://nick.zoic.org/art/syntax-tree-editor/) 

It doesn't do anything much yet.
