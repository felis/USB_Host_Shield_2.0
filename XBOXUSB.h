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

#define XBOX_REPORT_BUFFER_SIZE 14 // Size of the input report buffer

// Used in control endpoint header for HID Commands
#define bmREQ_HID_OUT USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define HID_REQUEST_SET_REPORT      0x09

#define XBOX_MAX_ENDPOINTS   3

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
    // byte location | bit location    
    UP = (2 << 8) | 0x01,
    DOWN = (2 << 8) | 0x02,
    LEFT = (2 << 8) | 0x04,
    RIGHT = (2 << 8) | 0x08,
    
    START = (2 << 8) | 0x10,
    BACK = (2 << 8) | 0x20,
    L3 = (2 << 8) | 0x40,
    R3 = (2 << 8) | 0x80,
    
    L1 = (3 << 8) | 0x01,
    R1 = (3 << 8) | 0x02,
    XBOX = (3 << 8) | 0x04,
    
    A = (3 << 8) | 0x10,
    B = (3 << 8) | 0x20,
    X = (3 << 8) | 0x40,
    Y = (3 << 8) | 0x80,  
    
    // These buttons are analog
    L2 = 4,
    R2 = 5,
};
enum AnalogHat {
    LeftHatX = 6,
    LeftHatY = 8,
    RightHatX = 10,
    RightHatY = 12,
};

class XBOXUSB : public USBDeviceConfig {
public:
    XBOXUSB(USB *pUsb);
    
    // USBDeviceConfig implementation
    virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
    virtual uint8_t Release();
    virtual uint8_t Poll();
    virtual uint8_t GetAddress() { return bAddress; };
    virtual bool isReady() { return bPollEnable; };
    
    /* XBOX Controller Readings */
    uint8_t getButton(Button b);
    int16_t getAnalogHat(AnalogHat a);
    
    /* Commands for Dualshock 3 and Navigation controller */    
    void setAllOff() { setRumbleOn(0,0); setLedOff(); };
    void setRumbleOff() { setRumbleOn(0,0); };
    void setRumbleOn(uint8_t lValue, uint8_t rValue);
    void setLedOff();
    void setLedOn(LED l); 
    void setLedBlink(LED l);
    void setLedMode(LEDMode lm);
        
    bool Xbox360Connected;// Variable used to indicate if the XBOX 360 controller is successfully connected
    bool buttonChanged;//Indicate if a button has been changed
    bool buttonPressed;//Indicate if a button has been pressed
    bool buttonReleased;//Indicate if a button has been released
    
protected:           
    /* mandatory members */
    USB *pUsb;
    uint8_t	bAddress; // device address
    EpInfo epInfo[XBOX_MAX_ENDPOINTS]; //endpoint info structure
    
private:    
    bool bPollEnable;    

    uint32_t ButtonState;
    uint32_t OldButtonState;     
    
    uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
    uint8_t writeBuf[EP_MAXPKTSIZE]; // General purpose buffer for output data
    
    void readReport(); // read incoming data
    void printReport(); // print incoming date - Uncomment for debugging

    /* Private commands */
    void XboxCommand(uint8_t* data, uint16_t nbytes);
};
#endif
