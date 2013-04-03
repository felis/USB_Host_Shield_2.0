#if !defined(__HIDJOYSTICKRPTPARSER_H__)
#define __HIDJOYSTICKRPTPARSER_H__

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "avrpins.h"
#include "max3421e.h"
#include "usbhost.h"
#include "usb_ch9.h"
#include "Usb.h"

#if defined(ARDUINO) && ARDUINO >=100
#include "Arduino.h"
#else
#include <WProgram.h>
#endif

#include "printhex.h"
#include "hexdump.h"
#include "message.h"
#include "confdescparser.h"
#include "hid.h"

struct GamePadEventData
{
	uint8_t X, Y, Z1, Z2, Rz;
};

class JoystickEvents
{
public:
	virtual void OnGamePadChanged(const GamePadEventData *evt);
	virtual void OnHatSwitch(uint8_t hat);
	virtual void OnButtonUp(uint8_t but_id);
	virtual void OnButtonDn(uint8_t but_id);
};

#define RPT_GEMEPAD_LEN		5

class JoystickReportParser : public HIDReportParser
{
	JoystickEvents		*joyEvents;

	uint8_t				oldPad[RPT_GEMEPAD_LEN];
	uint8_t				oldHat;
	uint16_t			oldButtons;

public:
	JoystickReportParser(JoystickEvents *evt);

	virtual void Parse(HID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

#endif // __HIDJOYSTICKRPTPARSER_H__
