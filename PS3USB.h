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

#define PS3_REPORT_BUFFER_SIZE      48 // Size of the output report buffer for the Dualshock and Navigation controllers
#define MOVE_REPORT_BUFFER_SIZE     7 // Size of the output report buffer for the Move Controller

// used in control endpoint header for HID Commands
#define bmREQ_HID_OUT USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define HID_REQUEST_SET_REPORT      0x09

#define PS3_MAX_ENDPOINTS   3

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
    SELECT = (2 << 8) | 0x01,
    L3 = (2 << 8) | 0x02,
    R3 = (2 << 8) | 0x04,
    START = (2 << 8) | 0x08,
    UP = (2 << 8) | 0x10,
    RIGHT = (2 << 8) | 0x20,
    DOWN = (2 << 8) | 0x40,
    LEFT = (2 << 8) | 0x80,
    
    L2 = (3 << 8) | 0x01,
    R2 = (3 << 8) | 0x02,
    L1 = (3 << 8) | 0x04,
    R1 = (3 << 8) | 0x08,
    TRIANGLE = (3 << 8) | 0x10,
    CIRCLE = (3 << 8) | 0x20,
    CROSS = (3 << 8) | 0x40,
    SQUARE = (3 << 8) | 0x80,
    
    PS = (4 << 8) | 0x01, 
};
enum AnalogButton {
    // Sixaxis Dualshcock 3 & Navigation controller
    UP_ANALOG = 14,
    RIGHT_ANALOG = 15,
    DOWN_ANALOG = 16,
    LEFT_ANALOG = 17,
    
    L2_ANALOG = 18,
    R2_ANALOG = 19,
    L1_ANALOG = 20,
    R1_ANALOG = 21,
    TRIANGLE_ANALOG = 22,
    CIRCLE_ANALOG = 23,
    CROSS_ANALOG = 24,
    SQUARE_ANALOG = 25,  
};
enum AnalogHat {
    LeftHatX = 6,
    LeftHatY = 7,
    RightHatX = 8,
    RightHatY = 9,
};
enum Sensor {
    // Sensors inside the Sixaxis Dualshock 3 controller
    aX = 41,
    aY = 43,
    aZ = 45,
    gZ = 47, 
};
enum Angle {
    Pitch = 0x01,
    Roll = 0x02,
};
enum Status {
    // byte location | bit location
    Plugged = (29 << 8) | 0x02,
    Unplugged = (29 << 8) | 0x03,
    
    Charging = (30 << 8) | 0xEE,
    NotCharging = (30 << 8) | 0xF1,
    Shutdown = (30 << 8) | 0x01,
    Dying = (30 << 8) | 0x02,
    Low = (30 << 8) | 0x03,
    High = (30 << 8) | 0x04,
    Full = (30 << 8) | 0x05,
   
    CableRumble = (31 << 8) | 0x10, // Opperating by USB and rumble is turned on
    Cable = (31 << 8) | 0x12, // Opperating by USB and rumble is turned off 
    BluetoothRumble = (31 << 8) | 0x14, // Opperating by bluetooth and rumble is turned on
    Bluetooth = (31 << 8) | 0x16, // Opperating by bluetooth and rumble is turned off                        
};
enum Rumble {
    RumbleHigh = 0x10,
    RumbleLow = 0x20,            
};

class PS3USB : public USBDeviceConfig {
public:
    PS3USB(USB *pUsb, uint8_t btadr5=0, uint8_t btadr4=0, uint8_t btadr3=0, uint8_t btadr2=0, uint8_t btadr1=0, uint8_t btadr0=0);
    
    // USBDeviceConfig implementation
    virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
    virtual uint8_t Release();
    virtual uint8_t Poll();
    virtual uint8_t GetAddress() { return bAddress; };
    virtual bool isReady() { return bPollEnable; };    
    
    void setBdaddr(uint8_t* BDADDR);
    void setMoveBdaddr(uint8_t* BDADDR);
    
    /* PS3 Controller Commands */
    bool getButton(Button b);
    uint8_t getAnalogButton(AnalogButton a);
    uint8_t getAnalogHat(AnalogHat a);
    uint16_t getSensor(Sensor a);
    double getAngle(Angle a);
    bool getStatus(Status c);  
    String getStatusString();
    
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
    bool PS3NavigationConnected;// Variable used to indicate if the navigation controller is successfully connected */
    bool buttonChanged;//Indicate if a button has been changed
    bool buttonPressed;//Indicate if a button has been pressed
    bool buttonReleased;//Indicate if a button has been released
    
protected:           
    /* mandatory members */
    USB *pUsb;
    uint8_t	bAddress; // device address
    EpInfo epInfo[PS3_MAX_ENDPOINTS]; //endpoint info structure
    
private:    
    bool bPollEnable;
    
    uint32_t timer; // used to continuously set PS3 Move controller Bulb and rumble values

    uint32_t ButtonState;
    uint32_t OldButtonState;     
    
    uint8_t my_bdaddr[6]; // Change to your dongles Bluetooth address in the constructor
    uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
    uint8_t writeBuf[EP_MAXPKTSIZE]; // General purpose buffer for output data
    
    void readReport(); // read incoming data
    void printReport(); // print incoming date - Uncomment for debugging

    /* Private commands */
    void PS3_Command(uint8_t* data, uint16_t nbytes);
    void enable_sixaxis();//Command used to enable the Dualshock 3 and Navigation controller to send data via USB
    void Move_Command(uint8_t* data, uint16_t nbytes);    
};
#endif
