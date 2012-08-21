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

#include "Wii.h"
#define DEBUG // Uncomment to print data for debugging
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the Wiimote

WII::WII(BTD *p, uint8_t btadr5, uint8_t btadr4, uint8_t btadr3, uint8_t btadr2, uint8_t btadr1, uint8_t btadr0):
pBtd(p) // pointer to USB class instance - mandatory
{
    if (pBtd)
        pBtd->wiiServiceID = pBtd->registerServiceClass(this); // Register it as a Bluetooth service
    
    pBtd->disc_bdaddr[5] = btadr5; // Change to your dongle's Bluetooth address instead
    pBtd->disc_bdaddr[4] = btadr4;
    pBtd->disc_bdaddr[3] = btadr3;
    pBtd->disc_bdaddr[2] = btadr2;
    pBtd->disc_bdaddr[1] = btadr1;
    pBtd->disc_bdaddr[0] = btadr0;
            
    HIDBuffer[0] = 0xA2;// HID BT DATA_request (0x50) | Report Type (Output 0x02)
    
    /* Set device cid for the control and intterrupt channelse - LSB */
    control_dcid[0] = 0x60;//0x0060
    control_dcid[1] = 0x00;
    interrupt_dcid[0] = 0x61;//0x0061
    interrupt_dcid[1] = 0x00;
    
    Reset();
}
void WII::Reset() {
    connected = false;
    l2cap_event_flag = 0; // Reset flags
    l2cap_state = L2CAP_WAIT;
}

void WII::disconnect() { // Use this void to disconnect any of the controllers
    //First the HID interrupt channel has to be disconencted, then the HID control channel and finally the HCI connection
    pBtd->l2cap_disconnection_request(hci_handle,0x0A, interrupt_scid, interrupt_dcid);
    Reset();
    l2cap_state = L2CAP_INTERRUPT_DISCONNECT;
}

void WII::ACLData(uint8_t* l2capinbuf) {
    if (((l2capinbuf[0] | (l2capinbuf[1] << 8)) == (hci_handle | 0x2000))) { //acl_handle_ok
        if ((l2capinbuf[6] | (l2capinbuf[7] << 8)) == 0x0001) { //l2cap_control - Channel ID for ACL-U
            if (l2capinbuf[8] == L2CAP_CMD_COMMAND_REJECT) {
#ifdef DEBUG
                Notify(PSTR("\r\nL2CAP Command Rejected - Reason: "));
                PrintHex<uint8_t>(l2capinbuf[13]);
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[12]);
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[17]);
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[16]);
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[15]);
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[14]);
#endif
            }
            else if (l2capinbuf[8] == L2CAP_CMD_CONNECTION_RESPONSE) {
                if (((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) && ((l2capinbuf[18] | (l2capinbuf[19] << 8)) == SUCCESSFUL)) { // Success
                    if (l2capinbuf[14] == control_dcid[0] && l2capinbuf[15] == control_dcid[1]) { // Success
                        //Serial.print("\r\nHID Control Connection Complete");
                        identifier = l2capinbuf[9];
                        control_scid[0] = l2capinbuf[12];
                        control_scid[1] = l2capinbuf[13];
                        l2cap_event_flag |= L2CAP_FLAG_CONTROL_CONNECTED;
                    }
                    else if (l2capinbuf[14] == interrupt_dcid[0] && l2capinbuf[15] == interrupt_dcid[1]) {
                        //Serial.print("\r\nHID Interrupt Connection Complete");
                        identifier = l2capinbuf[9];
                        interrupt_scid[0] = l2capinbuf[12];
                        interrupt_scid[1] = l2capinbuf[13];
                        l2cap_event_flag |= L2CAP_FLAG_INTERRUPT_CONNECTED;
                    }
                }
            }
            else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_RESPONSE) {
                if ((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) { // Success
                    if (l2capinbuf[12] == control_dcid[0] && l2capinbuf[13] == control_dcid[1]) {
                        //Serial.print("\r\nHID Control Configuration Complete");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_FLAG_CONFIG_CONTROL_SUCCESS;
                    }
                    else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1]) {
                        //Serial.print("\r\nHID Interrupt Configuration Complete");
                        identifier = l2capinbuf[9];                        
                        l2cap_event_flag |= L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS;
                    }
                }
            }
            else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_REQUEST) {
                if (l2capinbuf[12] == control_dcid[0] && l2capinbuf[13] == control_dcid[1]) {
                    //Serial.print("\r\nHID Control Configuration Request");
                    pBtd->l2cap_config_response(hci_handle, l2capinbuf[9], control_scid);
                }
                else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1]) {
                    //Serial.print("\r\nHID Interrupt Configuration Request");
                    pBtd->l2cap_config_response(hci_handle, l2capinbuf[9], interrupt_scid);
                }
            }
            else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_REQUEST) {
                if (l2capinbuf[12] == control_dcid[0] && l2capinbuf[13] == control_dcid[1]) {
#ifdef DEBUG
                    Notify(PSTR("\r\nDisconnect Request: Control Channel"));
#endif
                    connected = false;
                    identifier = l2capinbuf[9];
                    pBtd->l2cap_disconnection_response(hci_handle,identifier,control_dcid,control_scid);                    
                    Reset();
                }
                else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1]) {
#ifdef DEBUG
                    Notify(PSTR("\r\nDisconnect Request: Interrupt Channel"));
#endif
                    connected = false;
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
                identifier = l2capinbuf[9];
                Notify(PSTR("\r\nL2CAP Unknown Signaling Command: "));
                PrintHex<uint8_t>(l2capinbuf[8]);
            }
