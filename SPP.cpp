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

#include "SPP.h"
#define DEBUG // Uncomment to print data for debugging
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report sent to the Arduino

/*
 * CRC (reversed crc) lookup table as calculated by the table generator in ETSI TS 101 369 V6.3.0.
 */
const uint8_t rfcomm_crc_table[256] PROGMEM = {    /* reversed, 8-bit, poly=0x07 */
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75, 0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69, 0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D, 0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51, 0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05, 0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19, 0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D, 0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21, 0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95, 0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89, 0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD, 0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1, 0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5, 0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9, 0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD, 0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1, 0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF
};

SPP::SPP(BTD *p, const char* name, const char* pin):
pBtd(p) // Pointer to BTD class instance - mandatory
{
    if (pBtd)
		pBtd->registerServiceClass(this); // Register it as a Bluetooth service
    
    pBtd->btdName = name;
    pBtd->btdPin = pin;
    
    /* Set device cid for the SDP and RFCOMM channelse */
    sdp_dcid[0] = 0x50; // 0x0050
    sdp_dcid[1] = 0x00;
    rfcomm_dcid[0] = 0x51; // 0x0051
    rfcomm_dcid[1] = 0x00;
    
    Reset();
}
void SPP::Reset() {
    connected = false;
    RFCOMMConnected = false;
    SDPConnected = false;
    l2cap_sdp_state = L2CAP_SDP_WAIT;
    l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;
    l2cap_event_flag = 0;    
}
void SPP::disconnect(){
    connected = false;
    // First the two L2CAP channels has to be disconencted and then the HCI connection
    if(RFCOMMConnected)
        pBtd->l2cap_disconnection_request(hci_handle,0x0A, rfcomm_scid, rfcomm_dcid);
    if(RFCOMMConnected && SDPConnected)
        delay(1); // Add delay between commands
    if(SDPConnected)
        pBtd->l2cap_disconnection_request(hci_handle,0x0B, sdp_scid, sdp_dcid);
    l2cap_sdp_state = L2CAP_DISCONNECT_RESPONSE;
}
void SPP::ACLData(uint8_t* l2capinbuf) {
    if(!pBtd->l2capConnectionClaimed && !connected && !RFCOMMConnected && !SDPConnected) {
        if (l2capinbuf[8] == L2CAP_CMD_CONNECTION_REQUEST) {
            if(((l2capinbuf[12] | (l2capinbuf[13] << 8)) == SDP_PSM) || ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == RFCOMM_PSM)) {                
                pBtd->l2capConnectionClaimed = true; // Claim that the incoming connection belongs to this service
                hci_handle = pBtd->hci_handle; // Store the HCI Handle for the connection
                l2cap_sdp_state = L2CAP_SDP_WAIT; // Reset state
                l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT; // Reset state
            }
        }
    }
    if (((l2capinbuf[0] | (l2capinbuf[1] << 8)) == (hci_handle | 0x2000))) { // acl_handle_ok
        if ((l2capinbuf[6] | (l2capinbuf[7] << 8)) == 0x0001) { //l2cap_control - Channel ID for ACL-U
            if (l2capinbuf[8] == L2CAP_CMD_COMMAND_REJECT) {
#ifdef DEBUG
                Notify(PSTR("\r\nL2CAP Command Rejected - Reason: "));
                PrintHex<uint8_t>(l2capinbuf[13]);
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[12]);
                Notify(PSTR(" Data: "));
                PrintHex<uint8_t>(l2capinbuf[17]);
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[16]);
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[15]);
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[14]);
#endif
            } else if (l2capinbuf[8] == L2CAP_CMD_CONNECTION_REQUEST) {
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
                if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == SDP_PSM) { // It doesn't matter if it receives another reqeust, since it waits for the channel to disconnect in the L2CAP_SDP_DONE state, and the l2cap_event_flag will be cleared if so
                    identifier = l2capinbuf[9];
                    sdp_scid[0] = l2capinbuf[14];
                    sdp_scid[1] = l2capinbuf[15];
                    l2cap_event_flag |= L2CAP_FLAG_CONNECTION_SDP_REQUEST;
                } else if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == RFCOMM_PSM) { // ----- || -----
                    identifier = l2capinbuf[9];
                    rfcomm_scid[0] = l2capinbuf[14];
                    rfcomm_scid[1] = l2capinbuf[15];
                    l2cap_event_flag |= L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST;
                }
            } else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_RESPONSE) {
                if ((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) { // Success
                    if (l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
                        //Serial.print("\r\nSDP Configuration Complete");
                        l2cap_event_flag |= L2CAP_FLAG_CONFIG_SDP_SUCCESS;
                    }
                    else if (l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
                        //Serial.print("\r\nRFCOMM Configuration Complete");
                        l2cap_event_flag |= L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS;
                    }
                }
            } else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_REQUEST) {
                if (l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
                    //Serial.print("\r\nSDP Configuration Request");
                    identifier = l2capinbuf[9];
                    l2cap_event_flag |= L2CAP_FLAG_CONFIG_SDP_REQUEST;
                }
                else if (l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
                    //Serial.print("\r\nRFCOMM Configuration Request");
                    identifier = l2capinbuf[9];
                    l2cap_event_flag |= L2CAP_FLAG_CONFIG_RFCOMM_REQUEST;
                }
            } else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_REQUEST) {
                if (l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
                    //Notify(PSTR("\r\nDisconnect Request: SDP Channel"));
                    identifier = l2capinbuf[9];
                    l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_SDP_REQUEST;
                } else if (l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
                    //Notify(PSTR("\r\nDisconnect Request: RFCOMM Channel"));
                    identifier = l2capinbuf[9];
                    l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST;
                }
            } else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_RESPONSE) {
                if (l2capinbuf[12] == sdp_scid[0] && l2capinbuf[13] == sdp_scid[1]) {
                    //Serial.print("\r\nDisconnect Response: SDP Channel");
                    identifier = l2capinbuf[9];
                    l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_RESPONSE;
                } else if (l2capinbuf[12] == rfcomm_scid[0] && l2capinbuf[13] == rfcomm_scid[1]) {
                    //Serial.print("\r\nDisconnect Response: RFCOMM Channel");
                    identifier = l2capinbuf[9];
                    l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_RESPONSE;
                }
            } else if (l2capinbuf[8] == L2CAP_CMD_INFORMATION_REQUEST) {
#ifdef DEBUG
                Notify(PSTR("\r\nInformation request"));
#endif
                identifier = l2capinbuf[9];
                pBtd->l2cap_information_response(hci_handle,identifier,l2capinbuf[12],l2capinbuf[13]);
            }
