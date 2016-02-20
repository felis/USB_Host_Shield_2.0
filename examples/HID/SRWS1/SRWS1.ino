/*
 Example sketch for the SteelSeries SRW-S1 Steering Wheel - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <SPI.h>
#include "SRWS1.h"

USB Usb;
SRWS1 srw1(&Usb);

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); // Halt
  }
  Serial.println(F("\r\nSteelSeries SRW-S1 Steering Wheel example started"));
}

void loop() {
  Usb.Task();

  if (srw1.connected()) {
#if 1 // Set to 1 in order to show a crazy strobe light effect
    static uint32_t timer;
    if (millis() - timer > 12) {
      timer = millis(); // Reset timer
      static uint16_t leds = 0;

      /*D_PrintHex<uint16_t > (leds, 0x80);
      Serial.println();*/
      srw1.setLeds(leds); // Update LEDs

      static bool dirUp = true;
      if (dirUp) {
        leds <<= 1;
        if (leds == 0x8000) // All are actually turned off, as there is only 15 LEDs
          dirUp = false; // If we have reached the end i.e. all LEDs are off, then change direction
        if (!(leds & 0x8000)) // If last bit is not set set the lowest bit
          leds |= 1; // Set lowest bit
      } else {
        leds >>= 1;
        if (leds == 0) // Check if all LEDs are off
          dirUp = true; // If all LEDs are off, then repeat the sequence
        if (!(leds & 0x1)) // If last bit is not set set the lowest bit
          leds |= 1 << 15; // Set top bit
      }
    }
#endif
  }
}

