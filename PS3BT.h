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

#ifndef _ps3bt_h_
#define _ps3bt_h_

#include "BTD.h"
#include "PS3Enums.h"

#define HID_BUFFERSIZE              50 // Size of the buffer for the Playstation Motion Controller

/* Bluetooth L2CAP states for L2CAP_task() */
#define L2CAP_WAIT                   0
#define L2CAP_CONTROL_REQUEST        1
#define L2CAP_CONTROL_SUCCESS        2
#define L2CAP_INTERRUPT_SETUP        3
#define L2CAP_INTERRUPT_REQUEST      4
#define L2CAP_INTERRUPT_SUCCESS      5
#define L2CAP_HID_ENABLE_SIXAXIS     6
#define L2CAP_HID_PS3_LED            7
#define L2CAP_DONE                   8
#define L2CAP_INTERRUPT_DISCONNECT   9
#define L2CAP_CONTROL_DISCONNECT     10

/* L2CAP event flags */
#define L2CAP_FLAG_CONNECTION_CONTROL_REQUEST     0x01
#define L2CAP_FLAG_CONFIG_CONTROL_REQUEST         0x02
#define L2CAP_FLAG_CONFIG_CONTROL_SUCCESS         0x04
#define L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST   0x08
#define L2CAP_FLAG_CONFIG_INTERRUPT_REQUEST       0x10
#define L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS       0x20
#define L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE    0x40
#define L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE  0x80

/*Macros for L2CAP event flag tests */
#define l2cap_connection_request_control_flag (l2cap_event_flag & L2CAP_FLAG_CONNECTION_CONTROL_REQUEST)
#define l2cap_config_request_control_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_CONTROL_REQUEST)
#define l2cap_config_success_control_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_CONTROL_SUCCESS)
#define l2cap_connection_request_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST)
#define l2cap_config_request_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_INTERRUPT_REQUEST)
#define l2cap_config_success_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS)
#define l2cap_disconnect_response_control_flag (l2cap_event_flag & L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE)
#define l2cap_disconnect_response_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE)

/**
 * This BluetoothService class implements support for all the official PS3 Controllers:
 * Dualshock 3, Navigation or a Motion controller via Bluetooth.
 *
 * Information about the protocol can be found at the wiki: https://github.com/felis/USB_Host_Shield_2.0/wiki/PS3-Information.
 */
class PS3BT : public BluetoothService {
public:
        /**
         * Constructor for the PS3BT class.
         * @param  pBtd   Pointer to BTD class instance.
         * @param  btadr5,btadr4,btadr3,btadr2,btadr1,btadr0
         * Pass your dongles Bluetooth address into the constructor,
         * This will set BTD#my_bdaddr, so you don't have to plug in the dongle before pairing with your controller.
         */
        PS3BT(BTD *pBtd, uint8_t btadr5 = 0, uint8_t btadr4 = 0, uint8_t btadr3 = 0, uint8_t btadr2 = 0, uint8_t btadr1 = 0, uint8_t btadr0 = 0);

        /** @name BluetoothService implementation */
        /**
         * Used to pass acldata to the services.
         * @param ACLData Incoming acldata.
         */
        virtual void ACLData(uint8_t* ACLData);
        /** Used to run part of the state maschine. */
        virtual void Run();
        /** Use this to reset the service. */
        virtual void Reset();
        /** Used this to disconnect any of the controllers. */
        virtual void disconnect();
        /**@}*/

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
         * Used to read the sensors inside the Dualshock 3 and Move controller.
         * @param  a
         * The Dualshock 3 has a 3-axis accelerometer and a 1-axis gyro inside.
         * The Move controller has a 3-axis accelerometer, a 3-axis gyro, a 3-axis magnetometer
         * and a temperature sensor inside.
         * @return   Return the raw sensor value.
         */
        int16_t getSensor(Sensor a);
        /**
         * Use this to get ::Pitch and ::Roll calculated using the accelerometer.
         * @param  a Either ::Pitch or ::Roll.
         * @return   Return the angle in the range of 0-360.
         */
        double getAngle(Angle a);
        /**
         * Read the sensors inside the Move controller.
         * @param  a ::aXmove, ::aYmove, ::aZmove, ::gXmove, ::gYmove, ::gZmove, ::mXmove, ::mYmove, and ::mXmove.
         * @return   The value in SI units.
         */
        double get9DOFValues(Sensor a);
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
        /**
         * Read the temperature from the Move controller.
         * @return The temperature in degrees celsius.
         */
        String getTemperature();

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

        /** Variable used to indicate if the normal Playstation controller is successfully connected. */
        bool PS3Connected;
        /** Variable used to indicate if the Move controller is successfully connected. */
        bool PS3MoveConnected;
        /** Variable used to indicate if the Navigation controller is successfully connected. */
        bool PS3NavigationConnected;

private:
        /* Mandatory members */
        BTD *pBtd;

        /**
         * Called when the controller is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        void onInit();
        void (*pFuncOnInit)(void); // Pointer to function called in onInit()

        void L2CAP_task(); // L2CAP state machine

        /* Variables filled from HCI event management */
        int16_t hci_handle;
        uint8_t remote_name[30]; // First 30 chars of remote name
        bool activeConnection; // Used to indicate if it's already has established a connection

        /* variables used by high level L2CAP task */
        uint8_t l2cap_state;
        uint16_t l2cap_event_flag; // L2CAP flags of received Bluetooth events

        unsigned long timer;

        uint32_t ButtonState;
        uint32_t OldButtonState;
        uint32_t ButtonClickState;

        uint32_t timerHID; // Timer used see if there has to be a delay before a new HID command
        uint32_t timerBulbRumble; // used to continuously set PS3 Move controller Bulb and rumble values

        uint8_t l2capinbuf[BULK_MAXPKTSIZE]; // General purpose buffer for L2CAP in data
        uint8_t HIDBuffer[HID_BUFFERSIZE]; // Used to store HID commands
        uint8_t HIDMoveBuffer[HID_BUFFERSIZE]; // Used to store HID commands for the Move controller

        /* L2CAP Channels */
        uint8_t control_scid[2]; // L2CAP source CID for HID_Control
        uint8_t control_dcid[2]; // 0x0040
        uint8_t interrupt_scid[2]; // L2CAP source CID for HID_Interrupt
        uint8_t interrupt_dcid[2]; // 0x0041
        uint8_t identifier; // Identifier for connection

        /* HID Commands */
        void HID_Command(uint8_t* data, uint8_t nbytes);
        void HIDMove_Command(uint8_t* data, uint8_t nbytes);
        void enable_sixaxis(); // Command used to enable the Dualshock 3 and Navigation controller to send data via Bluetooth
};
#endif
