/*
 Example sketch for the AMBX library - developed by Aran Vink <aranvink@gmail.com>
 */

#include <AMBX.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB Usb;
AMBX AMBX(&Usb); // This will just create the instance

bool printAngle;
uint8_t state = 0;

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); //halt
  }
  Serial.print(F("\r\nAMBX USB Library Started"));
}
void loop() {
  Usb.Task();

  if (AMBX.AMBXConnected) { // One can only set the color of the bulb, set the rumble, set and get the bluetooth address and calibrate the magnetometer via USB
    if (state == 0) {

    } else if (state == 1) {
      AMBX.setAllLights(Red);
    } else if (state == 2) {
      AMBX.setAllLights(Green);
    } else if (state == 3) {
      AMBX.setAllLights(Blue);
    } else if (state == 4) {
      AMBX.setAllLights(White);
    }

    //Example using single light:
    //AMBX.setLight(Wallwasher_center, White);

    state++;
    if (state > 4)
      state = 0;
    delay(1000);
  }
  delay(10);
}
