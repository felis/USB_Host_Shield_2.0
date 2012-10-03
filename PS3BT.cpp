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

#include "PS3BT.h"
#define DEBUG // Uncomment to print data for debugging
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the PS3 Controllers

const uint8_t OUTPUT_REPORT_BUFFER[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 
    0xff, 0x27, 0x10, 0x00, 0x32, 
    0xff, 0x27, 0x10, 0x00, 0x32, 
    0xff, 0x27, 0x10, 0x00, 0x32, 
    0xff, 0x27, 0x10, 0x00, 0x32, 
    0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

PS3BT::PS3BT(BTD *p, uint8_t btadr5, uint8_t btadr4, uint8_t btadr3, uint8_t btadr2, uint8_t btadr1, uint8_t btadr0):
pBtd(p) // pointer to USB class instance - mandatory
{
    if (pBtd)
		pBtd->registerServiceClass(this); // Register it as a Bluetooth service
    
    pBtd->my_bdaddr[5] = btadr5; // Change to your dongle's Bluetooth address instead
    pBtd->my_bdaddr[4] = btadr4;
    pBtd->my_bdaddr[3] = btadr3;
    pBtd->my_bdaddr[2] = btadr2;
    pBtd->my_bdaddr[1] = btadr1;
    pBtd->my_bdaddr[0] = btadr0;
            
    HIDBuffer[0] = 0x52;// HID BT Set_report (0x50) | Report Type (Output 0x02)
    HIDBuffer[1] = 0x01;// Report ID
    
    //Needed for PS3 Move Controller commands to work via bluetooth
    HIDMoveBuffer[0] = 0xA2;// HID BT DATA_request (0xA0) | Report Type (Output 0x02)
    HIDMoveBuffer[1] = 0x02;// Report ID
    
    /* Set device cid for the control and intterrupt channelse - LSB */
    control_dcid[0] = 0x40;//0x0040
    control_dcid[1] = 0x00;
    interrupt_dcid[0] = 0x41;//0x0041
    interrupt_dcid[1] = 0x00;
    
    Reset();
}
bool PS3BT::getButtonPress(Button b) {
    return (ButtonState & (uint32_t)b);
}
bool PS3BT::getButtonClick(Button b) {
    bool click = (ButtonClickState & (uint32_t)b);
    ButtonClickState &= ~((uint32_t)b);  // clear "click" event
    return click;
}
uint8_t PS3BT::getAnalogButton(AnalogButton a) {
    if (l2capinbuf == NULL)
        return 0;
    return (uint8_t)(l2capinbuf[(uint16_t)a]);
}
uint8_t PS3BT::getAnalogHat(AnalogHat a) {
    if (l2capinbuf == NULL)
        return 0;                        
    return (uint8_t)(l2capinbuf[(uint16_t)a]);
}
int16_t PS3BT::getSensor(Sensor a) {
    if (l2capinbuf == NULL)
        return 0;
    if (a == aX || a == aY || a == aZ || a == gZ)
        return ((l2capinbuf[(uint16_t)a] << 8) | l2capinbuf[(uint16_t)a + 1]);
    else if (a == mXmove || a == mYmove || a == mZmove) { // These are all 12-bits long
        if (a == mXmove || a == mYmove)
            return (((l2capinbuf[(uint16_t)a] & 0x0F) << 8) | (l2capinbuf[(uint16_t)a + 1]));
        else // mZmove            
            return ((l2capinbuf[(uint16_t)a] << 4) | (l2capinbuf[(uint16_t)a + 1] & 0xF0 ) >> 4);
    }
    else if (a == tempMove) // The tempearature is 12 bits long too
        return ((l2capinbuf[(uint16_t)a] << 4) | ((l2capinbuf[(uint16_t)a + 1] & 0xF0) >> 4));
    else // aXmove, aYmove, aZmove, gXmove, gYmove and gZmove
        return (l2capinbuf[(uint16_t)a] | (l2capinbuf[(uint16_t)a + 1] << 8));
}
double PS3BT::getAngle(Angle a) {
    double accXval;
    double accYval;
    double accZval;
    
    if(PS3Connected) {
        // Data for the Kionix KXPC4 used in the DualShock 3
        const double zeroG = 511.5; // 1.65/3.3*1023 (1,65V)
        accXval = -((double)getSensor(aX)-zeroG);
        accYval = -((double)getSensor(aY)-zeroG);
        accZval = -((double)getSensor(aZ)-zeroG);
    } else if(PS3MoveConnected) {
        // It's a Kionix KXSC4 inside the Motion controller
        const uint16_t zeroG = 0x8000;
        accXval = -(int16_t)(getSensor(aXmove)-zeroG);
        accYval = (int16_t)(getSensor(aYmove)-zeroG);
        accZval = (int16_t)(getSensor(aZmove)-zeroG);
    }
    
    // Convert to 360 degrees resolution
    // atan2 outputs the value of -π to π (radians)
    // We are then converting it to 0 to 2π and then to degrees
    if (a == Pitch) {
        double angle = (atan2(accYval,accZval)+PI)*RAD_TO_DEG;
        return angle;
    } else {
        double angle = (atan2(accXval,accZval)+PI)*RAD_TO_DEG;
        return angle;
    }
}
double PS3BT::get9DOFValues(Sensor a) { // Thanks to Manfred Piendl
    int16_t value = getSensor(a);
    if (a == mXmove || a == mYmove || a == mZmove) {
        if (value > 2047)
            value -= 0x1000;
        return (double)value/3.2;  // unit: muT = 10^(-6) Tesla
    }
    else if (a == aXmove || a == aYmove || a == aZmove) {
        if (value < 0)
            value += 0x8000;
        else
            value -= 0x8000;
        return (double)value/442.0; // unit: m/(s^2)
    }
    else if (a == gXmove || a == gYmove || a == gZmove) {
        if (value < 0)
            value += 0x8000;
        else
            value -= 0x8000;
        return (double)value/9.6; // unit: deg/s
    }
    else
        return 0;
}
String PS3BT::getTemperature() {
    if(PS3MoveConnected) {
        int16_t input = getSensor(tempMove);    
        
        String output = String(input/100);
        output += ".";
        if(input%100 < 10)
            output += "0";
        output += String(input%100);
        
        return output;        
    }
}
bool PS3BT::getStatus(Status c) {
    if (l2capinbuf == NULL)
        return false;
    if (l2capinbuf[(uint16_t)c >> 8] == ((uint8_t)c & 0xff))
        return true;
    return false;
}
String PS3BT::getStatusString() {
    if (PS3Connected || PS3NavigationConnected) {
        char statusOutput[100];
        
        strcpy(statusOutput,"ConnectionStatus: ");
        
        if (getStatus(Plugged)) strcat(statusOutput,"Plugged");
        else if (getStatus(Unplugged)) strcat(statusOutput,"Unplugged");
        else strcat(statusOutput,"Error");
        
        
        strcat(statusOutput," - PowerRating: ");
        if (getStatus(Charging)) strcat(statusOutput,"Charging");
        else if (getStatus(NotCharging)) strcat(statusOutput,"Not Charging");
        else if (getStatus(Shutdown)) strcat(statusOutput,"Shutdown");
        else if (getStatus(Dying)) strcat(statusOutput,"Dying");
        else if (getStatus(Low)) strcat(statusOutput,"Low");
        else if (getStatus(High)) strcat(statusOutput,"High");
        else if (getStatus(Full)) strcat(statusOutput,"Full");
        else strcat(statusOutput,"Error");
        
        strcat(statusOutput," - WirelessStatus: ");
        
        if (getStatus(CableRumble)) strcat(statusOutput,"Cable - Rumble is on");
        else if (getStatus(Cable)) strcat(statusOutput,"Cable - Rumble is off");
        else if (getStatus(BluetoothRumble)) strcat(statusOutput,"Bluetooth - Rumble is on");
        else if (getStatus(Bluetooth)) strcat(statusOutput,"Bluetooth - Rumble is off");
        else strcat(statusOutput,"Error");
        
        return statusOutput;
        
    }
    else if(PS3MoveConnected) {
        char statusOutput[50];
        
        strcpy(statusOutput,"PowerRating: ");
        
        if (getStatus(MoveCharging)) strcat(statusOutput,"Charging");
        else if (getStatus(MoveNotCharging)) strcat(statusOutput,"Not Charging");
        else if (getStatus(MoveShutdown)) strcat(statusOutput,"Shutdown");
        else if (getStatus(MoveDying)) strcat(statusOutput,"Dying");
        else if (getStatus(MoveLow)) strcat(statusOutput,"Low");
        else if (getStatus(MoveHigh)) strcat(statusOutput,"High");
        else if (getStatus(MoveFull)) strcat(statusOutput,"Full");
        else strcat(statusOutput,"Error");
        
        return statusOutput;
    }
}
void PS3BT::Reset() {
    PS3Connected = false;
    PS3MoveConnected = false;
    PS3NavigationConnected = false;
    l2cap_event_flag = 0; // Reset flags
    l2cap_state = L2CAP_WAIT;
    
    // Needed for PS3 Dualshock Controller commands to work via bluetooth
    for (uint8_t i = 0; i < OUTPUT_REPORT_BUFFER_SIZE; i++)
        HIDBuffer[i + 2] = pgm_read_byte(&OUTPUT_REPORT_BUFFER[i]); // First two bytes reserved for report type and ID    
}

void PS3BT::disconnect() { // Use this void to disconnect any of the controllers
    //First the HID interrupt channel has to be disconencted, then the HID control channel and finally the HCI connection
    pBtd->l2cap_disconnection_request(hci_handle,0x0A, interrupt_scid, interrupt_dcid);
    Reset();
    l2cap_state = L2CAP_INTERRUPT_DISCONNECT;
}

void PS3BT::ACLData(uint8_t* ACLData) {
    if(!pBtd->l2capConnectionClaimed && !PS3Connected && !PS3MoveConnected && !PS3NavigationConnected) {
        if (ACLData[8] == L2CAP_CMD_CONNECTION_REQUEST) {
            if((ACLData[12] | (ACLData[13] << 8)) == HID_CTRL_PSM) {
                pBtd->l2capConnectionClaimed = true; // Claim that the incoming connection belongs to this service
                hci_handle = pBtd->hci_handle; // Store the HCI Handle for the connection
                l2cap_state = L2CAP_WAIT;
                for(uint8_t i = 0; i < 30; i++)
                    remote_name[i] = pBtd->remote_name[i]; // Store the remote name for the connection
#ifdef DEBUG
                if(pBtd->hci_version < 3) { // Check the HCI Version of the Bluetooth dongle
                    Notify(PSTR("\r\nYour dongle may not support reading the analog buttons, sensors and status\r\nYour HCI Version is: "));
                    Serial.print(pBtd->hci_version);
                    Notify(PSTR("\r\nBut should be at least 3\r\nThis means that it doesn't support Bluetooth Version 2.0+EDR"));
                }
#endif                
            }
        }
    }
    if (((ACLData[0] | (ACLData[1] << 8)) == (hci_handle | 0x2000))) { //acl_handle_ok
        for(uint8_t i = 0; i < BULK_MAXPKTSIZE; i++)
            l2capinbuf[i] = ACLData[i];
        if ((l2capinbuf[6] | (l2capinbuf[7] << 8)) == 0x0001) { //l2cap_control - Channel ID for ACL-U
            if (l2capinbuf[8] == L2CAP_CMD_COMMAND_REJECT) {
#ifdef DEBUG
                    Notify(PSTR("\r\nL2CAP Command Rejected - Reason: "));
                    PrintHex<uint8_t>(l2capinbuf[13]);
                    Serial.print(" ");
                    PrintHex<uint8_t>(l2capinbuf[12]);
                    Serial.print(" Data: ");
                    PrintHex<uint8_t>(l2capinbuf[17]);
                    Serial.print(" ");
                    PrintHex<uint8_t>(l2capinbuf[16]);
                    Serial.print(" ");                
                    PrintHex<uint8_t>(l2capinbuf[15]);
                    Serial.print(" ");
                    PrintHex<uint8_t>(l2capinbuf[14]);     
#endif
                }
                else if (l2capinbuf[8] == L2CAP_CMD_CONNECTION_REQUEST) {                    
#ifdef EXTRADEBUG
                    Notify(PSTR("\r\nL2CAP Connection Request - PSM: "));
                    PrintHex<uint8_t>(l2capinbuf[13]);
                    Notify(PSTR(" "));
                    PrintHex<uint8_t>(l2capinbuf[12]);
                    Notify(PSTR(" SCID: "));
                    PrintHex<uint8_t>(l2capinbuf[15]);
                    Notify(PSTR(" "));
                    PrintHex<uint8_t>(l2capinbuf[14]);
                    Notify(PSTR(" Identifier: "));
                    PrintHex<uint8_t>(l2capinbuf[9]);
#endif
                    if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == HID_CTRL_PSM) {                    
                        identifier = l2capinbuf[9];
                        control_scid[0] = l2capinbuf[14];
                        control_scid[1] = l2capinbuf[15];
                        l2cap_event_flag |= L2CAP_FLAG_CONNECTION_CONTROL_REQUEST;
                    }
                    else if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == HID_INTR_PSM) {
                        identifier = l2capinbuf[9];
                        interrupt_scid[0] = l2capinbuf[14];
                        interrupt_scid[1] = l2capinbuf[15];
                        l2cap_event_flag |= L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST;
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_RESPONSE) {
                    if ((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) { // Success
                        if (l2capinbuf[12] == control_dcid[0] && l2capinbuf[13] == control_dcid[1]) {
                            //Serial.print("\r\nHID Control Configuration Complete");
                            l2cap_event_flag |= L2CAP_FLAG_CONFIG_CONTROL_SUCCESS;
                        }
                        else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1]) {
                            //Serial.print("\r\nHID Interrupt Configuration Complete");
                            l2cap_event_flag |= L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS;
                        }
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_REQUEST) {
                    if (l2capinbuf[12] == control_dcid[0] && l2capinbuf[13] == control_dcid[1]) {
                        //Serial.print("\r\nHID Control Configuration Request");
                        identifier = l2capinbuf[9]; 
                        l2cap_event_flag |= L2CAP_FLAG_CONFIG_CONTROL_REQUEST;
                    }
                    else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1]) {
                        //Serial.print("\r\nHID Interrupt Configuration Request");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_FLAG_CONFIG_INTERRUPT_REQUEST;
                    }
                }                                    
                else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_REQUEST) {
                    if (l2capinbuf[12] == control_dcid[0] && l2capinbuf[13] == control_dcid[1]) {
#ifdef DEBUG
                        Notify(PSTR("\r\nDisconnect Request: Control Channel"));
#endif
                        identifier = l2capinbuf[9];
                        pBtd->l2cap_disconnection_response(hci_handle,identifier,control_dcid,control_scid);
                        Reset();
                    }
                    else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1]) {
#ifdef DEBUG
                        Notify(PSTR("\r\nDisconnect Request: Interrupt Channel"));                
#endif
                        identifier = l2capinbuf[9];
                        pBtd->l2cap_disconnection_response(hci_handle,identifier,interrupt_dcid,interrupt_scid);
                        Reset();
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_RESPONSE) {
                    if (l2capinbuf[12] == control_scid[0] && l2capinbuf[13] == control_scid[1]) {                                        
                        //Serial.print("\r\nDisconnect Response: Control Channel");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE;
                    }
                    else if (l2capinbuf[12] == interrupt_scid[0] && l2capinbuf[13] == interrupt_scid[1]) {                                        
                        //Serial.print("\r\nDisconnect Response: Interrupt Channel");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE;
                    }
                }