#ifdef EXTRADEBUG
            else {
                Notify(PSTR("\r\nL2CAP Unknown Signaling Command: "));
                PrintHex<uint8_t>(l2capinbuf[8]);                
            }
#endif
        } else if (l2capinbuf[6] == sdp_dcid[0] && l2capinbuf[7] == sdp_dcid[1]) { // SDP
            if(l2capinbuf[8] == SDP_SERVICE_SEARCH_ATTRIBUTE_REQUEST_PDU) {
                /*
                Serial.print("\r\nUUID: 0x");
                Serial.print(l2capinbuf[16],HEX);
                Serial.print(" ");
                Serial.print(l2capinbuf[17],HEX);
                */
                if ((l2capinbuf[16] << 8 | l2capinbuf[17]) == SERIALPORT_UUID) {
                    if(firstMessage) {
                        serialPortResponse1(l2capinbuf[9],l2capinbuf[10]);
                        firstMessage = false;
                    } else {
                        serialPortResponse2(l2capinbuf[9],l2capinbuf[10]); // Serialport continuation state
                        firstMessage = true;
                    }
                } else if ((l2capinbuf[16] << 8 | l2capinbuf[17]) == L2CAP_UUID) {
                    if(firstMessage) {
                        l2capResponse1(l2capinbuf[9],l2capinbuf[10]);
                        firstMessage = false;
                    } else {
                        l2capResponse2(l2capinbuf[9],l2capinbuf[10]); // L2CAP continuation state
                        firstMessage = true;
                    }
                } else
                    serviceNotSupported(l2capinbuf[9],l2capinbuf[10]); // The service is not supported
            }
        } else if (l2capinbuf[6] == rfcomm_dcid[0] && l2capinbuf[7] == rfcomm_dcid[1]) { // RFCOMM
            rfcommChannel = l2capinbuf[8] & 0xF8;
            rfcommDirection = l2capinbuf[8] & 0x04;
            rfcommCommandResponse = l2capinbuf[8] & 0x02;
            rfcommChannelType = l2capinbuf[9] & 0xEF;
            rfcommPfBit = l2capinbuf[9] & 0x10;
        
            if(rfcommChannel>>3 != 0x00)
                rfcommChannelConnection = rfcommChannel;
                
#ifdef EXTRADEBUG
            Notify(PSTR("\r\nRFCOMM Channel: "));
            Serial.print(rfcommChannel>>3,HEX);
            Notify(PSTR(" Direction: "));
            Serial.print(rfcommDirection>>2,HEX);
            Notify(PSTR(" CommandResponse: "));
            Serial.print(rfcommCommandResponse>>1,HEX);
            Notify(PSTR(" ChannelType: "));
            Serial.print(rfcommChannelType,HEX);
            Notify(PSTR(" PF_BIT: "));
            Serial.print(rfcommPfBit,HEX);
#endif
            if (rfcommChannelType == RFCOMM_DISC) {
#ifdef DEBUG
                Notify(PSTR("\r\nReceived Disconnect RFCOMM Command on channel: "));
                Serial.print(rfcommChannel>>3,HEX);
#endif
                connected = false;
                sendRfcomm(rfcommChannel,rfcommDirection,rfcommCommandResponse,RFCOMM_UA,rfcommPfBit,rfcommbuf,0x00); // UA Command
            }
            if(connected) {
                /* Read the incoming message */
                if(rfcommChannelType == RFCOMM_UIH && rfcommChannel == rfcommChannelConnection) {
                    uint8_t length = l2capinbuf[10] >> 1; // Get length
                    uint8_t offset = l2capinbuf[4]-length-4; // See if there is credit
                    if(rfcommAvailable + length <= sizeof(rfcommDataBuffer)) { // Don't add data to buffer if it would be full
                        for(uint8_t i = 0; i < length; i++)
                            rfcommDataBuffer[rfcommAvailable+i] = l2capinbuf[11+i+offset];
                        rfcommAvailable += length;
                    }
#ifdef EXTRADEBUG
                    Notify(PSTR("\r\nRFCOMM Data Available: "));
                    Serial.print(rfcommAvailable);
                    if (offset) {
                        Notify(PSTR(" - Credit: 0x"));
                        Serial.print(l2capinbuf[11],HEX);
                    }
#endif
#ifdef PRINTREPORT // Uncomment "#define PRINTREPORT" to print the report send to the Arduino via Bluetooth
                    for(uint8_t i = 0; i < length; i++)
                        Serial.write(l2capinbuf[i+11+offset]);
#endif
                }
            } else {
                if(rfcommChannelType == RFCOMM_SABM) { // SABM Command - this is sent twice: once for channel 0 and then for the channel to establish
#ifdef DEBUG
                    Notify(PSTR("\r\nReceived SABM Command"));
#endif                        
                    sendRfcomm(rfcommChannel,rfcommDirection,rfcommCommandResponse,RFCOMM_UA,rfcommPfBit,rfcommbuf,0x00); // UA Command
                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_PN_CMD) { // UIH Parameter Negotiation Command
#ifdef DEBUG
                    Notify(PSTR("\r\nReceived UIH Parameter Negotiation Command"));
#endif
                    rfcommbuf[0] = BT_RFCOMM_PN_RSP; // UIH Parameter Negotiation Response
                    rfcommbuf[1] = l2capinbuf[12]; // Length and shiftet like so: length << 1 | 1
                    rfcommbuf[2] = l2capinbuf[13]; // Channel: channel << 1 | 1
                    rfcommbuf[3] = 0xE0; // Pre difined for Bluetooth, see 5.5.3 of TS 07.10 Adaption for RFCOMM
                    rfcommbuf[4] = 0x00; // Priority
                    rfcommbuf[5] = 0x00; // Timer
                    rfcommbuf[6] = 0x40; // Max Fram Size LSB - we will just set this to 64
                    rfcommbuf[7] = 0x00; // Max Fram Size MSB
                    rfcommbuf[8] = 0x00; // MaxRatransm.
                    rfcommbuf[9] = 0x00; // Number of Frames
                    sendRfcomm(rfcommChannel,rfcommDirection,0,RFCOMM_UIH,rfcommPfBit,rfcommbuf,0x0A);
                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_MSC_CMD) { // UIH Modem Status Command
#ifdef DEBUG
                    Notify(PSTR("\r\nSend UIH Modem Status Response"));
#endif
                    rfcommbuf[0] = BT_RFCOMM_MSC_RSP; // UIH Modem Status Response
                    rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                    rfcommbuf[2] = l2capinbuf[13]; // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                    rfcommbuf[3] = l2capinbuf[14];
                    sendRfcomm(rfcommChannel,rfcommDirection,0,RFCOMM_UIH,rfcommPfBit,rfcommbuf,0x04);
                        
                    delay(1);
#ifdef DEBUG
                    Notify(PSTR("\r\nSend UIH Modem Status Command"));
#endif
                    rfcommbuf[0] = BT_RFCOMM_MSC_CMD; // UIH Modem Status Command
                    rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                    rfcommbuf[2] = l2capinbuf[13]; // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                    rfcommbuf[3] = 0x8D; // Can receive frames (YES), Ready to Communicate (YES), Ready to Receive (YES), Incomig Call (NO), Data is Value (YES)
                        
                    sendRfcomm(rfcommChannel,rfcommDirection,0,RFCOMM_UIH,rfcommPfBit,rfcommbuf,0x04);
                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_MSC_RSP) { // UIH Modem Status Response
                    if(!creditSent) {
#ifdef DEBUG
                        Notify(PSTR("\r\nSend UIH Command with credit"));
#endif
                        sendRfcommCredit(rfcommChannelConnection,rfcommDirection,0,RFCOMM_UIH,0x10,sizeof(rfcommDataBuffer)); // Send credit
                        creditSent = true;
                        timer = millis();
                        waitForLastCommand = true;
                    }
                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[10] == 0x01) { // UIH Command with credit
#ifdef DEBUG
                    Notify(PSTR("\r\nReceived UIH Command with credit"));
#endif                                              
                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_RPN_CMD) { // UIH Remote Port Negotiation Command
#ifdef DEBUG
                    Notify(PSTR("\r\nReceived UIH Remote Port Negotiation Command"));
#endif
                    rfcommbuf[0] = BT_RFCOMM_RPN_RSP; // Command
                    rfcommbuf[1] = l2capinbuf[12]; // Length and shiftet like so: length << 1 | 1
                    rfcommbuf[2] = l2capinbuf[13]; // Channel: channel << 1 | 1
                    rfcommbuf[3] = l2capinbuf[14]; // Pre difined for Bluetooth, see 5.5.3 of TS 07.10 Adaption for RFCOMM
                    rfcommbuf[4] = l2capinbuf[15]; // Priority
                    rfcommbuf[5] = l2capinbuf[16]; // Timer
                    rfcommbuf[6] = l2capinbuf[17]; // Max Fram Size LSB
                    rfcommbuf[7] = l2capinbuf[18]; // Max Fram Size MSB
                    rfcommbuf[8] = l2capinbuf[19]; // MaxRatransm.
                    rfcommbuf[9] = l2capinbuf[20]; // Number of Frames
                    sendRfcomm(rfcommChannel,rfcommDirection,0,RFCOMM_UIH,rfcommPfBit,rfcommbuf,0x0A); // UIH Remote Port Negotiation Response
#ifdef DEBUG
                    Notify(PSTR("\r\nRFCOMM Connection is now established\r\n"));
#endif                  
                    waitForLastCommand = false;
                    creditSent = false;
                    connected = true; // The RFCOMM channel is now established
                }
#ifdef DEBUG
                else if(rfcommChannelType != RFCOMM_DISC) {
                    Notify(PSTR("\r\nUnsupported RFCOMM Data - ChannelType: "));
                    PrintHex<uint8_t>(rfcommChannelType);
                    Notify(PSTR(" Command: "));
                    PrintHex<uint8_t>(l2capinbuf[11]);
                }
#endif                
            }
        }
