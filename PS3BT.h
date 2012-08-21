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

#define HID_BUFFERSIZE              50 // size of the buffer for the Playstation Motion Controller
#define OUTPUT_REPORT_BUFFER_SIZE   48 //Size of the output report buffer for the controllers

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

enum LED {
    LED1 = 0x01,
    LED2 = 0x02,
    LED3 = 0x04,
    LED4 = 0x08,
    
    LED5 = 0x09,
    LED6 = 0x0A,
    LED7 = 0x0C,
    LED8 = 0x0D,
    LED9 = 0x0E,
    LED10 = 0x0F,
};
enum Colors {
    // Used to set the colors of the move controller            
    Red = 0xFF0000, // r = 255, g = 0, b = 0
    Green = 0xFF00, // r = 0, g = 255, b = 0
    Blue = 0xFF, // r = 0, g = 0, b = 255
    
    Yellow = 0xFFEB04, // r = 255, g = 235, b = 4
    Lightblue = 0xFFFF, // r = 0, g = 255, b = 255
    Purble = 0xFF00FF, // r = 255, g = 0, b = 255
    
    White = 0xFFFFFF, // r = 255, g = 255, b = 255
    Off = 0x00, // r = 0, g = 0, b = 0
};

enum Button {
    // byte location | bit location
    
    // Sixaxis Dualshcock 3 & Navigation controller 
    SELECT = (11 << 8) | 0x01,
    L3 = (11 << 8) | 0x02,
    R3 = (11 << 8) | 0x04,
    START = (11 << 8) | 0x08,
    UP = (11 << 8) | 0x10,
    RIGHT = (11 << 8) | 0x20,
    DOWN = (11 << 8) | 0x40,
    LEFT = (11 << 8) | 0x80,
    
    L2 = (12 << 8) | 0x01,
    R2 = (12 << 8) | 0x02,
    L1 = (12 << 8) | 0x04,
    R1 = (12 << 8) | 0x08,
    TRIANGLE = (12 << 8) | 0x10,
    CIRCLE = (12 << 8) | 0x20,
    CROSS = (12 << 8) | 0x40,
    SQUARE = (12 << 8) | 0x80,
    
    PS = (13 << 8) | 0x01,
    
    MOVE = (13/*12*/ << 8) | 0x08, // covers 12 bits - we only need to read the top 8            
    T = (13/*12*/ << 8) | 0x10, // covers 12 bits - we only need to read the top 8
    
    
    // These are the true locations for the Move controller, but to make the same syntax for all controllers, it is handled by getButton()
/*   
    // Playstation Move Controller 
    SELECT_MOVE = (10 << 8) | 0x01,
    START_MOVE = (10 << 8) | 0x08,
    
    TRIANGLE_MOVE = (11 << 8) | 0x10,
    CIRCLE_MOVE = (11 << 8) | 0x20,
    CROSS_MOVE = (11 << 8) | 0x40,
    SQUARE_MOVE = (11 << 8) | 0x80,
    
    PS_MOVE = (12 << 8) | 0x01,
    MOVE_MOVE = (12 << 8) | 0x08, // covers 12 bits - we only need to read the top 8            
    T_MOVE = (12 << 8) | 0x10, // covers 12 bits - we only need to read the top 8     
 */
};
enum AnalogButton {
    //Sixaxis Dualshcock 3 & Navigation controller
    UP_ANALOG = 23,
    RIGHT_ANALOG = 24,
    DOWN_ANALOG = 25,
    LEFT_ANALOG = 26,
    
    L2_ANALOG = 27,
    R2_ANALOG = 28,
    L1_ANALOG = 29,
    R1_ANALOG = 30,
    TRIANGLE_ANALOG = 31,
    CIRCLE_ANALOG = 32,
    CROSS_ANALOG = 33,
    SQUARE_ANALOG = 34,
    
    //Playstation Move Controller
    T_ANALOG = 15, // Both at byte 14 (last reading) and byte 15 (current reading)
};
enum AnalogHat {
    LeftHatX = 15,
    LeftHatY = 16,
    RightHatX = 17,
    RightHatY = 18,
};
enum Sensor {
    //Sensors inside the Sixaxis Dualshock 3 controller
    aX = 50,
    aY = 52,
    aZ = 54,
    gZ = 56,
    
    //Sensors inside the move motion controller - it only reads the 2nd frame
    aXmove = 28,
    aZmove = 30,
    aYmove = 32,
    
    gXmove = 40,
    gZmove = 42,
    gYmove = 44,
    
    tempMove = 46,
    
    mXmove = 47,
    mZmove = 49,
    mYmove = 50,            
};
enum Angle {
    Pitch = 0x01,
    Roll = 0x02,
};
enum Status {
    // byte location | bit location
    Plugged = (38 << 8) | 0x02,
    Unplugged = (38 << 8) | 0x03,
    
