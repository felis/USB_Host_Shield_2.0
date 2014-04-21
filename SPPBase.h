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

#ifndef _sppbase_h_
#define _sppbase_h_

#include "BTD.h"

/* Used for SDP */
#define SDP_SERVICE_SEARCH_ATTRIBUTE_REQUEST_PDU    0x06 // See the RFCOMM specs
#define SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_PDU   0x07 // See the RFCOMM specs
#define SDP_SERVICE_SEARCH_REQUEST_PDU 0x02
#define SDP_SERVICE_SEARCH_RESPONSE_PDU 0x03

#define SERIALPORT_UUID     0x1101 // See http://www.bluetooth.org/Technical/AssignedNumbers/service_discovery.htm
#define L2CAP_UUID          0x0100

/* Used for RFCOMM */
#define RFCOMM_SABM     0x2F
#define RFCOMM_UA       0x63
#define RFCOMM_UIH      0xEF
#define RFCOMM_DM       0x0F
#define RFCOMM_DISC     0x43

#define extendAddress   0x01 // Always 1

// Multiplexer message types
#define BT_RFCOMM_PN_CMD     0x83
#define BT_RFCOMM_PN_RSP     0x81
#define BT_RFCOMM_MSC_CMD    0xE3
#define BT_RFCOMM_MSC_RSP    0xE1
#define BT_RFCOMM_RPN_CMD    0x93
#define BT_RFCOMM_RPN_RSP    0x91
/*
#define BT_RFCOMM_TEST_CMD   0x23
#define BT_RFCOMM_TEST_RSP   0x21
#define BT_RFCOMM_FCON_CMD   0xA3
#define BT_RFCOMM_FCON_RSP   0xA1
#define BT_RFCOMM_FCOFF_CMD  0x63
#define BT_RFCOMM_FCOFF_RSP  0x61
#define BT_RFCOMM_RLS_CMD    0x53
#define BT_RFCOMM_RLS_RSP    0x51
#define BT_RFCOMM_NSC_RSP    0x11
*/

/*
 * CRC (reversed crc) lookup table as calculated by the table generator in ETSI TS 101 369 V6.3.0.
 */
const uint8_t rfcomm_crc_table[256] PROGMEM = {/* reversed, 8-bit, poly=0x07 */
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

/**
 * This BluetoothService class implements the Serial Port Protocol (SPP).
 * It inherits the Arduino Stream class. This allows it to use all the standard Arduino print and stream functions.
 */
class SPPBase : public BluetoothService, public Stream {
public:
        /**
         * Constructor for the SPPBase class.
         * @param  p   Pointer to BTD class instance.
         */
        SPPBase(BTD *p);

        /**
         * Used to provide Boolean tests for the class.
         * @return Return true if SPP communication is connected.
         */
        operator bool() {
                return connected;
        };

        /** Variable used to indicate if the connection is established. */
        bool connected;

        /** @name Serial port profile (SPP) Print functions */
        /**
         * Get number of bytes waiting to be read.
         * @return Return the number of bytes ready to be read.
         */
        virtual int available(void);
        /** Send out all bytes in the buffer. */
        virtual void flush(void);
        /**
         * Used to read the next value in the buffer without advancing to the next one.
         * @return Return the byte. Will return -1 if no bytes are available.
         */
        virtual int peek(void);
        /**
         * Used to read the buffer.
         * @return Return the byte. Will return -1 if no bytes are available.
         */
        virtual int read(void);

#if defined(ARDUINO) && ARDUINO >=100
        /**
         * Writes the byte to send to a buffer. The message is send when either send() or after Usb.Task() is called.
         * @param  data The byte to write.
         * @return      Return the number of bytes written.
         */
        virtual size_t write(uint8_t data);
        /**
         * Writes the bytes to send to a buffer. The message is send when either send() or after Usb.Task() is called.
         * @param  data The data array to send.
         * @param  size Size of the data.
         * @return      Return the number of bytes written.
         */
        virtual size_t write(const uint8_t* data, size_t size);
        /** Pull in write(const char *str) from Print */
        using Print::write;
#else
        /**
         * Writes the byte to send to a buffer. The message is send when either send() or after Usb.Task() is called.
         * @param  data The byte to write.
         */
        virtual void write(uint8_t data);
        /**
         * Writes the bytes to send to a buffer. The message is send when either send() or after Usb.Task() is called.
         * @param data The data array to send.
         * @param size Size of the data.
         */
        virtual void write(const uint8_t* data, size_t size);
#endif

        /** Discard all the bytes in the buffer. */
        void discard(void);
        /**
         * This will send all the bytes in the buffer.
         * This is called whenever Usb.Task() is called,
         * but can also be called via this function.
         */
        void send(void);
        /**@}*/
protected:
        /** @name BluetoothService implementation */
        /**
         * Used to pass acldata to the services.
         * @param ACLData Incoming acldata.
         */
        virtual void ACLData(uint8_t *ACLData) = 0;
        /** Used to establish the connection automatically. */
        virtual void Run() = 0;
        /** Use this to reset the service. */
        virtual void Reset() = 0;
        /** Used this to disconnect the virtual serial port. */
        virtual void disconnect();
        /**@}*/

        /* Pointer to Bluetooth dongle library instance */
        BTD *pBtd;

        /* Set true when a channel is created */
        bool SDPConnected;
        bool RFCOMMConnected;

        uint16_t hci_handle; // The HCI Handle for the connection

        /* Variables used by L2CAP state machines */
        uint8_t l2cap_sdp_state;
        uint8_t l2cap_rfcomm_state;

        uint8_t l2capoutbuf[BULK_MAXPKTSIZE]; // General purpose buffer for l2cap out data
        uint8_t rfcommbuf[10]; // Buffer for RFCOMM Commands

        /* L2CAP Channels */
        uint8_t sdp_scid[2]; // L2CAP source CID for SDP
        uint8_t sdp_dcid[2]; // 0x0050
        uint8_t rfcomm_scid[2]; // L2CAP source CID for RFCOMM
        uint8_t rfcomm_dcid[2]; // 0x0051
        uint8_t identifier; // Identifier for command

        /* RFCOMM Variables */
        uint8_t rfcommChannel;
        uint8_t rfcommChannelConnection; // This is the channel the SPP channel will be running at
        uint8_t rfcommDirection;
        uint8_t rfcommCommandResponse;
        uint8_t rfcommChannelType;
        uint8_t rfcommPfBit;

        bool creditSent;

        uint8_t rfcommDataBuffer[100]; // Create a 100 sized buffer for incoming data
        uint8_t sppOutputBuffer[100]; // Create a 100 sized buffer for outgoing SPP data
        uint8_t sppIndex;
        uint8_t rfcommAvailable;

        uint8_t bytesRead; // Counter to see when it's time to send more credit

        virtual void SDP_Command(uint8_t *data, uint8_t nbytes) = 0; // Used for SDP commands

        /* RFCOMM Commands */
        virtual void RFCOMM_Command(uint8_t *data, uint8_t nbytes) = 0; // Used for RFCOMM commands
        void sendRfcomm(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t *data, uint8_t length);
        void sendRfcommCredit(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t credit);
        uint8_t calcFcs(uint8_t *data);
        bool checkFcs(uint8_t *data, uint8_t fcs);
        uint8_t crc(uint8_t *data);
};

#endif
