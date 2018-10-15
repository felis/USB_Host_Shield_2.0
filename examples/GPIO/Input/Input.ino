/*  Example of reading USB Host Shield GPI input
    Author: Brian Walton (brian@riban.co.uk)
*/
#include <UHS2_gpio.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

#define INPUT_PIN 0
#define OUTPUT_PIN 0

USB Usb; // Create an UHS2 interface object
UHS2_GPIO Gpio(&Usb); // Create a GPIO object

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
  // Get the value of input, set value of output
  int nValue = Gpio.digitalRead(INPUT_PIN);
  nValue = (nValue ? LOW : HIGH);
  Gpio.digitalWrite(OUTPUT_PIN, nValue);
}

