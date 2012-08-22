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
//#define PRINTREPORT // Uncomment to print the report send by the Wii controllers

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
    wiimoteConnected = false;
    nunchuckConnected = false;
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
                    wiimoteConnected = false;
                    nunchuckConnected = false;
                    identifier = l2capinbuf[9];
                    pBtd->l2cap_disconnection_response(hci_handle,identifier,control_dcid,control_scid);                    
                    Reset();
                }
                else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1]) {
#ifdef DEBUG
                    Notify(PSTR("\r\nDisconnect Request: Interrupt Channel"));
#endif
                    wiimoteConnected = false;
                    nunchuckConnected = false;
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
            if(wiimoteConnected) {
                if(l2capinbuf[8] == 0xA1) { // HID_THDR_DATA_INPUT
                    if(l2capinbuf[9] >= 0x30 && l2capinbuf[9] <= 0x37) { // These reports include the buttons
                        if(nunchuckConnected)
                            ButtonState = (uint32_t)((l2capinbuf[10] & 0x1F) | ((uint16_t)(l2capinbuf[11] & 0x9F) << 8) | ((uint32_t)(l2capinbuf[20] & 0x03) << 16));
                        else
                            ButtonState = (uint32_t)((l2capinbuf[10] & 0x1F) | ((uint16_t)(l2capinbuf[11] & 0x9F) << 8));
#ifdef PRINTREPORT
                        Notify(PSTR("ButtonState: "));
                        PrintHex<uint32_t>(ButtonState);
                        Notify(PSTR("\r\n"));
#endif
                        if(ButtonState != OldButtonState)
                            ButtonClickState = ButtonState; // Update click state variable
                        OldButtonState = ButtonState;
                    }
                    if(l2capinbuf[9] == 0x31 || l2capinbuf[9] == 0x35) { // Read the accelerometer
                        accX = ((l2capinbuf[12] << 2) | (l2capinbuf[10] & 0x60 >> 5))-500;
                        accY = ((l2capinbuf[13] << 2) | (l2capinbuf[11] & 0x20 >> 4))-500;
                        accZ = ((l2capinbuf[14] << 2) | (l2capinbuf[11] & 0x40 >> 5))-500;
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
                    }
                    switch (l2capinbuf[9]) {
                        case 0x20: // Status Information
                            // (a1) 20 BB BB LF 00 00 VV
                            if(l2capinbuf[12] & 0x02) { // Check if a extension is connected
#ifdef DEBUG
                                Notify(PSTR("\r\nExtension connected"));
#endif
                                setReportMode(false,0x35); // Also read the extension                                
                                activateState = 1;
                            }
                            else {
#ifdef DEBUG
                                Notify(PSTR("\r\nExtension disconnected"));
#endif
                                nunchuckConnected = false;
                                setReportMode(false,0x31); // If there is no extension connected we will read the button and accelerometer
                            }
                            break;
                        case 0x21: // Read Memory Data
                            if((l2capinbuf[12] & 0x0F) == 0) { // No error
#ifdef EXTRADEBUG
                                Notify(PSTR("\r\nGot report: "));
                                PrintHex<uint8_t>(l2capinbuf[13]);
                                PrintHex<uint8_t>(l2capinbuf[14]);
                                Notify(PSTR("\r\nData: "));
                                for(uint8_t i = 0; i < ((l2capinbuf[12] >> 4)+1); i++) // bit 4-7 is the length-1
                                    PrintHex<uint8_t>(l2capinbuf[15+i]);
#endif
                                if(l2capinbuf[15] == 0x00 && l2capinbuf[16] == 0x00 && l2capinbuf[17] == 0xA4 && l2capinbuf[18] == 0x20 && l2capinbuf[19] == 0x00 && l2capinbuf[20] == 0x00) { // See // http://wiibrew.org/wiki/Wiimote/Extension_Controllers
#ifdef DEBUG
                                   Notify(PSTR("\r\nNunchuck connected"));
#endif
                                    nunchuckConnected = true;
                                    ButtonState |= (Z | C); // Since the Nunchuck button are cleared when pressed we set the buttonstates like so
                                    ButtonClickState |= (Z | C);                                    
                                }
                            }
#ifdef DEBUG
                            else {
                                Notify(PSTR("\r\nReport Error: "));
                                PrintHex<uint8_t>(l2capinbuf[13]);
                                PrintHex<uint8_t>(l2capinbuf[14]);
                            }
#endif                            
                            break;
                        case 0x22: // Acknowledge output report, return function result
#ifdef DEBUG
                            if(l2capinbuf[13] != 0x00) { // Check if there is an error
                                Notify(PSTR("\r\nCommand failed: "));
                                PrintHex<uint8_t>(l2capinbuf[12]);
                            } else if(l2capinbuf[12] == 0x16 && activateState > 20)
                                Notify(PSTR("\r\nExtension activated"));
#endif                                      
                            break;
                        case 0x30: // Core buttons
                            // (a1) 30 BB BB
                            break;
                        case 0x31: // Core Buttons and Accelerometer
                            // (a1) 31 BB BB AA AA AA
                            break;
                        case 0x32: // Core Buttons with 8 Extension bytes
                            // (a1) 32 BB BB EE EE EE EE EE EE EE EE                             
                            break;
                        case 0x34: // Core Buttons with 19 Extension bytes
                            // (a1) 34 BB BB EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE
                            break;
                        case 0x35: // Core Buttons and Accelerometer with 16 Extension Bytes
                            // (a1) 35 BB BB AA AA AA EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE
                            if(activateState == 10) {
                                activateExtension1();
                                activateState = 11;
                            } else if(activateState == 20) {
                                activateExtension2();
                                activateState = 21;                                
                            } else if(activateState == 30) {
                                readExtensionType();
                                activateState = 31;
                            } else if(activateState < 31)
                                activateState++; // We make this counter as there has to be a short delay between the commands

                            hatValues[0] = l2capinbuf[15];
                            hatValues[1] = l2capinbuf[16];

                            accX = ((l2capinbuf[17] << 2) | (l2capinbuf[20] & 0x0C >> 2))-416;
                            accY = ((l2capinbuf[18] << 2) | (l2capinbuf[20] & 0x30 >> 4))-416;
                            accZ = ((l2capinbuf[19] << 2) | (l2capinbuf[20] & 0xC0 >> 6))-416;
                            /*
                            Notify(PSTR("\r\naccX: "));
                            Serial.print(accX);
                            Notify(PSTR("\taccY: "));
                            Serial.print(accY);
                            Notify(PSTR("\taccZ: "));
                            Serial.print(accZ);
                            */     
                            nunchuckPitch = (atan2(accY,accZ)+PI)*RAD_TO_DEG;
                            nunchuckRoll = (atan2(accX,accZ)+PI)*RAD_TO_DEG;
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
            wiimoteConnected = true;
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
    rumbleBit = 0x00;
    HID_Command(HIDBuffer, 3);
}
void WII::setRumbleOff() {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] &= ~0x01; // Bit 0 control the rumble
    rumbleBit = 0x00;
    HID_Command(HIDBuffer, 3);    
}
void WII::setRumbleOn() {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] |= 0x01; // Bit 0 control the rumble
    rumbleBit = 0x01;
    HID_Command(HIDBuffer, 3);
}
void WII::setRumbleToggle() {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] ^= 0x01; // Bit 0 control the rumble
    rumbleBit ^= rumbleBit;
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
        cmd_buf[2] = 0x04 | rumbleBit;
    else
        cmd_buf[2] = 0x00 | rumbleBit;
    cmd_buf[3] = mode;
    HID_Command(cmd_buf, 4);
}
void WII::statusRequest() {
    uint8_t cmd_buf[3];
    cmd_buf[0] = 0xA2; // HID BT DATA_request (0x50) | Report Type (Output 0x02)
    cmd_buf[1] = 0x15;
    cmd_buf[2] = rumbleBit;
    HID_Command(cmd_buf, 3);
}

