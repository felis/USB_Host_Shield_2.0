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

#ifndef _switch_pro_bt_h_
#define _switch_pro_bt_h_

#include "BTHID.h"
#include "SwitchProParser.h"

/**
 * This class implements support for the Switch Pro controller via Bluetooth.
 * It uses the BTHID class for all the Bluetooth communication.
 */
class SwitchProBT : public BTHID, public SwitchProParser {
public:
        /**
         * Constructor for the SwitchProBT class.
         * @param  p     Pointer to the BTD class instance.
         * @param  pair  Set this to true in order to pair with the device. If the argument is omitted then it will not pair with it. One can use ::PAIR to set it to true.
         * @param  pin   Write the pin to BTD#btdPin. If argument is omitted, then "0000" will be used.
         */
        SwitchProBT(BTD *p, bool pair = false, const char *pin = "0000") :
        BTHID(p, pair, pin) {
                SwitchProParser::Reset();
        };

        /**
         * Used to check if a Switch Pro controller is connected.
         * @return Returns true if it is connected.
         */
        bool connected() {
                return BTHID::connected;
        };

protected:
        /** @name BTHID implementation */
        /**
         * Used to parse Bluetooth HID data.
         * @param len The length of the incoming data.
         * @param buf Pointer to the data buffer.
         */
        virtual void ParseBTHIDData(uint8_t len, uint8_t *buf) {
                SwitchProParser::Parse(len, buf);
        };

        /**
         * Called when a device is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        virtual void OnInitBTHID() {
                SwitchProParser::Reset();

                // Only call this is a user function has not been set
                if (!pFuncOnInit) {
                        setLedOn(LED1); // Turn on the LED1
                        setLedHomeOn(); // Turn on the home LED
                }
        };

        /** Used to reset the different buffers to there default values */
        virtual void ResetBTHID() {
                SwitchProParser::Reset();
        };
        /**@}*/

        /** @name SwitchProParser implementation */
        virtual void sendOutputReport(uint8_t *data, uint8_t len) {
                uint8_t buf[1 /* BT DATA Output Report */ + len];

                // Send as a Bluetooth HID DATA output report on the interrupt channel
                buf[0] = 0xA2; // HID BT DATA (0xA0) | Report Type (Output 0x02)
                memcpy(&buf[1], data, len);

                // Send the Bluetooth DATA output report on the interrupt channel
                pBtd->L2CAP_Command(hci_handle, buf, sizeof(buf), interrupt_scid[0], interrupt_scid[1]);
        };
        /**@}*/
};
#endif