    Charging = (39 << 8) | 0xEE,
    NotCharging = (39 << 8) | 0xF1,
    Shutdown = (39 << 8) | 0x01,
    Dying = (39 << 8) | 0x02,
    Low = (39 << 8) | 0x03,
    High = (39 << 8) | 0x04,
    Full = (39 << 8) | 0x05,
    
    MoveCharging = (21 << 8) | 0xEE,
    MoveNotCharging = (21 << 8) | 0xF1,
    MoveShutdown = (21 << 8) | 0x01,
    MoveDying = (21 << 8) | 0x02,
    MoveLow = (21 << 8) | 0x03,
    MoveHigh = (21 << 8) | 0x04,
    MoveFull = (21 << 8) | 0x05,
    
    CableRumble = (40 << 8) | 0x10,//Opperating by USB and rumble is turned on
    Cable = (40 << 8) | 0x12,//Opperating by USB and rumble is turned off 
    BluetoothRumble = (40 << 8) | 0x14,//Opperating by bluetooth and rumble is turned on
    Bluetooth = (40 << 8) | 0x16,//Opperating by bluetooth and rumble is turned off                        
};
enum Rumble {
    RumbleHigh = 0x10,
    RumbleLow = 0x20,            
};

class PS3BT : public BluetoothService {
public:
    PS3BT(BTD *pBtd, uint8_t btadr5=0, uint8_t btadr4=0, uint8_t btadr3=0, uint8_t btadr2=0, uint8_t btadr1=0, uint8_t btadr0=0);
    
    // BluetoothService implementation
    virtual void ACLData(uint8_t* ACLData); // Used to pass acldata to the services
    virtual void Run(); // Used to run part of the state maschine
    virtual void Reset(); // Use this to reset the service
    virtual void disconnect(); // Use this void to disconnect any of the controllers
            
    /* PS3 Controller Commands */
    bool getButton(Button b);
    uint8_t getAnalogButton(AnalogButton a);
    uint8_t getAnalogHat(AnalogHat a);
    int16_t getSensor(Sensor a);
    double getAngle(Angle a);
    bool getStatus(Status c);  
    String getStatusString();    
    String getTemperature();
    
    /* HID Commands */
    /* Commands for Dualshock 3 and Navigation controller */    
    void setAllOff();
    void setRumbleOff();
    void setRumbleOn(Rumble mode);
    void setLedOff(LED a);
    void setLedOn(LED a);    
    void setLedToggle(LED a);
    
    /* Commands for Motion controller only */    
    void moveSetBulb(uint8_t r, uint8_t g, uint8_t b);//Use this to set the Color using RGB values
    void moveSetBulb(Colors color);//Use this to set the Color using the predefined colors in "enum Colors"
    void moveSetRumble(uint8_t rumble);
    
    bool PS3Connected;// Variable used to indicate if the normal playstation controller is successfully connected
    bool PS3MoveConnected;// Variable used to indicate if the move controller is successfully connected
    bool PS3NavigationConnected;// Variable used to indicate if the navigation controller is successfully connected
    bool buttonChanged;//Indicate if a button has been changed
    bool buttonPressed;//Indicate if a button has been pressed
    bool buttonReleased;//Indicate if a button has been released
    
private:
    /* mandatory members */
    BTD *pBtd;
    
    void L2CAP_task(); // L2CAP state machine
    
    /* Variables filled from HCI event management */
    int16_t hci_handle;
    uint8_t remote_name[30]; // first 30 chars of remote name          
    
    /* variables used by high level L2CAP task */    
    uint8_t l2cap_state;
    uint16_t l2cap_event_flag;// l2cap flags of received bluetooth events
    
    unsigned long timer;
    
    uint32_t ButtonState;
    uint32_t OldButtonState;
    uint32_t timerHID;// timer used see if there has to be a delay before a new HID command
    uint32_t timerBulbRumble;// used to continuously set PS3 Move controller Bulb and rumble values
    
    uint8_t l2capinbuf[BULK_MAXPKTSIZE]; // General purpose buffer for l2cap in data
    uint8_t HIDBuffer[HID_BUFFERSIZE];// Used to store HID commands
    uint8_t HIDMoveBuffer[HID_BUFFERSIZE];// Used to store HID commands for the Move controller   
    
    /* L2CAP Channels */
    uint8_t control_scid[2];// L2CAP source CID for HID_Control                
    uint8_t control_dcid[2];//0x0040
    uint8_t interrupt_scid[2];// L2CAP source CID for HID_Interrupt        
    uint8_t interrupt_dcid[2];//0x0041
    uint8_t identifier;//Identifier for connection    
    
    /* HID Commands */
    void HID_Command(uint8_t* data, uint8_t nbytes);
    void HIDMove_Command(uint8_t* data, uint8_t nbytes);
    void enable_sixaxis();//Command used to enable the Dualshock 3 and Navigation controller to send data via USB
};
#endif