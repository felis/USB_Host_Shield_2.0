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

WII::WII(BTD *p, bool pair):
pBtd(p) // pointer to USB class instance - mandatory
{
    if (pBtd)
        pBtd->wiiServiceID = pBtd->registerServiceClass(this); // Register it as a Bluetooth service
    
    pBtd->pairWithWii = pair;
            
    HIDBuffer[0] = 0xA2;// HID BT DATA_request (0xA0) | Report Type (Output 0x02)
    
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
    motionPlusConnected = false;
    activateNunchuck = false;
    motionValuesReset = false;
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
    if (((l2capinbuf[0] | (l2capinbuf[1] << 8)) == (hci_handle | 0x2000)) || pBtd->incomingWii) { // acl_handle_ok or it's a new connection
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
                        if(motionPlusConnected) {
                            if(l2capinbuf[20] & 0x02) // Only update the wiimote buttons, since the extension bytes are from the Motion Plus
                                ButtonState = (uint32_t)((l2capinbuf[10] & 0x1F) | ((uint16_t)(l2capinbuf[11] & 0x9F) << 8) | ((uint32_t)(ButtonState & 0xFFFF0000)));
                            else if (nunchuckConnected) // Update if it's a report from the Nunchuck
                                ButtonState = (uint32_t)((l2capinbuf[10] & 0x1F) | ((uint16_t)(l2capinbuf[11] & 0x9F) << 8) | ((uint32_t)((~l2capinbuf[20]) & 0x0C) << 14));
                            //else if(classicControllerConnected) // Update if it's a report from the Classic Controller                                
                        }
                        else if(nunchuckConnected) // The Nunchuck is directly connected
                            ButtonState = (uint32_t)((l2capinbuf[10] & 0x1F) | ((uint16_t)(l2capinbuf[11] & 0x9F) << 8) | ((uint32_t)((~l2capinbuf[20]) & 0x03) << 16));
                        //else if(classicControllerConnected) // The Classic Controller is directly connected
                        else if(!unknownExtensionConnected)
                            ButtonState = (uint32_t)((l2capinbuf[10] & 0x1F) | ((uint16_t)(l2capinbuf[11] & 0x9F) << 8));
#ifdef PRINTREPORT
                        Notify(PSTR("ButtonState: "));
                        PrintHex<uint32_t>(ButtonState);
                        Notify(PSTR("\r\n"));
#endif
                        if(ButtonState != OldButtonState) {                            
                            ButtonClickState = ButtonState & ~OldButtonState; // Update click state variable
                            OldButtonState = ButtonState;
                        }
                    }
                    if(l2capinbuf[9] == 0x31 || l2capinbuf[9] == 0x35) { // Read the accelerometer
                        accX = ((l2capinbuf[12] << 2) | (l2capinbuf[10] & 0x60 >> 5))-500;
                        accY = ((l2capinbuf[13] << 2) | (l2capinbuf[11] & 0x20 >> 4))-500;
                        accZ = ((l2capinbuf[14] << 2) | (l2capinbuf[11] & 0x40 >> 5))-500;                        
                        wiiMotePitch = (atan2(accY,accZ)+PI)*RAD_TO_DEG;
                        wiiMoteRoll = (atan2(accX,accZ)+PI)*RAD_TO_DEG;
                    }
                    switch (l2capinbuf[9]) {
                        case 0x20: // Status Information - (a1) 20 BB BB LF 00 00 VV                            
                            if(l2capinbuf[12] & 0x02) { // Check if a extension is connected
#ifdef DEBUG
                                if(!unknownExtensionConnected)
                                    Notify(PSTR("\r\nExtension connected"));
#endif
                                unknownExtensionConnected = true;
                                setReportMode(false,0x35); // Also read the extension
                            }
                            else {
#ifdef DEBUG
                                Notify(PSTR("\r\nExtension disconnected"));
#endif
                                if(motionPlusConnected) {
#ifdef DEBUG
                                    Notify(PSTR(" - from Motion Plus"));
#endif
                                    l2cap_event_flag &= ~WII_FLAG_NUNCHUCK_CONNECTED;
                                    if(!activateNunchuck) // If it's already trying to initialize the Nunchuck don't set it to false
                                        nunchuckConnected = false;
                                    //else if(classicControllerConnected)
                                }
                                else if(nunchuckConnected) {
#ifdef DEBUG
                                    Notify(PSTR(" - Nunchuck"));
#endif
                                    nunchuckConnected = false; // It must be the Nunchuck controller then
                                    l2cap_event_flag &= ~WII_FLAG_NUNCHUCK_CONNECTED;
                                    setLedStatus();
                                    setReportMode(false,0x31); // If there is no extension connected we will read the button and accelerometer
                                } else {
                                    setReportMode(false,0x31); // If there is no extension connected we will read the button and accelerometer                                    
                                }
                            }
                            break;
                        case 0x21: // Read Memory Data
                            if((l2capinbuf[12] & 0x0F) == 0) { // No error
                                // See: http://wiibrew.org/wiki/Wiimote/Extension_Controllers
                                if(l2capinbuf[15] == 0x00 && l2capinbuf[16] == 0x00 && l2capinbuf[17] == 0xA4 && l2capinbuf[18] == 0x20 && l2capinbuf[19] == 0x00 && l2capinbuf[20] == 0x00) { 
#ifdef DEBUG
                                    Notify(PSTR("\r\nNunchuck connected"));
#endif
                                    l2cap_event_flag |= WII_FLAG_NUNCHUCK_CONNECTED;                                    
                                } else if(l2capinbuf[15] == 0x00 && l2capinbuf[16] == 0x00 && l2capinbuf[17] == 0xA6 && l2capinbuf[18] == 0x20 && l2capinbuf[19] == 0x00 && l2capinbuf[20] == 0x05) {
#ifdef DEBUG
                                    Notify(PSTR("\r\nMotion Plus connected"));
#endif
                                    l2cap_event_flag |= WII_FLAG_MOTION_PLUS_CONNECTED;
                                } else if(l2capinbuf[15] == 0x00 && l2capinbuf[16] == 0x00 && l2capinbuf[17] == 0xA4 && l2capinbuf[18] == 0x20 && l2capinbuf[19] == 0x04 && l2capinbuf[20] == 0x05) {
#ifdef DEBUG
                                    Notify(PSTR("\r\nMotion Plus activated in normal mode"));
#endif
                                    motionPlusConnected = true;
                                } else if(l2capinbuf[15] == 0x00 && l2capinbuf[16] == 0x00 && l2capinbuf[17] == 0xA4 && l2capinbuf[18] == 0x20 && l2capinbuf[19] == 0x05 && l2capinbuf[20] == 0x05) {
#ifdef DEBUG
                                    Notify(PSTR("\r\nMotion Plus activated in Nunchuck pass-through mode"));
#endif
                                    activateNunchuck = false;
                                    motionPlusConnected = true;
                                    nunchuckConnected = true;
                                } else if(l2capinbuf[15] == 0x00 && l2capinbuf[16] == 0x00 && l2capinbuf[17] == 0xA6 && l2capinbuf[18] == 0x20 && (l2capinbuf[19] == 0x00 || l2capinbuf[19] == 0x04 || l2capinbuf[19] == 0x05 || l2capinbuf[19] == 0x07) && l2capinbuf[20] == 0x05) {
#ifdef DEBUG
                                    Notify(PSTR("\r\nInactive Wii Motion Plus"));
                                    Notify(PSTR("\r\nPlease unplug the Motion Plus, disconnect the Wiimote and then replug the Motion Plus Extension"));
#endif
                                    stateCounter = 300; // Skip the rest in "L2CAP_CHECK_MOTION_PLUS_STATE"
                                }
#ifdef DEBUG
                                else {
                                    Notify(PSTR("\r\nUnknown Device: "));
                                    PrintHex<uint8_t>(l2capinbuf[13]);
                                    PrintHex<uint8_t>(l2capinbuf[14]);
                                    Notify(PSTR("\r\nData: "));
                                    for(uint8_t i = 0; i < ((l2capinbuf[12] >> 4)+1); i++) { // bit 4-7 is the length-1
                                        PrintHex<uint8_t>(l2capinbuf[15+i]);
                                        Notify(PSTR(" "));
                                    }
                                }
#endif
                            }
#ifdef EXTRADEBUG
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
                            }
#endif                                      
                            break;
                        case 0x30: // Core buttons - (a1) 30 BB BB
                            break;
                        case 0x31: // Core Buttons and Accelerometer - (a1) 31 BB BB AA AA AA
                            pitch = wiiMotePitch; // The pitch is just equal to the angle calculated from the wiimote as there is no Motion Plus connected
                            roll = wiiMoteRoll;
                            break;
                        case 0x32: // Core Buttons with 8 Extension bytes - (a1) 32 BB BB EE EE EE EE EE EE EE EE
                        case 0x34: // Core Buttons with 19 Extension bytes - (a1) 34 BB BB EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE
                            break;
                        case 0x35: // Core Buttons and Accelerometer with 16 Extension Bytes
                            // (a1) 35 BB BB AA AA AA EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE                             
                            if(motionPlusConnected) {
                                if(l2capinbuf[20] & 0x02) { // Check if it's a report from the Motion controller or the extension
                                    if(motionValuesReset) { // We will only use the values when the gyro value has been set
                                        gyroYawRaw = ((l2capinbuf[15] | ((l2capinbuf[18] & 0xFC) << 6))-gyroYawZero);
                                        gyroRollRaw = ((l2capinbuf[16] | ((l2capinbuf[19] & 0xFC) << 6))-gyroRollZero);
                                        gyroPitchRaw = ((l2capinbuf[17] | ((l2capinbuf[20] & 0xFC) << 6))-gyroPitchZero);

                                        yawGyroSpeed = (double)gyroYawRaw/((double)gyroYawZero/yawGyroScale);
                                        rollGyroSpeed = -(double)gyroRollRaw/((double)gyroRollZero/rollGyroScale); // We invert these values so they will fit the acc values
                                        pitchGyroSpeed = (double)gyroPitchRaw/((double)gyroPitchZero/pitchGyroScale);
                                        
                                        /* The onboard gyro has two ranges for slow and fast mode */
                                        if(!(l2capinbuf[18] & 0x02)) // Check if fast more is used
                                            yawGyroSpeed *= 4.545;
                                        if(!(l2capinbuf[18] & 0x01)) // Check if fast more is used
                                            pitchGyroSpeed *= 4.545;
                                        if(!(l2capinbuf[19] & 0x02)) // Check if fast more is used
                                            rollGyroSpeed *= 4.545;
                                                                                
                                        pitch = (0.93*(pitch+(pitchGyroSpeed*(double)(micros()-timer)/1000000)))+(0.07*wiiMotePitch); // Use a complimentary filter to calculate the angle
                                        roll = (0.93*(roll+(rollGyroSpeed*(double)(micros()-timer)/1000000)))+(0.07*wiiMoteRoll);
                                        
                                        gyroYaw += (yawGyroSpeed*((double)(micros()-timer)/1000000));
                                        gyroRoll += (rollGyroSpeed*((double)(micros()-timer)/1000000));
                                        gyroPitch += (pitchGyroSpeed*((double)(micros()-timer)/1000000));
                                        timer = micros();
                                        /*
                                        // Uncomment these lines to tune the gyro scale variabels
                                        Serial.print("\r\ngyroYaw: ");
                                        Serial.print(gyroYaw);
                                        Serial.print("\tgyroRoll: ");
                                        Serial.print(gyroRoll);
                                        Serial.print("\tgyroPitch: ");
                                        Serial.print(gyroPitch);
                                        */
                                        /*
                                        Serial.print("\twiiMoteRoll: ");                                        
                                        Serial.print(wiiMoteRoll);
                                        Serial.print("\twiiMotePitch: ");
                                        Serial.print(wiiMotePitch);
                                        */ 
                                    } else {
                                        if((micros() - timer) > 1000000) { // Loop for 1 sec before resetting the values
#ifdef DEBUG
                                            Notify(PSTR("\r\nThe gyro values has been reset"));
#endif
                                            gyroYawZero = (l2capinbuf[15] | ((l2capinbuf[18] & 0xFC) << 6));
                                            gyroRollZero = (l2capinbuf[16] | ((l2capinbuf[19] & 0xFC) << 6));
                                            gyroPitchZero = (l2capinbuf[17] | ((l2capinbuf[20] & 0xFC) << 6));
                                                                                        
                                            rollGyroScale = 500; // You might need to adjust these
                                            pitchGyroScale = 400;
                                            yawGyroScale = 415;
                                            
                                            motionValuesReset = true;
                                            timer = micros();
                                        }                                        
                                    }
                                } else {
                                    if(nunchuckConnected) {
                                        hatValues[0] = l2capinbuf[15];
                                        hatValues[1] = l2capinbuf[16];
                                        accX = ((l2capinbuf[17] << 2) | (l2capinbuf[20] & 0x10 >> 3))-416;
                                        accY = ((l2capinbuf[18] << 2) | (l2capinbuf[20] & 0x20 >> 4))-416;
                                        accZ = (((l2capinbuf[19] & 0xFE) << 2) | (l2capinbuf[20] & 0xC0 >> 5))-416;
                                        nunchuckPitch = (atan2(accY,accZ)+PI)*RAD_TO_DEG;
                                        nunchuckRoll = (atan2(accX,accZ)+PI)*RAD_TO_DEG;
                                    }
                                    //else if(classicControllerConnected) { }                                                                        
                                }
                                if(l2capinbuf[19] & 0x01) {
                                    if(!extensionConnected) {
                                        extensionConnected = true;
                                        unknownExtensionConnected = true;
#ifdef DEBUG
                                        Notify(PSTR("\r\nExtension connected to Motion Plus"));
#endif
                                    }
                                }
                                else {
                                    if(extensionConnected && !unknownExtensionConnected) {
                                        extensionConnected = false;
                                        unknownExtensionConnected = true;
#ifdef DEBUG
                                        Notify(PSTR("\r\nExtension disconnected from Motion Plus"));
#endif
                                        nunchuckConnected = false; // There is no extension connected to the Motion Plus if this report is sent
                                    }
                                }
                                    
                            } else if(nunchuckConnected) {
                                hatValues[0] = l2capinbuf[15];
                                hatValues[1] = l2capinbuf[16];
                                accX = ((l2capinbuf[17] << 2) | (l2capinbuf[20] & 0x0C >> 2))-416;
                                accY = ((l2capinbuf[18] << 2) | (l2capinbuf[20] & 0x30 >> 4))-416;
                                accZ = ((l2capinbuf[19] << 2) | (l2capinbuf[20] & 0xC0 >> 6))-416;
                                nunchuckPitch = (atan2(accY,accZ)+PI)*RAD_TO_DEG;
                                nunchuckRoll = (atan2(accX,accZ)+PI)*RAD_TO_DEG;
                                
                                pitch = wiiMotePitch; // The pitch is just equal to the angle calculated from the wiimote as there is no Motion Plus connected
                                roll = wiiMoteRoll;
                            }
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
        /* These states are used if the Wiimote is the host */
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
                
                l2cap_state = L2CAP_INTERRUPT_CONFIG_REQUEST;
            }
            break;

        /* These states are used if the Arduino is the host */
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
            if(l2cap_config_success_interrupt_flag) { // Now the HID channels is established
#ifdef DEBUG
                Notify(PSTR("\r\nHID Channels Established"));
#endif
                pBtd->connectToWii = false;
                pBtd->pairWithWii = false;
                wiimoteConnected = true;
                stateCounter = 0;
                l2cap_state = L2CAP_CHECK_MOTION_PLUS_STATE;
            }
            break;
            
        /* The next states are in run() */            
            
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
            } else if (l2cap_connection_request_control_flag) {
                hci_handle = pBtd->hci_handle;
                pBtd->incomingWii = false;
#ifdef DEBUG
                Notify(PSTR("\r\nHID Control Incoming Connection Request"));
#endif
                pBtd->l2cap_connection_response(hci_handle,identifier, control_dcid, control_scid, PENDING);
                delay(1);
                pBtd->l2cap_connection_response(hci_handle,identifier, control_dcid, control_scid, SUCCESSFUL);
                identifier++;
                delay(1);
                pBtd->l2cap_config_request(hci_handle,identifier, control_scid);
                l2cap_state = L2CAP_CONTROL_SUCCESS;
            }
            break;           
            
        case L2CAP_CHECK_MOTION_PLUS_STATE:
