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

 Enhanced by Dmitry Pakhomenko to initiate connection with remote SPP-aware device
 04.04.2014, Magictale Electronics
 */


#include "SPPi.h"
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report sent to the Arduino

/*
 * "RFCOMM UUID" signature, a channel number comes immediately after it
 */
const uint8_t rfcomm_uuid_sign[6] PROGMEM = { 0x35, 0x05, 0x19, 0x00, 0x03, 0x08 };

SPPi::SPPi(BTD *p, const char* name, const char* pin, bool pair, uint8_t *addr) :
SPPBase(p)
{
    if (pBtd)
        pBtd->registerServiceClass(this); // Register it as a Bluetooth service

    pBtd->btdName = name;
    pBtd->btdPin = pin;

    if (addr) // Make sure address is set
        pBtd->pairWithOtherDevice = pair;

    for (uint8_t i = 0; i < 6; i++)
        pBtd->remote_bdaddr[i] = addr[i];

    /* Set device cid for the SDP and RFCOMM channels */
    sdp_scid[0] = 0x50; // 0x0050
    sdp_scid[1] = 0x00;

    rfcomm_scid[0] = 0x51; // 0x0051
    rfcomm_scid[1] = 0x00;

    Reset();
}

void SPPi::Reset() {
    connected = false;
    RFCOMMConnected = false;
    SDPConnected = false;
    l2cap_sdp_state = L2CAP_SDP_WAIT;
    l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;
    sppIndex = 0;

    rfcomm_uuid_sign_idx = 0;
    rfcomm_found = false;
}

