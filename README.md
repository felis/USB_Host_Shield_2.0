# USB Host Library Rev.2.0

The code is released under the GNU General Public License.
__________

# Summary
This is Revision 2.0 of MAX3421E-based USB Host Shield Library for AVR's.

Project main web site is: <http://www.circuitsathome.com>.

Some information can also be found at: <http://blog.tkjelectronics.dk/>.

The shield can be purchased at the main site: <http://www.circuitsathome.com/products-page/arduino-shields> or from [TKJ Electronics](http://tkjelectronics.com/): <http://shop.tkjelectronics.dk/product_info.php?products_id=43>.

![USB Host Shield](http://www.circuitsathome.com/wp/wp-content/uploads/2012/02/UHS_20_main-288x216.jpg)

For more information about the hardware see the [Hardware Manual](http://www.circuitsathome.com/usb-host-shield-hardware-manual).

# Developed By

* __Oleg Mazurov, Circuits@Home__ - <mazurov@circuitsathome.com>
* __Alexei Glushchenko, Circuits@Home__ - <alex-gl@mail.ru>
	* Developers of the USB Core, HID, FTDI, ADK, ACM, and PL2303 libraries
* __Kristian Lauszus, TKJ Electronics__ - <kristianl@tkjelectronics.com>
	* Developer of the [BTD](#bluetooth-libraries), [SPP](#spp-library), [PS3](#ps3-library), [Wii](#wii-library), and [Xbox](#xbox-library) libraries
* __Andrew Kroll__ - <xxxajk@gmail.com>
	* Major contributor to mass storage code

# How to include the library

First download the library by clicking on the following link: <https://github.com/felis/USB_Host_Shield_2.0/archive/master.zip>.

Then uncompress the zip folder and rename the directory to "USB\_Host\_Shield\_20", as any special characters are not supported by the Arduino IDE.

Now open up the Arduino IDE and open "File>Preferences". There you will see the location of your sketchbook. Open that directory and create a directory called "libraries" inside that directory.
Now move the "USB\_Host\_Shield\_20" directory to the "libraries" directory.

The final structure should look like this:

* Arduino/
	* libraries/
		* USB\_Host\_Shield\_20/

Now quit the Arduino IDE and reopen it.

Now you should be able to go open all the examples codes by navigating to "File>Examples>USB\_Host\_Shield\_20" and then select the example you will like to open.

For more information visit the following site: <http://arduino.cc/en/Guide/Libraries>.

# How to use the library

### Documentation

Documentation for the library can be found at the following link: <http://felis.github.com/USB_Host_Shield_2.0/>.

### Enable debugging

By default serial debugging is disabled. To turn it on uncomment ```#define DEBUG_USB_HOST``` in [message.h](message.h).

### Arduino ADK
To use this library with the official [Arduino ADK](http://arduino.cc/en/Main/ArduinoBoardADK) uncomment the following line in [avrpins.h](avrpins.h):

```
#define BOARD_MEGA_ADK
```

### [Bluetooth libraries](BTD.cpp)

The [BTD library](BTD.cpp) is a general purpose library for an ordinary Bluetooth dongle.
This library make it easy to add support for different Bluetooth services like a PS3 or a Wii controller or SPP which is a virtual serial port via Bluetooth.
Some different examples can be found in the [example directory](examples/Bluetooth).

The BTD library will also make it possible to use multiple services at once, the following example sketch is an example of this:
<https://github.com/felis/USB_Host_Shield_2.0/blob/master/examples/Bluetooth/PS3SPP/PS3SPP.ino>

### [SPP library](SPP.cpp)

SPP stands for "Serial Port Profile" and is a Bluetooth protocol that implements a virtual comport which allows you to send data back and forth from your computer/phone to your Arduino via Bluetooth.
It has been tested successfully on Windows, Mac OS X, Linux, and Android.

More information can be found at these blog posts:

* <http://www.circuitsathome.com/mcu/bluetooth-rfcommspp-service-support-for-usb-host-2-0-library-released>
* <http://blog.tkjelectronics.dk/2012/07/rfcommspp-library-for-arduino/>

To implement the SPP protocol I used a Bluetooth sniffing tool called [PacketLogger](http://www.tkjelectronics.com/uploads/PacketLogger.zip) developed by Apple. 
It enables me to see the Bluetooth communication between my Mac and any device.

### PS3 Library

These libraries consist of the [PS3BT](PS3BT.cpp) and [PS3USB](PS3USB.cpp). These libraries allows you to use a Dualshock 3, Navigation or a Motion controller with the USB Host Shield both via Bluetooth and USB.

In order to use your Playstation controller via Bluetooth you have to set the Bluetooth address of the dongle internally to your PS3 Controller. This can be achieved by plugging the controller in via USB and letting the library set it automatically.

__Note:__ To obtain the address you have to plug in the Bluetooth dongle before connecting the controller, or alternatively you could set it in code like so: <https://github.com/felis/USB_Host_Shield_2.0/blob/master/examples/Bluetooth/PS3BT/PS3BT.ino#L12>.

For more information about the PS3 protocol see the official wiki: <https://github.com/felis/USB_Host_Shield_2.0/wiki/PS3-Information>.

Also take a look at the blog posts:

* <http://blog.tkjelectronics.dk/2012/01/ps3-controller-bt-library-for-arduino/>
* <http://www.circuitsathome.com/mcu/sony-ps3-controller-support-added-to-usb-host-library>
* <http://www.circuitsathome.com/mcu/arduino/interfacing-ps3-controllers-via-usb>

A special thanks go to the following people:

1. _Richard Ibbotson_ who made this excellent guide: <http://www.circuitsathome.com/mcu/ps3-and-wiimote-game-controllers-on-the-arduino-host-shield-part>
2. _Tomoyuki Tanaka_ for releasing his code for the Arduino USB Host shield connected to the wiimote: <http://www.circuitsathome.com/mcu/rc-car-controlled-by-wii-remote-on-arduino>

Also a big thanks all the people behind these sites about the Motion controller:

* <http://thp.io/2010/psmove/>
* <http://www.copenhagengamecollective.org/unimove/>
* <https://github.com/thp/psmoveapi>
* <http://code.google.com/p/moveonpc/>

### Xbox Libraries

The library supports both the original Xbox controller via USB and the Xbox 360 controller both via USB and wirelessly.

#### Xbox library

The [XBOXOLD](XBOXOLD.cpp) class implements support for the original Xbox controller via USB.

All the information are from the following sites:

* <https://github.com/torvalds/linux/blob/master/Documentation/input/xpad.txt>
* <https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c>
* <http://euc.jp/periphs/xbox-controller.ja.html>
* <https://github.com/Grumbel/xboxdrv/blob/master/PROTOCOL#L15>

#### Xbox 360 Library

The library support one Xbox 360 via USB or up to four Xbox 360 controllers wirelessly by using a [Xbox 360 wireless receiver](http://blog.tkjelectronics.dk/wp-content/uploads/xbox360-wireless-receiver.jpg).

To use it via USB use the [XBOXUSB](XBOXUSB.cpp) library or to use it wirelessly use the [XBOXRECV](XBOXRECV.cpp) library.

__Note that a Wireless controller can NOT be used via USB!__

Examples code can be found in the [examples directory](examples/Xbox).

Also see the following blog posts:

* <http://www.circuitsathome.com/mcu/xbox360-controller-support-added-to-usb-host-shield-2-0-library>
* <http://blog.tkjelectronics.dk/2012/07/xbox-360-controller-support-added-to-the-usb-host-library/>
* <http://blog.tkjelectronics.dk/2012/12/xbox-360-receiver-added-to-the-usb-host-library/>

All the information regarding the Xbox 360 controller protocol are form these sites:

* <http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller/UsbInfo>
* <http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller/WirelessUsbInfo>
* <https://github.com/Grumbel/xboxdrv/blob/master/PROTOCOL>

### [Wii library](Wii.cpp)

The [Wii](Wii.cpp) library support the Wiimote, but also the Nunchuch and Motion Plus extensions via Bluetooth. The Wii U Pro Controller is also supported via Bluetooth.

First you have to pair with the controller, this is done automatically by the library if you create the instance like so:

```
WII Wii(&Btd,PAIR);
```

And then press 1 & 2 at once on the Wiimote or press sync if you are using a Wii U Pro Controller.

After that you can simply create the instance like so:

```
WII Wii(&Btd);
```

Then just press any button any button on the Wiimote and it will connect to the dongle.

Take a look at the example for more information: <https://github.com/felis/USB_Host_Shield_2.0/blob/master/examples/Bluetooth/Wii/Wii.ino>.

Also take a look at the blog post:

* <http://blog.tkjelectronics.dk/2012/08/wiimote-added-to-usb-host-library/>

All the information about the Wii controllers are from these sites:

* <http://wiibrew.org/wiki/Wiimote>
* <http://wiibrew.org/wiki/Wiimote/Extension_Controllers>
* <http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Nunchuck>
* <http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Wii_Motion_Plus>
* The old library created by _Tomoyuki Tanaka_: <https://github.com/moyuchin/WiiRemote_on_Arduino> also helped a lot.

# FAQ

> When I plug my device into the USB connector nothing happens?

Try to connect a external power supply to the Arduino - this solves the problem in most cases.