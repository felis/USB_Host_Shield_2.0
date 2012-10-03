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

#define PS3_REPORT_BUFFER_SIZE      48 // Size of the output report buffer for the Dualshock and Navigation controllers
#define MOVE_REPORT_BUFFER_SIZE     7 // Size of the output report buffer for the Move Controller

// used in control endpoint header for HID Commands
#define bmREQ_HID_OUT USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define HID_REQUEST_SET_REPORT      0x09

#define PS3_MAX_ENDPOINTS   3

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
    /*
       getButtonPress will return true as long as the button is held down
       While getButtonClick will only return it once
       So you instance if you need to increase a variable once you would use getButtonClick,
       but if you need to drive a robot forward you would use getButtonPress
    */
    bool getButtonPress(Button b);
    bool getButtonClick(Button b);
    
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