void SPPi::ACLData(uint8_t *l2capinbuf) {

#ifdef EXTRADEBUG
    Notify(PSTR("\r\nIncoming Packet: "), 0x80);

    for (uint8_t i = 0; i < (l2capinbuf[2] + 4); i++) {
        D_PrintHex<uint8_t > (l2capinbuf[i], 0x80);
        Notify(PSTR(" "), 0x80);
    }
    Notify(PSTR("\r\n"), 0x80);
#endif

    //if((l2capinbuf[0] | (uint16_t)l2capinbuf[1] << 8) == (hci_handle | 0x2000U)) { // acl_handle_ok
    if(UHS_ACL_HANDLE_OK(l2capinbuf, hci_handle)) { // acl_handle_ok
        if((l2capinbuf[6] | (l2capinbuf[7] << 8)) == 0x0001U) { // l2cap_control - Channel ID for ACL-U
            if (l2capinbuf[8] == L2CAP_CMD_COMMAND_REJECT) {
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
            }else if (l2capinbuf[8] == L2CAP_CMD_CONNECTION_RESPONSE) {
                if (((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) && ((l2capinbuf[18] | (l2capinbuf[19] << 8)) == SUCCESSFUL)) { // Success
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nL2CAP Connection Response"), 0x80);
#endif
                    if (l2capinbuf[14] == sdp_scid[0] && l2capinbuf[15] == sdp_scid[1]) {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nSDP Connection Response"), 0x80);
#endif
                        sdp_dcid[0] = l2capinbuf[12];
                        sdp_dcid[1] = l2capinbuf[13];

                        identifier++;
                        l2cap_sdp_state = L2CAP_SDP_CONN_RESPONSE;
                    } else if (l2capinbuf[14] == rfcomm_scid[0] && l2capinbuf[15] == rfcomm_scid[1]) {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nRFComm Connection Response"), 0x80);
#endif
                        rfcomm_dcid[0] = l2capinbuf[12];
                        rfcomm_dcid[1] = l2capinbuf[13];

                        identifier++;
                        l2cap_rfcomm_state = L2CAP_RFCOMM_CONN_RESPONSE;
                    }
                }
            } else if(l2capinbuf[8] == L2CAP_CMD_CONFIG_RESPONSE) {
                if((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) { // Success
                    if(l2capinbuf[12] == sdp_scid[0] && l2capinbuf[13] == sdp_scid[1]) {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nSDP Configuration Response Received"), 0x80);
                        Notify(PSTR("\r\nSDP Successfully Configured"), 0x80);
#endif
                        l2cap_sdp_state = L2CAP_SDP_SERVICE_SEARCH_ATTR1;
                        identifier++;
                        SDP_Service_Search_Attr(0, identifier);
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nSDP Service Search Attribute Request 1 Sent"), 0x80);
#endif
                    } else if(l2capinbuf[12] == rfcomm_scid[0] && l2capinbuf[13] == rfcomm_scid[1]) {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nRFComm Configuration Response Received"), 0x80);
                        Notify(PSTR("\r\nRFComm Successfully Configured"), 0x80);
#endif
                        l2cap_sdp_state = L2CAP_RFCOMM_DONE;
                        identifier++;
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nRFComm SABM Sent"), 0x80);
#endif
                        rfcommAvailable = 0; // Reset number of bytes available
                        bytesRead = 0; // Reset number of bytes received
                        RFCOMMConnected = true;

                        //   channel direction, CR,channelType, pfBit,    data,      length
                        sendRfcomm(0, 0, (1 << 1), RFCOMM_SABM, (1 << 4), rfcommbuf, 0);
                    }
                }
            } else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_REQUEST) {
                if (l2capinbuf[12] == sdp_scid[0] && l2capinbuf[13] == sdp_scid[1]) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nSDP Configuration Request Received"), 0x80);
#endif
                    identifier = l2capinbuf[9];
                    pBtd->l2cap_config_response(hci_handle, identifier, sdp_dcid);
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nSDP Configuration Response Sent"), 0x80);
#endif
                    l2cap_sdp_state = L2CAP_SDP_CONFIG_REQUEST;
                    identifier++;
                    delay(1);
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nSDP Configuration Request Sent"), 0x80);
#endif
                    pBtd->l2cap_config_request(hci_handle, identifier, sdp_dcid);

                }else if (l2capinbuf[12] == rfcomm_scid[0] && l2capinbuf[13] == rfcomm_scid[1]) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nRFComm Configuration Request Received"), 0x80);
#endif
                    identifier = l2capinbuf[9];

                    pBtd->l2cap_config_response(hci_handle, identifier, rfcomm_dcid);
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nRFComm Configuration Response Sent"), 0x80);
#endif
                    l2cap_rfcomm_state = L2CAP_RFCOMM_CONFIG_REQUEST;
                    identifier++;
                    delay(1);
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nRFComm Configuration Request Sent"), 0x80);
#endif
                    pBtd->l2cap_config_request(hci_handle, identifier, rfcomm_dcid);
                }

            } else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_REQUEST) {
                if (l2capinbuf[12] == sdp_scid[0] && l2capinbuf[13] == sdp_scid[1]) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nDisconnect Request: SDP Channel"), 0x80);
#endif
                    identifier = l2capinbuf[9];

                    SDPConnected = false;
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nDisconnected SDP Channel"), 0x80);
#endif
                    pBtd->l2cap_disconnection_response(hci_handle, identifier, sdp_scid, sdp_dcid);
                    l2cap_sdp_state = L2CAP_SDP_WAIT;

                }else if (l2capinbuf[12] == rfcomm_scid[0] && l2capinbuf[13] == rfcomm_scid[1]) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nDisconnect Request: RFComm Channel"), 0x80);
#endif
                    identifier = l2capinbuf[9];

                    RFCOMMConnected = false;
                    connected = false;
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nDisconnected RFComm Channel"), 0x80);
#endif
                    pBtd->l2cap_disconnection_response(hci_handle, identifier, rfcomm_dcid, rfcomm_scid);
                    l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;

                }
            } else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_RESPONSE) {
                if (l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nDisconnect Response: SDP Channel"), 0x80);
#endif
                    SDPConnected = false;
                    l2cap_sdp_state = L2CAP_SDP_WAIT;

                }else if (l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nDisconnect Response: RFComm Channel"), 0x80);
                    Notify(PSTR("\r\nDisconnected L2CAP Connection"), 0x80);
#endif
                    RFCOMMConnected = false;

                    pBtd->hci_disconnect(hci_handle);
                    hci_handle = -1; // Reset handle

                    l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;
                }

            } else if (l2capinbuf[8] == L2CAP_CMD_INFORMATION_REQUEST) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nInformation request"), 0x80);
