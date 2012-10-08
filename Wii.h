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

#ifndef _wii_h_
#define _wii_h_

#include "BTD.h"

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

enum LED {
    LED1 = 0x10,
    LED2 = 0x20,
    LED3 = 0x40,
    LED4 = 0x80,
    
    LED5 = 0x90,
    LED6 = 0xA0,
    LED7 = 0xC0,
    LED8 = 0xD0,
    LED9 = 0xE0,
    LED10 = 0xF0,
};

enum Button {
    LEFT  = 0x00001,
    RIGHT = 0x00002,
    DOWN  = 0x00004,
    UP    = 0x00008,
    PLUS  = 0x00010,
    
    TWO   = 0x00100,
    ONE   = 0x00200,
    B     = 0x00400,
    A     = 0x00800,
    MINUS = 0x01000,
    HOME  = 0x08000,
    
    Z     = 0x10000,
    C     = 0x20000,
};
enum AnalogHat {
    HatX = 0,
    HatY = 1,
};

class WII : public BluetoothService {
public:
    WII(BTD *p, bool pair=false);
    
    // BluetoothService implementation
    virtual void ACLData(uint8_t* ACLData); // Used to pass acldata to the services
    virtual void Run(); // Used to run part of the state maschine
    virtual void Reset(); // Use this to reset the service
    virtual void disconnect(); // Use this void to disconnect any of the controllers
                
    /*
      getButtonPress will return true as long as the button is held down
      While getButtonClick will only return it once
      So you instance if you need to increase a variable once you would use getButtonClick,
      but if you need to drive a robot forward you would use getButtonPress
    */
    bool getButtonPress(Button b); // This will read true as long as the button is held down
    bool getButtonClick(Button b); // This will only be true when the button is clicked the first time
    
    uint8_t getAnalogHat(AnalogHat a); // Used to read the joystick of the Nunchuck

    double getPitch() { return pitch; }; // Fusioned angle using a complimentary filter if the Motion Plus is connected
    double getRoll() { return roll; }; // Fusioned angle using a complimentary filter if the Motion Plus is connected
    double getYaw() { return gyroYaw; }; // This is the yaw calculated by the gyro
    
    void setAllOff(); // Turn both rumble and all LEDs off
    void setRumbleOff();
    void setRumbleOn();
    void setRumbleToggle();
    void setLedOff(LED a);
    void setLedOn(LED a);
    void setLedToggle(LED a);
    void setLedStatus(); // This will set the LEDs, so the user can see which connections are active
    
    bool wiimoteConnected; // Variable used to indicate if a Wiimote is connected
    bool nunchuckConnected; // Variable used to indicate if a Nunchuck controller is connected
    bool motionPlusConnected; // Variable used to indicate if a Nunchuck controller is connected
    
    /* IMU Data, might be usefull if you need to do something more advanced than just calculating the angle */
    
    double wiiMotePitch; // Pitch and roll calculated from the accelerometer inside the Wiimote
    double wiiMoteRoll;
    double nunchuckPitch; // Pitch and roll calculated from the accelerometer inside the Nunchuck
    double nunchuckRoll;
    
    int16_t accX; // Accelerometer values used to calculate pitch and roll
    int16_t accY;
    int16_t accZ;
    
    /* Variables for the gyro inside the Motion Plus */
    double gyroPitch; // This is the pitch calculated by the gyro - use this to tune pitchGyroScale
    double gyroRoll; // This is the roll calculated by the gyro - use this to tune rollGyroScale
    double gyroYaw; // This is the yaw calculated by the gyro - use this to tune yawGyroScale

    double pitchGyroSpeed; // The speed in deg/s from the gyro
    double rollGyroSpeed;
    double yawGyroSpeed;
    
    uint16_t pitchGyroScale; // You might need to fine-tune these values
    uint16_t rollGyroScale;
    uint16_t yawGyroScale;
    
    int16_t gyroYawRaw; // Raw value read directly from the Motion Plus
    int16_t gyroRollRaw;
    int16_t gyroPitchRaw;
    
    int16_t gyroYawZero; // These values are set when the controller is first initialized
    int16_t gyroRollZero;
    int16_t gyroPitchZero;
    
private:
    /* Mandatory members */
    BTD *pBtd;
    
    void L2CAP_task(); // L2CAP state machine
    
    /* Variables filled from HCI event management */
    uint16_t hci_handle;
    
    /* variables used by high level L2CAP task */    
    uint8_t l2cap_state;
    uint16_t l2cap_event_flag;// l2cap flags of received bluetooth events    
    
    uint32_t ButtonState;
    uint32_t OldButtonState;
    uint32_t ButtonClickState;
    uint8_t hatValues[2];
    
    uint8_t HIDBuffer[3];// Used to store HID commands
    
    uint16_t stateCounter;
    bool unknownExtensionConnected;
    bool extensionConnected;
    
    /* L2CAP Channels */
    uint8_t control_scid[2]; // L2CAP source CID for HID_Control
    uint8_t control_dcid[2]; // 0x0060
    uint8_t interrupt_scid[2]; // L2CAP source CID for HID_Interrupt
    uint8_t interrupt_dcid[2]; // 0x0061
    uint8_t identifier; // Identifier for connection
    
    /* HID Commands */
    void HID_Command(uint8_t* data, uint8_t nbytes);
    void setReportMode(bool continuous, uint8_t mode);
    void statusRequest();
    
    void writeData(uint32_t offset, uint8_t size, uint8_t* data);
    void initExtension1();
    void initExtension2();
    
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
};
#endif