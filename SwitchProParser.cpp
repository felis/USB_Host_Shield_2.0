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
 */

#include "SwitchProParser.h"

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
//#define PRINTREPORT // Uncomment to print the report send by the Switch Pro Controller

int8_t SwitchProParser::getButtonIndexSwitchPro(ButtonEnum b) {
    const int8_t index = ButtonIndex(b);
    if ((uint8_t) index >= (sizeof(SWITCH_PRO_BUTTONS) / sizeof(SWITCH_PRO_BUTTONS[0]))) return -1;
    return index;
}

bool SwitchProParser::checkDpad(ButtonEnum b) {
        switch (b) {
                case UP:
                        return switchProData.btn.dpad == DPAD_LEFT_UP || switchProData.btn.dpad == DPAD_UP || switchProData.btn.dpad == DPAD_UP_RIGHT;
                case RIGHT:
                        return switchProData.btn.dpad == DPAD_UP_RIGHT || switchProData.btn.dpad == DPAD_RIGHT || switchProData.btn.dpad == DPAD_RIGHT_DOWN;
                case DOWN:
                        return switchProData.btn.dpad == DPAD_RIGHT_DOWN || switchProData.btn.dpad == DPAD_DOWN || switchProData.btn.dpad == DPAD_DOWN_LEFT;
                case LEFT:
                        return switchProData.btn.dpad == DPAD_DOWN_LEFT || switchProData.btn.dpad == DPAD_LEFT || switchProData.btn.dpad == DPAD_LEFT_UP;
                default:
                        return false;
        }
}

bool SwitchProParser::getButtonPress(ButtonEnum b) {
        const int8_t index = getButtonIndexSwitchPro(b); if (index < 0) return 0;
        if (index <= LEFT) // Dpad
                return checkDpad(b);
        else
                return switchProData.btn.val & (1UL << pgm_read_byte(&SWITCH_PRO_BUTTONS[index]));
}

bool SwitchProParser::getButtonClick(ButtonEnum b) {
        const int8_t index = getButtonIndexSwitchPro(b); if (index < 0) return 0;
        uint32_t mask = 1UL << pgm_read_byte(&SWITCH_PRO_BUTTONS[index]);
        bool click = buttonClickState.val & mask;
        buttonClickState.val &= ~mask; // Clear "click" event
        return click;
}

int16_t SwitchProParser::getAnalogHat(AnalogHatEnum a) {
        return switchProData.hatValue[(uint8_t)a] - 0x7FFF; // Subtract the center value
}