#ifdef EXTRADEBUG
        else {
            Notify(PSTR("\r\nUnsupported L2CAP Data - Channel ID: "));
            PrintHex<uint8_t>(l2capinbuf[7]);
            Notify(PSTR(" "));
            PrintHex<uint8_t>(l2capinbuf[6]);
        }
#endif
        SDP_task();
        RFCOMM_task();
    }
}
void SPP::Run() {
    if(waitForLastCommand && (millis() - timer) > 100) { // We will only wait 100ms and see if the UIH Remote Port Negotiation Command is send, as some deviced don't send it
#ifdef DEBUG
        Notify(PSTR("\r\nRFCOMM Connection is now established - Automatic\r\n"));
#endif
        creditSent = false;
        waitForLastCommand = false;
        connected = true; // The RFCOMM channel is now established
    }    
}
void SPP::SDP_task() {
    switch (l2cap_sdp_state)
    {
        case L2CAP_SDP_WAIT:
            if (l2cap_connection_request_sdp_flag) {
                l2cap_event_flag &= ~L2CAP_FLAG_CONNECTION_SDP_REQUEST; // Clear flag
#ifdef DEBUG
                Notify(PSTR("\r\nSDP Incoming Connection Request"));
#endif
                pBtd->l2cap_connection_response(hci_handle,identifier, sdp_dcid, sdp_scid, PENDING);
                delay(1);
                pBtd->l2cap_connection_response(hci_handle,identifier, sdp_dcid, sdp_scid, SUCCESSFUL);                        
                identifier++;
                delay(1);
                pBtd->l2cap_config_request(hci_handle,identifier, sdp_scid);                   
                l2cap_sdp_state = L2CAP_SDP_REQUEST; 
            }            
            break;
        case L2CAP_SDP_REQUEST:
            if (l2cap_config_request_sdp_flag) {
                l2cap_event_flag &= ~L2CAP_FLAG_CONFIG_SDP_REQUEST; // Clear flag
#ifdef DEBUG
                Notify(PSTR("\r\nSDP Configuration Request"));
#endif                
                pBtd->l2cap_config_response(hci_handle,identifier, sdp_scid);                        
                l2cap_sdp_state = L2CAP_SDP_SUCCESS;
            }
            break;
        case L2CAP_SDP_SUCCESS:
            if (l2cap_config_success_sdp_flag) {
                l2cap_event_flag &= ~L2CAP_FLAG_CONFIG_SDP_SUCCESS; // Clear flag
#ifdef DEBUG
                Notify(PSTR("\r\nSDP Successfully Configured"));
#endif
                firstMessage = true; // Reset bool
                SDPConnected = true;
                l2cap_sdp_state = L2CAP_SDP_DONE;
            }
            break;
        case L2CAP_SDP_DONE:
            if(l2cap_disconnect_request_sdp_flag) {
                l2cap_event_flag &= ~L2CAP_FLAG_DISCONNECT_SDP_REQUEST; // Clear flag
                SDPConnected = false;
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected SDP Channel"));
#endif
                pBtd->l2cap_disconnection_response(hci_handle,identifier,sdp_dcid,sdp_scid);                
                l2cap_sdp_state = L2CAP_SDP_WAIT;
            }
            break;
        case L2CAP_DISCONNECT_RESPONSE: // This is for both disconnection response from the RFCOMM and SDP channel if they were connected
            if (l2cap_disconnect_response_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected L2CAP Connection"));
#endif
                RFCOMMConnected = false;
                SDPConnected = false;
                pBtd->hci_disconnect(hci_handle);
                l2cap_event_flag = 0; // Reset flags
                l2cap_sdp_state = L2CAP_SDP_WAIT;
                l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;
            }
            break;    
    }
}
void SPP::RFCOMM_task()
{         
    switch (l2cap_rfcomm_state)
    {
        case L2CAP_RFCOMM_WAIT:            
            if(l2cap_connection_request_rfcomm_flag) {
                l2cap_event_flag &= ~L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST; // Clear flag
#ifdef DEBUG
                Notify(PSTR("\r\nRFCOMM Incoming Connection Request"));
#endif
                pBtd->l2cap_connection_response(hci_handle,identifier, rfcomm_dcid, rfcomm_scid, PENDING);                        
                delay(1);
                pBtd->l2cap_connection_response(hci_handle,identifier, rfcomm_dcid, rfcomm_scid, SUCCESSFUL);                        
                identifier++;
                delay(1);
                pBtd->l2cap_config_request(hci_handle,identifier, rfcomm_scid);                        
                l2cap_rfcomm_state = L2CAP_RFCOMM_REQUEST;                
            } 
            break;                    
        case L2CAP_RFCOMM_REQUEST:
            if (l2cap_config_request_rfcomm_flag) {
                l2cap_event_flag &= ~L2CAP_FLAG_CONFIG_RFCOMM_REQUEST; // Clear flag
#ifdef DEBUG
                Notify(PSTR("\r\nRFCOMM Configuration Request"));
#endif
                pBtd->l2cap_config_response(hci_handle,identifier, rfcomm_scid);                        
                l2cap_rfcomm_state = L2CAP_RFCOMM_SUCCESS;
            }
            break;
        case L2CAP_RFCOMM_SUCCESS:
            if (l2cap_config_success_rfcomm_flag) {
                l2cap_event_flag &= ~L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS; // Clear flag
#ifdef DEBUG
                Notify(PSTR("\r\nRFCOMM Successfully Configured"));
#endif                
                rfcommAvailable = 0; // Reset number of bytes available
                bytesRead = 0; // Reset number of bytes received
                RFCOMMConnected = true;
                l2cap_rfcomm_state = L2CAP_RFCOMM_DONE;
            }
            break;            
        case L2CAP_RFCOMM_DONE:
            if(l2cap_disconnect_request_rfcomm_flag) {
                l2cap_event_flag &= ~L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST; // Clear flag
                RFCOMMConnected = false;
                connected = false;
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected RFCOMM Channel"));
#endif
                pBtd->l2cap_disconnection_response(hci_handle,identifier,rfcomm_dcid,rfcomm_scid);
                l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;
            }
            break;                    
    }
}
/************************************************************/
/*                    SDP Commands                          */
/************************************************************/
void SPP::SDP_Command(uint8_t* data, uint8_t nbytes) { // See page 223 in the Bluetooth specs
    pBtd->L2CAP_Command(hci_handle,data,nbytes,sdp_scid[0],sdp_scid[1]);
}
void SPP::serviceNotSupported(uint8_t transactionIDHigh, uint8_t transactionIDLow) { // See page 235 in the Bluetooth specs
    l2capoutbuf[0] = SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_PDU;
    l2capoutbuf[1] = transactionIDHigh;
    l2capoutbuf[2] = transactionIDLow;
    l2capoutbuf[3] = 0x00; // Parameter Length
    l2capoutbuf[4] = 0x05; // Parameter Length
    l2capoutbuf[5] = 0x00; // AttributeListsByteCount
    l2capoutbuf[6] = 0x02; // AttributeListsByteCount
    
    /* Attribute ID/Value Sequence: */
    l2capoutbuf[7] = 0x35;
    l2capoutbuf[8] = 0x00;
    l2capoutbuf[9] = 0x00;
    
    SDP_Command(l2capoutbuf,10);
}
void SPP::serialPortResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
    l2capoutbuf[0] = SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_PDU;
    l2capoutbuf[1] = transactionIDHigh;
    l2capoutbuf[2] = transactionIDLow;
    l2capoutbuf[3] = 0x00; // Parameter Length
    l2capoutbuf[4] = 0x2B; // Parameter Length
    l2capoutbuf[5] = 0x00; // AttributeListsByteCount
    l2capoutbuf[6] = 0x26; // AttributeListsByteCount

    /* Attribute ID/Value Sequence: */
    l2capoutbuf[7] = 0x36;
    l2capoutbuf[8] = 0x00;
    l2capoutbuf[9] = 0x3C;
    l2capoutbuf[10] = 0x36;
    l2capoutbuf[11] = 0x00;
    
    l2capoutbuf[12] = 0x39;
    l2capoutbuf[13] = 0x09;
    l2capoutbuf[14] = 0x00;
    l2capoutbuf[15] = 0x00;
    l2capoutbuf[16] = 0x0A;
    l2capoutbuf[17] = 0x00;
    l2capoutbuf[18] = 0x01;
    l2capoutbuf[19] = 0x00;
    l2capoutbuf[20] = 0x06;
    l2capoutbuf[21] = 0x09;
    l2capoutbuf[22] = 0x00;
    l2capoutbuf[23] = 0x01;
    l2capoutbuf[24] = 0x35;
    l2capoutbuf[25] = 0x03;
    l2capoutbuf[26] = 0x19;
    l2capoutbuf[27] = 0x11;
    
    l2capoutbuf[28] = 0x01;
    l2capoutbuf[29] = 0x09;
    l2capoutbuf[30] = 0x00;
    l2capoutbuf[31] = 0x04;
    l2capoutbuf[32] = 0x35;
    l2capoutbuf[33] = 0x0C;
    l2capoutbuf[34] = 0x35;
    l2capoutbuf[35] = 0x03;
    l2capoutbuf[36] = 0x19;
    l2capoutbuf[37] = 0x01;
    l2capoutbuf[38] = 0x00;
    l2capoutbuf[39] = 0x35;
    l2capoutbuf[40] = 0x05;
    l2capoutbuf[41] = 0x19;
    l2capoutbuf[42] = 0x00;
    l2capoutbuf[43] = 0x03;
    
    l2capoutbuf[44] = 0x08;
    l2capoutbuf[45] = 0x02; // Two extra bytes
    l2capoutbuf[46] = 0x00; // 25 (0x19) more bytes to come
    l2capoutbuf[47] = 0x19; 
    
    SDP_Command(l2capoutbuf,48);    
}
void SPP::serialPortResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
    l2capoutbuf[0] = SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_PDU;
    l2capoutbuf[1] = transactionIDHigh;
    l2capoutbuf[2] = transactionIDLow;
    l2capoutbuf[3] = 0x00; // Parameter Length
    l2capoutbuf[4] = 0x1C; // Parameter Length
    l2capoutbuf[5] = 0x00; // AttributeListsByteCount
    l2capoutbuf[6] = 0x19; // AttributeListsByteCount
    
    /* Attribute ID/Value Sequence: */
    l2capoutbuf[7] = 0x01;
    l2capoutbuf[8] = 0x09;
    l2capoutbuf[9] = 0x00;
    l2capoutbuf[10] = 0x06;
    l2capoutbuf[11] = 0x35;
    
    l2capoutbuf[12] = 0x09;
    l2capoutbuf[13] = 0x09;
    l2capoutbuf[14] = 0x65;
    l2capoutbuf[15] = 0x6E;
    l2capoutbuf[16] = 0x09;
    l2capoutbuf[17] = 0x00;
    l2capoutbuf[18] = 0x6A;
    l2capoutbuf[19] = 0x09;
    l2capoutbuf[20] = 0x01;
    l2capoutbuf[21] = 0x00;
    l2capoutbuf[22] = 0x09;
    l2capoutbuf[23] = 0x01;
    l2capoutbuf[24] = 0x00;
    l2capoutbuf[25] = 0x25;

    l2capoutbuf[26] = 0x05; // Name length 
    l2capoutbuf[27] = 'T';
    l2capoutbuf[28] = 'K';
    l2capoutbuf[29] = 'J';
    l2capoutbuf[30] = 'S';
    l2capoutbuf[31] = 'P';
    l2capoutbuf[32] = 0x00; // No more data

    SDP_Command(l2capoutbuf,33);
}
void SPP::l2capResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
    serialPortResponse1(transactionIDHigh,transactionIDLow); // These has to send all the supported functions, since it only supports virtual serialport it just sends the message again
}
void SPP::l2capResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
    serialPortResponse2(transactionIDHigh,transactionIDLow); // Same data as serialPortResponse2  
}
/************************************************************/
/*                    RFCOMM Commands                       */
/************************************************************/
void SPP::RFCOMM_Command(uint8_t* data, uint8_t nbytes) {
    if ((millis() - printTimer) < 10)// Check if is has been more than 10ms since last command
        delay((uint32_t)(10 - (millis() - printTimer))); // There have to be a delay between commands
    pBtd->L2CAP_Command(hci_handle,data,nbytes,rfcomm_scid[0],rfcomm_scid[1]);
    printTimer = millis();
}