#ifdef EXTRADEBUG
                else {
                    Notify(PSTR("\r\nL2CAP Unknown Signaling Command: "));
                    PrintHex<uint8_t>(l2capinbuf[8]);
                }
#endif
        } else if (l2capinbuf[6] == interrupt_dcid[0] && l2capinbuf[7] == interrupt_dcid[1]) { // l2cap_interrupt
            //Serial.print("\r\nL2CAP Interrupt");
            if(PS3Connected || PS3MoveConnected || PS3NavigationConnected) {
                /* Read Report */
                if(l2capinbuf[8] == 0xA1) { // HID_THDR_DATA_INPUT
                    if(PS3Connected || PS3NavigationConnected)
                        ButtonState = (uint32_t)(l2capinbuf[11] | ((uint16_t)l2capinbuf[12] << 8) | ((uint32_t)l2capinbuf[13] << 16));
                    else if(PS3MoveConnected)
                        ButtonState = (uint32_t)(l2capinbuf[10] | ((uint16_t)l2capinbuf[11] << 8) | ((uint32_t)l2capinbuf[12] << 16));
                    
                    //Notify(PSTR("\r\nButtonState");
                    //PrintHex<uint32_t>(ButtonState);
                    
                    if(ButtonState != OldButtonState) {
                        ButtonClickState = ButtonState & ~OldButtonState; // Update click state variable
                        OldButtonState = ButtonState;
                    }
                    
#ifdef PRINTREPORT // Uncomment "#define PRINTREPORT" to print the report send by the PS3 Controllers
                    for(uint8_t i = 10; i < 58;i++) {
                        PrintHex<uint8_t>(l2capinbuf[i]);
                        Serial.print(" ");
                    }
                    Serial.println();
#endif
                }
            }
        }
        L2CAP_task();
    }
}
void PS3BT::L2CAP_task() {
    switch (l2cap_state) {
        case L2CAP_WAIT:
            if (l2cap_connection_request_control_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Control Incoming Connection Request"));
#endif
                pBtd->l2cap_connection_response(hci_handle,identifier, control_dcid, control_scid, PENDING);
                delay(1);
                pBtd->l2cap_connection_response(hci_handle,identifier, control_dcid, control_scid, SUCCESSFUL);
                identifier++;
                delay(1);
                pBtd->l2cap_config_request(hci_handle,identifier, control_scid);
                l2cap_state = L2CAP_CONTROL_REQUEST;
            }
            break;
        case L2CAP_CONTROL_REQUEST:
            if (l2cap_config_request_control_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Control Configuration Request"));
#endif
                pBtd->l2cap_config_response(hci_handle,identifier, control_scid);
                l2cap_state = L2CAP_CONTROL_SUCCESS;
            }
            break;
            
        case L2CAP_CONTROL_SUCCESS:
            if (l2cap_config_success_control_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Control Successfully Configured"));
#endif
                l2cap_state = L2CAP_INTERRUPT_SETUP;
            }
            break;
        case L2CAP_INTERRUPT_SETUP:
            if (l2cap_connection_request_interrupt_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Interrupt Incoming Connection Request"));
#endif
                pBtd->l2cap_connection_response(hci_handle,identifier, interrupt_dcid, interrupt_scid, PENDING);
                delay(1);
                pBtd->l2cap_connection_response(hci_handle,identifier, interrupt_dcid, interrupt_scid, SUCCESSFUL);
                identifier++;
                delay(1);
                pBtd->l2cap_config_request(hci_handle,identifier, interrupt_scid);
                
                l2cap_state = L2CAP_INTERRUPT_REQUEST;
            }
            break;
        case L2CAP_INTERRUPT_REQUEST:
            if (l2cap_config_request_interrupt_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Interrupt Configuration Request"));
#endif
                pBtd->l2cap_config_response(hci_handle,identifier, interrupt_scid);
                l2cap_state = L2CAP_INTERRUPT_SUCCESS;
            }
            break;
        case L2CAP_INTERRUPT_SUCCESS:
            if (l2cap_config_success_interrupt_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Interrupt Successfully Configured"));
#endif
                if(remote_name[0] == 'M') { // First letter in Motion Controller ('M')
                    for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++) // Reset l2cap in buffer as it sometimes read it as a button has been pressed
                        l2capinbuf[i] = 0;
                    ButtonState = 0;
                    OldButtonState = 0;
                    
                    l2cap_state = L2CAP_HID_PS3_LED;
                } else
                    l2cap_state = L2CAP_HID_ENABLE_SIXAXIS;
                timer = millis();
            }
            break;

        /* These states are handled in Run() */
            
        case L2CAP_INTERRUPT_DISCONNECT:
            if (l2cap_disconnect_response_interrupt_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected Interrupt Channel"));
#endif
                identifier++;
                pBtd->l2cap_disconnection_request(hci_handle, identifier, control_scid, control_dcid);                
                l2cap_state = L2CAP_CONTROL_DISCONNECT;
            }
            break;
            
        case L2CAP_CONTROL_DISCONNECT:
            if (l2cap_disconnect_response_control_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected Control Channel"));
#endif
                pBtd->hci_disconnect(hci_handle);
                l2cap_event_flag = 0; // Reset flags
                l2cap_state = L2CAP_WAIT;
            }
            break;
    }    
}
void PS3BT::Run() {
    switch (l2cap_state) {
        case L2CAP_HID_ENABLE_SIXAXIS:
            if(millis() - timer > 1000) { // loop 1 second before sending the command
                for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++) // Reset l2cap in buffer as it sometimes read it as a button has been pressed
                    l2capinbuf[i] = 0;
                ButtonState = 0;
                OldButtonState = 0;
                
                enable_sixaxis();
                for (uint8_t i = 15; i < 19; i++)
                    l2capinbuf[i] = 0x7F; // Set the analog joystick values to center position
                l2cap_state = L2CAP_HID_PS3_LED;
                timer = millis();
            }
            break;
            
        case L2CAP_HID_PS3_LED:
            if(millis() - timer > 1000) { // loop 1 second before sending the command
                if (remote_name[0] == 'P') { // First letter in PLAYSTATION(R)3 Controller ('P')
                    setLedOn(LED1);
#ifdef DEBUG
                    Notify(PSTR("\r\nDualshock 3 Controller Enabled\r\n"));
#endif
                    PS3Connected = true;
                } else if (remote_name[0] == 'N') { // First letter in Navigation Controller ('N')
                    setLedOn(LED1); // This just turns LED constantly on, on the Navigation controller
#ifdef DEBUG
                    Notify(PSTR("\r\nNavigation Controller Enabled\r\n"));
#endif
                    PS3NavigationConnected = true;
                } else if(remote_name[0] == 'M') { // First letter in Motion Controller ('M')
                    moveSetBulb(Red);
                    timerBulbRumble = millis();
#ifdef DEBUG
                    Notify(PSTR("\r\nMotion Controller Enabled\r\n"));
#endif
                    PS3MoveConnected = true;
                }
                l2cap_state = L2CAP_DONE;
            }
            break;
            
        case L2CAP_DONE:
            if (PS3MoveConnected) { //The Bulb and rumble values, has to be send at aproximatly every 5th second for it to stay on
                if (millis() - timerBulbRumble > 4000) { //Send at least every 4th second
                    HIDMove_Command(HIDMoveBuffer, HID_BUFFERSIZE);//The Bulb and rumble values, has to be written again and again, for it to stay turned on
                    timerBulbRumble = millis();
                }
            }
            break;
    }
}

