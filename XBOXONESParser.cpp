/* Copyright (C) 2020 Kristian Sloth Lauszus. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Sloth Lauszus
 Web      :  https://lauszus.com
 e-mail   :  lauszus@gmail.com
 */

#include "XBOXONESParser.h"

// To enable serial debugging see "settings.h"
//#define PRINTREPORT // Uncomment to print the report send by the Xbox One S Controller

/** Buttons on the controller */
const uint8_t XBOX_ONE_S_BUTTONS[] PROGMEM = {
        UP, // UP
        RIGHT, // RIGHT
        DOWN, // DOWN
        LEFT, // LEFT

        0x0E, // VIEW
        0x0F, // MENU
        0x10, // L3
        0x11, // R3

        0, 0, // Skip L2 and R2 as these are analog buttons
        0x0C, // L1
        0x0D, // R1

        0x09, // B
        0x08, // A
        0x0A, // X
        0x0B, // Y
        0, // XBOX - this is sent in another report
};

enum DPADEnum {
        DPAD_OFF = 0x0,
        DPAD_UP = 0x1,
        DPAD_UP_RIGHT = 0x2,
        DPAD_RIGHT = 0x3,
        DPAD_RIGHT_DOWN = 0x4,
        DPAD_DOWN = 0x5,
        DPAD_DOWN_LEFT = 0x6,
        DPAD_LEFT = 0x7,
        DPAD_LEFT_UP = 0x8,
};

int8_t XBOXONESParser::getButtonIndexXboxOneS(ButtonEnum b) {
        const int8_t index = ButtonIndex(b);
        if ((uint8_t) index >= (sizeof(XBOX_ONE_S_BUTTONS) / sizeof(XBOX_ONE_S_BUTTONS[0]))) return -1;
        return index;
}

bool XBOXONESParser::checkDpad(ButtonEnum b) {
        switch (b) {
                case UP:
                        return xboxOneSData.btn.dpad == DPAD_LEFT_UP || xboxOneSData.btn.dpad == DPAD_UP || xboxOneSData.btn.dpad == DPAD_UP_RIGHT;
                case RIGHT:
                        return xboxOneSData.btn.dpad == DPAD_UP_RIGHT || xboxOneSData.btn.dpad == DPAD_RIGHT || xboxOneSData.btn.dpad == DPAD_RIGHT_DOWN;
                case DOWN:
                        return xboxOneSData.btn.dpad == DPAD_RIGHT_DOWN || xboxOneSData.btn.dpad == DPAD_DOWN || xboxOneSData.btn.dpad == DPAD_DOWN_LEFT;
                case LEFT:
                        return xboxOneSData.btn.dpad == DPAD_DOWN_LEFT || xboxOneSData.btn.dpad == DPAD_LEFT || xboxOneSData.btn.dpad == DPAD_LEFT_UP;
                default:
                        return false;
        }
}

uint16_t XBOXONESParser::getButtonPress(ButtonEnum b) {
        const int8_t index = getButtonIndexXboxOneS(b); if (index < 0) return 0;
        if (index == ButtonIndex(L2))
                return xboxOneSData.trigger[0];
        else if (index == ButtonIndex(R2))
                return xboxOneSData.trigger[1];
        else if (index <= LEFT) // Dpad
                return checkDpad(b);
        else if (index == ButtonIndex(XBOX))
                return xboxButtonState;
        return xboxOneSData.btn.val & (1UL << pgm_read_byte(&XBOX_ONE_S_BUTTONS[index]));
}

bool XBOXONESParser::getButtonClick(ButtonEnum b) {
        const int8_t index = getButtonIndexXboxOneS(b); if (index < 0) return 0;
        if(index == ButtonIndex(L2)) {
                if(L2Clicked) {
                        L2Clicked = false;
                        return true;
                }
                return false;
        } else if(index == ButtonIndex(R2)) {
                if(R2Clicked) {
                        R2Clicked = false;
                        return true;
                }
                return false;
        } else if (index == ButtonIndex(XBOX)) {
                bool click = xboxbuttonClickState;
                xboxbuttonClickState = 0; // Clear "click" event
                return click;
        }
        uint32_t mask = 1UL << pgm_read_byte(&XBOX_ONE_S_BUTTONS[index]);
        bool click = buttonClickState.val & mask;
        buttonClickState.val &= ~mask; // Clear "click" event
        return click;
}

int16_t XBOXONESParser::getAnalogHat(AnalogHatEnum a) {
        return xboxOneSData.hatValue[(uint8_t)a] - 32768; // Convert to signed integer
}

