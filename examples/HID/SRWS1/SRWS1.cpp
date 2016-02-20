/* Copyright (C) 2016 Kristian Lauszus, TKJ Electronics. All rights reserved.

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

#include "SRWS1.h"

enum DPADEnum {
        DPAD_UP = 0x0,
        DPAD_UP_RIGHT = 0x1,
        DPAD_RIGHT = 0x2,
        DPAD_RIGHT_DOWN = 0x3,
        DPAD_DOWN = 0x4,
        DPAD_DOWN_LEFT = 0x5,
        DPAD_LEFT = 0x6,
        DPAD_LEFT_UP = 0x7,
        DPAD_OFF = 0xF,
};

void SRWS1::ParseHIDData(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
        if (HIDUniversal::VID != STEELSERIES_VID || HIDUniversal::PID != STEELSERIES_SRWS1_PID) // Make sure the right device is actually connected
                return;
#if 0
        if (len && buf)  {
                Notify(PSTR("\r\n"), 0x80);
                for (uint8_t i = 0; i < len; i++) {
                        D_PrintHex<uint8_t > (buf[i], 0x80);
                        Notify(PSTR(" "), 0x80);
                }
        }
#else
        memcpy(&srws1Data, buf, min(len, sizeof(srws1Data)));

        if (srws1Data.leftTrigger) {
                Serial.print(F("L2: "));
                Serial.println(srws1Data.leftTrigger);
        }
        if (srws1Data.rightTrigger) {
                Serial.print(F("R2: "));
                Serial.println(srws1Data.rightTrigger);
        }
        if (srws1Data.btn.select) {
                //Serial.println("Select");
                Serial.println(srws1Data.tilt);
        }

        if (srws1Data.btn.back) Serial.println(F("Back"));
        if (srws1Data.btn.lookLeft) Serial.println(F("Look Left"));
        if (srws1Data.btn.lights) Serial.println(F("Lights"));
        if (srws1Data.btn.lookBack) Serial.println(F("Look Back"));
        if (srws1Data.btn.rearBrakeBalance) Serial.println(F("R. Brake Balance"));
        if (srws1Data.btn.frontBrakeBalance) Serial.println(F("F. Brake Balance"));
        if (srws1Data.btn.requestPit) Serial.println(F("Request Pit"));
        if (srws1Data.btn.leftGear) Serial.println(F("Left Gear"));

        if (srws1Data.btn.camera) Serial.println(F("Camera"));
        if (srws1Data.btn.lookRight) Serial.println(F("Look right"));
        if (srws1Data.btn.boost) Serial.println(F("Boost"));
        if (srws1Data.btn.horn) Serial.println(F("Horn"));
        if (srws1Data.btn.hud) Serial.println(F("HUD"));
        if (srws1Data.btn.launchControl) Serial.println(F("Launch Control"));
        if (srws1Data.btn.speedLimiter) Serial.println(F("Speed Limiter"));
        if (srws1Data.btn.rightGear) Serial.println(F("Right gear"));

        static SRWS1DataButtons buttonClickState, oldButtonState;
        if (srws1Data.btn.val != oldButtonState.val) { // Check if anything has changed
                buttonClickState.val = srws1Data.btn.val & ~oldButtonState.val; // Update click state variable
                oldButtonState.val = srws1Data.btn.val;
        }

        if (buttonClickState.lights) {
                buttonClickState.lights = 0; // Clear event
                static uint16_t leds = 0;
                leds = leds << 1 | 1;
                if (leds == 0xFFFF)
                        leds = 0; // Clear LEDs variable
                setLeds(leds); // Note: disable the strobe light effect
        }

        if (srws1Data.assists) Serial.println(srws1Data.assists);
        if (srws1Data.steeringSensitivity) Serial.println(srws1Data.steeringSensitivity);
        if (srws1Data.assistValues) Serial.println(srws1Data.assistValues);

        switch (srws1Data.btn.dpad) {
                case DPAD_UP:
                        Serial.println(F("Up"));
                        break;
                case DPAD_UP_RIGHT:
                        Serial.println(F("UP & RIGHT"));
                        break;
                case DPAD_RIGHT:
                        Serial.println(F("Right"));
                        break;
                case DPAD_RIGHT_DOWN:
                        Serial.println(F("Right & down"));
                        break;
                case DPAD_DOWN:
                        Serial.println(F("Down"));
                        break;
                case DPAD_DOWN_LEFT:
                        Serial.println(F("Down & left"));
                        break;
                case DPAD_LEFT:
                        Serial.println(F("Left"));
                        break;
                case DPAD_LEFT_UP:
                        Serial.println(F("Left & up"));
                        break;
                case DPAD_OFF:
                        break;
                default:
                        Serial.print(F("Unknown state: "));
                        D_PrintHex<uint8_t > (srws1Data.btn.dpad, 0x80);
                        Serial.println();
                break;
        }
#endif
}

// See: https://github.com/torvalds/linux/blob/master/drivers/hid/hid-steelseries.c
void SRWS1::setLeds(uint16_t leds) {
        uint8_t buf[3];
        buf[0] = 0x40; // Report ID
        buf[1] = leds & 0xFF;
        buf[2] = leds >> 8;
        pUsb->outTransfer(bAddress, epInfo[ hidInterfaces[0].epIndex[epInterruptOutIndex] ].epAddr, sizeof(buf), buf);
}