/************************************************************/
/*                    Memmory Commands                      */
/************************************************************/
void WII::writeData(uint32_t offset, uint8_t size, uint8_t* data) {
    uint8_t cmd_buf[23];
    cmd_buf[0] = 0xA2; // HID BT DATA_request (0x50) | Report Type (Output 0x02)
    cmd_buf[1] = 0x16; // Write data
    cmd_buf[2] = 0x04 | rumbleBit; // Write to memory, clear bit 2 to write to EEPROM
    cmd_buf[3] = (uint8_t)((offset & 0xFF0000) >> 16);
    cmd_buf[4] = (uint8_t)((offset & 0xFF00) >> 8);
    cmd_buf[5] = (uint8_t)(offset & 0xFF);
    cmd_buf[6] = size;
    uint8_t i = 0;
    for(; i < size; i++)
        cmd_buf[7+i] = data[i];
    for(; i < 16; i++) // Set the rest to zero
        cmd_buf[7+i] = 0x00;
    HID_Command(cmd_buf,23);
}
void WII::activateExtension1() {
    uint8_t buf[1];
    buf[0] = 0x55;
    writeData(0xA400F0,1,buf);
}
void WII::activateExtension2() {
    uint8_t buf[1];
    buf[0] = 0x00;
    writeData(0xA400FB,1,buf);
}
void WII::readData(uint32_t offset, uint16_t size, bool EEPROM) {
    uint8_t cmd_buf[8];
    cmd_buf[0] = 0xA2; // HID BT DATA_request (0x50) | Report Type (Output 0x02)
    cmd_buf[1] = 0x17; // Read data
    if(EEPROM)
        cmd_buf[2] = 0x00 | rumbleBit; // Read from EEPROM
    else
        cmd_buf[2] = 0x04 | rumbleBit; // Read from memory    
    cmd_buf[3] = (uint8_t)((offset & 0xFF0000) >> 16);
    cmd_buf[4] = (uint8_t)((offset & 0xFF00) >> 8);
    cmd_buf[5] = (uint8_t)(offset & 0xFF);
    cmd_buf[6] = (uint8_t)((size & 0xFF00) >> 8);
    cmd_buf[7] = (uint8_t)(size & 0xFF);
    
    HID_Command(cmd_buf,8);
}
void WII::readExtensionType() {
    readData(0xA400FA,6,false);
}
void WII::readCalData() {
    readData(0x0016,8,true);
}

/************************************************************/
/*                    WII Commands                          */
/************************************************************/

bool WII::getButtonPress(Button b) { // Return true when a button is pressed
    bool press = (ButtonState & (uint32_t)b);    
    if(b == Z || b == C)
        return !press; // The nunchuck buttons are cleared when pressed
    else
        return press;
}
bool WII::getButtonClick(Button b) { // Only return true when a button is clicked
    bool click = (ButtonClickState & (uint32_t)b);
    if(b == Z || b == C) {
        click = !click; // The nunchuck buttons are cleared when pressed
        ButtonClickState |= (uint32_t)b;  // clear "click" event
    } else
        ButtonClickState &= ~((uint32_t)b);  // clear "click" event
    return click;
}
uint8_t WII::getAnalogHat(AnalogHat a) {
    if(!nunchuckConnected)
        return 127; // Center position
    else {
        uint8_t output = hatValues[(uint8_t)a];
        if(output == 0xFF) // The joystick will only read 255 when the cable is unplugged, so we will just return the center position
            return 127;
        else
            return output;
    }
}