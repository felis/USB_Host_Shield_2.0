/* Copyright (C) 2014 Kristian Lauszus, TKJ Electronics. All rights reserved.

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

#include "SPPBase.h"

SPPBase::SPPBase(BTD *p) : pBtd(p) {};

void SPPBase::disconnect() {
        connected = false;
        // First the two L2CAP channels has to be disconnected and then the HCI connection
        if (RFCOMMConnected)
                pBtd->l2cap_disconnection_request(hci_handle, ++identifier, rfcomm_scid, rfcomm_dcid);
        if (RFCOMMConnected && SDPConnected)
                delay(1); // Add delay between commands
        if (SDPConnected)
                pBtd->l2cap_disconnection_request(hci_handle, ++identifier, sdp_scid, sdp_dcid);
        l2cap_sdp_state = L2CAP_DISCONNECT_RESPONSE;
}

void SPPBase::sendRfcomm(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t* data, uint8_t length) {
        l2capoutbuf[0] = channel | direction | CR | extendAddress; // RFCOMM Address
        l2capoutbuf[1] = channelType | pfBit; // RFCOMM Control
        l2capoutbuf[2] = length << 1 | 0x01; // Length and format (always 0x01 bytes format)
        uint8_t i = 0;
        for (; i < length; i++)
                l2capoutbuf[i + 3] = data[i];
        l2capoutbuf[i + 3] = calcFcs(l2capoutbuf);
#ifdef EXTRADEBUG
        Notify(PSTR(" - RFCOMM Data: "), 0x80);
        for (i = 0; i < length + 4; i++) {
                D_PrintHex<uint8_t > (l2capoutbuf[i], 0x80);
                Notify(PSTR(" "), 0x80);
        }
#endif
        RFCOMM_Command(l2capoutbuf, length + 4);
}

void SPPBase::sendRfcommCredit(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t credit) {
        l2capoutbuf[0] = channel | direction | CR | extendAddress; // RFCOMM Address
        l2capoutbuf[1] = channelType | pfBit; // RFCOMM Control
        l2capoutbuf[2] = 0x01; // Length = 0
        l2capoutbuf[3] = credit; // Credit
        l2capoutbuf[4] = calcFcs(l2capoutbuf);
#ifdef EXTRADEBUG
        Notify(PSTR(" - RFCOMM Credit Data: "), 0x80);
        for (uint8_t i = 0; i < 5; i++) {
                D_PrintHex<uint8_t > (l2capoutbuf[i], 0x80);
                Notify(PSTR(" "), 0x80);
        }
#endif
        RFCOMM_Command(l2capoutbuf, 5);
}


/* CRC on 2 bytes */
uint8_t SPPBase::crc(uint8_t *data) {
        return (pgm_read_byte(&rfcomm_crc_table[pgm_read_byte(&rfcomm_crc_table[0xFF ^ data[0]]) ^ data[1]]));
}

/* Calculate FCS */
uint8_t SPPBase::calcFcs(uint8_t *data) {
        uint8_t temp = crc(data);
        if ((data[1] & 0xEF) == RFCOMM_UIH)
                return (0xFF - temp); // FCS on 2 bytes
        else
                return (0xFF - pgm_read_byte(&rfcomm_crc_table[temp ^ data[2]])); // FCS on 3 bytes
}

/* Check FCS */
bool SPPBase::checkFcs(uint8_t *data, uint8_t fcs) {
        uint8_t temp = crc(data);
        if ((data[1] & 0xEF) != RFCOMM_UIH)
                temp = pgm_read_byte(&rfcomm_crc_table[temp ^ data[2]]); // FCS on 3 bytes
        return (pgm_read_byte(&rfcomm_crc_table[temp ^ fcs]) == 0xCF);
}

/* Serial commands */
#if defined(ARDUINO) && ARDUINO >=100

size_t SPPBase::write(uint8_t data) {
        return write(&data, 1);
}
#else

void SPPBase::write(uint8_t data) {
        write(&data, 1);
}
#endif

#if defined(ARDUINO) && ARDUINO >=100

size_t SPPBase::write(const uint8_t *data, size_t size) {
#else

void SPPBase::write(const uint8_t *data, size_t size) {
#endif
        for(uint8_t i = 0; i < size; i++) {
                if(sppIndex >= sizeof (sppOutputBuffer) / sizeof (sppOutputBuffer[0]))
                        send(); // Send the current data in the buffer
                sppOutputBuffer[sppIndex++] = data[i]; // All the bytes are put into a buffer and then send using the send() function
        }
#if defined(ARDUINO) && ARDUINO >=100
        return size;
#endif
}

void SPPBase::send() {
        if(!connected || !sppIndex)
                return;
        uint8_t length; // This is the length of the string we are sending
        uint8_t offset = 0; // This is used to keep track of where we are in the string

        l2capoutbuf[0] = rfcommChannelConnection | 0 | 0 | extendAddress; // RFCOMM Address
        l2capoutbuf[1] = RFCOMM_UIH; // RFCOMM Control

        while(sppIndex) { // We will run this while loop until this variable is 0
                if(sppIndex > (sizeof (l2capoutbuf) - 4)) // Check if the string is larger than the outgoing buffer
                        length = sizeof (l2capoutbuf) - 4;
                else
                        length = sppIndex;

                l2capoutbuf[2] = length << 1 | 1; // Length
                uint8_t i = 0;
                for(; i < length; i++)
                        l2capoutbuf[i + 3] = sppOutputBuffer[i + offset];
                l2capoutbuf[i + 3] = calcFcs(l2capoutbuf); // Calculate checksum

                RFCOMM_Command(l2capoutbuf, length + 4);

                sppIndex -= length;
                offset += length; // Increment the offset
        }
}

int SPPBase::available(void) {
        return rfcommAvailable;
};

void SPPBase::flush(void) {
        send();
};

void SPPBase::discard(void) {
        rfcommAvailable = 0;
}

int SPPBase::peek(void) {
        if(rfcommAvailable == 0) // Don't read if there is nothing in the buffer
                return -1;
        return rfcommDataBuffer[0];
}

int SPPBase::read(void) {
        if(rfcommAvailable == 0) // Don't read if there is nothing in the buffer
                return -1;
        uint8_t output = rfcommDataBuffer[0];
        for(uint8_t i = 1; i < rfcommAvailable; i++)
                rfcommDataBuffer[i - 1] = rfcommDataBuffer[i]; // Shift the buffer one left
        rfcommAvailable--;
        bytesRead++;
        if(bytesRead > (sizeof (rfcommDataBuffer) - 5)) { // We will send the command just before it runs out of credit
                bytesRead = 0;
                sendRfcommCredit(rfcommChannelConnection, rfcommDirection, 0, RFCOMM_UIH, 0x10, sizeof (rfcommDataBuffer)); // Send more credit
#ifdef EXTRADEBUG
                Notify(PSTR("\r\nSent "), 0x80);
                Notify((uint8_t)sizeof (rfcommDataBuffer), 0x80);
                Notify(PSTR(" more credit"), 0x80);
#endif
        }
        return output;
}