void XBOXONESParser::Parse(uint8_t len, uint8_t *buf) {
        if (len > 1 && buf)  {
#ifdef PRINTREPORT
                Notify(PSTR("\r\n"), 0x80);
                for (uint8_t i = 0; i < len; i++) {
                        D_PrintHex<uint8_t > (buf[i], 0x80);
                        Notify(PSTR(" "), 0x80);
                }
#endif

                if (buf[0] == 0x01) // Check report ID
                        memcpy(&xboxOneSData, buf + 1, min((uint8_t)(len - 1), MFK_CASTUINT8T sizeof(xboxOneSData)));
                else if (buf[0] == 0x02) { // This report contains the Xbox button
                        xboxButtonState = buf[1];
                        if(xboxButtonState != xboxOldButtonState) {
                            xboxbuttonClickState = xboxButtonState & ~xboxOldButtonState; // Update click state variable
                            xboxOldButtonState = xboxButtonState;
                        }
                        return;
                } else if (buf[0] == 0x04) // Heartbeat
                        return;
                else {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nUnknown report id: "), 0x80);
                        D_PrintHex<uint8_t > (buf[0], 0x80);
#endif
                        return;
                }

                if (xboxOneSData.btn.val != oldButtonState.val) { // Check if anything has changed
                        buttonClickState.val = xboxOneSData.btn.val & ~oldButtonState.val; // Update click state variable
                        oldButtonState.val = xboxOneSData.btn.val;

                        // The DPAD buttons does not set the different bits, but set a value corresponding to the buttons pressed, we will simply set the bits ourself
                        uint8_t newDpad = 0;
                        if (checkDpad(UP))
                                newDpad |= 1 << UP;
                        if (checkDpad(RIGHT))
                                newDpad |= 1 << RIGHT;
                        if (checkDpad(DOWN))
                                newDpad |= 1 << DOWN;
                        if (checkDpad(LEFT))
                                newDpad |= 1 << LEFT;
                        if (newDpad != oldDpad) {
                                buttonClickState.dpad = newDpad & ~oldDpad; // Override values
                                oldDpad = newDpad;
                        }
                }

                // Handle click detection for triggers
                if(xboxOneSData.trigger[0] != 0 && triggerOld[0] == 0)
                        L2Clicked = true;
                triggerOld[0] = xboxOneSData.trigger[0];
                if(xboxOneSData.trigger[1] != 0 && triggerOld[1] == 0)
                        R2Clicked = true;
                triggerOld[1] = xboxOneSData.trigger[1];
        }
}

void XBOXONESParser::Reset() {
        uint8_t i;
        for (i = 0; i < sizeof(xboxOneSData.hatValue) / sizeof(xboxOneSData.hatValue[0]); i++)
                xboxOneSData.hatValue[i] = 32768; // Center value
        xboxOneSData.btn.val = 0;
        oldButtonState.val = 0;
        for (i = 0; i < sizeof(xboxOneSData.trigger) / sizeof(xboxOneSData.trigger[0]); i++)
                xboxOneSData.trigger[i] = 0;

        xboxOneSData.btn.dpad = DPAD_OFF;
        oldButtonState.dpad = DPAD_OFF;
        buttonClickState.dpad = 0;
        oldDpad = 0;
};

void XBOXONESParser::setRumbleOff() {
        // See: https://lore.kernel.org/patchwork/patch/973394/
        uint8_t buf[8];
        buf[0] = 0x0F; // Disable all rumble motors
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        buf[4] = 0;
        buf[5] = 0; // Duration of effect in 10 ms
        buf[6] = 0; // Start delay in 10 ms
        buf[7] = 0; // Loop count
        sendOutputReport(buf, sizeof(buf));
}

void XBOXONESParser::setRumbleOn(uint8_t leftTrigger, uint8_t rightTrigger, uint8_t leftMotor, uint8_t rightMotor) {
        // See: https://lore.kernel.org/patchwork/patch/973394/
        uint8_t buf[8];
        buf[0] = 1 << 3 /* Left trigger */ | 1 << 2 /* Right trigger */ | 1 << 1 /* Left motor */ | 1 << 0 /* Right motor */;
        buf[1] = leftTrigger;
        buf[2] = rightTrigger;
        buf[3] = leftMotor;
        buf[4] = rightMotor;
        buf[5] = 255; // Duration of effect in 10 ms
        buf[6] = 0; // Start delay in 10 ms
        buf[7] = 255; // Loop count
        sendOutputReport(buf, sizeof(buf));
}