#ifdef DEBUG
            if(stateCounter == 0) // Only print onnce
                Notify(PSTR("\r\nChecking if a Motion Plus is connected"));
#endif
            stateCounter++;
            if(stateCounter%100 == 0)
                checkMotionPresent(); // Check if there is a motion plus connected
            if(motion_plus_connected_flag) {
                stateCounter = 0;
                l2cap_state = L2CAP_INIT_MOTION_PLUS_STATE;
                timer = micros();
                
                if(unknownExtensionConnected) {
#ifdef DEBUG
                    Notify(PSTR("\r\nA extension is also connected"));
#endif
                    activateNunchuck = true; // For we will just set this to true as this the only extension supported so far
                }
                
            }
            else if(stateCounter == 301) { // We will try three times to check for the motion plus
#ifdef DEBUG                
                Notify(PSTR("\r\nNo Motion Plus was detected"));
#endif
                stateCounter = 0;
                l2cap_state = L2CAP_CHECK_EXTENSION_STATE;
            }
            break;
            
        case L2CAP_CHECK_EXTENSION_STATE: // This is used to check if there is anything plugged in to the extension port
#ifdef DEBUG
            if(stateCounter == 0) // Only print onnce
                Notify(PSTR("\r\nChecking if there is any extension connected"));
#endif
            stateCounter++; // We use this counter as there has to be a short delay between the commands
            if(stateCounter == 1)
                statusRequest(); // See if a new device has connected
            if(stateCounter == 100) {
                if(unknownExtensionConnected) // Check if there is a extension is connected to the port
                    initExtension1();
                else
                    stateCounter = 399;
            } else if(stateCounter == 200)
                    initExtension2();
            else if(stateCounter == 300) {
                readExtensionType();
                unknownExtensionConnected = false;
            } else if(stateCounter == 400) {
                stateCounter = 0;
                l2cap_state = L2CAP_LED_STATE;
            }
            break;
            
        case L2CAP_INIT_MOTION_PLUS_STATE:
            stateCounter++;
            if(stateCounter == 1)
                initMotionPlus();
            else if(stateCounter == 100)
                activateMotionPlus();
            else if(stateCounter == 200)
                readExtensionType(); // Check if it has been activated
            else if(stateCounter == 300) {
                stateCounter = 0;                
                unknownExtensionConnected = false; // The motion plus will send a status report when it's activated, we will set this to false so it doesn't reinitialize the Motion Plus
                l2cap_state = L2CAP_LED_STATE;
            }
            break;
            
        case L2CAP_LED_STATE:
            if(nunchuck_connected_flag)
                nunchuckConnected = true;
            setLedStatus();
            l2cap_state = L2CAP_DONE;
            break;
             
        case L2CAP_DONE:
            if(unknownExtensionConnected) {
#ifdef DEBUG
                if(stateCounter == 0) // Only print once
                    Notify(PSTR("\r\nChecking extension port"));
#endif
                stateCounter++; // We will use this counter as there has to be a short delay between the commands
                if(stateCounter == 50)
                    statusRequest();
                else if(stateCounter == 100)
                    initExtension1();
                else if(stateCounter == 150)
                    if((extensionConnected && motionPlusConnected) || (unknownExtensionConnected && !motionPlusConnected))
                        initExtension2();
                    else
                        stateCounter = 299; // There is no extension connected
                else if(stateCounter == 200)
                    readExtensionType();
                else if(stateCounter == 250) {
                    if(nunchuck_connected_flag) {
#ifdef DEBUG
                        Notify(PSTR("\r\nNunchuck was reconnected"));
#endif
                        activateNunchuck = true;
                        nunchuckConnected = true;
                    }
                    if(!motionPlusConnected)
                        stateCounter = 449;
                }
                else if (stateCounter == 300) {
                    if(motionPlusConnected) {
#ifdef DEBUG
                        Notify(PSTR("\r\nReactivating the Motion Plus"));
#endif
                        initMotionPlus();
                    } else
                        stateCounter = 449;
                }
                else if(stateCounter == 350)
                    activateMotionPlus();
                else if(stateCounter == 400)
                    readExtensionType(); // Check if it has been activated
                else if(stateCounter == 450) {
                    setLedStatus();
                    stateCounter = 0;
                    unknownExtensionConnected = false;
                }
            } else
                stateCounter = 0;
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
void WII::setLedStatus() {
    HIDBuffer[1] = 0x11;
    HIDBuffer[2] = (HIDBuffer[2] & 0x01); // Keep the rumble bit
    if(wiimoteConnected)
        HIDBuffer[2] |= 0x10; // If it's connected LED1 will light up
    if(motionPlusConnected)
        HIDBuffer[2] |= 0x20; // If it's connected LED2 will light up
    if(nunchuckConnected)
        HIDBuffer[2] |= 0x40; // If it's connected LED3 will light up

    HID_Command(HIDBuffer, 3);
}
void WII::setReportMode(bool continuous, uint8_t mode) {
    uint8_t cmd_buf[4];
    cmd_buf[0] = 0xA2; // HID BT DATA_request (0xA0) | Report Type (Output 0x02)
    cmd_buf[1] = 0x12;
    if(continuous)
        cmd_buf[2] = 0x04 | (HIDBuffer[2] & 0x01); // Keep the rumble bit
    else
        cmd_buf[2] = 0x00 | (HIDBuffer[2] & 0x01); // Keep the rumble bit
    cmd_buf[3] = mode;
    HID_Command(cmd_buf, 4);
}
void WII::statusRequest() {
    uint8_t cmd_buf[3];
    cmd_buf[0] = 0xA2; // HID BT DATA_request (0xA0) | Report Type (Output 0x02)
    cmd_buf[1] = 0x15;
    cmd_buf[2] = (HIDBuffer[2] & 0x01); // Keep the rumble bit
    HID_Command(cmd_buf, 3);
}

