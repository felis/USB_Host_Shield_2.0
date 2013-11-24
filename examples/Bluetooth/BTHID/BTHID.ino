/*
 Example sketch for the HID Bluetooth library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <BTHID.h>
#include <usbhub.h>

USB Usb;
//USBHub Hub1(&Usb); // Some dongles have a hub inside

BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so
/* You can create the instance of the class in two ways */
BTHID hid(&Btd, PAIR, "0000"); // This will start an inquiry and then pair with your Wiimote - you only have to do this once
//BTHID hid(&Btd); // After that you can simply create the instance like so and then press any button on the Wiimote

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); //halt
  }
  Serial.print(F("\r\nHID Bluetooth Library Started"));
}
void loop() {
  Usb.Task();
  if (hid.connected) {
    if (hid.getButtonClick(LEFT)) // Print mouse buttons
      Serial.println(F("Left"));
    if (hid.getButtonClick(RIGHT))
      Serial.println(F("Right"));
    if (hid.getButtonClick(MIDDLE))
      Serial.println(F("Middle"));
  }
}