#endif
                identifier = l2capinbuf[9];
                pBtd->l2cap_information_response(hci_handle, identifier, l2capinbuf[12], l2capinbuf[13]);
            }
            else {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nL2CAP Unknown Signaling Command: "), 0x80);
                D_PrintHex<uint8_t > (l2capinbuf[8], 0x80);
#endif
            }
        } else if (l2capinbuf[6] == sdp_scid[0] && l2capinbuf[7] == sdp_scid[1]) { // SDP
#ifdef EXTRADEBUG
            Notify(PSTR("\r\nSDP data:"), 0x80);
#endif
            if (l2capinbuf[8] == SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_PDU) {

                if (l2cap_sdp_state == L2CAP_SDP_SERVICE_SEARCH_ATTR1){
#ifdef EXTRADEBUG
                    Notify(PSTR(" - SDP Service Search Attribute Response 1: "), 0x80);
#endif
                    remainingBytes = l2capinbuf[l2capinbuf[2] + 3];

                    parseAttrReply(l2capinbuf);

                    l2cap_sdp_state = L2CAP_SDP_SERVICE_SEARCH_ATTR2;
                    identifier++;
                    if (remainingBytes) {
                        SDP_Service_Search_Attr(0, identifier, remainingBytes);
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nSDP Service Search Attribute Request 2 Sent"), 0x80);
#endif
                    } else {
                        l2cap_sdp_state = L2CAP_SDP_DONE;

                        if (rfcomm_found) {
                            pBtd->l2cap_connection_request(hci_handle, identifier, rfcomm_scid, RFCOMM_PSM);
#ifdef DEBUG_USB_HOST
                            Notify(PSTR("\r\nRFComm Connection Request Sent"), 0x80);
#endif
                            l2cap_rfcomm_state = L2CAP_RFCOMM_REQUEST;
                        } else{
#ifdef DEBUG_USB_HOST
                            Notify(PSTR("\r\nRFComm Channel Number not found"), 0x80);
#endif
                        }
                    }

                } else if (l2cap_sdp_state == L2CAP_SDP_SERVICE_SEARCH_ATTR2){
#ifdef EXTRADEBUG
                    Notify(PSTR(" - SDP Service Search Attribute Response 2: "), 0x80);
#endif

                    parseAttrReply(l2capinbuf);

                    l2cap_sdp_state = L2CAP_SDP_DONE;
                    identifier++;

                    if (rfcomm_found) {
                        pBtd->l2cap_connection_request(hci_handle, identifier, rfcomm_scid, RFCOMM_PSM);
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nRFComm Connection Request Sent"), 0x80);
#endif
                        l2cap_rfcomm_state = L2CAP_RFCOMM_REQUEST;
                    }else{
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nRFComm Channel Number not found"), 0x80);
#endif
                    }
                }
            }
            else {
#ifdef EXTRADEBUG
                Notify(PSTR("\r\nUnknown PDU: "), 0x80);
                D_PrintHex<uint8_t > (l2capinbuf[8], 0x80);
#endif
            }

        } else if (l2capinbuf[6] == rfcomm_scid[0] && l2capinbuf[7] == rfcomm_scid[1]) { // RFCOMM

            rfcommChannel = l2capinbuf[8] & 0xF8;
            rfcommDirection = l2capinbuf[8] & 0x04;
            rfcommCommandResponse = l2capinbuf[8] & 0x02;
            rfcommChannelType = l2capinbuf[9] & 0xEF;
            rfcommPfBit = l2capinbuf[9] & 0x10;

#ifdef EXTRADEBUG
            Notify(PSTR("\r\nRFComm Channel: "), 0x80);
            D_PrintHex<uint8_t > (rfcommChannel >> 3, 0x80);
            Notify(PSTR(" Direction: "), 0x80);
            D_PrintHex<uint8_t > (rfcommDirection >> 2, 0x80);
            Notify(PSTR(" CommandResponse: "), 0x80);
            D_PrintHex<uint8_t > (rfcommCommandResponse >> 1, 0x80);
            Notify(PSTR(" ChannelType: "), 0x80);
            D_PrintHex<uint8_t > (rfcommChannelType, 0x80);
            Notify(PSTR(" PF_BIT: "), 0x80);
            D_PrintHex<uint8_t > (rfcommPfBit >> 4, 0x80);
#endif
            if (rfcommChannelType == RFCOMM_DISC) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nReceived Disconnect RFComm Command on channel: "), 0x80);
                D_PrintHex<uint8_t > (rfcommChannel, 0x80);
#endif
                connected = false;
//              sendRfcomm(rfcommChannel, rfcommDirection, rfcommCommandResponse, RFCOMM_UA, rfcommPfBit, rfcommbuf, 0x00); // UA Command
            }

            if (connected)
            {
                /* Read the incoming message */
                if (rfcommChannelType == RFCOMM_UIH && rfcommChannel == (rfcommChannelConnection << 3)) {
                    uint8_t length = l2capinbuf[10] >> 1; // Get length
                    uint8_t offset = l2capinbuf[4] - length - 4; // Check if there is credit
                    if (checkFcs(&l2capinbuf[8], l2capinbuf[11 + length + offset])) {
                        uint8_t i = 0;
                        for (; i < length; i++) {
                            if (rfcommAvailable + i >= sizeof (rfcommDataBuffer)) {
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
                        if (offset) {
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
                    for (uint8_t i = 0; i < length; i++)
                        Notifyc(l2capinbuf[i + 11 + offset], 0x80);
#endif
                } else if (rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_RPN_CMD) { // UIH Remote Port Negotiation Command
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nReceived UIH Remote Port Negotiation Command"), 0x80);
#endif
/*                    rfcommbuf[0] = BT_RFCOMM_RPN_RSP; // Command
                    rfcommbuf[1] = l2capinbuf[12]; // Length and shiftet like so: length << 1 | 1
                    rfcommbuf[2] = l2capinbuf[13]; // Channel: channel << 1 | 1
                    rfcommbuf[3] = l2capinbuf[14]; // Pre difined for Bluetooth, see 5.5.3 of TS 07.10 Adaption for RFCOMM
                    rfcommbuf[4] = l2capinbuf[15]; // Priority
                    rfcommbuf[5] = l2capinbuf[16]; // Timer
                    rfcommbuf[6] = l2capinbuf[17]; // Max Fram Size LSB
                    rfcommbuf[7] = l2capinbuf[18]; // Max Fram Size MSB
                    rfcommbuf[8] = l2capinbuf[19]; // MaxRatransm.
                    rfcommbuf[9] = l2capinbuf[20]; // Number of Frames
                    sendRfcomm(rfcommChannel, rfcommDirection, 0, RFCOMM_UIH, rfcommPfBit, rfcommbuf, 0x0A); // UIH Remote Port Negotiation Response*/
                } else if (rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_MSC_CMD) { // UIH Modem Status Command
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nSend UIH Modem Status Response"), 0x80);
#endif
                    /*rfcommbuf[0] = BT_RFCOMM_MSC_RSP; // UIH Modem Status Response
                    rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                    rfcommbuf[2] = l2capinbuf[13]; // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                    rfcommbuf[3] = l2capinbuf[14];
                    sendRfcomm(rfcommChannel, rfcommDirection, 0, RFCOMM_UIH, rfcommPfBit, rfcommbuf, 0x04);*/
                }
            }else{

                if (rfcommChannelType == RFCOMM_SABM) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nReceived SAMB RFComm Packet on channel: "), 0x80);
                    D_PrintHex<uint8_t > (rfcommChannel >> 3, 0x80);
#endif
                } else if (rfcommChannelType == RFCOMM_UA) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nReceived UA RFComm Packet on channel: "), 0x80);
                    D_PrintHex<uint8_t > (rfcommChannel >> 3, 0x80);
#endif
                    if ((rfcommChannel >> 3) == 0)
                    {
                        //Reply on 1-st SAMB command on channel 0
                        rfcommbuf[0] = BT_RFCOMM_PN_CMD; // UIH Parameter Negotiation Request
                        rfcommbuf[1] = ((8 << 1) | 1); // Length and shiftet like so: length << 1 | 1
                        rfcommbuf[2] = ((0x10<< 1)); // Channel: channel << 1 | 1 TODO: replace channel with variable
                        rfcommbuf[3] = 0xE0; // Pre defined for Bluetooth, see 5.5.3 of TS 07.10 Adaption for RFCOMM
                        rfcommbuf[4] = 0x00; // Priority
                        rfcommbuf[5] = 0x00; // Timer
                        rfcommbuf[6] = BULK_MAXPKTSIZE - 14; // Max Fram Size LSB - set to the size of received data (50)
                        rfcommbuf[7] = 0x00; // Max Fram Size MSB
                        rfcommbuf[8] = 0x00; // MaxRetransm.
                        rfcommbuf[9] = 0x00; // Number of Frames
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nSent UAH RFComm Cmd BT_RFCOMM_PN_CMD (Parameter Negotiation Request) on channel: "), 0x80);
                        D_PrintHex<uint8_t > (rfcommChannel >> 3, 0x80);
#endif
                        //   channel direction, CR,channelType,pfBit, data, length
                        sendRfcomm(0, 0, (1 << 1), RFCOMM_UIH, 0, rfcommbuf, 0x0A);
                   }else{
                        //Reply on 2-nd SAMB command on channel 0
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nSend UIH RFComm Cmd BT_RFCOMM_MSC_CMD (Modem Status Command)"), 0x80);
#endif
                        rfcommbuf[0] = BT_RFCOMM_MSC_CMD; // UIH Modem Status Command
                        rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                        rfcommbuf[2] = (1 << 0) | (1 << 1) | (0 << 2) | (rfcommChannelConnection << 3); //0x83 // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                        rfcommbuf[3] = 0x8D; // Can receive frames (YES), Ready to Communicate (YES), Ready to Receive (YES), Incomig Call (NO), Data is Value (YES)

                        //   channel direction, CR,channelType,pfBit, data, length
                        sendRfcomm(0, 0, (1 << 1), RFCOMM_UIH, 0, rfcommbuf, 0x04);
                   }
                } else if (rfcommChannelType == RFCOMM_UIH) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nReceived UIH RFComm Packet on channel: "), 0x80);
                    D_PrintHex<uint8_t > (rfcommChannel >> 3, 0x80);
#endif
                    if (l2capinbuf[11] == BT_RFCOMM_PN_RSP)
                    {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR(" - BT_RFCOMM_PN_RSP (Parameter Negotiation Response)"), 0x80);
                        Notify(PSTR("\r\nRFComm 2-nd SABM Sent"), 0x80);
#endif
                        //   channel direction, CR,channelType, pfBit,    data,      length
                        sendRfcomm((rfcommChannelConnection << 3), 0, (1 << 1), RFCOMM_SABM, (1 << 4), rfcommbuf, 0);
                    }else if (l2capinbuf[11] == BT_RFCOMM_MSC_CMD)
                    {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR(" - BT_RFCOMM_MSC_CMD (Modem Status Cmd)"), 0x80);
#endif
                        rfcommbuf[0] = BT_RFCOMM_MSC_RSP; // UIH Modem Status Command
                        rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                        rfcommbuf[2] = (1 << 0) | (1 << 1) | (0 << 2) | (rfcommChannelConnection << 3); //0x83 // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                        rfcommbuf[3] = 0x8D; // Can receive frames (YES), Ready to Communicate (YES), Ready to Receive (YES), Incomig Call (NO), Data is Value (YES)
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nSend UIH RFComm Cmd BT_RFCOMM_MSC_RSP (Modem Status Response)"), 0x80);
#endif
                        //   channel direction, CR,channelType,pfBit, data, length
                        sendRfcomm(0, 0, (1 << 1), RFCOMM_UIH, 0, rfcommbuf, 0x04);
                    }else if (l2capinbuf[11] == BT_RFCOMM_MSC_RSP)
                    {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR(" - BT_RFCOMM_MSC_RSP (Modem Status Response)"), 0x80);
                        Notify(PSTR("\r\nRFComm Cmd with Credit Sent"), 0x80);
#endif
                        sendRfcommCredit((0x10 << 3), 0, (1 << 1), RFCOMM_UIH, 0x10,
                            sizeof (rfcommDataBuffer)); // Send credit

                        connected = true; // The RFCOMM channel is now established
                        sppIndex = 0;
                    }else if (l2capinbuf[11] == BT_RFCOMM_RPN_CMD)
                    {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR(" - BT_RFCOMM_RPN_CMD (Remote Port Negotiation Cmd)"), 0x80);