/************************************************************/
/*                    Memmory Commands                      */
/************************************************************/
void WII::writeData(uint32_t offset, uint8_t size, uint8_t* data) {
    uint8_t cmd_buf[23];
    cmd_buf[0] = 0xA2; // HID BT DATA_request (0xA0) | Report Type (Output 0x02)
    cmd_buf[1] = 0x16; // Write data
    cmd_buf[2] = 0x04 | (HIDBuffer[2] & 0x01); // Write to memory, clear bit 2 to write to EEPROM
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
void WII::initExtension1() {
    uint8_t buf[1];
    buf[0] = 0x55;
    writeData(0xA400F0,1,buf);
}
void WII::initExtension2() {
    uint8_t buf[1];
    buf[0] = 0x00;
    writeData(0xA400FB,1,buf);
}
void WII::initMotionPlus() {
    uint8_t buf[1];
    buf[0] = 0x55;
    writeData(0xA600F0,1,buf);
}
void WII::activateMotionPlus() {
    uint8_t buf[1];
    if(activateNunchuck) {
#ifdef DEBUG
        Notify(PSTR("\r\nActivating Motion Plus in pass-through mode"));
#endif
        buf[0] = 0x05; // Activate nunchuck pass-through mode
    }
    //else if(classicControllerConnected && extensionConnected)
        //buf[0] = 0x07;
    else {
#ifdef DEBUG
        Notify(PSTR("\r\nActivating Motion Plus in normal mode"));
#endif
        buf[0] = 0x04; // Don't use any extension
    }
    writeData(0xA600FE,1,buf);
}
void WII::readData(uint32_t offset, uint16_t size, bool EEPROM) {
    uint8_t cmd_buf[8];
    cmd_buf[0] = 0xA2; // HID BT DATA_request (0xA0) | Report Type (Output 0x02)
    cmd_buf[1] = 0x17; // Read data
    if(EEPROM)
        cmd_buf[2] = 0x00 | (HIDBuffer[2] & 0x01); // Read from EEPROM
    else
        cmd_buf[2] = 0x04 | (HIDBuffer[2] & 0x01); // Read from memory    
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
void WII::checkMotionPresent() {
    readData(0xA600FA,6,false);
}

/************************************************************/
/*                    WII Commands                          */
/************************************************************/

bool WII::getButtonPress(Button b) { // Return true when a button is pressed
    return (ButtonState & (uint32_t)b);
}
bool WII::getButtonClick(Button b) { // Only return true when a button is clicked
    bool click = (ButtonClickState & (uint32_t)b);
    ButtonClickState &= ~((uint32_t)b);  // clear "click" event
    return click;
}
uint8_t WII::getAnalogHat(AnalogHat a) {
    if(!nunchuckConnected)
        return 127; // Return center position
    else {
        uint8_t output = hatValues[(uint8_t)a];
        if(output == 0xFF || output == 0x00) // The joystick will only read 255 or 0 when the cable is unplugged or initializing, so we will just return the center position
            return 127;
        else
            return output;
    }
}