void SPP::sendRfcomm(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t* data, uint8_t length) {
    l2capoutbuf[0] = channel | direction | CR | extendAddress; // RFCOMM Address
    l2capoutbuf[1] = channelType | pfBit; // RFCOMM Control
    l2capoutbuf[2] = length << 1 | 0x01; // Length and format (allways 0x01 bytes format)
    uint8_t i = 0;
    for(; i < length; i++)
        l2capoutbuf[i+3] = data[i];
    l2capoutbuf[i+3] = calcFcs(l2capoutbuf);
#ifdef EXTRADEBUG
    Notify(PSTR(" - RFCOMM Data: "));
    for(i = 0; i < length+4; i++) {
        Serial.print(l2capoutbuf[i],HEX);
        Notify(PSTR(" "));
    }
#endif    
    RFCOMM_Command(l2capoutbuf,length+4);
}                     

void SPP::sendRfcommCredit(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t credit) {
    l2capoutbuf[0] = channel | direction | CR | extendAddress; // RFCOMM Address
    l2capoutbuf[1] = channelType | pfBit; // RFCOMM Control
    l2capoutbuf[2] = 0x01; // Length = 0
    l2capoutbuf[3] = credit; // Credit
    l2capoutbuf[4] = calcFcs(l2capoutbuf);                                                    
#ifdef EXTRADEBUG
    Notify(PSTR(" - RFCOMM Credit Data: "));
    for(uint8_t i = 0; i < 5; i++) {
        Serial.print(l2capoutbuf[i],HEX);
        Notify(PSTR(" "));
    }
#endif
    RFCOMM_Command(l2capoutbuf,5);
}