#endif
                        //Just a stub. This command is optional according to the spec
                    }else if (l2capinbuf[11] == BT_RFCOMM_RPN_RSP)
                    {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR(" - BT_RFCOMM_RPN_RSP (Remote Port Negotiation Response)"), 0x80);
#endif
                        //Just a stub. This command is optional according to the spec
                    }else
                    {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR(" - Unknown Response: "), 0x80);
                        D_PrintHex<uint8_t > (l2capinbuf[11], 0x80);
#endif
                    }
                } else if (rfcommChannelType == RFCOMM_DM) {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nReceived DM RFComm Command on channel: "), 0x80);
                    D_PrintHex<uint8_t > (rfcommChannel >> 3, 0x80);
#endif
                } else {
#ifdef DEBUG_USB_HOST
                    Notify(PSTR("\r\nUnknown Response, rfCommChannelType: "), 0x80);
                    D_PrintHex<uint8_t > (rfcommChannelType, 0x80);
#endif
                }
            }
        } else {
#ifdef EXTRADEBUG
            Notify(PSTR("\r\nUnsupported L2CAP Data - Channel ID: "), 0x80);
            D_PrintHex<uint8_t > (l2capinbuf[7], 0x80);
            Notify(PSTR(" "), 0x80);
            D_PrintHex<uint8_t > (l2capinbuf[6], 0x80);
            Notify(PSTR("\r\n"), 0x80);

            for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++) {
                D_PrintHex<uint8_t > (l2capinbuf[i], 0x80);
                Notify(PSTR(" "), 0x80);
            }
