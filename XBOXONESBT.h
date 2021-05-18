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

#ifndef _xboxonesbt_h_
#define _xboxonesbt_h_

#include "BTHID.h"
#include "XBOXONESParser.h"

/**
 * This class implements support for the Xbox One S controller via Bluetooth.
 * It uses the BTHID class for all the Bluetooth communication.
 */
class XBOXONESBT : public BTHID, public XBOXONESParser {
public:
        /**
         * Constructor for the XBOXONESBT class.
         * @param  p     Pointer to the BTD class instance.
         * @param  pair  Set this to true in order to pair with the device. If the argument is omitted then it will not pair with it. One can use ::PAIR to set it to true.
         */
        XBOXONESBT(BTD *p, bool pair = false) :
        BTHID(p, pair) {
                XBOXONESParser::Reset();
                pBtd->useSimplePairing = true; // The Xbox One S controller only works via simple pairing
        };

        /**
         * Used to check if a Xbox One S controller is connected.
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
                XBOXONESParser::Parse(len, buf);
        };

        /**
         * Called when a device is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        virtual void OnInitBTHID() {
                XBOXONESParser::Reset();
        };

        /** Used to reset the different buffers to there default values */
        virtual void ResetBTHID() {
                XBOXONESParser::Reset();
        };
        /**@}*/

        /** @name XBOXONESParser implementation */
        virtual void sendOutputReport(uint8_t *data, uint8_t nbytes) {
                // See: https://lore.kernel.org/patchwork/patch/973394/
                uint8_t buf[nbytes + 2];
                buf[0] = 0xA2; // HID BT DATA (0xA0) | Report Type (Output 0x02)
                buf[1] = 0x03; // Report ID
                memcpy(buf + 2, data, nbytes);

                // Send the Bluetooth DATA output report on the interrupt channel
                pBtd->L2CAP_Command(hci_handle, buf, sizeof(buf), interrupt_scid[0], interrupt_scid[1]);
        };
        /**@}*/
};
#endif