/************************************************************/
/*                    HID Commands                          */
/************************************************************/

//Playstation Sixaxis Dualshock and Navigation Controller commands
void PS3BT::HID_Command(uint8_t* data, uint8_t nbytes) {    
    if (millis() - timerHID <= 250)// Check if is has been more than 250ms since last command                
        delay((uint32_t)(250 - (millis() - timerHID)));//There have to be a delay between commands
    pBtd->L2CAP_Command(hci_handle,data,nbytes,control_scid[0],control_scid[1]); // Both the Navigation and Dualshock controller sends data via the control channel
    timerHID = millis();
}
void PS3BT::setAllOff() {
    for (uint8_t i = 0; i < OUTPUT_REPORT_BUFFER_SIZE; i++)
        HIDBuffer[i + 2] = pgm_read_byte(&OUTPUT_REPORT_BUFFER[i]);//First two bytes reserved for report type and ID
    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);
}
void PS3BT::setRumbleOff() {
    HIDBuffer[3] = 0x00;
    HIDBuffer[4] = 0x00;//low mode off
    HIDBuffer[5] = 0x00;
    HIDBuffer[6] = 0x00;//high mode off
    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);
}
void PS3BT::setRumbleOn(Rumble mode) {
    /* Still not totally sure how it works, maybe something like this instead?
     * 3 - duration_right
     * 4 - power_right
     * 5 - duration_left
     * 6 - power_left
     */
    if ((mode & 0x30) > 0) {
        HIDBuffer[3] = 0xfe;
        HIDBuffer[5] = 0xfe;        
        if (mode == RumbleHigh) {
            HIDBuffer[4] = 0;//low mode off
            HIDBuffer[6] = 0xff;//high mode on
        }
        else {
            HIDBuffer[4] = 0xff;//low mode on
            HIDBuffer[6] = 0;//high mode off
        }        
        HID_Command(HIDBuffer, HID_BUFFERSIZE);
    }
}
void PS3BT::setLedOff(LED a) {
    HIDBuffer[11] &= ~((uint8_t)(((uint16_t)a & 0x0f) << 1));    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);               
}
void PS3BT::setLedOn(LED a) {
    HIDBuffer[11] |= (uint8_t)(((uint16_t)a & 0x0f) << 1);    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);            
}
void PS3BT::setLedToggle(LED a) {
    HIDBuffer[11] ^= (uint8_t)(((uint16_t)a & 0x0f) << 1);    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);            
}
void PS3BT::enable_sixaxis() { //Command used to enable the Dualshock 3 and Navigation controller to send data via USB
    uint8_t cmd_buf[6];
    cmd_buf[0] = 0x53;// HID BT Set_report (0x50) | Report Type (Feature 0x03)
    cmd_buf[1] = 0xF4;// Report ID
    cmd_buf[2] = 0x42;// Special PS3 Controller enable commands
    cmd_buf[3] = 0x03;
    cmd_buf[4] = 0x00;
    cmd_buf[5] = 0x00;
    
    HID_Command(cmd_buf, 6);
}

