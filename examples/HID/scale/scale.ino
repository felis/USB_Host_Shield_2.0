/* Digital Scale Output. Written for Stamps.com Model 510  */
/* 5lb Digital Scale; any HID scale with Usage page 0x8d should work */

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

#include "scale_rptparser.h"

#include <printhex.h>
#include <message.h>
#include <hexdump.h>
#include <parsetools.h>

USB                                             Usb;
USBHub                                          Hub(&Usb);
HIDUniversal                                    Hid(&Usb);
Max_LCD                                       LCD(&Usb);
ScaleEvents                                  ScaleEvents(&LCD);
ScaleReportParser                            Scale(&ScaleEvents);

void setup()
{
  Serial.begin( 115200 );
  Serial.println("Start");

  if (Usb.Init() == -1)
      Serial.println("OSC did not start.");
      
    // set up the LCD's number of rows and columns: 
    LCD.begin(16, 2);
    LCD.clear();
    LCD.home();
    LCD.setCursor(0,0);
    LCD.write('R');  
      
  delay( 200 );

  if (!Hid.SetReportParser(0, &Scale))
      ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  ); 
}

void loop()
{
    Usb.Task();
}

