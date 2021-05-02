/* Copyright (C) 2021 Kristian Sloth Lauszus. All rights reserved.

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

 Thanks to Joseph Duchesne for the initial code.
 Based on Ludwig Füchsl's https://github.com/Ohjurot/DualSense-Windows PS5 port
 and the series of patches found here: https://patchwork.kernel.org/project/linux-input/cover/20201219062336.72568-1-roderick@gaikai.com/
 */

#include "PS5Parser.h"

enum DPADEnum {
        DPAD_UP = 0x0,
        DPAD_UP_RIGHT = 0x1,
        DPAD_RIGHT = 0x2,
        DPAD_RIGHT_DOWN = 0x3,
        DPAD_DOWN = 0x4,
        DPAD_DOWN_LEFT = 0x5,
        DPAD_LEFT = 0x6,
        DPAD_LEFT_UP = 0x7,
        DPAD_OFF = 0x8,
};

// To enable serial debugging see "settings.h"
//#define PRINTREPORT // Uncomment to print the report send by the PS5 Controller

int8_t PS5Parser::getButtonIndexPS5(ButtonEnum b) {
    const int8_t index = ButtonIndex(b);
    if ((uint8_t) index >= (sizeof(PS5_BUTTONS) / sizeof(PS5_BUTTONS[0]))) return -1;
    return index;
}

bool PS5Parser::checkDpad(ButtonEnum b) {
        switch (b) {
                case UP:
                        return ps5Data.btn.dpad == DPAD_LEFT_UP || ps5Data.btn.dpad == DPAD_UP || ps5Data.btn.dpad == DPAD_UP_RIGHT;
                case RIGHT:
                        return ps5Data.btn.dpad == DPAD_UP_RIGHT || ps5Data.btn.dpad == DPAD_RIGHT || ps5Data.btn.dpad == DPAD_RIGHT_DOWN;
                case DOWN:
                        return ps5Data.btn.dpad == DPAD_RIGHT_DOWN || ps5Data.btn.dpad == DPAD_DOWN || ps5Data.btn.dpad == DPAD_DOWN_LEFT;
                case LEFT:
                        return ps5Data.btn.dpad == DPAD_DOWN_LEFT || ps5Data.btn.dpad == DPAD_LEFT || ps5Data.btn.dpad == DPAD_LEFT_UP;
                default:
                        return false;
        }
}

bool PS5Parser::getButtonPress(ButtonEnum b) {
        const int8_t index = getButtonIndexPS5(b); if (index < 0) return 0;
        if (index <= LEFT) // Dpad
                return checkDpad(b);
        else
                return ps5Data.btn.val & (1UL << pgm_read_byte(&PS5_BUTTONS[index]));
}

bool PS5Parser::getButtonClick(ButtonEnum b) {
        const int8_t index = getButtonIndexPS5(b); if (index < 0) return 0;
        uint32_t mask = 1UL << pgm_read_byte(&PS5_BUTTONS[index]);
        bool click = buttonClickState.val & mask;
        buttonClickState.val &= ~mask; // Clear "click" event
        return click;
}

uint8_t PS5Parser::getAnalogButton(ButtonEnum b) {
        const int8_t index = getButtonIndexPS5(b); if (index < 0) return 0;
        if (index == ButtonIndex(L2)) // These are the only analog buttons on the controller
                return ps5Data.trigger[0];
        else if (index == ButtonIndex(R2))
                return ps5Data.trigger[1];
        return 0;
}

uint8_t PS5Parser::getAnalogHat(AnalogHatEnum a) {
        return ps5Data.hatValue[(uint8_t)a];
}

void PS5Parser::Parse(uint8_t len, uint8_t *buf) {
        if (len > 1 && buf)  {
#ifdef PRINTREPORT
                Notify(PSTR("\r\nLen: "), 0x80); Notify(len, 0x80);
                Notify(PSTR(", data: "), 0x80);
                for (uint8_t i = 0; i < len; i++) {
                        D_PrintHex<uint8_t > (buf[i], 0x80);
                        Notify(PSTR(" "), 0x80);
                }
#endif

                if (buf[0] == 0x01) // Check report ID
                        memcpy(&ps5Data, buf + 1, min((uint8_t)(len - 1), MFK_CASTUINT8T sizeof(ps5Data)));
                else if (buf[0] == 0x31) { // This report is send via Bluetooth, it has an offset of 1 compared to the USB data
                        if (len < 3) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nReport is too short: "), 0x80);
                                D_PrintHex<uint8_t > (len, 0x80);
#endif
                                return;
                        }
                        memcpy(&ps5Data, buf + 2, min((uint8_t)(len - 2), MFK_CASTUINT8T sizeof(ps5Data)));
                } else {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nUnknown report id: "), 0x80);
                        D_PrintHex<uint8_t > (buf[0], 0x80);
                        Notify(PSTR(", len: "), 0x80);
                        D_PrintHex<uint8_t > (len, 0x80);
#endif
                        return;
                }

                if (ps5Data.btn.val != oldButtonState.val) { // Check if anything has changed
                        buttonClickState.val = ps5Data.btn.val & ~oldButtonState.val; // Update click state variable
                        oldButtonState.val = ps5Data.btn.val;

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

                message_counter++;
        }

        if (ps5Output.reportChanged || leftTrigger.reportChanged || rightTrigger.reportChanged)
                sendOutputReport(&ps5Output); // Send output report
}


void PS5Parser::Reset() {
        uint8_t i;
        for (i = 0; i < sizeof(ps5Data.hatValue); i++)
                ps5Data.hatValue[i] = 127; // Center value
        ps5Data.btn.val = 0;
        oldButtonState.val = 0;
        for (i = 0; i < sizeof(ps5Data.trigger); i++)
                ps5Data.trigger[i] = 0;
        for (i = 0; i < sizeof(ps5Data.xy.finger)/sizeof(ps5Data.xy.finger[0]); i++)
                ps5Data.xy.finger[i].touching = 1; // The bit is cleared if the finger is touching the touchpad

        ps5Data.btn.dpad = DPAD_OFF;
        oldButtonState.dpad = DPAD_OFF;
        buttonClickState.dpad = 0;
        oldDpad = 0;

        leftTrigger.Reset();
        rightTrigger.Reset();

        ps5Output.bigRumble = ps5Output.smallRumble = 0;
        ps5Output.microphoneLed = 0;
        ps5Output.disableLeds = 0;
        ps5Output.playerLeds = 0;
        ps5Output.r = ps5Output.g = ps5Output.b = 0;
        ps5Output.reportChanged = false;
};
