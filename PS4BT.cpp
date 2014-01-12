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
        0x11, // TOUCHPAD
};

/** Analog buttons on the controller */
const uint8_t PS4_ANALOG_BUTTONS[] PROGMEM = {
        0, 0, 0, 0, 0, 0, 0, 0, // Skip UP, RIGHT, DOWN, LEFT, SHARE, OPTIONS, L3, and R3
        0, // L2
        1, // R2
};

bool PS4BT::checkDpad(DPADEnum b) {
	switch (b) {
		case DPAD_UP:
			return ps4Data.btn.dpad == DPAD_LEFT_UP || ps4Data.btn.dpad == DPAD_UP || ps4Data.btn.dpad == DPAD_UP_RIGHT;
		case DPAD_RIGHT:
			return ps4Data.btn.dpad == DPAD_UP_RIGHT || ps4Data.btn.dpad == DPAD_RIGHT || ps4Data.btn.dpad == DPAD_RIGHT_DOWN;
		case DPAD_DOWN:
			return ps4Data.btn.dpad == DPAD_RIGHT_DOWN || ps4Data.btn.dpad == DPAD_DOWN || ps4Data.btn.dpad == DPAD_DOWN_LEFT;
		case DPAD_LEFT:
			return ps4Data.btn.dpad == DPAD_DOWN_LEFT || ps4Data.btn.dpad == DPAD_LEFT || ps4Data.btn.dpad == DPAD_LEFT_UP;
		default:
			return false;
	}
}

bool PS4BT::getButtonPress(ButtonEnum b) {
	uint8_t button = pgm_read_byte(&PS4_BUTTONS[(uint8_t)b]);
	if (b <= LEFT) // Dpad
		return checkDpad((DPADEnum)button);
	else {
		uint8_t index = button < 8 ? 0 : button < 16 ? 1 : 2;
		uint8_t mask = (1 << (button - 8 * index));
		return ps4Data.btn.val[index] & mask;
	}
}

bool PS4BT::getButtonClick(ButtonEnum b) {
	uint8_t mask, index = 0;
	if (b <= LEFT) // Dpad
		mask = 1 << b;
	else {
		uint8_t button = pgm_read_byte(&PS4_BUTTONS[(uint8_t)b]);
		index = button < 8 ? 0 : button < 16 ? 1 : 2;
		mask = (1 << (button - 8 * index));
	}

	bool click = buttonClickState.val[index] & mask;
	buttonClickState.val[index] &= ~mask; // Clear "click" event
	return click;
}

uint8_t PS4BT::getAnalogButton(ButtonEnum a) {
	return ps4Data.trigger[pgm_read_byte(&PS4_ANALOG_BUTTONS[(uint8_t)a])];
}

uint8_t PS4BT::getAnalogHat(AnalogHatEnum a) {
	return ps4Data.hatValue[(uint8_t)a];
}

void PS4BT::Parse(HID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
	if (len == sizeof(PS4Data) && buf)  {
		memcpy(&ps4Data, buf, len);

		for (uint8_t i = 0; i < sizeof(ps4Data.btn); i++) {
			if (ps4Data.btn.val[i] != oldButtonState.val[i]) { // Check if anything has changed
				buttonClickState.val[i] = ps4Data.btn.val[i] & ~oldButtonState.val[i]; // Update click state variable
				oldButtonState.val[i] = ps4Data.btn.val[i];
				if (i == 0) { // The DPAD buttons does not set the different bits, but set a value corresponding to the buttons pressed, we will simply set the bits ourself
					uint8_t newDpad = 0;
					if (checkDpad(DPAD_UP))
						newDpad |= 1 << UP;
					if (checkDpad(DPAD_RIGHT))
						newDpad |= 1 << RIGHT;
					if (checkDpad(DPAD_DOWN))
						newDpad |= 1 << DOWN;
					if (checkDpad(DPAD_LEFT))
						newDpad |= 1 << LEFT;
					if (newDpad != oldDpad) {
						buttonClickState.dpad = newDpad & ~oldDpad; // Override values
						oldDpad = newDpad;
					}
				}
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