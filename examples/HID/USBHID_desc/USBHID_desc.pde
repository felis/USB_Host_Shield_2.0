#include <avr/pgmspace.h>

#include <avrpins.h>
#include <max3421e.h>
#include <usbhost.h>
#include <usb_ch9.h>
#include <Usb.h>
#include <usbhub.h>
#include <avr/pgmspace.h>
#include <address.h>
#include <hid.h>
#include <hiduniversal.h>
#include <hidescriptorparser.h>
#include <printhex.h>
#include <message.h>
#include <hexdump.h>
#include <parsetools.h>

#include "pgmstrings.h"  

USB                                             Usb;
USBHub                                          Hub(&Usb);
HIDUniversal                                    Hid(&Usb);
UniversalReportParser                           Uni;

void setup()
{
  Serial.begin( 115200 );
  Serial.println("Start");

  if (Usb.Init() == -1)
      Serial.println("OSC did not start.");
      
  delay( 200 );

  if (!Hid.SetReportParser(0, &Uni))
      ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  ); 
}

void loop()
{
    Usb.Task();
}

