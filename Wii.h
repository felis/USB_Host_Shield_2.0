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

 IR camera support added by Allan Glover (adglover9.81@gmail.com) and Kristian Lauszus
 */

#ifndef _wii_h_
#define _wii_h_

#include "BTD.h"
#include "controllerEnums.h"

/** You will have to uncomment this to use the IR camera */
//#define WIICAMERA

/* Bluetooth L2CAP states for L2CAP_task() */
#define L2CAP_WAIT                      0

// These states are used if the Wiimote is the host
#define L2CAP_CONTROL_SUCCESS           1
#define L2CAP_INTERRUPT_SETUP           2

// These states are used if the Arduino is the host
#define L2CAP_CONTROL_CONNECT_REQUEST   3
#define L2CAP_CONTROL_CONFIG_REQUEST    4
#define L2CAP_INTERRUPT_CONNECT_REQUEST 5

#define L2CAP_INTERRUPT_CONFIG_REQUEST  6

#define L2CAP_CHECK_MOTION_PLUS_STATE   7
#define L2CAP_CHECK_EXTENSION_STATE     8
#define L2CAP_INIT_MOTION_PLUS_STATE    9
#define L2CAP_LED_STATE                 10
#define L2CAP_DONE                      11

#define L2CAP_INTERRUPT_DISCONNECT      12
#define L2CAP_CONTROL_DISCONNECT        13

/* L2CAP event flags */
#define L2CAP_FLAG_CONTROL_CONNECTED                0x001
#define L2CAP_FLAG_INTERRUPT_CONNECTED              0x002
#define L2CAP_FLAG_CONFIG_CONTROL_SUCCESS           0x004
#define L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS         0x008
#define L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE      0x040
#define L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE    0x080
#define L2CAP_FLAG_CONNECTION_CONTROL_REQUEST       0x100
#define L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST     0x200

/* Macros for L2CAP event flag tests */
#define l2cap_connected_control_flag (l2cap_event_flag & L2CAP_FLAG_CONTROL_CONNECTED)
#define l2cap_connected_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_INTERRUPT_CONNECTED)
#define l2cap_config_success_control_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_CONTROL_SUCCESS)
#define l2cap_config_success_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS)
#define l2cap_disconnect_response_control_flag (l2cap_event_flag & L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE)
#define l2cap_disconnect_response_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE)
#define l2cap_connection_request_control_flag (l2cap_event_flag & L2CAP_FLAG_CONNECTION_CONTROL_REQUEST)
#define l2cap_connection_request_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST)

/* Wii event flags */
#define WII_FLAG_MOTION_PLUS_CONNECTED              0x400
#define WII_FLAG_NUNCHUCK_CONNECTED                 0x800

#define motion_plus_connected_flag (l2cap_event_flag & WII_FLAG_MOTION_PLUS_CONNECTED)
#define nunchuck_connected_flag (l2cap_event_flag & WII_FLAG_NUNCHUCK_CONNECTED)

#define PAIR    1

/** Enum used to read the joystick on the Nunchuck. */
enum Hat {
        /** Read the x-axis on the Nunchuck joystick. */
        HatX = 0,
        /** Read the y-axis on the Nunchuck joystick. */
        HatY = 1,
};

/**
 * This BluetoothService class implements support for the Wiimote including the Nunchuck and Motion Plus extension.
 *
 * It also support the Wii U Pro Controller.
 */
class WII : public BluetoothService {
public:
        /**
         * Constructor for the WII class.
         * @param  p   Pointer to BTD class instance.
         * @param  pair   Set this to true in order to pair with the Wiimote. If the argument is omitted then it won't pair with it.
         * One can use ::PAIR to set it to true.
         */
        WII(BTD *p, bool pair = false);

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

        /** @name Wii Controller functions */
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

        /** @name Wii Controller functions */
        /** Call this to start the paring sequence with a controller */
        void pair(void) {
                if(pBtd)
                        pBtd->pairWithWiimote();
        }
        /**
         * Used to read the joystick of the Nunchuck.
         * @param  a Either ::HatX or ::HatY.
         * @return   Return the analog value in the range from approximately 25-230.
         */
        uint8_t getAnalogHat(Hat a);
        /**
         * Used to read the joystick of the Wii U Pro Controller.
         * @param  a Either ::LeftHatX, ::LeftHatY, ::RightHatX or ::RightHatY.
         * @return   Return the analog value in the range from approximately 800-3200.
         */
        uint16_t getAnalogHat(AnalogHat a);

        /**
         * Pitch calculated from the Wiimote. A complimentary filter is used if the Motion Plus is connected.
         * @return Pitch in the range from 0-360.
         */
        double getPitch() {
                return pitch;
        };

        /**
         * Roll calculated from the Wiimote. A complimentary filter is used if the Motion Plus is connected.
         * @return Roll in the range from 0-360.
         */
        double getRoll() {
                return roll;
        };

        /**
         * This is the yaw calculated by the gyro.
         *
         * <B>NOTE:</B> This angle will drift a lot and is only available if the Motion Plus extension is connected.
         * @return The angle calculated using the gyro.
         */
        double getYaw() {
                return gyroYaw;
        };