/* CRC on 2 bytes */
uint8_t SPP::__crc(uint8_t* data) {
    return(pgm_read_byte(&rfcomm_crc_table[pgm_read_byte(&rfcomm_crc_table[0xff ^ data[0]]) ^ data[1]]));
}

/* Calculate FCS - we never actually check if the host sends correct FCS to the Arduino */
uint8_t SPP::calcFcs(uint8_t *data) {
    if((data[1] & 0xEF) == RFCOMM_UIH)
        return (0xff - __crc(data)); // FCS on 2 bytes
    else
        return (0xff - pgm_read_byte(&rfcomm_crc_table[__crc(data) ^ data[2]])); // FCS on 3 bytes
}

/* Serial commands */
void SPP::print(const String &str) {
    uint8_t length = str.length();
    if(length > (sizeof(l2capoutbuf)-4))
        length = sizeof(l2capoutbuf)-4;
    l2capoutbuf[0] = rfcommChannelConnection | 0 | 0 | extendAddress;; // RFCOMM Address
    l2capoutbuf[1] = RFCOMM_UIH; // RFCOMM Control
    l2capoutbuf[2] = length << 1 | 1; // Length
    uint8_t i = 0;    
    for(; i < length; i++)
        l2capoutbuf[i+3] = str[i];
    l2capoutbuf[i+3] = calcFcs(l2capoutbuf);
    
    RFCOMM_Command(l2capoutbuf,length+4);
}
void SPP::print(const char* data) {
    uint8_t length = strlen(data);
    if(length > (sizeof(l2capoutbuf)-4))
        length = sizeof(l2capoutbuf)-4;
    l2capoutbuf[0] = rfcommChannelConnection | 0 | 0 | extendAddress;; // RFCOMM Address
    l2capoutbuf[1] = RFCOMM_UIH; // RFCOMM Control                                                                
    l2capoutbuf[2] = length << 1 | 1; // Length
    uint8_t i = 0;
    for(; i < length; i++)
        l2capoutbuf[i+3] = data[i];                                                                
    l2capoutbuf[i+3] = calcFcs(l2capoutbuf);                            
    
    RFCOMM_Command(l2capoutbuf,length+4);
}
void SPP::print(uint8_t data) {
    print(&data,1);
}
void SPP::print(uint8_t* array, uint8_t length) {
    if(length > (sizeof(l2capoutbuf)-4))
        length = sizeof(l2capoutbuf)-4;
    l2capoutbuf[0] = rfcommChannelConnection | 0 | 0 | extendAddress;; // RFCOMM Address
    l2capoutbuf[1] = RFCOMM_UIH; // RFCOMM Control
    l2capoutbuf[2] = length << 1 | 1; // Length
    uint8_t i = 0;
    for(; i < length; i++)
        l2capoutbuf[i+3] = array[i];
    l2capoutbuf[i+3] = calcFcs(l2capoutbuf);
    
    RFCOMM_Command(l2capoutbuf,length+4);
}
void SPP::print(const __FlashStringHelper *ifsh) {
    const char PROGMEM *p = (const char PROGMEM *)ifsh;
    size_t size = 0;
    while (1) { // Calculate the size of the string
        uint8_t c = pgm_read_byte(p+size);
        if (c == 0)
            break;
        size++;
    }
    uint8_t buf[size];
    
    for(uint8_t i = 0; i < size; i++)
        buf[i] = pgm_read_byte(p++);
    
    print(buf,size);
}
void SPP::println(const String &str) {
    String output = str + "\r\n";
    print(output);
}
void SPP::println(const char* data) {
    char output[strlen(data)+3];
    strcpy(output,data);
    strcat(output,"\r\n");
    print(output);
}
void SPP::println(uint8_t data) {
    uint8_t buf[3] = {data, '\r', '\n'};
    print(buf,3);
}
void SPP::println(uint8_t* array, uint8_t length) {
    uint8_t buf[length+2];
    memcpy(buf,array,length);
    buf[length] = '\r';
    buf[length+1] = '\n';    
    print(buf,length+2);
}
void SPP::println(const __FlashStringHelper *ifsh) {
    const char PROGMEM *p = (const char PROGMEM *)ifsh;    
    size_t size = 0;
    while (1) { // Calculate the size of the string
        uint8_t c = pgm_read_byte(p+size);
        if (c == 0)
            break;
        size++;
    }
    uint8_t buf[size+2];
    
    for(uint8_t i = 0; i < size; i++)
        buf[i] = pgm_read_byte(p++);
    
    buf[size] = '\r';
    buf[size+1] = '\n';
    print(buf,size+2);
}
void SPP::println(void) {
    uint8_t buf[2] = {'\r','\n'};
    print(buf,2);
}