void SwitchProParser::Parse(uint8_t len, uint8_t *buf) {
        if (len > 1 && buf)  {
#ifdef PRINTREPORT
                Notify(PSTR("\r\nLen: "), 0x80); Notify(len, 0x80);
                Notify(PSTR(", data: "), 0x80);
                for (uint8_t i = 0; i < len; i++) {
                        D_PrintHex<uint8_t > (buf[i], 0x80);
                        Notify(PSTR(" "), 0x80);
                }
#endif

                if (buf[0] == 0x3F) // Simple input report
                        memcpy(&switchProData, buf + 1, min((uint8_t)(len - 1), MFK_CASTUINT8T sizeof(switchProData)));
                else if (buf[0] == 0x21) // Subcommand reply
                        return;
                else {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nUnknown report id: "), 0x80);
                        D_PrintHex<uint8_t > (buf[0], 0x80);
                        Notify(PSTR(", len: "), 0x80);
                        D_PrintHex<uint8_t > (len, 0x80);
#endif
                        return;
                }

                // Workaround issue with the controller sending invalid joystick values when it is connected
                for (uint8_t i = 0; i < sizeof(switchProData.hatValue) / sizeof(switchProData.hatValue[0]); i++) {
                        if (switchProData.hatValue[i] < 1000 || switchProData.hatValue[i] > 0xFFFF - 1000)
                                switchProData.hatValue[i] = 0x7FFF; // Center value
                }

                if (switchProData.btn.val != oldButtonState.val) { // Check if anything has changed
                        buttonClickState.val = switchProData.btn.val & ~oldButtonState.val; // Update click state variable
                        oldButtonState.val = switchProData.btn.val;

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

        if (switchProOutput.ledReportChanged || switchProOutput.ledHomeReportChanged)
                sendLedOutputReport(); // Send output report
        else if (switchProOutput.leftRumbleOn || switchProOutput.rightRumbleOn) {
                // We need to send the rumble report repeatedly to keep it on
                uint32_t now = millis();
                if  (now - rumble_on_timer > 1000) {
                        rumble_on_timer = now;
                        sendRumbleOutputReport();
                }
        } else
                rumble_on_timer = 0;
}

void SwitchProParser::sendLedOutputReport() {
        // See: https://github.com/Dan611/hid-procon
        //      https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering
        //      https://github.com/HisashiKato/USB_Host_Shield_Library_2.0_BTXBOX/blob/master/src/SWProBTParser.h#L152-L153
        uint8_t buf[14] = { 0 };
        buf[0x00] = 0x01; // Report ID - PROCON_CMD_AND_RUMBLE
        buf[0x01] = output_sequence_counter++; // Lowest 4-bit is a sequence number, which needs to be increased for every report

        // Left rumble data
        if (switchProOutput.leftRumbleOn) {
                buf[0x02 + 0] = 0x28;
                buf[0x02 + 1] = 0x88;
                buf[0x02 + 2] = 0x60;
                buf[0x02 + 3] = 0x61;
        } else {
                buf[0x02 + 0] = 0x00;
                buf[0x02 + 1] = 0x01;
                buf[0x02 + 2] = 0x40;
                buf[0x02 + 3] = 0x40;
        }

        // Right rumble data
        if (switchProOutput.rightRumbleOn) {
                buf[0x02 + 4] = 0x28;
                buf[0x02 + 5] = 0x88;
                buf[0x02 + 6] = 0x60;
                buf[0x02 + 7] = 0x61;
        } else {
                buf[0x02 + 4] = 0x00;
                buf[0x02 + 5] = 0x01;
                buf[0x02 + 6] = 0x40;
                buf[0x02 + 7] = 0x40;
        }

        // Sub commands
        if (switchProOutput.ledReportChanged) {
                switchProOutput.ledReportChanged = false;

                // See: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_subcommands_notes.md#subcommand-0x30-set-player-lights
                buf[0x0A + 0] = 0x30; // PROCON_CMD_LED

                buf[0x0A + 1] = switchProOutput.ledMask; // Lower 4-bits sets the LEDs constantly on, the higher 4-bits can be used to flash the LEDs

                sendOutputReport(buf, 10 + 2);
        } else if (switchProOutput.ledHomeReportChanged) {
                 switchProOutput.ledHomeReportChanged = false;

                // It is possible set up to 15 mini cycles, but we simply just set the LED constantly on/off
                // See: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_subcommands_notes.md#subcommand-0x38-set-home-light
                buf[0x0A + 0] = 0x38; // PROCON_CMD_LED_HOME

                buf[0x0A + 1] = (0 /* Number of cycles */ << 4) | (switchProOutput.ledHome ? 0xF : 0) /* Global mini cycle duration */;
                buf[0x0A + 2] = (0xF /* LED start intensity */ << 4) | 0x0 /* Number of full cycles */;
                buf[0x0A + 3] = (0xF /* Mini Cycle 1 LED intensity */ << 4) | 0x0 /* Mini Cycle 2 LED intensity */;

                sendOutputReport(buf, 10 + 4);
        }
}

void SwitchProParser::sendRumbleOutputReport() {
        // See: https://github.com/Dan611/hid-procon
        //      https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering
        //      https://github.com/HisashiKato/USB_Host_Shield_Library_2.0_BTXBOX/blob/master/src/SWProBTParser.h#L152-L153

        uint8_t buf[10] = { 0 };
        buf[0x00] = 0x10; // Report ID - PROCON_CMD_RUMBLE_ONLY
        buf[0x01] = output_sequence_counter++; // Lowest 4-bit is a sequence number, which needs to be increased for every report

        // Left rumble data
        if (switchProOutput.leftRumbleOn) {
                buf[0x02 + 0] = 0x28;
                buf[0x02 + 1] = 0x88;
                buf[0x02 + 2] = 0x60;
                buf[0x02 + 3] = 0x61;
        } else {
                buf[0x02 + 0] = 0x00;
                buf[0x02 + 1] = 0x01;
                buf[0x02 + 2] = 0x40;
                buf[0x02 + 3] = 0x40;
        }

        // Right rumble data
        if (switchProOutput.rightRumbleOn) {
                buf[0x02 + 4] = 0x28;
                buf[0x02 + 5] = 0x88;
                buf[0x02 + 6] = 0x60;
                buf[0x02 + 7] = 0x61;
        } else {
                buf[0x02 + 4] = 0x00;
                buf[0x02 + 5] = 0x01;
                buf[0x02 + 6] = 0x40;
                buf[0x02 + 7] = 0x40;
        }

        sendOutputReport(buf, 10);
}

void SwitchProParser::Reset() {
        for (uint8_t i = 0; i < sizeof(switchProData.hatValue) / sizeof(switchProData.hatValue[0]); i++)
                switchProData.hatValue[i] = 0x7FFF; // Center value
        switchProData.btn.val = 0;
        oldButtonState.val = 0;

        switchProData.btn.dpad = DPAD_OFF;
        oldButtonState.dpad = DPAD_OFF;
        buttonClickState.dpad = 0;
        oldDpad = 0;
        output_sequence_counter = 0;
        rumble_on_timer = 0;

        switchProOutput.leftRumbleOn = false;
        switchProOutput.rightRumbleOn = false;
        switchProOutput.ledMask = 0;
        switchProOutput.ledHome = false;
        switchProOutput.ledReportChanged = false;
        switchProOutput.ledHomeReportChanged = false;
};
