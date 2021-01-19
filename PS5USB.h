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

 Thanks to Joseph Duchesne for the initial port. Data structure mapping partially based
 on values from Ludwig Füchsl's https://github.com/Ohjurot/DualSense-Windows
 */

#ifndef _ps5usb_h_
#define _ps5usb_h_

#include "hiduniversal.h"
#include "PS5Parser.h"

#define PS5_VID         0x054C // Sony Corporation
#define PS5_PID         0x0CE6 // PS5 Controller

/**
 * This class implements support for the PS5 controller via USB.
 * It uses the HIDUniversal class for all the USB communication.
 */
class PS5USB : public HIDUniversal, public PS5Parser {
public:
        /**
         * Constructor for the PS5USB class.
         * @param  p   Pointer to the USB class instance.
         */
        PS5USB(USB *p) :
        HIDUniversal(p) {
                PS5Parser::Reset();
        };

        /**
         * Used to check if a PS5 controller is connected.
         * @return Returns true if it is connected.
         */
        bool connected() {
                return HIDUniversal::isReady() && HIDUniversal::VID == PS5_VID && HIDUniversal::PID == PS5_PID;
        };

        /**
         * Used to call your own function when the device is successfully initialized.
         * @param funcOnInit Function to call.
         */
        void attachOnInit(void (*funcOnInit)(void)) {
                pFuncOnInit = funcOnInit;
        };

protected:
        /** @name HIDUniversal implementation */
        /**
         * Used to parse USB HID data.
         * @param hid       Pointer to the HID class.
         * @param is_rpt_id Only used for Hubs.
         * @param len       The length of the incoming data.
         * @param buf       Pointer to the data buffer.
         */
        virtual void ParseHIDData(USBHID *hid __attribute__((unused)), bool is_rpt_id __attribute__((unused)), uint8_t len, uint8_t *buf) {
                if (HIDUniversal::VID == PS5_VID && HIDUniversal::PID == PS5_PID)
                        PS5Parser::Parse(len, buf);
        };

        /**
         * Called when a device is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        virtual uint8_t OnInitSuccessful() {
                if (HIDUniversal::VID == PS5_VID && HIDUniversal::PID == PS5_PID) {
                        PS5Parser::Reset();
                        if (pFuncOnInit)
                                pFuncOnInit(); // Call the user function
                        else
                                setLed(Red); // Set the LED to red, so it is consistent with the PS5BT driver
                };
                return 0;
        };
        /**@}*/

        /** @name PS5Parser implementation */
        virtual void sendOutputReport(PS5Output *output) { // Source: https://github.com/chrippa/ds4drv
                // PS4 Source: https://github.com/chrippa/ds4drv
                // PS5 values from https://www.reddit.com/r/gamedev/comments/jumvi5/dualsense_haptics_leds_and_more_hid_output_report/,
                // Ludwig Füchsl's https://github.com/Ohjurot/DualSense-Windows and
                // the series of patches found here: https://patchwork.kernel.org/project/linux-input/cover/20201219062336.72568-1-roderick@gaikai.com/
                uint8_t buf[1 /* report id */ + 47 /* common */];
                memset(buf, 0, sizeof(buf));

                buf[0x00] = 0x02; // Report ID

                buf[0x01] = 0xFF; // feature flags 1
                buf[0x02]= 0xF7;  // feature flags 2

                buf[0x03] = output->smallRumble; // Small Rumble
                buf[0x04] = output->bigRumble; // Big rumble

                // 5-7 headphone, speaker, mic volume, audio flags

                buf[0x09] = (uint8_t)output->microphoneLed;

                // 0x0A mute flags

                // Adaptive Triggers: 0x0B-0x14 right, 0x15 unknown, 0x16-0x1F left
                rightTrigger.processTrigger(&buf[0x0B]); // right
                leftTrigger.processTrigger(&buf[0x16]); // left

                // 0x20-0x24 unknown
                // 0x25 trigger motor effect strengths
                // 0x26 speaker volume

                // player LEDs
                buf[0x27] = 0x03; // led brightness, pulse
                buf[0x2A] = output->disableLeds ? 0x01 : 0x2; // led pulse option
                // buf[0x2B] LED brightness, 0 = full, 1= medium, 2 = low
                buf[0x2C] = output->playerLeds; // 5 white player LEDs

                // lightbar
                buf[0x2D] = output->r; // Red
                buf[0x2E] = output->g; // Green
                buf[0x2F] = output->b; // Blue

                output->reportChanged = false;

                // There is no need to calculate a crc32 when the controller is connected via USB

                pUsb->outTransfer(bAddress, epInfo[ hidInterfaces[0].epIndex[epInterruptOutIndex] ].epAddr, sizeof(buf), buf);
        };
        /**@}*/

        /** @name USBDeviceConfig implementation */
        /**
         * Used by the USB core to check what this driver support.
         * @param  vid The device's VID.
         * @param  pid The device's PID.
         * @return     Returns true if the device's VID and PID matches this driver.
         */
        virtual bool VIDPIDOK(uint16_t vid, uint16_t pid) {
                return (vid == PS5_VID && pid == PS5_PID);
        };
        /**@}*/

private:
        void (*pFuncOnInit)(void); // Pointer to function called in onInit()
};
#endif
