/* Copyright (C) 2014 Kristian Lauszus, TKJ Electronics. All rights reserved.

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

#ifndef _ps4bt_h_
#define _ps4bt_h_

#include "BTHID.h"
#include "controllerEnums.h"

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

union PS4Buttons {
        struct {
                uint8_t dpad : 4;
                uint8_t square : 1;
                uint8_t cross : 1;
                uint8_t circle : 1;
                uint8_t triangle : 1;

                uint8_t l1 : 1;
                uint8_t r1 : 1;
                uint8_t l2 : 1;
                uint8_t r2 : 1;
                uint8_t share : 1;
                uint8_t options : 1;
                uint8_t l3 : 1;
                uint8_t r3 : 1;

                uint8_t ps : 1;
                uint8_t touchpad : 1;
                uint8_t dummy : 6;
        };
        uint8_t val[3];
};

struct PS4Data {
        uint8_t hatValue[4];
        PS4Buttons btn;
        uint8_t trigger[2];
};

/**
 * This class implements support for the PS4 controller via Bluetooth.
 * It uses the BTHID class for all the Bluetooth communication.
 */
class PS4BT : public HIDReportParser, public BTHIDService {
public:
        /**
         * Constructor for the PS4BT class.
         * @param  p   Pointer to the BTHID class instance.
         */
        PS4BT(BTHID *p) :
        pBthid(p) {
                pBthid->SetReportParser(KEYBOARD_PARSER_ID, this);
                pBthid->registerServiceClass(this); // Register it as a Bluetooth HID service
                Reset();
        };

        virtual void Parse(HID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);

        /** @name PS4 Controller functions */
        /**
         * getButtonPress(ButtonEnum b) will return true as long as the button is held down.
         *
         * While getButtonClick(ButtonEnum b) will only return it once.
         *
         * So you instance if you need to increase a variable once you would use getButtonClick(ButtonEnum b),
         * but if you need to drive a robot forward you would use getButtonPress(ButtonEnum b).
         * @param  b          ::ButtonEnum to read.
         * @return            getButtonPress(ButtonEnum b) will return a true as long as a button is held down, while getButtonClick(ButtonEnum b) will return true once for each button press.
         */
        bool getButtonPress(ButtonEnum b);
        bool getButtonClick(ButtonEnum b);
        /**@}*/
        /** @name PS4 Controller functions */
        /**
         * Used to get the analog value from button presses.
         * @param  a The ::ButtonEnum to read.
         * The supported buttons are:
         * ::UP, ::RIGHT, ::DOWN, ::LEFT, ::L1, ::L2, ::R1, ::R2,
         * ::TRIANGLE, ::CIRCLE, ::CROSS, ::SQUARE, and ::T.
         * @return   Analog value in the range of 0-255.
         */
        uint8_t getAnalogButton(ButtonEnum a);

        /**
         * Used to read the analog joystick.
         * @param  a ::LeftHatX, ::LeftHatY, ::RightHatX, and ::RightHatY.
         * @return   Return the analog value in the range of 0-255.
         */
        uint8_t getAnalogHat(AnalogHatEnum a);
        /**@}*/

        /** True if a device is connected */
        bool connected() {
                if (pBthid)
                        return pBthid->connected;
                return false;
        };

        /** Used this to disconnect the devices. */
        void disconnect() {
                if (pBthid)
                        pBthid->disconnect();
        };

        /** Call this to start the paring sequence with a device */
        void pair(void) {
                if (pBthid)
                        pBthid->pair();
        };

        /**
         * Used to call your own function when the device is successfully initialized.
         * @param funcOnInit Function to call.
         */
        void attachOnInit(void (*funcOnInit)(void)) {
                pFuncOnInit = funcOnInit;
        };

        /** @name BTHIDService implementation */
        /** Used to reset the different buffers to there default values */
        virtual void Reset() {
                uint8_t i;
                for (0; i < sizeof(ps4Data.hatValue); i++)
                        ps4Data.hatValue[i] = 127;
                for (0; i < sizeof(PS4Buttons); i++) {
                        ps4Data.btn.val[i] = 0;
                        oldButtonState.val[i] = 0;
                }
                for (0; i < sizeof(ps4Data.trigger); i++)
                        ps4Data.trigger[i] = 0;

                ps4Data.btn.dpad = DPAD_OFF;
                oldButtonState.dpad = DPAD_OFF;
                buttonClickState.dpad = 0;
                oldDpad = 0;
        };

        /**
         * Called when a device is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        virtual void onInit() {
                if (pFuncOnInit)
                        pFuncOnInit(); // Call the user function
        };
        /**@}*/

private:
        void (*pFuncOnInit)(void); // Pointer to function called in onInit()

        bool checkDpad(DPADEnum b); // Used to check PS4 DPAD buttons

        BTHID *pBthid; // Pointer to BTHID instance
        PS4Data ps4Data;
        PS4Buttons oldButtonState, buttonClickState;
        uint8_t oldDpad;
};
#endif