#endif
        }
    }else{
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nBad ACL handle: "), 0x80);
        D_PrintHex<uint8_t > (l2capinbuf[0], 0x80);
        Notify(PSTR(" "), 0x80);
        D_PrintHex<uint8_t > (l2capinbuf[1], 0x80);
        Notify(PSTR(" "), 0x80);
#endif
    }
}

void SPPi::Run() {
    if (pBtd->pairWithOtherDevice){
        if (l2cap_sdp_state == L2CAP_SDP_WAIT) {
            if (pBtd->connectToOtherDevice && !pBtd->l2capConnectionClaimed && !connected) {
                pBtd->l2capConnectionClaimed = true;
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nSDP Connection Request Sent"), 0x80);
#endif
                hci_handle = pBtd->hci_handle; // Store the HCI Handle for the connection
                identifier = 0;

                pBtd->l2cap_connection_request(hci_handle, identifier, sdp_scid, SDP_PSM);
                l2cap_sdp_state = L2CAP_SDP_REQUEST;
            }
        }
    }
}

/************************************************************/
/*                    SDP Commands                          */
/************************************************************/
void SPPi::SDP_Command(uint8_t *data, uint8_t nbytes) { // See page 223 in the Bluetooth specs
    pBtd->L2CAP_Command(hci_handle, data, nbytes, sdp_dcid[0], sdp_dcid[1]);
}

