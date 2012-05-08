
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

class HIDUniversal2 : public HIDUniversal
{
public:
    HIDUniversal2(USB *usb) : HIDUniversal(usb) {};
    
protected:
    virtual uint8_t OnInitSuccessful();
};

uint8_t HIDUniversal2::OnInitSuccessful()
{
    uint8_t    rcode;
    
    HexDumper<USBReadParser, uint16_t, uint16_t>    Hex;
    ReportDescParser                                Rpt;

    if (rcode = GetReportDescr(0, &Hex))
        goto FailGetReportDescr1;
	        
    if (rcode = GetReportDescr(0, &Rpt))
	goto FailGetReportDescr2;

    return 0;

FailGetReportDescr1:
    USBTRACE("GetReportDescr1:");
    goto Fail;

FailGetReportDescr2:
    USBTRACE("GetReportDescr2:");
    goto Fail;

Fail:
    Serial.println(rcode, HEX);
    Release();
    return rcode;
}

USB                                             Usb;
//USBHub                                          Hub(&Usb);
HIDUniversal2                                   Hid(&Usb);
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

