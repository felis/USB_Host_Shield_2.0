#include <avr/pgmspace.h>
#include <avrpins.h>
#include <max3421e.h>
#include <usbhost.h>
#include <usb_ch9.h>
#include <Usb.h>
#include <usbhub.h>
#include <avr/pgmspace.h>
#include <address.h>
#include <hidboot.h>

#include <printhex.h>
#include <message.h>
#include <hexdump.h>
#include <parsetools.h>

class MouseRptParser : public MouseReportParser
{
protected:
	virtual void OnMouseMove	(MOUSEINFO *mi);
	virtual void OnLeftButtonUp	(MOUSEINFO *mi);
	virtual void OnLeftButtonDown	(MOUSEINFO *mi);
	virtual void OnRightButtonUp	(MOUSEINFO *mi);
	virtual void OnRightButtonDown	(MOUSEINFO *mi);
	virtual void OnMiddleButtonUp	(MOUSEINFO *mi);
	virtual void OnMiddleButtonDown	(MOUSEINFO *mi);
};
void MouseRptParser::OnMouseMove(MOUSEINFO *mi)	
{
    Serial.print("dx=");
    Serial.print(mi->dX, DEC);
    Serial.print(" dy=");
    Serial.println(mi->dY, DEC);
};
void MouseRptParser::OnLeftButtonUp	(MOUSEINFO *mi)	
{
    Serial.println("L Butt Up");
};
void MouseRptParser::OnLeftButtonDown	(MOUSEINFO *mi)
{
    Serial.println("L Butt Dn");
};
void MouseRptParser::OnRightButtonUp	(MOUSEINFO *mi)
{
    Serial.println("R Butt Up");
};
void MouseRptParser::OnRightButtonDown	(MOUSEINFO *mi)
{
    Serial.println("R Butt Dn");
};
void MouseRptParser::OnMiddleButtonUp	(MOUSEINFO *mi)
{
    Serial.println("M Butt Up");
};
void MouseRptParser::OnMiddleButtonDown	(MOUSEINFO *mi)
{
    Serial.println("M Butt Dn");
};

USB     Usb;
//USBHub     Hub(&Usb);
HIDBoot<HID_PROTOCOL_MOUSE>    Mouse(&Usb);

uint32_t next_time;

MouseRptParser Prs;

void setup()
{
    Serial.begin( 115200 );
    Serial.println("Start");

    if (Usb.Init() == -1)
        Serial.println("OSC did not start.");
      
    delay( 200 );
  
    next_time = millis() + 5000;
  
    Mouse.SetReportParser(0, (HIDReportParser*)&Prs);
}

void loop()
{
  Usb.Task();
}