/* These must be used to print numbers */
void SPP::printNumber(int16_t n) {
    bool negative = false;
    if(n < 0) {
        negative = true;
        n = -n;
    }        
    uint16_t temp = n;
    uint8_t digits = 0;
    while (temp) {
        temp /= 10;
        digits++;
    }        
    if(digits == 0)
        print("0");
    else {
        uint8_t buf[digits+1]; // Add one extra in case it is negative
        for(uint8_t i = 1; i < digits+1; i++) {
            if(negative)
                buf[digits-i+1] = n%10 + '0'; // Get number and convert to ASCII Character
            else
                buf[digits-i] = n%10 + '0'; // Get number and convert to ASCII Character
            n /= 10;
        }
        if(negative) {
            buf[0] = '-';        
            print(buf,digits+1);
        } else
            print(buf,digits);
    }
}
void SPP::printNumberln(int16_t n) {
    bool negative = false;
    if(n < 0) {
        negative = true;
        n = -n;
    }
    uint16_t temp = n;
    uint8_t digits = 0;
    while (temp) {
        temp /= 10;
        digits++;
    }
    if(digits == 0)
        print("0\r\n");
    else {
        uint8_t buf[digits+3]; // Add one extra in case it is negative
        for(uint8_t i = 1; i < digits+1; i++) {
            if(negative)
                buf[digits-i+1] = n%10 + '0'; // Get number and convert to ASCII Character
            else
                buf[digits-i] = n%10 + '0'; // Get number and convert to ASCII Character
            n /= 10;
        }        
        if(negative) {
            buf[0] = '-';
            buf[digits+1] = '\r';
            buf[digits+2] = '\n';
            print(buf,digits+3);
        } else {
            buf[digits] = '\r';
            buf[digits+1] = '\n';
            print(buf,digits+2);
        }
    }
}
void SPP::printNumber(double n, uint8_t digits) {
    print(doubleToString(n,digits));
}
void SPP::printNumberln(double n, uint8_t digits) {
    char buf[12];
    strcpy(buf,doubleToString(n,digits));
    strcat(buf,"\r\n");
    print(buf);
}
const char* SPP::doubleToString(double input, uint8_t digits) {
    char output[10];
    char buffer[10];
    if(input < 0) {
        strcpy(output,"-");
        input = -input;
    }
    else
        strcpy(output,"");
    
    // Round correctly
    double rounding = 0.5;
    for (uint8_t i=0; i<digits; i++)
        rounding /= 10.0;
    input += rounding;
    
    unsigned long intpart = (unsigned long)input;
    itoa(intpart,buffer,10); // Convert to string
    strcat(output,buffer);
    strcat(output,".");
    double fractpart = (input-(double)intpart);
    fractpart *= pow(10,digits);
    for(uint8_t i=1;i<digits;i++) { // Put zeroes in front of number
        if(fractpart < pow(10,digits-i)) {
            strcat(output,"0");
        }
    }
    itoa((unsigned long)fractpart,buffer,10); // Convert to string
    strcat(output,buffer);
    return output;
}

uint8_t SPP::read() {
    if(rfcommAvailable == 0) // Don't read if there is nothing in the buffer
        return 0;
    uint8_t output = rfcommDataBuffer[0];
    for(uint8_t i = 1; i < rfcommAvailable; i++)
        rfcommDataBuffer[i-1] = rfcommDataBuffer[i]; // Shift the buffer one left
    rfcommAvailable--;
    bytesRead++;
    if(bytesRead > (sizeof(rfcommDataBuffer)-5)) { // We will send the command just before it runs out of credit
        bytesRead = 0;
        sendRfcommCredit(rfcommChannelConnection,rfcommDirection,0,RFCOMM_UIH,0x10,sizeof(rfcommDataBuffer)); // Send more credit
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nSent "));
        Serial.print(sizeof(rfcommDataBuffer));
        Notify(PSTR(" more credit"));
#endif
    }
    return output;
}