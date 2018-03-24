/*
 *******************************************************************************
 * USB-MIDI class driver for USB Host Shield 2.0 Library
 * Copyright (c) 2012-2018 Yuuichi Akagawa
 *
 * Idea from LPK25 USB-MIDI to Serial MIDI converter
 *   by Collin Cunningham - makezine.com, narbotic.com
 *
 * for use with USB Host Shield 2.0 from Circuitsathome.com
 * https://github.com/felis/USB_Host_Shield_2.0
 *******************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *******************************************************************************
 */

#if !defined(_USBH_MIDI_H_)
#define _USBH_MIDI_H_
//#define DEBUG_USB_HOST
#include "Usb.h"

#define MIDI_MAX_ENDPOINTS 5 //endpoint 0, bulk_IN(MIDI), bulk_OUT(MIDI), bulk_IN(VSP), bulk_OUT(VSP)
#define USB_SUBCLASS_MIDISTREAMING 3
#define DESC_BUFF_SIZE        256
#define MIDI_EVENT_PACKET_SIZE 64
#define MIDI_MAX_SYSEX_SIZE   256
class USBH_MIDI;

class USBH_MIDI : public USBDeviceConfig
{
protected:
        static const uint8_t    epDataInIndex;          // DataIn endpoint index(MIDI)
        static const uint8_t    epDataOutIndex;         // DataOUT endpoint index(MIDI)
        static const uint8_t    epDataInIndexVSP;       // DataIn endpoint index(Vendor Specific Protocl)
        static const uint8_t    epDataOutIndexVSP;      // DataOUT endpoint index(Vendor Specific Protocl)

        /* mandatory members */
        USB      *pUsb;
        uint8_t  bAddress;
        uint8_t  bConfNum;    // configuration number
        uint8_t  bNumEP;      // total number of EP in the configuration
        bool     bPollEnable;
        bool     isMidiFound;
        uint16_t pid, vid;    // ProductID, VendorID
        uint8_t  bTransferTypeMask;
        /* Endpoint data structure */
        EpInfo  epInfo[MIDI_MAX_ENDPOINTS];
        /* MIDI Event packet buffer */
        uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
        uint8_t readPtr;

        uint8_t parseConfigDescr(uint8_t addr, uint8_t conf);
        uint16_t countSysExDataSize(uint8_t *dataptr);
        void setupDeviceSpecific();
#ifdef DEBUG_USB_HOST
        void PrintEndpointDescriptor( const USB_ENDPOINT_DESCRIPTOR* ep_ptr );
#endif
public:
        USBH_MIDI(USB *p);
        // Misc functions
        operator bool() { return (pUsb->getUsbTaskState()==USB_STATE_RUNNING); }
        uint16_t idVendor() { return vid; }
        uint16_t idProduct() { return pid; }
        // Methods for recieving and sending data
        uint8_t RecvData(uint16_t *bytes_rcvd, uint8_t *dataptr);
        uint8_t RecvData(uint8_t *outBuf, bool isRaw=false);
        uint8_t RecvRawData(uint8_t *outBuf);
        uint8_t SendData(uint8_t *dataptr, uint8_t nCable=0);
        uint8_t lookupMsgSize(uint8_t midiMsg, uint8_t cin=0);
        uint8_t SendSysEx(uint8_t *dataptr, uint16_t datasize, uint8_t nCable=0);
        uint8_t extractSysExData(uint8_t *p, uint8_t *buf);
        uint8_t SendRawData(uint16_t bytes_send, uint8_t *dataptr);
        // backward compatibility functions
        inline uint8_t RcvData(uint16_t *bytes_rcvd, uint8_t *dataptr) { return RecvData(bytes_rcvd, dataptr); };
        inline uint8_t RcvData(uint8_t *outBuf) { return RecvData(outBuf); };

        // USBDeviceConfig implementation
        virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        virtual uint8_t Release();
        virtual uint8_t GetAddress() { return bAddress; };
};
#endif //_USBH_MIDI_H_
