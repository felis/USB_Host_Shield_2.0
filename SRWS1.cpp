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
#endif
        memcpy(&srws1Data, buf, min(len, sizeof(srws1Data)));

        static SRWS1DataButtons oldButtonState;
        if (srws1Data.btn.val != oldButtonState.val) { // Check if anything has changed
                buttonClickState.val = srws1Data.btn.val & ~oldButtonState.val; // Update click state variable
                oldButtonState.val = srws1Data.btn.val;
        }
}

// See: https://github.com/torvalds/linux/blob/master/drivers/hid/hid-steelseries.c
void SRWS1::setLeds(uint16_t leds) {
        uint8_t buf[3];
        buf[0] = 0x40; // Report ID
        buf[1] = leds & 0xFF;
        buf[2] = (leds >> 8) & 0x7F;
        pUsb->outTransfer(bAddress, epInfo[ hidInterfaces[0].epIndex[epInterruptOutIndex] ].epAddr, sizeof(buf), buf);
}

