/*  Example of reading and writing USB Host Shield GPI output using low-level functions
    This example uses low-level UHS interface. See Blink for example of using "Wiring" style interface
    Author: Brian Walton (brian@riban.co.uk)
*/
#include <Usb.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB     Usb;

void setup() {
  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
    Serial.println("OSC did not start.");

  delay( 200 );
}

void loop() {
  // Get the current output value, toggle then wait half a second
  uint8_t nGPO = Usb.gpioRdOutput();
  uint8_t nValue = ((nGPO & 0x01) == 0x01) ? 0 : 1;
  nGPO &= 0xFE; // Clear bit 0
  nGPO |= nValue;
  Usb.gpioWr(nGPO);
  Serial.print(nValue ? "+" : "."); // Debug to show what the output should be doing
  delay(500);
}

