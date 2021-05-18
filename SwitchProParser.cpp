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

// To enable serial debugging see "settings.h"
//#define PRINTREPORT // Uncomment to print the report send by the Switch Pro Controller

int8_t SwitchProParser::getButtonIndexSwitchPro(ButtonEnum b) {
    const int8_t index = ButtonIndex(b);
    if ((uint8_t) index >= (sizeof(SWITCH_PRO_BUTTONS) / sizeof(SWITCH_PRO_BUTTONS[0]))) return -1;
    return index;
}

bool SwitchProParser::getButtonPress(ButtonEnum b) {
        const int8_t index = getButtonIndexSwitchPro(b); if (index < 0) return 0;
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
        switch((uint8_t)a) {
                case 0:
                        return switchProData.leftHatX - 2048; // Subtract the center value
                case 1:
                        return 2048 - switchProData.leftHatY; // Invert, so it follows the same coordinate as the simple report
                case 2:
                        return switchProData.rightHatX - 2048; // Subtract the center value
                default:
                        return 2048 - switchProData.rightHatY; // Invert, so it follows the same coordinate as the simple report
        }
}

void SwitchProParser::Parse(uint8_t len, uint8_t *buf) {
        if (len > 0 && buf)  {
#ifdef PRINTREPORT
                Notify(PSTR("\r\nLen: "), 0x80); Notify(len, 0x80);
                Notify(PSTR(", data: "), 0x80);
                for (uint8_t i = 0; i < len; i++) {
                        D_PrintHex<uint8_t > (buf[i], 0x80);
                        Notify(PSTR(" "), 0x80);
                }
#endif

                // This driver always uses the standard full report that includes the IMU data.
                // The downside is that it requires more processing power, as the data is send contentiously
                // while the simple input report is only send when the button state changes however the simple
                // input report is not available via USB and does not include the IMU data.

                if (buf[0] == 0x3F) // Simple input report via Bluetooth
                        switchProOutput.enableFullReportMode = true; // Switch over to the full report
                else if (buf[0] == 0x30) { // Standard full mode
                        if (len < 3) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nReport is too short: "), 0x80);
                                D_PrintHex<uint8_t > (len, 0x80);
#endif
                                return;
                        }
                        memcpy(&switchProData, buf + 2, min((uint8_t)(len - 2), MFK_CASTUINT8T sizeof(switchProData)));

                        if (switchProData.btn.val != oldButtonState.val) { // Check if anything has changed
                                buttonClickState.val = switchProData.btn.val & ~oldButtonState.val; // Update click state variable
                                oldButtonState.val = switchProData.btn.val;
                        }

                        message_counter++;
                } else if (buf[0] == 0x21) {
                        // Subcommand reply via Bluetooth
                } else if (buf[0] == 0x81) {
                        // Subcommand reply via USB
                } else {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nUnknown report id: "), 0x80);
                        D_PrintHex<uint8_t > (buf[0], 0x80);
                        Notify(PSTR(", len: "), 0x80);
                        D_PrintHex<uint8_t > (len, 0x80);
#endif
                }
        }

        if (switchProOutput.sendHandshake)
                sendHandshake();
        else if (switchProOutput.disableTimeout)
                disableTimeout();
        else if (switchProOutput.ledReportChanged || switchProOutput.ledHomeReportChanged ||
                switchProOutput.enableFullReportMode || switchProOutput.enableImu != -1)
                sendOutputCmd();
        else if (switchProOutput.leftRumbleOn || switchProOutput.rightRumbleOn) {
                // We need to send the rumble report repeatedly to keep it on
                uint32_t now = millis();
                if  (now - rumble_on_timer > 1000) {
                        rumble_on_timer = now;
                        sendRumbleOutputReport();
                }
        }
}

void SwitchProParser::sendOutputCmd() {
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
        } else if (switchProOutput.enableFullReportMode) {
                switchProOutput.enableFullReportMode = false;

                // See: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_subcommands_notes.md#subcommand-0x03-set-input-report-mode
                buf[0x0A + 0] = 0x03; // PROCON_CMD_MODE
                buf[0x0A + 1] = 0x30; // PROCON_ARG_INPUT_FULL

                sendOutputReport(buf, 10 + 2);
        } else if (switchProOutput.enableImu != -1) {
                // See: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_subcommands_notes.md#subcommand-0x40-enable-imu-6-axis-sensor
                buf[0x0A + 0] = 0x40; // PROCON_CMD_GYRO
                buf[0x0A + 1] = switchProOutput.enableImu ? 1 : 0; // The new state is stored in the variable
                switchProOutput.enableImu = -1;

                sendOutputReport(buf, 12);
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
        // Center joysticks
        switchProData.leftHatX = switchProData.leftHatY = switchProData.rightHatX = switchProData.rightHatY = 2048;

        // Reset buttons variables
        switchProData.btn.val = 0;
        oldButtonState.val = 0;
        buttonClickState.val = 0;

        output_sequence_counter = 0;
        rumble_on_timer = 0;

        switchProOutput.leftRumbleOn = false;
        switchProOutput.rightRumbleOn = false;
        switchProOutput.ledMask = 0;
        switchProOutput.ledHome = false;
        switchProOutput.ledReportChanged = false;
        switchProOutput.ledHomeReportChanged = false;
        switchProOutput.enableFullReportMode = false;
        switchProOutput.enableImu = -1;
        switchProOutput.sendHandshake = false;
        switchProOutput.disableTimeout = false;
}
