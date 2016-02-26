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

#ifndef __srws1_h__
#define __srws1_h__

#include <hiduniversal.h>

#define STEELSERIES_VID       0x1038
#define STEELSERIES_SRWS1_PID 0x1410

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

union SRWS1DataButtons {
        struct {
                uint8_t dpad : 4;
                uint8_t dummy : 3;
                uint8_t select : 1;

                uint8_t back : 1;
                uint8_t lookLeft : 1;
                uint8_t lights : 1;
                uint8_t lookBack : 1;
                uint8_t rearBrakeBalance : 1;
                uint8_t frontBrakeBalance : 1;
                uint8_t requestPit : 1;
                uint8_t leftGear : 1;

                uint8_t camera : 1;
                uint8_t lookRight : 1;
                uint8_t boost : 1;
                uint8_t horn : 1;
                uint8_t hud : 1;
                uint8_t launchControl : 1;
                uint8_t speedLimiter : 1;
                uint8_t rightGear : 1;
        } __attribute__((packed));
        uint32_t val : 24;
} __attribute__((packed));

struct SRWS1Data {
        int16_t tilt; // Range [-1800:1800]
        uint16_t rightTrigger : 12; // Range [0:1023] i.e. only 10 bits
        uint16_t leftTrigger : 12; // Range [0:1023] i.e. only 10 bits
        SRWS1DataButtons btn;
        uint8_t assists : 4;
        uint8_t steeringSensitivity : 4;
        uint8_t assistValues : 4;
} __attribute__((packed));

class SRWS1 : public HIDUniversal {
public:
        SRWS1(USB *p) : HIDUniversal(p) {};
        bool connected() {
                return HIDUniversal::isReady() && HIDUniversal::VID == STEELSERIES_VID && HIDUniversal::PID == STEELSERIES_SRWS1_PID;
        };
        void setLeds(uint16_t leds);
        SRWS1Data srws1Data;
        SRWS1DataButtons buttonClickState;

private:
        void ParseHIDData(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf); // Called by the HIDUniversal library
        uint8_t OnInitSuccessful() { // Called by the HIDUniversal library on success
                if (HIDUniversal::VID != STEELSERIES_VID || HIDUniversal::PID != STEELSERIES_SRWS1_PID) // Make sure the right device is actually connected
                        return 1;
                setLeds(0);
                return 0;
        };
};

#endif
