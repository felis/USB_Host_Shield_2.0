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

#ifndef _switch_pro_parser_h_
#define _switch_pro_parser_h_

#include "Usb.h"
#include "controllerEnums.h"

/** Used to set the LEDs on the controller */
const uint8_t SWITCH_PRO_LEDS[] PROGMEM = {
        0x00, // OFF
        0x01, // LED1
        0x02, // LED2
        0x04, // LED3
        0x08, // LED4

        0x09, // LED5
        0x0A, // LED6
        0x0C, // LED7
        0x0D, // LED8
        0x0E, // LED9
        0x0F, // LED10
};

/** Buttons on the controller */
const uint8_t SWITCH_PRO_BUTTONS[] PROGMEM = {
        0x10, // UP
        0x11, // RIGHT
        0x12, // DOWN
        0x13, // LEFT

        0x0D, // Capture
        0x09, // PLUS
        0x0A, // L3
        0x0B, // R3

        0x08, // MINUS
        0x0C, // HOME
        0, 0, // Skip

        0x00, // B
        0x01, // A
        0x03, // X
        0x02, // Y

        0x04, // L
        0x05, // R
        0x06, // ZL
        0x07, // ZR
};

union SwitchProButtons {
        struct {
                uint8_t b : 1;
                uint8_t a : 1;
                uint8_t y : 1;
                uint8_t x : 1;

                uint8_t l : 1;
                uint8_t r : 1;
                uint8_t zl : 1;
                uint8_t zr : 1;

                uint8_t minus : 1;
                uint8_t plus : 1;
                uint8_t l3 : 1;
                uint8_t r3 : 1;

                uint8_t home : 1;
                uint8_t capture : 1;
                uint8_t dummy1 : 2;

                uint8_t dpad : 4;
                uint8_t dummy2 : 4;
        } __attribute__((packed));
        uint32_t val : 24;
} __attribute__((packed));

struct SwitchProData {
        /* Button and joystick values */
        SwitchProButtons btn; // 0-2 bytes
        uint16_t hatValue[4]; // 3-10 bytes
} __attribute__((packed));

struct SwitchProOutput {
        bool leftRumbleOn;
        bool rightRumbleOn;
        uint8_t ledMask; // Higher nibble flashes the LEDs, lower nibble set them on/off
        bool ledHome;

        // Used to only send the report when the state changes
        bool ledReportChanged;
        bool ledHomeReportChanged;
} __attribute__((packed));

/** This class parses all the data sent by the Switch Pro controller */
class SwitchProParser {
public:
        /** Constructor for the SwitchProParser class. */
        SwitchProParser() : output_sequence_counter(0) {
                Reset();
        };

        /** @name Switch Pro Controller functions */
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
        /** @name Switch Pro Controller functions */
        /**
         * Used to read the analog joystick.
         * @param  a ::LeftHatX, ::LeftHatY, ::RightHatX, and ::RightHatY.
         * @return   Return the analog value as a signed 16-bit value.
         */
        int16_t getAnalogHat(AnalogHatEnum a);

#if 0
        /**
         * Get the angle of the controller calculated using the accelerometer.
         * @param  a Either ::Pitch or ::Roll.
         * @return   Return the angle in the range of 0-360.
         */
        float getAngle(AngleEnum a) {
                if (a == Pitch)
                        return (atan2f(-ps5Data.accY, -ps5Data.accZ) + PI) * RAD_TO_DEG;
                else
                        return (atan2f(ps5Data.accX, -ps5Data.accZ) + PI) * RAD_TO_DEG;
        };

        /**
         * Used to get the raw values from the 3-axis gyroscope and 3-axis accelerometer inside the PS5 controller.
         * @param  s The sensor to read.
         * @return   Returns the raw sensor reading.
         */
        int16_t getSensor(SensorEnum s) {
                switch(s) {
                        case gX:
                                return ps5Data.gyroX;
                        case gY:
                                return ps5Data.gyroY;
                        case gZ:
                                return ps5Data.gyroZ;
                        case aX:
                                return ps5Data.accX;
                        case aY:
                                return ps5Data.accY;
                        case aZ:
                                return ps5Data.accZ;
                        default:
                                return 0;
                }
        };
#endif

        /** Turn both rumble and the LEDs off. */
        void setAllOff() {
                setRumbleOff();
                setLedOff();
                setLedHomeOff();
        };

        /** Set rumble off. */
        void setRumbleOff() {
                setRumble(false, false);
        }

        /** Toggle rumble. */
        void setRumbleToggle() {
                setRumble(!switchProOutput.leftRumbleOn, !switchProOutput.rightRumbleOn);
        }

