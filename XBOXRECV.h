/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.

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

 getBatteryLevel and checkStatus functions made by timstamp.co.uk found using BusHound from Perisoft.net
 */

#ifndef _xboxrecv_h_
#define _xboxrecv_h_

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Usb.h"
#include "xboxEnums.h"

/* Data Xbox 360 taken from descriptors */
#define EP_MAXPKTSIZE       32 // max size for data via USB

/* Endpoint types */
#define EP_INTERRUPT        0x03

/* Names we give to the 9 Xbox360 pipes */
#define XBOX_CONTROL_PIPE   0
#define XBOX_INPUT_PIPE_1   1
#define XBOX_OUTPUT_PIPE_1  2
#define XBOX_INPUT_PIPE_2   3
#define XBOX_OUTPUT_PIPE_2  4
#define XBOX_INPUT_PIPE_3   5
#define XBOX_OUTPUT_PIPE_3  6
#define XBOX_INPUT_PIPE_4   7
#define XBOX_OUTPUT_PIPE_4  8

// PID and VID of the different devices
#define XBOX_VID                                0x045E  // Microsoft Corporation
#define XBOX_WIRELESS_RECEIVER_PID              0x0719  // Microsoft Wireless Gaming Receiver
#define XBOX_WIRELESS_RECEIVER_THIRD_PARTY_PID  0x0291  // Third party Wireless Gaming Receiver

#define MADCATZ_VID                             0x1BAD  // For unofficial Mad Catz receivers

#define XBOX_MAX_ENDPOINTS   9

/**
 * This class implements support for a Xbox Wireless receiver.
 *
 * Up to four controllers can connect to one receiver, if more is needed one can use a second receiver via the USBHub class.
 */
class XBOXRECV : public USBDeviceConfig {
public:
        /**
         * Constructor for the XBOXRECV class.
         * @param  pUsb   Pointer to USB class instance.
         */
        XBOXRECV(USB *pUsb);

        /** @name USBDeviceConfig implementation */
        /**
         * Initialize the Xbox wireless receiver.
         * @param  parent   Hub number.
         * @param  port     Port number on the hub.
         * @param  lowspeed Speed of the device.
         * @return          0 on success.
         */
        virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        /**
         * Release the USB device.
         * @return 0 on success.
         */
        virtual uint8_t Release();
        /**
         * Poll the USB Input endpoins and run the state machines.
         * @return 0 on success.
         */
        virtual uint8_t Poll();

        /**
         * Get the device address.
         * @return The device address.
         */
        virtual uint8_t GetAddress() {
                return bAddress;
        };

        /**
         * Used to check if the controller has been initialized.
         * @return True if it's ready.
         */
        virtual bool isReady() {
                return bPollEnable;
        };
        /**@}*/

        /** @name Xbox Controller functions */
        /**
         * getButtonPress(uint8_t controller, Button b) will return true as long as the button is held down.
         *
         * While getButtonClick(uint8_t controller, Button b) will only return it once.
         *
         * So you instance if you need to increase a variable once you would use getButtonClick(uint8_t controller, Button b),
         * but if you need to drive a robot forward you would use getButtonPress(uint8_t controller, Button b).
         * @param  controller The controller to read from.
         * @param  b          ::Button to read.
         * @return            getButtonClick(uint8_t controller, Button b) will return a bool, but getButtonPress(uint8_t controller, Button b)
         * will return a byte if reading ::L2 or ::R2.
         */
        uint8_t getButtonPress(uint8_t controller, Button b);
        bool getButtonClick(uint8_t controller, Button b);
        /**@}*/

        /** @name Xbox Controller functions */
        /**
         * Return the analog value from the joysticks on the controller.
         * @param  controller The controller to read from.
         * @param  a          Either ::LeftHatX, ::LeftHatY, ::RightHatX or ::RightHatY.
         * @return            Returns a signed 16-bit integer.
         */
        int16_t getAnalogHat(uint8_t controller, AnalogHat a);

        /**
         * Turn rumble off and all the LEDs on the specific controller.
         * @param  controller The controller to write to.
         */
        void setAllOff(uint8_t controller) {
                setRumbleOn(controller, 0, 0);
                setLedOff(controller);
        };

        /**
         * Turn rumble off the specific controller.
         * @param  controller The controller to write to.
         */
        void setRumbleOff(uint8_t controller) {
                setRumbleOn(controller, 0, 0);
        };
        /**
         * Turn rumble on.
         * @param controller The controller to write to.
         * @param lValue     Left motor (big weight) inside the controller.
         * @param rValue     Right motor (small weight) inside the controller.
         */
        void setRumbleOn(uint8_t controller, uint8_t lValue, uint8_t rValue);
        /**
         * Set LED value. Without using the ::LED or ::LEDMode enum.
         * @param controller The controller to write to.
         * @param value      See:
         * setLedOff(uint8_t controller), setLedOn(uint8_t controller, LED l),
         * setLedBlink(uint8_t controller, LED l), and setLedMode(uint8_t controller, LEDMode lm).
         */
        void setLedRaw(uint8_t controller, uint8_t value);

        /**
         * Turn all LEDs off the specific controller.
         * @param controller The controller to write to.
         */
        void setLedOff(uint8_t controller) {
                setLedRaw(controller, 0);
        };
        /**
         * Turn on a LED by using the ::LED enum.
         * @param controller The controller to write to.
         * @param l          ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
         */
        void setLedOn(uint8_t controller, LED l);
        /**
         * Turn on a LED by using the ::LED enum.
         * @param controller The controller to write to.
         * @param l          ::ALL, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
         */
        void setLedBlink(uint8_t controller, LED l);
        /**
         * Used to set special LED modes supported by the Xbox controller.
         * @param controller The controller to write to.
         * @param lm         See ::LEDMode.
         */
        void setLedMode(uint8_t controller, LEDMode lm);
        /**
         * Used to get the battery level from the controller.
         * @param  controller The controller to read from.
         * @return            Returns the battery level as an integer in the range of 0-3.
         */
        uint8_t getBatteryLevel(uint8_t controller);
        /**
         * Used to check if a button has changed.
         * @param  controller The controller to read from.
         * @return            True if a button has changed.
         */
        bool buttonChanged(uint8_t controller);
        /**@}*/

        /** True if a wireless receiver is connected. */
        bool XboxReceiverConnected;
        /** Variable used to indicate if the XBOX 360 controller is successfully connected. */
        uint8_t Xbox360Connected[4];

protected:
        /** Pointer to USB class instance. */
        USB *pUsb;
        /** Device address. */
        uint8_t bAddress;
        /** Endpoint info structure. */
        EpInfo epInfo[XBOX_MAX_ENDPOINTS];

private:
        bool bPollEnable;

        /* Variables to store the buttons */
        uint32_t ButtonState[4];
        uint32_t OldButtonState[4];
        uint16_t ButtonClickState[4];
        int16_t hatValue[4][4];
        uint16_t controllerStatus[4];
        bool buttonStateChanged[4]; // True if a button has changed

        bool L2Clicked[4]; // These buttons are analog, so we use we use these bools to check if they where clicked or not
        bool R2Clicked[4];

        unsigned long timer; // Timing for checkStatus() signals

        uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
        uint8_t writeBuf[EP_MAXPKTSIZE]; // General purpose buffer for output data

        void readReport(uint8_t controller); // read incoming data
        void printReport(uint8_t controller, uint8_t nBytes); // print incoming date - Uncomment for debugging

        /* Private commands */
        void XboxCommand(uint8_t controller, uint8_t* data, uint16_t nbytes);
        void checkStatus();
};
#endif