void SPPi::SDP_Service_Search_Attr(uint8_t transactionIDHigh, uint8_t transactionIDLow, uint8_t remainingLen) {
    l2capoutbuf[0] = SDP_SERVICE_SEARCH_ATTRIBUTE_REQUEST_PDU;
    l2capoutbuf[1] = transactionIDHigh;
    l2capoutbuf[2] = transactionIDLow;

    l2capoutbuf[3] = 0x00; // Parameter Length
    if (remainingLen == 0)
        l2capoutbuf[4] = 0x0F; // Parameter Length
    else
        l2capoutbuf[4] = 0x11; // Parameter Length

    l2capoutbuf[5] = 0x35; // Data Element Sequence, data size in next 8 bits
    l2capoutbuf[6] = 0x03; // Data size

    l2capoutbuf[7] = 0x19; // 00011 001 UUID, 2 bytes
    l2capoutbuf[8] = 0x01;
    l2capoutbuf[9] = 0x00; // 0x0100 - L2CAP_UUID

    l2capoutbuf[10] = 0x00;
    l2capoutbuf[11] = 0x26;// 0x0026 Maximum attribute byte count

    l2capoutbuf[12] = 0x35;// Data Element Sequence, data size in next 8 bits
    l2capoutbuf[13] = 0x05;// Data size

    l2capoutbuf[14] = 0x0A;// 00001 010 unsigned int, 4 bytes - Attribute ID range

    l2capoutbuf[15] = 0x00;
    l2capoutbuf[16] = 0x00;// range from 0x0000...

    l2capoutbuf[17] = 0xFF;
    l2capoutbuf[18] = 0xFF;// ... to 0xFFFF

    if (remainingLen == 0) {
        l2capoutbuf[19] = 0x00;// No more data
        SDP_Command(l2capoutbuf, 20);
    } else{
        l2capoutbuf[19] = 0x02;
        l2capoutbuf[20] = 0x0; //will be 0 anyway
        l2capoutbuf[21] = remainingLen;
        SDP_Command(l2capoutbuf, 22);
    }
}