        /**
         * Turn on/off rumble.
         * @param leftRumbleOn   Turn on/off left rumble motor.
         * @param rightRumbleOn  Turn on/off right rumble motor.
         */
        void setRumble(bool leftRumbleOn, bool rightRumbleOn) {
                switchProOutput.leftRumbleOn = leftRumbleOn;
                switchProOutput.rightRumbleOn = rightRumbleOn;
                switchProOutput.ledReportChanged = true; // Set this, so the rumble effect gets changed immediately
        }

        /**
         * Turn on/off the left rumble.
         * @param on   Turn on/off left rumble motor.
         */
        void setRumbleLeft(bool on) {
                switchProOutput.leftRumbleOn = on;
                switchProOutput.ledReportChanged = true; // Set this, so the rumble effect gets changed immediately
        }

        /**
         * Turn on/off the right rumble.
         * @param on   Turn on/off right rumble motor.
         */
        void setRumbleRight(bool on) {
                switchProOutput.rightRumbleOn = on;
                switchProOutput.ledReportChanged = true; // Set this, so the rumble effect gets changed immediately
        }

        /**
         * Set LED value without using the ::LEDEnum.
         * This can also be used to flash the LEDs by setting the high 4-bits of the mask.
         * @param value See: ::LEDEnum.
         */
        void setLedRaw(uint8_t mask) {
                switchProOutput.ledMask = mask;
                switchProOutput.ledReportChanged = true;
        }

        /** Turn all LEDs off. */
        void setLedOff() {
                setLedRaw(0);
        }

        /**
         * Turn the specific ::LEDEnum off.
         * @param a The ::LEDEnum to turn off.
         */
        void setLedOff(LEDEnum a) {
                switchProOutput.ledMask &= ~((uint8_t)(pgm_read_byte(&SWITCH_PRO_LEDS[(uint8_t)a]) & 0x0f));
                switchProOutput.ledReportChanged = true;
        }

        /**
         * Turn the specific ::LEDEnum on.
         * @param a The ::LEDEnum to turn on.
         */
        void setLedOn(LEDEnum a) {
                switchProOutput.ledMask |= (uint8_t)(pgm_read_byte(&SWITCH_PRO_LEDS[(uint8_t)a]) & 0x0f);
                switchProOutput.ledReportChanged = true;
        }

        /**
         * Toggle the specific ::LEDEnum.
         * @param a The ::LEDEnum to toggle.
         */
        void setLedToggle(LEDEnum a) {
                switchProOutput.ledMask ^= (uint8_t)(pgm_read_byte(&SWITCH_PRO_LEDS[(uint8_t)a]) & 0x0f);
                switchProOutput.ledReportChanged = true;
        }

        /** Turn home LED off. */
        void setLedHomeOff() {
                switchProOutput.ledHome = false;
                switchProOutput.ledHomeReportChanged = true;
        }

        /** Turn home LED on. */
        void setLedHomeOn() {
                switchProOutput.ledHome = true;
                switchProOutput.ledHomeReportChanged = true;
        }

        /** Toggle home LED. */
        void setLedHomeToggle() {
                switchProOutput.ledHome = !switchProOutput.ledHome;
                switchProOutput.ledHomeReportChanged = true;
        }

        /** Get the incoming message count. */
        uint16_t getMessageCounter(){
                return message_counter;
        }

protected:
        /**
         * Used to parse data sent from the Switch Pro controller.
         * @param len Length of the data.
         * @param buf Pointer to the data buffer.
         */
        void Parse(uint8_t len, uint8_t *buf);

        /** Used to reset the different buffers to their default values */
        void Reset();

        /**
         * Send the output to the Switch Pro controller. This is implemented in SwitchProBT.h and SwitchProUSB.h.
         * @param data  Pointer to buffer to send by the derived class.
         * @param len   Length of buffer.
         */
        virtual void sendOutputReport(uint8_t *data, uint8_t len) = 0;

private:
        static int8_t getButtonIndexSwitchPro(ButtonEnum b);
        bool checkDpad(ButtonEnum b); // Used to check Switch Pro DPAD buttons

        void sendLedOutputReport();
        void sendRumbleOutputReport();

        SwitchProData switchProData;
        SwitchProButtons oldButtonState, buttonClickState;
        SwitchProOutput switchProOutput;
        uint8_t oldDpad;
        uint16_t message_counter = 0;
        uint8_t output_sequence_counter : 4;
        uint32_t rumble_on_timer = 0;
};
#endif
