/* Copyright (C) 2014 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#include "PS4BT.h"

// To enable serial debugging see "settings.h"
//#define PRINTREPORT // Uncomment to print the report send by the PS4 Controller

/** Buttons on the controller */
const uint8_t PS4_BUTTONS[] PROGMEM = {
        DPAD_UP, // UP
        DPAD_RIGHT, // RIGHT
        DPAD_DOWN, // DOWN
        DPAD_LEFT, // LEFT

        0x0C, // SHARE
        0x0D, // OPTIONS
        0x0E, // L3
        0x0F, // R3

        0x0A, // L2
        0x0B, // R2
        0x08, // L1
        0x09, // R1

        0x07, // TRIANGLE
        0x06, // CIRCLE
        0x05, // CROSS
        0x04, // SQUARE

        0x10, // PS
        0x11, // KEYPAD
};

/** Analog buttons on the controller */
const uint8_t PS4_ANALOG_BUTTONS[] PROGMEM = {
        0, 0, 0, 0, 0, 0, 0, 0, // Skip UP_ANALOG, RIGHT_ANALOG, DOWN_ANALOG, LEFT_ANALOG, SELECT, L3, R3 and START
        0, // L2_ANALOG
        1, // R2_ANALOG
};

bool PS4BT::checkDpad(PS4Buttons ps4Buttons, DPADEnum b) {
	return ps4Buttons.dpad == b;
}

bool PS4BT::getButtonPress(ButtonEnum b) {
	uint8_t button = pgm_read_byte(&PS4_BUTTONS[(uint8_t)b]);
	if (b < 4) // Dpad
		return checkDpad(ps4Data.btn, (DPADEnum)button);
	else {
		uint8_t index = button < 8 ? 0 : button < 16 ? 1 : 2;
		uint8_t mask = (1 << (button - 8 * index));
		return ps4Data.btn.val[index] & mask;
	}
}

bool PS4BT::getButtonClick(ButtonEnum b) {
	uint8_t button = pgm_read_byte(&PS4_BUTTONS[(uint8_t)b]);
	if (b < 4) { // Dpad
		if (checkDpad(buttonClickState, (DPADEnum)button)) {
			buttonClickState.dpad = DPAD_OFF;
			return true;
		}
		return false;
	} else {
		uint8_t index = button < 8 ? 0 : button < 16 ? 1 : 2;
		uint8_t mask = (1 << (button - 8 * index));

		bool click = buttonClickState.val[index] & mask;
		buttonClickState.val[index] &= ~mask; // Clear "click" event
		return click;
	}
}

uint8_t PS4BT::getAnalogButton(ButtonEnum a) {
	return (uint8_t)(ps4Data.trigger[pgm_read_byte(&PS4_ANALOG_BUTTONS[(uint8_t)a])]);
}

uint8_t PS4BT::getAnalogHat(AnalogHatEnum a) {
	return ps4Data.hatValue[(uint8_t)a];
}

void PS4BT::Parse(HID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
	if (len == sizeof(PS4Data) && buf)  {
		memcpy(&ps4Data, buf, len);

		for (uint8_t i = 0; i < sizeof(ps4Data.btn); i++) {
			if (ps4Data.btn.val[i] != oldButtonState.val[i]) {
				buttonClickState.val[i] = ps4Data.btn.val[i] & ~oldButtonState.val[i]; // Update click state variable
				oldButtonState.val[i] = ps4Data.btn.val[i];
				if (i == 0)
					buttonClickState.dpad = ps4Data.btn.dpad; // The DPAD buttons does not set the different bits, but set a value corresponding to the buttons pressed
			}
		}
#ifdef PRINTREPORT
		for (uint8_t i = 0; i < len; i++) {
			D_PrintHex<uint8_t > (buf[i], 0x80);
			Notify(PSTR(" "), 0x80);
		}
		Notify(PSTR("\r\n"), 0x80);
#endif
	}
}