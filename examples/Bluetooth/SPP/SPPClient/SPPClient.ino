/*
 Example sketch for the RFCOMM/SPP Client Bluetooth library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <SPPi.h>
#include <usbhub.h>
// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

USB Usb;
//USBHub Hub1(&Usb); // Some dongles have a hub inside

BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so

uint8_t addr[6] = { 0x71, 0xB4, 0xB0, 0xC8, 0xBC, 0xC8 }; // Set this to the Bluetooth address you want to connect to
SPPi SerialBT(&Btd, true, addr);

boolean firstMessage = true;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  if (Usb.Init() == -1) {
    Serial.println(F("OSC did not start"));
    while (1); // Halt
  }
  Serial.print(F("\r\nSPP Client Started"));
}

void loop() {
  Usb.Task(); // The SPP data is actually not send until this is called, one could call SerialBT.send() directly as well

  if (SerialBT.connected) {
    if (firstMessage) {
      firstMessage = false;
      SerialBT.println(F("Hello from Arduino SPP client")); // Send welcome message
    }
    while (Serial.available())
      SerialBT.write(Serial.read());
    while (SerialBT.available())
      Serial.write(SerialBT.read());
  } else
    firstMessage = true;
}
