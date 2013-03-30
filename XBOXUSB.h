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
 */

#ifndef _xboxusb_h_
#define _xboxusb_h_

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

/* Names we give to the 3 Xbox360 pipes */
#define XBOX_CONTROL_PIPE    0
#define XBOX_INPUT_PIPE      1
#define XBOX_OUTPUT_PIPE     2

// PID and VID of the different devices
#define XBOX_VID                                0x045E  // Microsoft Corporation
#define XBOX_WIRELESS_PID                       0x028F  // Wireless controller only support charging
#define XBOX_WIRELESS_RECEIVER_PID              0x0719  // Microsoft Wireless Gaming Receiver
#define XBOX_WIRELESS_RECEIVER_THIRD_PARTY_PID  0x0291  // Third party Wireless Gaming Receiver

#define MADCATZ_VID                             0x1BAD  // For unofficial Mad Catz controllers
#define JOYTECH_VID                             0x162E  // For unofficial Joytech controllers

#define XBOX_REPORT_BUFFER_SIZE 14 // Size of the input report buffer

// Used in control endpoint header for HID Commands
#define bmREQ_HID_OUT USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define HID_REQUEST_SET_REPORT      0x09

#define XBOX_MAX_ENDPOINTS   3

/** This class implements support for a Xbox wired controller via USB. */
class XBOXUSB : public USBDeviceConfig {
public:
        /**
         * Constructor for the XBOXUSB class.
         * @param  pUsb   Pointer to USB class instance.
         */
        XBOXUSB(USB *pUsb);

        /** @name USBDeviceConfig implementation */
        /**
         * Initialize the Xbox Controller.
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
         * getButtonPress(Button b) will return true as long as the button is held down.
         *
         * While getButtonClick(Button b) will only return it once.
         *
         * So you instance if you need to increase a variable once you would use getButtonClick(Button b),
         * but if you need to drive a robot forward you would use getButtonPress(Button b).
         * @param  b          ::Button to read.
         * @return            getButtonClick(Button b) will return a bool, but getButtonPress(Button b)
         * will return a byte if reading ::L2 or ::R2.
         */
        uint8_t getButtonPress(Button b);
        bool getButtonClick(Button b);
        /**@}*/

        /** @name Xbox Controller functions */
        /**
         * Return the analog value from the joysticks on the controller.
         * @param  a          Either ::LeftHatX, ::LeftHatY, ::RightHatX or ::RightHatY.
         * @return            Returns a signed 16-bit integer.
         */
        int16_t getAnalogHat(AnalogHat a);

        /** Turn rumble off and all the LEDs on the controller. */
        void setAllOff() {
                setRumbleOn(0, 0);
                setLedRaw(0);
        };

        /** Turn rumble off the controller. */
        void setRumbleOff() {
                setRumbleOn(0, 0);
        };
        /**
         * Turn rumble on.
         * @param lValue     Left motor (big weight) inside the controller.
         * @param rValue     Right motor (small weight) inside the controller.
         */
        void setRumbleOn(uint8_t lValue, uint8_t rValue);
        /**
         * Set LED value. Without using the ::LED or ::LEDMode enum.
         * @param value      See:
         * setLedOff(), setLedOn(LED l),
         * setLedBlink(LED l), and setLedMode(LEDMode lm).
         */
        void setLedRaw(uint8_t value);

        /** Turn all LEDs off the controller. */
        void setLedOff() {
                setLedRaw(0);
        };
        /**
         * Turn on a LED by using the ::LED enum.
         * @param l          ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
         */
        void setLedOn(LED l);
        /**
         * Turn on a LED by using the ::LED enum.
         * @param l          ::ALL, ::LED1, ::LED2, ::LED3 and ::LED4 is supported by the Xbox controller.
         */
        void setLedBlink(LED l);
        /**
         * Used to set special LED modes supported by the Xbox controller.
         * @param controller The controller to write to.
         * @param lm         See ::LEDMode.
         */
        void setLedMode(LEDMode lm);
        /**@}*/

        /** True if a Xbox 360 controller is connected. */
        bool Xbox360Connected;

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
        uint32_t ButtonState;
        uint32_t OldButtonState;
        uint16_t ButtonClickState;
        int16_t hatValue[4];
        uint16_t controllerStatus;

        bool L2Clicked; // These buttons are analog, so we use we use these bools to check if they where clicked or not
        bool R2Clicked;

        uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
        uint8_t writeBuf[EP_MAXPKTSIZE]; // General purpose buffer for output data

        void readReport(); // read incoming data
        void printReport(); // print incoming date - Uncomment for debugging

        /* Private commands */
        void XboxCommand(uint8_t* data, uint16_t nbytes);
};
#endif
