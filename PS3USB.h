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

#ifndef _ps3usb_h_
#define _ps3usb_h_

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Usb.h"
#include "PS3Enums.h"

/* PS3 data taken from descriptors */
#define EP_MAXPKTSIZE       64 // max size for data via USB

/* Endpoint types */
#define EP_INTERRUPT        0x03

/* Names we give to the 3 ps3 pipes - this is only used for setting the bluetooth address into the ps3 controllers */
#define PS3_CONTROL_PIPE    0
#define PS3_OUTPUT_PIPE     1
#define PS3_INPUT_PIPE      2

//PID and VID of the different devices
#define PS3_VID             0x054C  // Sony Corporation
#define PS3_PID             0x0268  // PS3 Controller DualShock 3
#define PS3NAVIGATION_PID   0x042F  // Navigation controller
#define PS3MOVE_PID         0x03D5  // Motion controller

// used in control endpoint header for HID Commands
#define bmREQ_HID_OUT USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define HID_REQUEST_SET_REPORT      0x09

#define PS3_MAX_ENDPOINTS   3

/**
 * This class implements support for all the official PS3 Controllers:
 * Dualshock 3, Navigation or a Motion controller via USB.
 *
 * One can only set the color of the bulb, set the rumble, set and get the bluetooth address and calibrate the magnetometer via USB on the Move controller.
 *
 * Information about the protocol can be found at the wiki: https://github.com/felis/USB_Host_Shield_2.0/wiki/PS3-Information.
 */
class PS3USB : public USBDeviceConfig {
public:
        /**
         * Constructor for the PS3USB class.
         * @param  pUsb   Pointer to USB class instance.
         * @param  btadr5,btadr4,btadr3,btadr2,btadr1,btadr0
         * Pass your dongles Bluetooth address into the constructor,
         * so you are able to pair the controller with a Bluetooth dongle.
         */
        PS3USB(USB *pUsb, uint8_t btadr5 = 0, uint8_t btadr4 = 0, uint8_t btadr3 = 0, uint8_t btadr2 = 0, uint8_t btadr1 = 0, uint8_t btadr0 = 0);

        /** @name USBDeviceConfig implementation */
        /**
         * Initialize the PS3 Controller.
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

        /**
         * Used by the USB core to check what this driver support.
         * @param  vid The device's VID.
         * @param  pid The device's PID.
         * @return     Returns true if the device's VID and PID matches this driver.
         */
        virtual boolean VIDPIDOK(uint16_t vid, uint16_t pid) {
                return (vid == PS3_VID && (pid == PS3_PID || pid == PS3NAVIGATION_PID || pid == PS3MOVE_PID));
        };
        /**@}*/

        /**
         * Used to set the Bluetooth address inside the Dualshock 3 and Navigation controller.
         * @param BDADDR Your dongles Bluetooth address.
         */
        void setBdaddr(uint8_t* BDADDR);
        /**
         * Used to set the Bluetooth address inside the Move controller.
         * @param BDADDR Your dongles Bluetooth address.
         */
        void setMoveBdaddr(uint8_t* BDADDR);

        /** @name PS3 Controller functions */
        /**
         * getButtonPress(Button b) will return true as long as the button is held down.
         *
         * While getButtonClick(Button b) will only return it once.
         *
         * So you instance if you need to increase a variable once you would use getButtonClick(Button b),
         * but if you need to drive a robot forward you would use getButtonPress(Button b).
         */
        bool getButtonPress(Button b);
        bool getButtonClick(Button b);
        /**@}*/
        /** @name PS3 Controller functions */
        /**
         * Used to get the analog value from button presses.
         * @param  a The ::Button to read.
         * The supported buttons are:
         * ::UP, ::RIGHT, ::DOWN, ::LEFT, ::L1, ::L2, ::R1, ::R2,
         * ::TRIANGLE, ::CIRCLE, ::CROSS, ::SQUARE, and ::T.
         * @return   Analog value in the range of 0-255.
         */
        uint8_t getAnalogButton(Button a);
        /**
         * Used to read the analog joystick.
         * @param  a ::LeftHatX, ::LeftHatY, ::RightHatX, and ::RightHatY.
         * @return   Return the analog value in the range of 0-255.
         */
        uint8_t getAnalogHat(AnalogHat a);
        /**
         * Used to read the sensors inside the Dualshock 3 controller.
         * @param  a
         * The Dualshock 3 has a 3-axis accelerometer and a 1-axis gyro inside.
         * @return   Return the raw sensor value.
         */
        uint16_t getSensor(Sensor a);
        /**
         * Use this to get ::Pitch and ::Roll calculated using the accelerometer.
         * @param  a Either ::Pitch or ::Roll.
         * @return   Return the angle in the range of 0-360.
         */
        double getAngle(Angle a);
        /**
         * Get the ::Status from the controller.
         * @param  c The ::Status you want to read.
         * @return   True if correct and false if not.
         */
        bool getStatus(Status c);
        /**
         * Read all the available ::Status from the controller.
         * @return One large string with all the information.
         */
        String getStatusString();

