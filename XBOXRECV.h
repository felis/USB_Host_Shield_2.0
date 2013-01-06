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

#define MADCATZ_VID                             0x1BAD  // For unofficial Mad Catz controllers

#define XBOX_MAX_ENDPOINTS   9

enum LED {
    ALL = 0x01, // Used to blink all LEDs
    LED1 = 0x02,
    LED2 = 0x03,
    LED3 = 0x04,
    LED4 = 0x05,
};
enum LEDMode {
    ROTATING = 0x0A,
    FASTBLINK = 0x0B,
    SLOWBLINK = 0x0C,
    ALTERNATING = 0x0D,    
};

enum Button {
    L1 = 0x0001,
    R1 = 0x0002,
    XBOX = 0x0004,
    SYNC = 0x0008,
    
    A = 0x0010,
    B = 0x0020,
    X = 0x0040,
    Y = 0x0080,
    
    UP = 0x0100,
    DOWN = 0x0200,
    LEFT = 0x0400,
    RIGHT = 0x0800,
    
    START = 0x1000,
    BACK = 0x2000,
    L3 = 0x4000,
    R3 = 0x8000,
    
    // These buttons are analog buttons
    L2,
    R2,
};
enum AnalogHat {
    LeftHatX = 0,
    LeftHatY = 1,
    RightHatX = 2,
    RightHatY = 3,
};

class XBOXRECV : public USBDeviceConfig {
public:
    XBOXRECV(USB *pUsb);
    
    // USBDeviceConfig implementation
    virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
    virtual uint8_t Release();
    virtual uint8_t Poll();
    virtual uint8_t GetAddress() { return bAddress; };
    virtual bool isReady() { return bPollEnable; };
    
    /* 
     Xbox Controller Readings.
     getButtonPress will return true as long as the button is held down
     While getButtonClick will only return it once
     So for instance if you need to increase a variable once you would use getButtonClick,
     but if you need to drive a robot forward you would use getButtonPress
    */
    uint8_t getButtonPress(uint8_t controller, Button b);
    bool getButtonClick(uint8_t controller, Button b);
    int16_t getAnalogHat(uint8_t controller, AnalogHat a);
    
    /* Xbox Controller Command */
    void setAllOff(uint8_t controller) { setRumbleOn(controller,0,0); setLedOff(controller); };
    void setRumbleOff(uint8_t controller) { setRumbleOn(controller,0,0); };
    void setRumbleOn(uint8_t controller, uint8_t lValue, uint8_t rValue);
    void setLedRaw(uint8_t controller, uint8_t value);
    void setLedOff(uint8_t controller) { setLedRaw(controller,0); };
    void setLedOn(uint8_t controller, LED l);
    void setLedBlink(uint8_t controller, LED l);
    void setLedMode(uint8_t controller, LEDMode lm);
    uint8_t getBatteryLevel(uint8_t controller); // Returns the battery level in percentage in 25% steps
    
    bool XboxReceiverConnected; // True if a wireless receiver is connected
    uint8_t Xbox360Connected[4]; // Variable used to indicate if the XBOX 360 controller is successfully connected
    
protected:           
    /* Mandatory members */
    USB *pUsb;
    uint8_t	bAddress; // device address
    EpInfo epInfo[XBOX_MAX_ENDPOINTS]; //endpoint info structure
    
private:    
    bool bPollEnable;    

    /* Variables to store the buttons */
    uint32_t ButtonState[4];
    uint32_t OldButtonState[4];
    uint16_t ButtonClickState[4];
    int16_t hatValue[4][4];
    uint16_t controllerStatus[4];
    
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