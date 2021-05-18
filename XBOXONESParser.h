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

#ifndef _xboxonesparser_h_
#define _xboxonesparser_h_

#include "Usb.h"
#include "controllerEnums.h"

union XboxOneSButtons {
        struct {
                uint8_t dpad : 4;
                uint8_t reserved : 4;

                uint8_t a : 1;
                uint8_t b : 1;
                uint8_t x : 1;
                uint8_t y : 1;

                uint8_t l1 : 1;
                uint8_t r1 : 1;
                uint8_t view : 1;
                uint8_t menu : 1;

                uint8_t l3 : 1;
                uint8_t r3 : 1;
                uint8_t reserved2 : 6;
        } __attribute__((packed));
        uint32_t val : 24;
} __attribute__((packed));

struct XboxOneSData {
        /* Button and joystick values */
        uint16_t hatValue[4];
        uint16_t trigger[2];
        XboxOneSButtons btn;
} __attribute__((packed));

/** This class parses all the data sent by the Xbox One S controller */
class XBOXONESParser {
public:
        /** Constructor for the XBOXONESParser class. */
        XBOXONESParser() {
                Reset();
        };

        /** @name Xbox One S Controller functions */
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
        uint16_t getButtonPress(ButtonEnum b);
        bool getButtonClick(ButtonEnum b);
        /**@}*/

        /**
         * Used to read the analog joystick.
         * @param  a ::LeftHatX, ::LeftHatY, ::RightHatX, and ::RightHatY.
         * @return   Return the analog value as a 16-bit signed integer.
         */
        int16_t getAnalogHat(AnalogHatEnum a);

        /** Used to set the rumble off. */
        void setRumbleOff();

        /**
         * Used to turn on rumble continuously.
         * @param leftTrigger  Left trigger force.
         * @param rightTrigger Right trigger force.
         * @param leftMotor    Left motor force.
         * @param rightMotor   Right motor force.
         */
        void setRumbleOn(uint8_t leftTrigger, uint8_t rightTrigger, uint8_t leftMotor, uint8_t rightMotor);

protected:
        /**
         * Used to parse data sent from the Xbox One S controller.
         * @param len Length of the data.
         * @param buf Pointer to the data buffer.
         */
        void Parse(uint8_t len, uint8_t *buf);

        /** Used to reset the different buffers to their default values */
        void Reset();

        /**
         * Send the output to the Xbox One S controller. This is implemented in XBOXONESBT.h.
         * @param output Pointer to data buffer.
         * @param nbytes Bytes to send.
         */
        virtual void sendOutputReport(uint8_t *data, uint8_t nbytes) = 0;

private:
        static int8_t getButtonIndexXboxOneS(ButtonEnum b);

        bool checkDpad(ButtonEnum b); // Used to check Xbox One S DPAD buttons

        XboxOneSData xboxOneSData;
        XboxOneSButtons oldButtonState, buttonClickState;
        uint8_t oldDpad;

        // The Xbox button is sent in a separate report
        uint8_t xboxButtonState, xboxOldButtonState, xboxbuttonClickState;

        uint16_t triggerOld[2];
        bool L2Clicked; // These buttons are analog, so we use we use these bools to check if they where clicked or not
        bool R2Clicked;
};
#endif