        /** Used to set all LEDs and rumble off. */
        void setAllOff();
        /** Turn off rumble. */
        void setRumbleOff();
        /** Turn on rumble. */
        void setRumbleOn();
        /** Toggle rumble. */
        void setRumbleToggle();

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
         * This will set the LEDs, so the user can see which connections are active.
         *
         * The first ::LED indicate that the Wiimote is connected,
         *
         * the second ::LED indicate indicate that a Motion Plus is also connected
         *
         * the third ::LED will indicate that a Nunchuck controller is also connected.
         */
        void setLedStatus();

        /**
         * Return the battery level of the Wiimote.
         * @return The battery level in the range 0-255.
         */
        uint8_t getBatteryLevel();
        /**
         * Return the Wiimote state.
         * @return See: http://wiibrew.org/wiki/Wiimote#0x20:_Status.
         */
        uint8_t getWiiState() {
                return wiiState;
        };

        /**
         * Used to call your own function when the controller is successfully initialized.
         * @param funcOnInit Function to call.
         */
        void attachOnInit(void (*funcOnInit)(void)) {
                pFuncOnInit = funcOnInit;
        };
        /**@}*/

        /**@{*/
        /** Variable used to indicate if a Wiimote is connected. */
        bool wiimoteConnected;
        /** Variable used to indicate if a Nunchuck controller is connected. */
        bool nunchuckConnected;
        /** Variable used to indicate if a Nunchuck controller is connected. */
        bool motionPlusConnected;
        /** Variable used to indicate if a Wii U Pro controller is connected. */
        bool wiiUProControllerConnected;
        /**@}*/

        /* IMU Data, might be usefull if you need to do something more advanced than just calculating the angle */

        /**@{*/
        /** Pitch and roll calculated from the accelerometer inside the Wiimote. */
        double wiimotePitch;
        double wiimoteRoll;
        /**@}*/

        /**@{*/
        /** Pitch and roll calculated from the accelerometer inside the Nunchuck. */
        double nunchuckPitch;
        double nunchuckRoll;
        /**@}*/

        /**@{*/
        /** Accelerometer values used to calculate pitch and roll. */
        int16_t accX;
        int16_t accY;
        int16_t accZ;
        /**@}*/

        /* Variables for the gyro inside the Motion Plus */
        /** This is the pitch calculated by the gyro - use this to tune WII#pitchGyroScale. */
        double gyroPitch;
        /** This is the roll calculated by the gyro - use this to tune WII#rollGyroScale. */
        double gyroRoll;
        /** This is the yaw calculated by the gyro - use this to tune WII#yawGyroScale. */
        double gyroYaw;

        /**@{*/
        /** The speed in deg/s from the gyro. */
        double pitchGyroSpeed;
        double rollGyroSpeed;
        double yawGyroSpeed;
        /**@}*/

        /**@{*/
        /** You might need to fine-tune these values. */
        uint16_t pitchGyroScale;
        uint16_t rollGyroScale;
        uint16_t yawGyroScale;
        /**@}*/

        /**@{*/
        /** Raw value read directly from the Motion Plus. */
        int16_t gyroYawRaw;
        int16_t gyroRollRaw;
        int16_t gyroPitchRaw;
        /**@}*/

        /**@{*/
        /** These values are set when the controller is first initialized. */
        int16_t gyroYawZero;
        int16_t gyroRollZero;
        int16_t gyroPitchZero;
        /**@}*/

#ifdef WIICAMERA
        /** @name Wiimote IR camera functions
         * You will have to uncomment #WIICAMERA in Wii.h to use the IR camera.
         */
        /** Initialises the camera as per the steps from: http://wiibrew.org/wiki/Wiimote#IR_Camera */
        void IRinitialize();

        /**
         * IR object 1 x-position read from the Wii IR camera.
         * @return The x-position of the object in the range 0-1023.
         */
        uint16_t getIRx1() {
                return IR_object_x1;
        };

        /**
         * IR object 1 y-position read from the Wii IR camera.
         * @return The y-position of the object in the range 0-767.
         */
        uint16_t getIRy1() {
                return IR_object_y1;
        };

        /**
         * IR object 1 size read from the Wii IR camera.
         * @return The size of the object in the range 0-15.
         */
        uint8_t getIRs1() {
                return IR_object_s1;
        };

        /**
         * IR object 2 x-position read from the Wii IR camera.
         * @return The x-position of the object in the range 0-1023.
         */
        uint16_t getIRx2() {
                return IR_object_x2;
        };

        /**
         * IR object 2 y-position read from the Wii IR camera.
         * @return The y-position of the object in the range 0-767.
         */
        uint16_t getIRy2() {
                return IR_object_y2;
        };

        /**
         * IR object 2 size read from the Wii IR camera.
         * @return The size of the object in the range 0-15.
         */
        uint8_t getIRs2() {
                return IR_object_s2;
        };

