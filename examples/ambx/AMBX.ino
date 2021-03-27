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

uint8_t state = 0;
const long interval = 1000;
unsigned long previousMillis = 0;

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
    
    if (AMBX.AMBXConnected) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        
        if (state > 4) {
          state = 0;
        }
        if (state == 0) {
          Serial.print(F("\r\nRed"));
          AMBX.setAllLights(Red);
        } else if (state == 1) {
          Serial.print(F("\r\nGreen"));
          AMBX.setAllLights(Green);
        } else if (state == 2) {
          Serial.print(F("\r\nBlue"));
          AMBX.setAllLights(Blue);
        } else if (state == 3) {
          Serial.print(F("\r\nWhite"));
          AMBX.setAllLights(White);
        }
        state++;
      }
    }
  //Example using single light:
  //AMBX.setLight(Wallwasher_center, White);
}
