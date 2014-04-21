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

#include "SPPServer.h"
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report sent to the Arduino

SPPServer::SPPServer(BTD *p, const char *name, const char *pin) :
SPPBase(p)
{
        if(pBtd)
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

void SPPServer::Reset() {
        connected = false;
        RFCOMMConnected = false;
        SDPConnected = false;
        waitForLastCommand = false;
        l2cap_sdp_state = L2CAP_SDP_WAIT;
        l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;
        l2cap_event_flag = 0;
        sppIndex = 0;
}

void SPPServer::ACLData(uint8_t *l2capinbuf) {
        if(!connected) {
                if(l2capinbuf[8] == L2CAP_CMD_CONNECTION_REQUEST) {
                        if((l2capinbuf[12] | (l2capinbuf[13] << 8)) == SDP_PSM && !pBtd->sdpConnectionClaimed) {
                                pBtd->sdpConnectionClaimed = true;
                                hci_handle = pBtd->hci_handle; // Store the HCI Handle for the connection
                                l2cap_sdp_state = L2CAP_SDP_WAIT; // Reset state
                        } else if((l2capinbuf[12] | (l2capinbuf[13] << 8)) == RFCOMM_PSM && !pBtd->rfcommConnectionClaimed) {
                                pBtd->rfcommConnectionClaimed = true;
                                hci_handle = pBtd->hci_handle; // Store the HCI Handle for the connection
                                l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT; // Reset state
                        }
                }
        }
        //if((l2capinbuf[0] | (uint16_t)l2capinbuf[1] << 8) == (hci_handle | 0x2000U)) { // acl_handle_ok
        if(UHS_ACL_HANDLE_OK(l2capinbuf, hci_handle)) { // acl_handle_ok
                if((l2capinbuf[6] | (l2capinbuf[7] << 8)) == 0x0001U) { // l2cap_control - Channel ID for ACL-U
                        if(l2capinbuf[8] == L2CAP_CMD_COMMAND_REJECT) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nL2CAP Command Rejected - Reason: "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[13], 0x80);
                                Notify(PSTR(" "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[12], 0x80);
                                Notify(PSTR(" Data: "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[17], 0x80);
                                Notify(PSTR(" "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[16], 0x80);
                                Notify(PSTR(" "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[15], 0x80);
                                Notify(PSTR(" "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[14], 0x80);
#endif
                        } else if(l2capinbuf[8] == L2CAP_CMD_CONNECTION_REQUEST) {
#ifdef EXTRADEBUG
                                Notify(PSTR("\r\nL2CAP Connection Request - PSM: "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[13], 0x80);
                                Notify(PSTR(" "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[12], 0x80);
                                Notify(PSTR(" SCID: "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[15], 0x80);
                                Notify(PSTR(" "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[14], 0x80);
                                Notify(PSTR(" Identifier: "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[9], 0x80);
#endif
                                if((l2capinbuf[12] | (l2capinbuf[13] << 8)) == SDP_PSM) { // It doesn't matter if it receives another reqeust, since it waits for the channel to disconnect in the L2CAP_SDP_DONE state, and the l2cap_event_flag will be cleared if so
                                        identifier = l2capinbuf[9];
                                        sdp_scid[0] = l2capinbuf[14];
                                        sdp_scid[1] = l2capinbuf[15];
                                        l2cap_set_flag(L2CAP_FLAG_CONNECTION_SDP_REQUEST);
                                } else if((l2capinbuf[12] | (l2capinbuf[13] << 8)) == RFCOMM_PSM) { // ----- || -----
                                        identifier = l2capinbuf[9];
                                        rfcomm_scid[0] = l2capinbuf[14];
                                        rfcomm_scid[1] = l2capinbuf[15];
                                        l2cap_set_flag(L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST);
                                }
                        } else if(l2capinbuf[8] == L2CAP_CMD_CONFIG_RESPONSE) {
                                if((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) { // Success
                                        if(l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
                                                //Notify(PSTR("\r\nSDP Configuration Complete"), 0x80);
                                                l2cap_set_flag(L2CAP_FLAG_CONFIG_SDP_SUCCESS);
                                        } else if(l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
                                                //Notify(PSTR("\r\nRFCOMM Configuration Complete"), 0x80);
                                                l2cap_set_flag(L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS);
                                        }
                                }
                        } else if(l2capinbuf[8] == L2CAP_CMD_CONFIG_REQUEST) {
                                if(l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
                                        //Notify(PSTR("\r\nSDP Configuration Request"), 0x80);
                                        pBtd->l2cap_config_response(hci_handle, l2capinbuf[9], sdp_scid);
                                } else if(l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
                                        //Notify(PSTR("\r\nRFCOMM Configuration Request"), 0x80);
                                        pBtd->l2cap_config_response(hci_handle, l2capinbuf[9], rfcomm_scid);
                                }
                        } else if(l2capinbuf[8] == L2CAP_CMD_DISCONNECT_REQUEST) {
                                if(l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
                                        //Notify(PSTR("\r\nDisconnect Request: SDP Channel"), 0x80);
                                        identifier = l2capinbuf[9];
                                        l2cap_set_flag(L2CAP_FLAG_DISCONNECT_SDP_REQUEST);
                                } else if(l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
                                        //Notify(PSTR("\r\nDisconnect Request: RFCOMM Channel"), 0x80);
                                        identifier = l2capinbuf[9];
                                        l2cap_set_flag(L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST);
                                }
                        } else if(l2capinbuf[8] == L2CAP_CMD_DISCONNECT_RESPONSE) {
                                if(l2capinbuf[12] == sdp_scid[0] && l2capinbuf[13] == sdp_scid[1]) {
                                        //Notify(PSTR("\r\nDisconnect Response: SDP Channel"), 0x80);
                                        identifier = l2capinbuf[9];
                                        l2cap_set_flag(L2CAP_FLAG_DISCONNECT_RESPONSE);
                                } else if(l2capinbuf[12] == rfcomm_scid[0] && l2capinbuf[13] == rfcomm_scid[1]) {
                                        //Notify(PSTR("\r\nDisconnect Response: RFCOMM Channel"), 0x80);
                                        identifier = l2capinbuf[9];
                                        l2cap_set_flag(L2CAP_FLAG_DISCONNECT_RESPONSE);
                                }
                        } else if(l2capinbuf[8] == L2CAP_CMD_INFORMATION_REQUEST) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nInformation request"), 0x80);
#endif
                                identifier = l2capinbuf[9];
                                pBtd->l2cap_information_response(hci_handle, identifier, l2capinbuf[12], l2capinbuf[13]);
                        }
#ifdef EXTRADEBUG
                        else {
                                Notify(PSTR("\r\nL2CAP Unknown Signaling Command: "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[8], 0x80);
                        }
#endif
                } else if(l2capinbuf[6] == sdp_dcid[0] && l2capinbuf[7] == sdp_dcid[1]) { // SDP
                        if(l2capinbuf[8] == SDP_SERVICE_SEARCH_ATTRIBUTE_REQUEST_PDU) {
                                if(((l2capinbuf[16] << 8 | l2capinbuf[17]) == SERIALPORT_UUID) || ((l2capinbuf[16] << 8 | l2capinbuf[17]) == 0x0000 && (l2capinbuf[18] << 8 | l2capinbuf[19]) == SERIALPORT_UUID)) { // Check if it's sending the full UUID, see: https://www.bluetooth.org/Technical/AssignedNumbers/service_discovery.htm, we will just check the first four bytes
                                        if(firstMessage) {
                                                serialPortResponse1(l2capinbuf[9], l2capinbuf[10]);
                                                firstMessage = false;
                                        } else {
                                                serialPortResponse2(l2capinbuf[9], l2capinbuf[10]); // Serialport continuation state
                                                firstMessage = true;
                                        }
                                } else if(((l2capinbuf[16] << 8 | l2capinbuf[17]) == L2CAP_UUID) || ((l2capinbuf[16] << 8 | l2capinbuf[17]) == 0x0000 && (l2capinbuf[18] << 8 | l2capinbuf[19]) == L2CAP_UUID)) {
                                        if(firstMessage) {
                                                l2capResponse1(l2capinbuf[9], l2capinbuf[10]);
                                                firstMessage = false;
                                        } else {
                                                l2capResponse2(l2capinbuf[9], l2capinbuf[10]); // L2CAP continuation state
                                                firstMessage = true;
                                        }
                                } else
                                        serviceNotSupported(l2capinbuf[9], l2capinbuf[10]); // The service is not supported
#ifdef EXTRADEBUG
                                Notify(PSTR("\r\nUUID: "), 0x80);
                                uint16_t uuid;
                                if((l2capinbuf[16] << 8 | l2capinbuf[17]) == 0x0000) // Check if it's sending the UUID as a 128-bit UUID
                                        uuid = (l2capinbuf[18] << 8 | l2capinbuf[19]);
                                else // Short UUID
                                        uuid = (l2capinbuf[16] << 8 | l2capinbuf[17]);
                                D_PrintHex<uint16_t > (uuid, 0x80);

                                Notify(PSTR("\r\nLength: "), 0x80);
                                uint16_t length = l2capinbuf[11] << 8 | l2capinbuf[12];
                                D_PrintHex<uint16_t > (length, 0x80);
                                Notify(PSTR("\r\nData: "), 0x80);
                                for(uint8_t i = 0; i < length; i++) {
                                        D_PrintHex<uint8_t > (l2capinbuf[13 + i], 0x80);
                                        Notify(PSTR(" "), 0x80);
                                }
#endif
                        }
#ifdef EXTRADEBUG
                        else {
                                Notify(PSTR("\r\nUnknown PDU: "), 0x80);
                                D_PrintHex<uint8_t > (l2capinbuf[8], 0x80);
                        }
#endif
                } else if(l2capinbuf[6] == rfcomm_dcid[0] && l2capinbuf[7] == rfcomm_dcid[1]) { // RFCOMM
                        rfcommChannel = l2capinbuf[8] & 0xF8;
                        rfcommDirection = l2capinbuf[8] & 0x04;
                        rfcommCommandResponse = l2capinbuf[8] & 0x02;
                        rfcommChannelType = l2capinbuf[9] & 0xEF;
                        rfcommPfBit = l2capinbuf[9] & 0x10;

                        if(rfcommChannel >> 3 != 0x00)
                                rfcommChannelConnection = rfcommChannel;

#ifdef EXTRADEBUG
                        Notify(PSTR("\r\nRFCOMM Channel: "), 0x80);
                        D_PrintHex<uint8_t > (rfcommChannel >> 3, 0x80);
                        Notify(PSTR(" Direction: "), 0x80);
                        D_PrintHex<uint8_t > (rfcommDirection >> 2, 0x80);
                        Notify(PSTR(" CommandResponse: "), 0x80);
                        D_PrintHex<uint8_t > (rfcommCommandResponse >> 1, 0x80);
                        Notify(PSTR(" ChannelType: "), 0x80);
                        D_PrintHex<uint8_t > (rfcommChannelType, 0x80);
                        Notify(PSTR(" PF_BIT: "), 0x80);
                        D_PrintHex<uint8_t > (rfcommPfBit, 0x80);
#endif
                        if(rfcommChannelType == RFCOMM_DISC) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nReceived Disconnect RFCOMM Command on channel: "), 0x80);
                                D_PrintHex<uint8_t > (rfcommChannel >> 3, 0x80);
#endif
                                connected = false;
                                sendRfcomm(rfcommChannel, rfcommDirection, rfcommCommandResponse, RFCOMM_UA, rfcommPfBit, rfcommbuf, 0x00); // UA Command
                        }
                        if(connected) {
                                /* Read the incoming message */
                                if(rfcommChannelType == RFCOMM_UIH && rfcommChannel == rfcommChannelConnection) {
                                        uint8_t length = l2capinbuf[10] >> 1; // Get length
                                        uint8_t offset = l2capinbuf[4] - length - 4; // Check if there is credit
                                        if(checkFcs(&l2capinbuf[8], l2capinbuf[11 + length + offset])) {
                                                uint8_t i = 0;
                                                for(; i < length; i++) {
                                                        if(rfcommAvailable + i >= sizeof (rfcommDataBuffer)) {
#ifdef DEBUG_USB_HOST
                                                                Notify(PSTR("\r\nWarning: Buffer is full!"), 0x80);
#endif
                                                                break;
                                                        }
                                                        rfcommDataBuffer[rfcommAvailable + i] = l2capinbuf[11 + i + offset];
                                                }
                                                rfcommAvailable += i;
#ifdef EXTRADEBUG
                                                Notify(PSTR("\r\nRFCOMM Data Available: "), 0x80);
                                                Notify(rfcommAvailable, 0x80);
                                                if(offset) {
                                                        Notify(PSTR(" - Credit: 0x"), 0x80);
                                                        D_PrintHex<uint8_t > (l2capinbuf[11], 0x80);
                                                }
#endif
                                        }
#ifdef DEBUG_USB_HOST
                                        else
                                                Notify(PSTR("\r\nError in FCS checksum!"), 0x80);
#endif
#ifdef PRINTREPORT // Uncomment "#define PRINTREPORT" to print the report send to the Arduino via Bluetooth
                                        for(uint8_t i = 0; i < length; i++)
                                                Notifyc(l2capinbuf[i + 11 + offset], 0x80);
#endif
                                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_RPN_CMD) { // UIH Remote Port Negotiation Command
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nReceived UIH Remote Port Negotiation Command"), 0x80);
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
                                        sendRfcomm(rfcommChannel, rfcommDirection, 0, RFCOMM_UIH, rfcommPfBit, rfcommbuf, 0x0A); // UIH Remote Port Negotiation Response
                                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_MSC_CMD) { // UIH Modem Status Command
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nSend UIH Modem Status Response"), 0x80);
#endif
                                        rfcommbuf[0] = BT_RFCOMM_MSC_RSP; // UIH Modem Status Response
                                        rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                                        rfcommbuf[2] = l2capinbuf[13]; // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                                        rfcommbuf[3] = l2capinbuf[14];
                                        sendRfcomm(rfcommChannel, rfcommDirection, 0, RFCOMM_UIH, rfcommPfBit, rfcommbuf, 0x04);
                                }
                        } else {
                                if(rfcommChannelType == RFCOMM_SABM) { // SABM Command - this is sent twice: once for channel 0 and then for the channel to establish
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nReceived SABM Command"), 0x80);
#endif
                                        sendRfcomm(rfcommChannel, rfcommDirection, rfcommCommandResponse, RFCOMM_UA, rfcommPfBit, rfcommbuf, 0x00); // UA Command
                                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_PN_CMD) { // UIH Parameter Negotiation Command
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nReceived UIH Parameter Negotiation Command"), 0x80);
#endif
                                        rfcommbuf[0] = BT_RFCOMM_PN_RSP; // UIH Parameter Negotiation Response
                                        rfcommbuf[1] = l2capinbuf[12]; // Length and shiftet like so: length << 1 | 1
                                        rfcommbuf[2] = l2capinbuf[13]; // Channel: channel << 1 | 1
                                        rfcommbuf[3] = 0xE0; // Pre difined for Bluetooth, see 5.5.3 of TS 07.10 Adaption for RFCOMM
                                        rfcommbuf[4] = 0x00; // Priority
                                        rfcommbuf[5] = 0x00; // Timer
                                        rfcommbuf[6] = BULK_MAXPKTSIZE - 14; // Max Fram Size LSB - set to the size of received data (50)
                                        rfcommbuf[7] = 0x00; // Max Fram Size MSB
                                        rfcommbuf[8] = 0x00; // MaxRatransm.
                                        rfcommbuf[9] = 0x00; // Number of Frames
                                        sendRfcomm(rfcommChannel, rfcommDirection, 0, RFCOMM_UIH, rfcommPfBit, rfcommbuf, 0x0A);
                                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_MSC_CMD) { // UIH Modem Status Command
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nSend UIH Modem Status Response"), 0x80);
#endif
                                        rfcommbuf[0] = BT_RFCOMM_MSC_RSP; // UIH Modem Status Response
                                        rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                                        rfcommbuf[2] = l2capinbuf[13]; // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                                        rfcommbuf[3] = l2capinbuf[14];
                                        sendRfcomm(rfcommChannel, rfcommDirection, 0, RFCOMM_UIH, rfcommPfBit, rfcommbuf, 0x04);

                                        delay(1);
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nSend UIH Modem Status Command"), 0x80);
#endif
                                        rfcommbuf[0] = BT_RFCOMM_MSC_CMD; // UIH Modem Status Command
                                        rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                                        rfcommbuf[2] = l2capinbuf[13]; // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                                        rfcommbuf[3] = 0x8D; // Can receive frames (YES), Ready to Communicate (YES), Ready to Receive (YES), Incomig Call (NO), Data is Value (YES)

                                        sendRfcomm(rfcommChannel, rfcommDirection, 0, RFCOMM_UIH, rfcommPfBit, rfcommbuf, 0x04);
                                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_MSC_RSP) { // UIH Modem Status Response
                                        if(!creditSent) {
#ifdef DEBUG_USB_HOST
                                                Notify(PSTR("\r\nSend UIH Command with credit"), 0x80);
#endif
                                                sendRfcommCredit(rfcommChannelConnection, rfcommDirection, 0, RFCOMM_UIH, 0x10, sizeof (rfcommDataBuffer)); // Send credit
                                                creditSent = true;
                                                timer = millis();
                                                waitForLastCommand = true;
                                        }
                                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[10] == 0x01) { // UIH Command with credit
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nReceived UIH Command with credit"), 0x80);
#endif
                                } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_RPN_CMD) { // UIH Remote Port Negotiation Command
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nReceived UIH Remote Port Negotiation Command"), 0x80);
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
                                        sendRfcomm(rfcommChannel, rfcommDirection, 0, RFCOMM_UIH, rfcommPfBit, rfcommbuf, 0x0A); // UIH Remote Port Negotiation Response
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nRFCOMM Connection is now established\r\n"), 0x80);
#endif
                                        waitForLastCommand = false;
                                        creditSent = false;
                                        connected = true; // The RFCOMM channel is now established
                                        sppIndex = 0;
                                }
#ifdef EXTRADEBUG
                                else if(rfcommChannelType != RFCOMM_DISC) {
                                        Notify(PSTR("\r\nUnsupported RFCOMM Data - ChannelType: "), 0x80);
                                        D_PrintHex<uint8_t > (rfcommChannelType, 0x80);
                                        Notify(PSTR(" Command: "), 0x80);
                                        D_PrintHex<uint8_t > (l2capinbuf[11], 0x80);
                                }
#endif
                        }
                }
#ifdef EXTRADEBUG
                else {
                        Notify(PSTR("\r\nUnsupported L2CAP Data - Channel ID: "), 0x80);
                        D_PrintHex<uint8_t > (l2capinbuf[7], 0x80);
                        Notify(PSTR(" "), 0x80);
                        D_PrintHex<uint8_t > (l2capinbuf[6], 0x80);
                }
#endif
                SDP_task();
                RFCOMM_task();
        }
}

void SPPServer::Run() {
        if(waitForLastCommand && (millis() - timer) > 100) { // We will only wait 100ms and see if the UIH Remote Port Negotiation Command is send, as some deviced don't send it
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nRFCOMM Connection is now established - Automatic\r\n"), 0x80);
#endif
                creditSent = false;
                waitForLastCommand = false;
                connected = true; // The RFCOMM channel is now established
                sppIndex = 0;
        }
        send(); // Send all bytes currently in the buffer
}

void SPPServer::SDP_task() {
        switch(l2cap_sdp_state) {
                case L2CAP_SDP_WAIT:
                        if(l2cap_check_flag(L2CAP_FLAG_CONNECTION_SDP_REQUEST)) {
                                l2cap_clear_flag(L2CAP_FLAG_CONNECTION_SDP_REQUEST); // Clear flag
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nSDP Incoming Connection Request"), 0x80);
#endif
                                pBtd->l2cap_connection_response(hci_handle, identifier, sdp_dcid, sdp_scid, PENDING);
                                delay(1);
                                pBtd->l2cap_connection_response(hci_handle, identifier, sdp_dcid, sdp_scid, SUCCESSFUL);
                                identifier++;
                                delay(1);
                                pBtd->l2cap_config_request(hci_handle, identifier, sdp_scid);
                                l2cap_sdp_state = L2CAP_SDP_SUCCESS;
                        } else if(l2cap_check_flag(L2CAP_FLAG_DISCONNECT_SDP_REQUEST)) {
                                l2cap_clear_flag(L2CAP_FLAG_DISCONNECT_SDP_REQUEST); // Clear flag
                                SDPConnected = false;
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nDisconnected SDP Channel"), 0x80);
#endif
                                pBtd->l2cap_disconnection_response(hci_handle, identifier, sdp_dcid, sdp_scid);
                        }
                        break;
                case L2CAP_SDP_SUCCESS:
                        if(l2cap_check_flag(L2CAP_FLAG_CONFIG_SDP_SUCCESS)) {
                                l2cap_clear_flag(L2CAP_FLAG_CONFIG_SDP_SUCCESS); // Clear flag
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nSDP Successfully Configured"), 0x80);
#endif
                                firstMessage = true; // Reset bool
                                SDPConnected = true;
                                l2cap_sdp_state = L2CAP_SDP_WAIT;
                        }
                        break;

                case L2CAP_DISCONNECT_RESPONSE: // This is for both disconnection response from the RFCOMM and SDP channel if they were connected
                        if(l2cap_check_flag(L2CAP_FLAG_DISCONNECT_RESPONSE)) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nDisconnected L2CAP Connection"), 0x80);
#endif
                                pBtd->hci_disconnect(hci_handle);
                                hci_handle = -1; // Reset handle
                                Reset();
                        }
                        break;
        }
}

void SPPServer::RFCOMM_task() {
        switch(l2cap_rfcomm_state) {
                case L2CAP_RFCOMM_WAIT:
                        if(l2cap_check_flag(L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST)) {
                                l2cap_clear_flag(L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST); // Clear flag
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nRFCOMM Incoming Connection Request"), 0x80);
#endif
                                pBtd->l2cap_connection_response(hci_handle, identifier, rfcomm_dcid, rfcomm_scid, PENDING);
                                delay(1);
                                pBtd->l2cap_connection_response(hci_handle, identifier, rfcomm_dcid, rfcomm_scid, SUCCESSFUL);
                                identifier++;
                                delay(1);
                                pBtd->l2cap_config_request(hci_handle, identifier, rfcomm_scid);
                                l2cap_rfcomm_state = L2CAP_RFCOMM_SUCCESS;
                        } else if(l2cap_check_flag(L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST)) {
                                l2cap_clear_flag(L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST); // Clear flag
                                RFCOMMConnected = false;
                                connected = false;
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nDisconnected RFCOMM Channel"), 0x80);
#endif
                                pBtd->l2cap_disconnection_response(hci_handle, identifier, rfcomm_dcid, rfcomm_scid);
                        }
                        break;
                case L2CAP_RFCOMM_SUCCESS:
                        if(l2cap_check_flag(L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS)) {
                                l2cap_clear_flag(L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS); // Clear flag
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nRFCOMM Successfully Configured"), 0x80);
#endif
                                rfcommAvailable = 0; // Reset number of bytes available
                                bytesRead = 0; // Reset number of bytes received
                                RFCOMMConnected = true;
                                l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;
                        }
                        break;
        }
}
/************************************************************/
/*                    SDP Commands                          */
/************************************************************/
void SPPServer::SDP_Command(uint8_t *data, uint8_t nbytes) { // See page 223 in the Bluetooth specs
        pBtd->L2CAP_Command(hci_handle, data, nbytes, sdp_scid[0], sdp_scid[1]);
}

void SPPServer::serviceNotSupported(uint8_t transactionIDHigh, uint8_t transactionIDLow) { // See page 235 in the Bluetooth specs
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

        SDP_Command(l2capoutbuf, 10);
}

void SPPServer::serialPortResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
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

        SDP_Command(l2capoutbuf, 48);
}

void SPPServer::serialPortResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
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

        SDP_Command(l2capoutbuf, 33);
}

void SPPServer::l2capResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
        serialPortResponse1(transactionIDHigh, transactionIDLow); // These has to send all the supported functions, since it only supports virtual serialport it just sends the message again
}

void SPPServer::l2capResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
        serialPortResponse2(transactionIDHigh, transactionIDLow); // Same data as serialPortResponse2
}
/************************************************************/
/*                    RFCOMM Commands                       */
/************************************************************/
void SPPServer::RFCOMM_Command(uint8_t* data, uint8_t nbytes) {
        pBtd->L2CAP_Command(hci_handle, data, nbytes, rfcomm_scid[0], rfcomm_scid[1]);
}