        /**
         * IR object 3 x-position read from the Wii IR camera.
         * @return The x-position of the object in the range 0-1023.
         */
        uint16_t getIRx3() {
                return IR_object_x3;
        };

        /**
         * IR object 3 y-position read from the Wii IR camera.
         * @return The y-position of the object in the range 0-767.
         */
        uint16_t getIRy3() {
                return IR_object_y3;
        };

        /**
         * IR object 3 size read from the Wii IR camera.
         * @return The size of the object in the range 0-15.
         */
        uint8_t getIRs3() {
                return IR_object_s3;
        };

        /**
         * IR object 4 x-position read from the Wii IR camera.
         * @return The x-position of the object in the range 0-1023.
         */
        uint16_t getIRx4() {
                return IR_object_x4;
        };

        /**
         * IR object 4 y-position read from the Wii IR camera.
         * @return The y-position of the object in the range 0-767.
         */
        uint16_t getIRy4() {
                return IR_object_y4;
        };

        /**
         * IR object 4 size read from the Wii IR camera.
         * @return The size of the object in the range 0-15.
         */
        uint8_t getIRs4() {
                return IR_object_s4;
        };

        /**
         * Use this to check if the camera is enabled or not.
         * If not call WII#IRinitialize to initialize the IR camera.
         * @return     True if it's enabled, false if not.
         */
        bool isIRCameraEnabled() {
                return(wiiState & 0x08);
        };
        /**@}*/
#endif

private:
        BTD *pBtd; // Pointer to BTD instance

        /**
         * Called when the controller is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        void onInit();
        void (*pFuncOnInit)(void); // Pointer to function called in onInit()

        void L2CAP_task(); // L2CAP state machine

        /* Variables filled from HCI event management */
        uint16_t hci_handle;
        bool activeConnection; // Used to indicate if it's already has established a connection

        /* Variables used by high level L2CAP task */
        uint8_t l2cap_state;
        uint16_t l2cap_event_flag; // l2cap flags of received Bluetooth events

        uint32_t ButtonState;
        uint32_t OldButtonState;
        uint32_t ButtonClickState;
        uint16_t hatValues[4];

        uint8_t HIDBuffer[3]; // Used to store HID commands

        uint16_t stateCounter;
        bool unknownExtensionConnected;
        bool extensionConnected;
        bool checkExtension; // Set to false when getBatteryLevel() is called otherwise if should be true
        bool motionPlusInside; // True if it's a new Wiimote with the Motion Plus extension build into it

        /* L2CAP Channels */
        uint8_t control_scid[2]; // L2CAP source CID for HID_Control
        uint8_t control_dcid[2]; // 0x0060
        uint8_t interrupt_scid[2]; // L2CAP source CID for HID_Interrupt
        uint8_t interrupt_dcid[2]; // 0x0061
        uint8_t identifier; // Identifier for connection

        /* HID Commands */
        void HID_Command(uint8_t* data, uint8_t nbytes);
        void setReportMode(bool continuous, uint8_t mode);

        void writeData(uint32_t offset, uint8_t size, uint8_t* data);
        void initExtension1();
        void initExtension2();

        void statusRequest(); // Used to update the Wiimote state and battery level

        void readData(uint32_t offset, uint16_t size, bool EEPROM);
        void readExtensionType();
        void readCalData();

        void checkMotionPresent(); // Used to see if a Motion Plus is connected to the Wiimote
        void initMotionPlus();
        void activateMotionPlus();

        double pitch; // Fusioned angle using a complimentary filter if the Motion Plus is connected
        double roll; // Fusioned angle using a complimentary filter if the Motion Plus is connected

        bool activateNunchuck;
        bool motionValuesReset; // This bool is true when the gyro values has been reset
        unsigned long timer;

        uint8_t wiiState; // Stores the value in l2capinbuf[12] - (0x01: Battery is nearly empty), (0x02:  An Extension Controller is connected), (0x04: Speaker enabled), (0x08: IR enabled), (0x10: LED1, 0x20: LED2, 0x40: LED3, 0x80: LED4)
        uint8_t batteryLevel;

#ifdef WIICAMERA
        /* Private function and variables for the readings from the IR Camera */
        void enableIRCamera1(); // Sets bit 2 of output report 13
        void enableIRCamera2(); // Sets bit 2 of output report 1A
        void writeSensitivityBlock1();
        void writeSensitivityBlock2();
        void write0x08Value();
        void setWiiModeNumber(uint8_t mode_number);

        uint16_t IR_object_x1; // IR x position 10 bits
        uint16_t IR_object_y1; // IR y position 10 bits
        uint8_t IR_object_s1; // IR size value
        uint16_t IR_object_x2;
        uint16_t IR_object_y2;
        uint8_t IR_object_s2;
        uint16_t IR_object_x3; // IR x position 10 bits
        uint16_t IR_object_y3; // IR y position 10 bits
        uint8_t IR_object_s3; // IR size value
        uint16_t IR_object_x4;
        uint16_t IR_object_y4;
        uint8_t IR_object_s4;
#endif
};
#endif