        /** Used to set all LEDs and ::Rumble off. */
        void setAllOff();
        /** Turn off ::Rumble. */
        void setRumbleOff();
        /**
         * Turn on ::Rumble.
         * @param mode Either ::RumbleHigh or ::RumbleLow.
         */
        void setRumbleOn(Rumble mode);
        /**
         * Turn on ::Rumble using custom duration and power.
         * @param rightDuration The duration of the right/low rumble effect.
         * @param rightPower The intensity of the right/low rumble effect.
         * @param leftDuration The duration of the left/high rumble effect.
         * @param leftPower The intensity of the left/high rumble effect.
         */
        void setRumbleOn(uint8_t rightDuration, uint8_t rightPower, uint8_t leftDuration, uint8_t leftPower);

        /**
         * Set LED value without using the ::LED enum.
         * @param value See: ::LED enum.
         */
        void setLedRaw(uint8_t value);
        /**
         * Turn the specific ::LED off.
         * @param a The ::LED to turn off.
         */
        void setLedOff(LED a);
        /**
         * Turn the specific ::LED on.
         * @param a The ::LED to turn on.
         */
        void setLedOn(LED a);
        /**
         * Toggle the specific ::LED.
         * @param a The ::LED to toggle.
         */
        void setLedToggle(LED a);

        /**
         * Use this to set the Color using RGB values.
         * @param r,g,b RGB value.
         */
        void moveSetBulb(uint8_t r, uint8_t g, uint8_t b);
        /**
         * Use this to set the color using the predefined colors in ::Colors.
         * @param color The desired color.
         */
        void moveSetBulb(Colors color);
        /**
         * Set the rumble value inside the Move controller.
         * @param rumble The desired value in the range from 64-255.
         */
        void moveSetRumble(uint8_t rumble);

        /**
         * Used to call your own function when the controller is successfully initialized.
         * @param funcOnInit Function to call.
         */
        void attachOnInit(void (*funcOnInit)(void)) {
                pFuncOnInit = funcOnInit;
        };
        /**@}*/

        /** Variable used to indicate if the normal playstation controller is successfully connected. */
        bool PS3Connected;
        /** Variable used to indicate if the move controller is successfully connected. */
        bool PS3MoveConnected;
        /** Variable used to indicate if the navigation controller is successfully connected. */
        bool PS3NavigationConnected;

protected:
        /** Pointer to USB class instance. */
        USB *pUsb;
        /** Device address. */
        uint8_t bAddress;
        /** Endpoint info structure. */
        EpInfo epInfo[PS3_MAX_ENDPOINTS];

private:
        /**
         * Called when the controller is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        void onInit();
        void (*pFuncOnInit)(void); // Pointer to function called in onInit()
        
        bool bPollEnable;

        uint32_t timer; // used to continuously set PS3 Move controller Bulb and rumble values

        uint32_t ButtonState;
        uint32_t OldButtonState;
        uint32_t ButtonClickState;

        uint8_t my_bdaddr[6]; // Change to your dongles Bluetooth address in the constructor
        uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
        uint8_t writeBuf[EP_MAXPKTSIZE]; // General purpose buffer for output data

        void readReport(); // read incoming data
        void printReport(); // print incoming date - Uncomment for debugging

        /* Private commands */
        void PS3_Command(uint8_t* data, uint16_t nbytes);
        void enable_sixaxis(); // Command used to enable the Dualshock 3 and Navigation controller to send data via USB
        void Move_Command(uint8_t* data, uint16_t nbytes);
};
#endif