#endif
        } else if (l2capinbuf[6] == interrupt_dcid[0] && l2capinbuf[7] == interrupt_dcid[1]) { // l2cap_interrupt
            //Serial.print("\r\nL2CAP Interrupt");
            if(connected) {
                /* Read Report */
                if(l2capinbuf[8] == 0xA1) { // HID_THDR_DATA_INPUT
                    if(l2capinbuf[9] >= 0x30 && l2capinbuf[9] <= 0x37) { // These reports include the buttons
                        ButtonState = (uint16_t)((l2capinbuf[10] & 0x1F) | ((uint16_t)(l2capinbuf[11] & 0x9F) << 8));
                        ButtonClickState = ButtonState; // Update click state variable                        
#ifdef PRINTREPORT
                        Notify(PSTR("ButtonState: "));
                        PrintHex<uint16_t>(ButtonState);
                        Notify(PSTR("\r\n"));
#endif
                        if(ButtonState != OldButtonState) {
                            buttonChanged = true;
                            if(ButtonState != 0x0000) {
                                buttonPressed = true;
                                buttonReleased = false;
                            } else {
                                buttonPressed = false;
                                buttonReleased = true;
                            }
                        }
                        else {
                            buttonChanged = false;
                            buttonPressed = false;
                            buttonReleased = false;
                        }
                        OldButtonState = ButtonState;
                    }
                    if(l2capinbuf[9] == 0x31 || l2capinbuf[9] == 0x35) { // Read the accelerometer
                        int16_t accX = ((l2capinbuf[12] << 2) | (l2capinbuf[10] & 0x60 >> 5))-500;
                        int16_t accY = ((l2capinbuf[13] << 2) | (l2capinbuf[11] & 0x20 >> 4))-500;
                        int16_t accZ = ((l2capinbuf[14] << 2) | (l2capinbuf[11] & 0x40 >> 5))-500;
                        /*
                        Notify(PSTR("\r\naccX: "));
                        Serial.print(accX);
                        Notify(PSTR("\taccY: "));
                        Serial.print(accY);
                        Notify(PSTR("\taccZ: "));
                        Serial.print(accZ);
                        */                        
                        pitch = (atan2(accY,accZ)+PI)*RAD_TO_DEG;
                        roll = (atan2(accX,accZ)+PI)*RAD_TO_DEG;
                        /*
                        Notify(PSTR("\r\nPitch: "));
                        Serial.print(pitch);
                        Notify(PSTR("\tRoll: "));
                        Serial.print(roll);
                        */ 
                    }
                    switch (l2capinbuf[9]) {
                        case 0x20: // Status Information
                            // (a1) 20 BB BB LF 00 00 VV
                            if(l2capinbuf[12] & 0x02) // Check if a extension is connected
                                setReportMode(false,0x35); // Also read the extension
                            else
                                setReportMode(false,0x31); // If there is no extension connected we will read the button and accelerometer
                            break;
                        case 0x30: // Core buttons
                            // (a1) 30 BB BB
                            break;
                        case 0x31: // Core Buttons and Accelerometer
                            // (a1) 31 BB BB AA AA AA
                            break;
                        case 0x32: // Core Buttons with 8 Extension bytes
                            // (a1) 32 BB BB EE EE EE EE EE EE EE EE
                            /*
                            Notify(PSTR("\r\n"));
                            for (uint8_t i = 0; i < 8; i++) {
                                Serial.print(l2capinbuf[12+i]);
                                Notify(PSTR(" "));
                            }                     
                            */                             
                            break;
                        case 0x34: // Core Buttons with 19 Extension bytes
                            // (a1) 34 BB BB EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE 
                            /*
                            Notify(PSTR("\r\n"));
                            for (uint8_t i = 0; i < 19; i++) {
                                Serial.print(l2capinbuf[12+i]);
                                Notify(PSTR(" "));
                            }
                            */ 
                            break;
                        case 0x35: // Core Buttons and Accelerometer with 16 Extension Bytes
                            // (a1) 35 BB BB AA AA AA EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE
                            /*
                            Notify(PSTR("\r\n"));
                            for (uint8_t i = 0; i < 16; i++) {
                                Serial.print(l2capinbuf[15+i]);
                                Notify(PSTR(" "));
                            }
                            */ 
                            break;
#ifdef DEBUG
                        default:
                            Notify(PSTR("\r\nUnknown Report type: "));
                            Serial.print(l2capinbuf[9],HEX);
                            break;
#endif
                    }                    
                }
            }
        }
        L2CAP_task();
    }
}
void WII::L2CAP_task() {
    switch (l2cap_state) {
        case L2CAP_CONTROL_CONNECT_REQUEST:
            if (l2cap_connected_control_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nSend HID Control Config Request"));
#endif
                identifier++;
                pBtd->l2cap_config_request(hci_handle, identifier, control_scid);
                l2cap_state = L2CAP_CONTROL_CONFIG_REQUEST;
            }
            break;
            
        case L2CAP_CONTROL_CONFIG_REQUEST:
            if(l2cap_config_success_control_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nSend HID Interrupt Connection Request"));
#endif
                identifier++;
                pBtd->l2cap_connection_request(hci_handle,identifier,interrupt_dcid,HID_INTR_PSM);
                l2cap_state = L2CAP_INTERRUPT_CONNECT_REQUEST;
            }
            break;
            
        case L2CAP_INTERRUPT_CONNECT_REQUEST:
            if(l2cap_connected_interrupt_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nSend HID Interrupt Config Request"));
#endif
                identifier++;
                pBtd->l2cap_config_request(hci_handle, identifier, interrupt_scid);
                l2cap_state = L2CAP_INTERRUPT_CONFIG_REQUEST;
            }
            break;
            
            
        case L2CAP_INTERRUPT_CONFIG_REQUEST:
            if(l2cap_config_success_interrupt_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Channels Established"));
#endif
                statusRequest();
                l2cap_state = L2CAP_WII_STATUS_STATE;
            }
            break;
            
        case L2CAP_WII_STATUS_STATE:                        
            connected = true;
            pBtd->connectToWii = false;
            ButtonState = 0;
            OldButtonState = 0;
            ButtonClickState = 0;
            setLedOn(LED1);
            l2cap_state = L2CAP_DONE;
            break;
/*
        case L2CAP_WIIREMOTE_CAL_STATE:
            //Todo enable support for Motion Plus
            break;
*/ 
        case L2CAP_DONE:
            break;
            
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
void WII::Run() {
    switch (l2cap_state) {
        case L2CAP_WAIT:
            if(pBtd->connectToWii) {
#ifdef DEBUG
                Notify(PSTR("\r\nSend HID Control Connection Request"));
#endif
                hci_handle = pBtd->hci_handle; // Store the HCI Handle for the connection                
                l2cap_event_flag = 0; // Reset flags
                identifier = 0;
                pBtd->l2cap_connection_request(hci_handle,identifier,control_dcid,HID_CTRL_PSM);
                l2cap_state = L2CAP_CONTROL_CONNECT_REQUEST;                
            }
            break;
    }
}

/************************************************************/
/*                    HID Commands                          */
/************************************************************/
void WII::HID_Command(uint8_t* data, uint8_t nbytes) {
    pBtd->L2CAP_Command(hci_handle,data,nbytes,control_scid[0],control_scid[1]); // Both the Navigation and Dualshock controller sends data via the control channel
}
void WII::setAllOff() {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] = 0x00;
    HID_Command(HIDBuffer, 3);
}
void WII::setRumbleOff() {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] &= ~0x01; // Bit 0 control the rumble
    HID_Command(HIDBuffer, 3);    
}
void WII::setRumbleOn() {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] |= 0x01; // Bit 0 control the rumble
    HID_Command(HIDBuffer, 3);
}
void WII::setRumbleToggle() {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] ^= 0x01; // Bit 0 control the rumble
    HID_Command(HIDBuffer, 3);
}
void WII::setLedOff(LED a) {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] &= ~((uint8_t)a);
    HID_Command(HIDBuffer, 3);    
}
void WII::setLedOn(LED a) {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] |= (uint8_t)a;
    HID_Command(HIDBuffer, 3);
}
void WII::setLedToggle(LED a) {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] ^= (uint8_t)a;
    HID_Command(HIDBuffer, 3);
}
void WII::setReportMode(bool continuous, uint8_t mode) {
    uint8_t cmd_buf[4];
    cmd_buf[0] = 0xA2; // HID BT DATA_request (0x50) | Report Type (Output 0x02)
    cmd_buf[1] = 0x12;
    if(continuous)
        cmd_buf[2] = 0x04;
    else
        cmd_buf[2] = 0x00;
    cmd_buf[3] = mode;
    HID_Command(cmd_buf, 4);
}
void WII::statusRequest() {
    uint8_t cmd_buf[3];
    cmd_buf[0] = 0xA2; // HID BT DATA_request (0x50) | Report Type (Output 0x02)
    cmd_buf[1] = 0x15;
    cmd_buf[2] = 0x00;
    HID_Command(cmd_buf, 3);
}

/************************************************************/
/*                    WII Commands                          */
/************************************************************/

bool WII::getButtonPress(Button b) {    
    if(ButtonState & (uint16_t)b)
        return true;
    else
        return false;
}
bool WII::getButtonClick(Button b) {
    bool click = ((ButtonClickState & (uint16_t)b) != 0);
    ButtonClickState &= ~((uint16_t)b);  // clear "click" event
    return click;
} 