//Playstation Move Controller commands
void PS3BT::HIDMove_Command(uint8_t* data,uint8_t nbytes) {
    if (millis() - timerHID <= 250)// Check if is has been less than 200ms since last command
        delay((uint32_t)(250 - (millis() - timerHID)));//There have to be a delay between commands
    pBtd->L2CAP_Command(hci_handle,data,nbytes,interrupt_scid[0],interrupt_scid[1]); // The Move controller sends it's data via the intterrupt channel
    timerHID = millis();
}
void PS3BT::moveSetBulb(uint8_t r, uint8_t g, uint8_t b) { //Use this to set the Color using RGB values
    //set the Bulb's values into the write buffer            
    HIDMoveBuffer[3] = r;
    HIDMoveBuffer[4] = g;
    HIDMoveBuffer[5] = b;
    
    HIDMove_Command(HIDMoveBuffer, HID_BUFFERSIZE);   
}
void PS3BT::moveSetBulb(Colors color) { //Use this to set the Color using the predefined colors in enum
    moveSetBulb((uint8_t)(color >> 16),(uint8_t)(color >> 8),(uint8_t)(color));
}
void PS3BT::moveSetRumble(uint8_t rumble) {
#ifdef DEBUG
    if(rumble < 64 && rumble != 0) // The rumble value has to at least 64, or approximately 25% (64/255*100)
        Notify(PSTR("\r\nThe rumble value has to at least 64, or approximately 25%"));
#endif
    //set the rumble value into the write buffer
    HIDMoveBuffer[7] = rumble;
    
    HIDMove_Command(HIDMoveBuffer, HID_BUFFERSIZE);            
}