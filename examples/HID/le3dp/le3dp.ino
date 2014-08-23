/* Simplified Logitech Extreme 3D Pro Joystick Report Parser */

#include <hid.h>
#include <hiduniversal.h>
#include <usbhub.h>

#include "le3dp_rptparser.h"
// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

USB                                             Usb;
USBHub                                          Hub(&Usb);
HIDUniversal                                    Hid(&Usb);
JoystickEvents                                  JoyEvents;
JoystickReportParser                            Joy(&JoyEvents);

void setup()
{
  Serial.begin( 115200 );
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  Serial.println("Start");

  if (Usb.Init() == -1)
      Serial.println("OSC did not start.");

  delay( 200 );

  if (!Hid.SetReportParser(0, &Joy))
      ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  );
}

void loop()
{
    Usb.Task();
}

