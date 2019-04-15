/* Simplified Thrustmaster T.16000M FCS Joystick Report Parser */

#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

// Thrustmaster T.16000M HID report
struct GamePadEventData
{
  uint16_t	buttons;
  uint8_t		hat;
  uint16_t	x;
  uint16_t	y;
  uint8_t		twist;
  uint8_t		slider;
}__attribute__((packed));

class JoystickEvents
{
public:
	virtual void OnGamePadChanged(const GamePadEventData *evt);
};

#define RPT_GAMEPAD_LEN	sizeof(GamePadEventData)

class JoystickReportParser : public HIDReportParser
{
	JoystickEvents		*joyEvents;

  uint8_t oldPad[RPT_GAMEPAD_LEN];

public:
	JoystickReportParser(JoystickEvents *evt);

	virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};


JoystickReportParser::JoystickReportParser(JoystickEvents *evt) :
	joyEvents(evt)
{}

void JoystickReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf)
{
	// Checking if there are changes in report since the method was last called
  bool match = (sizeof(oldPad) == len) && (memcmp(oldPad, buf, len) == 0);

  // Calling Game Pad event handler
	if (!match && joyEvents) {
		joyEvents->OnGamePadChanged((const GamePadEventData*)buf);
    memcpy(oldPad, buf, len);
	}
}

void JoystickEvents::OnGamePadChanged(const GamePadEventData *evt)
{
	Serial.print("X: ");
	PrintHex<uint16_t>(evt->x, 0x80);
	Serial.print(" Y: ");
	PrintHex<uint16_t>(evt->y, 0x80);
	Serial.print(" Hat Switch: ");
	PrintHex<uint8_t>(evt->hat, 0x80);
	Serial.print(" Twist: ");
	PrintHex<uint8_t>(evt->twist, 0x80);
	Serial.print(" Slider: ");
	PrintHex<uint8_t>(evt->slider, 0x80);
  Serial.print(" Buttons: ");
	PrintHex<uint16_t>(evt->buttons, 0x80);
	Serial.println();
}

USB                                             Usb;
USBHub                                          Hub(&Usb);
HIDUniversal                                    Hid(&Usb);
JoystickEvents                                  JoyEvents;
JoystickReportParser                            Joy(&JoyEvents);

void setup()
{
  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
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
