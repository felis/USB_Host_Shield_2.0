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

#ifndef _switch_pro_usb_h_
#define _switch_pro_usb_h_

#include "hiduniversal.h"
#include "SwitchProParser.h"

#define SWITCH_PRO_VID  0x057E // Nintendo Corporation
#define SWITCH_PRO_PID  0x2009 // Switch Pro Controller

/**
 * This class implements support for the Switch Pro controller via USB.
 * It uses the HIDUniversal class for all the USB communication.
 */
class SwitchProUSB : public HIDUniversal, public SwitchProParser {
public:
        /**
         * Constructor for the SwitchProUSB class.
         * @param  p   Pointer to the USB class instance.
         */
        SwitchProUSB(USB *p) :
        HIDUniversal(p) {
                SwitchProParser::Reset();
        };

        /**
         * Used to check if a Switch Pro controller is connected.
         * @return Returns true if it is connected.
         */
        bool connected() {
                return HIDUniversal::isReady() && HIDUniversal::VID == SWITCH_PRO_VID && HIDUniversal::PID == SWITCH_PRO_PID;
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
                if (HIDUniversal::VID == SWITCH_PRO_VID && HIDUniversal::PID == SWITCH_PRO_PID)
                        SwitchProParser::Parse(len, buf);
        };

        /**
         * Called when a device is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        virtual uint8_t OnInitSuccessful() {
                if (HIDUniversal::VID == SWITCH_PRO_VID && HIDUniversal::PID == SWITCH_PRO_PID) {
                        SwitchProParser::Reset();

                        // We need to send a handshake and disable the timeout or the Pro controller will stop sending data via USB
                        // We can not send the commands quickly after each other, so we simply send out the commands at the same
                        // rate as the controller is sending data
                        switchProOutput.sendHandshake = switchProOutput.disableTimeout = true;

                        if (pFuncOnInit)
                                pFuncOnInit(); // Call the user function
                        else {
                                setLedOn(LED1); // Turn on the LED1
                                setLedHomeOn(); // Turn on the home LED
                        }
                }
                return 0;
        };
        /**@}*/

        /** @name SwitchProParser implementation */
        virtual void sendOutputReport(uint8_t *data, uint8_t len) {
                // Based on: https://github.com/Dan611/hid-procon
                // The first 8 bytes are always the same. The actual report follows
                uint8_t buf[8 + len];
                buf[0] = 0x80; // PROCON_REPORT_SEND_USB
                buf[1] = 0x92; // PROCON_USB_DO_CMD
                buf[2] = 0x00;
                buf[3] = 0x31;
                buf[4] = 0x00;
                buf[5] = 0x00;
                buf[6] = 0x00;
                buf[7] = 0x00;

                // Cope over the report
                memcpy(buf + 8, data, len);

                // Endpoint (control endpoint), Interface (0x00), Report Type (Output 0x02), Report ID (0x80), nbytes, data
                SetReport(epInfo[0].epAddr, 0, 0x02, buf[0], sizeof(buf), buf);
        };

        virtual void sendHandshake() {
                switchProOutput.sendHandshake = false;

                // See: https://github.com/Dan611/hid-procon/blob/master/hid-procon.c
                //      https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/USB-HID-Notes.md
                uint8_t buf[2] = { 0x80 /* PROCON_REPORT_SEND_USB */, 0x02 /* PROCON_USB_HANDSHAKE */ };

                // Endpoint (control endpoint), Interface (0x00), Report Type (Output 0x02), Report ID (0x80), nbytes, data
                SetReport(epInfo[0].epAddr, 0, 0x02, buf[0], sizeof(buf), buf);
        };

        virtual void disableTimeout() {
                switchProOutput.disableTimeout = false;

                // See: https://github.com/Dan611/hid-procon/blob/master/hid-procon.c
                //      https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/USB-HID-Notes.md
                uint8_t buf[2] = { 0x80 /* PROCON_REPORT_SEND_USB */, 0x04 /* PROCON_USB_ENABLE */ };

                // Endpoint (control endpoint), Interface (0x00), Report Type (Output 0x02), Report ID (0x80), nbytes, data
                SetReport(epInfo[0].epAddr, 0, 0x02, buf[0], sizeof(buf), buf);
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
                return (vid == SWITCH_PRO_VID && pid == SWITCH_PRO_PID);
        };
        /**@}*/

private:
        void (*pFuncOnInit)(void); // Pointer to function called in onInit()
};
#endif
