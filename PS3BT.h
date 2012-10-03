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

class PS3BT : public BluetoothService {
public:
    PS3BT(BTD *pBtd, uint8_t btadr5=0, uint8_t btadr4=0, uint8_t btadr3=0, uint8_t btadr2=0, uint8_t btadr1=0, uint8_t btadr0=0);
    
    // BluetoothService implementation
    virtual void ACLData(uint8_t* ACLData); // Used to pass acldata to the services
    virtual void Run(); // Used to run part of the state maschine
    virtual void Reset(); // Use this to reset the service
    virtual void disconnect(); // Use this void to disconnect any of the controllers
            
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
    int16_t getSensor(Sensor a);
    double getAngle(Angle a);
    double get9DOFValues(Sensor a);
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
    uint32_t ButtonClickState;
    
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