/************************************************************/
/*                    RFCOMM Commands                       */
/************************************************************/
void SPPi::RFCOMM_Command(uint8_t* data, uint8_t nbytes) {
        pBtd->L2CAP_Command(hci_handle, data, nbytes, rfcomm_dcid[0], rfcomm_dcid[1]);
}

void SPPi::parseAttrReply(uint8_t *l2capinbuf) {
    if ((l2capinbuf[2] + 4) < 15) return; // Sanity check

    if (rfcomm_found) {
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nChannel is already found"), 0x80);
#endif
        return;
    }

    if (rfcomm_uuid_sign_idx == sizeof(rfcomm_uuid_sign)) {
        // Signature has been already found but channel is in next packet
        rfcommChannelConnection = l2capinbuf[15];
        rfcomm_found = true;

#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nChannel found in second packet: "), 0x80);
        Notify((uint8_t)rfcommChannelConnection, 0x80);
#endif
        return;
    }

    uint8_t nextSignBt;
    for (uint8_t i = 15; i < (l2capinbuf[2] + 4); i++) { // Searching through packet payload only
        // Keep searching for signature
        if (l2capinbuf[i] == pgm_read_byte(&rfcomm_uuid_sign[rfcomm_uuid_sign_idx])) {
#ifdef EXTRADEBUG
            Notify(PSTR("\r\nFound match @"), 0x80);
            Notify((uint8_t)i, 0x80);
#endif
            rfcomm_uuid_sign_idx++;
            if (rfcomm_uuid_sign_idx == sizeof(rfcomm_uuid_sign)) {
                // Signature found, trying to get channel number
                if (l2capinbuf[i + 1] == 0x2) {
                    // Is the byte we are looking at the second last in the packet?
                    if ((l2capinbuf[2] + 1) ==  (i + 1)) {
                        // Oh well, channel number didn't fit in this packet, waiting for next
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nChannel will be in the next packet"), 0x80);
#endif
                        return;
                    }
                }
                rfcommChannelConnection = l2capinbuf[i + 1];
                rfcomm_found = true;
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nChannel found in first packet: "), 0x80);
                D_PrintHex<uint8_t > (rfcommChannelConnection, 0x80);
#endif
                return;
            }
        } else if ((l2capinbuf[i] == 0x2) && (i == l2capinbuf[2] + 1)) {
            // This is an indication of packet end with more data to come in next packet - do not reset rfcomm_uuid_sign_idx, we will continue when next packet arrives
#ifdef DEBUG_USB_HOST
            Notify(PSTR("\r\nEnd of packet reached, no full signature yet"), 0x80);
#endif
            return;
        } else {
            // Otherwise the signature is not found - start over again
            rfcomm_uuid_sign_idx = 0;
        }
    }
    return;
}
