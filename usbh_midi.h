/*
 *******************************************************************************
 * USB-MIDI class driver for USB Host Shield 2.0 Library
 * Copyright (c) 2012-2022 Yuuichi Akagawa
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
#include "Usb.h"

#define USBH_MIDI_VERSION 10000
#define MIDI_MAX_ENDPOINTS 3 //endpoint 0, bulk_IN(MIDI), bulk_OUT(MIDI)
#define USB_SUBCLASS_MIDISTREAMING 3
#define MIDI_EVENT_PACKET_SIZE 64
#define MIDI_MAX_SYSEX_SIZE   256

namespace _ns_USBH_MIDI {
const uint8_t cin2len[] PROGMEM =  {0, 0, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1};
const uint8_t sys2cin[] PROGMEM =  {0, 2, 3, 2, 0, 0, 5, 0, 0xf, 0, 0xf, 0xf, 0xf, 0, 0xf, 0xf};
}

// Endpoint Descriptor extracter Class
class UsbMidiConfigXtracter {
public:
        //virtual void ConfigXtract(const USB_CONFIGURATION_DESCRIPTOR *conf) = 0;
        //virtual void InterfaceXtract(uint8_t conf, const USB_INTERFACE_DESCRIPTOR *iface) = 0;

        virtual bool EndpointXtract(uint8_t conf __attribute__((unused)), uint8_t iface __attribute__((unused)), uint8_t alt __attribute__((unused)), uint8_t proto __attribute__((unused)), const USB_ENDPOINT_DESCRIPTOR *ep __attribute__((unused))) {
                return true;
        };
};
// Configuration Descriptor Parser Class
class MidiDescParser : public USBReadParser {
        UsbMidiConfigXtracter *theXtractor;
        MultiValueBuffer theBuffer;
        MultiByteValueParser valParser;
        ByteSkipper theSkipper;
        uint8_t varBuffer[16 /*sizeof(USB_CONFIGURATION_DESCRIPTOR)*/];

        uint8_t stateParseDescr; // ParseDescriptor state

        uint8_t dscrLen; // Descriptor length
        uint8_t dscrType; // Descriptor type
        uint8_t nEPs; // number of valid endpoint
        bool isMidiSearch; //Configuration mode true: MIDI, false: Vendor specific

        bool isGoodInterface; // Apropriate interface flag
        uint8_t confValue; // Configuration value

        bool ParseDescriptor(uint8_t **pp, uint16_t *pcntdn);

public:
        MidiDescParser(UsbMidiConfigXtracter *xtractor, bool modeMidi);
        void Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset);
        inline uint8_t getConfValue() { return confValue; };
        inline uint8_t getNumEPs() { return nEPs; };
};

/** This class implements support for a MIDI device. */
class USBH_MIDI : public USBDeviceConfig, public UsbMidiConfigXtracter
{
protected:
        static const uint8_t    epDataInIndex = 1;         // DataIn endpoint index(MIDI)
        static const uint8_t    epDataOutIndex= 2;         // DataOUT endpoint index(MIDI)

        /* mandatory members */
        USB      *pUsb;
        uint8_t  bAddress;
        bool     bPollEnable;
        uint16_t pid, vid;    // ProductID, VendorID
        uint8_t  bTransferTypeMask;
        /* Endpoint data structure */
        EpInfo  epInfo[MIDI_MAX_ENDPOINTS];
        /* MIDI Event packet buffer */
        uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
        uint8_t readPtr;

        uint16_t countSysExDataSize(uint8_t *dataptr);
        void setupDeviceSpecific();
        inline uint8_t convertStatus2Cin(uint8_t status) {
                return ((status < 0xf0) ? ((status & 0xF0) >> 4) : pgm_read_byte_near(_ns_USBH_MIDI::sys2cin + (status & 0x0F)));
        };
        inline uint8_t getMsgSizeFromCin(uint8_t cin) {
                return pgm_read_byte_near(_ns_USBH_MIDI::cin2len + cin);
        };

        /* UsbConfigXtracter implementation */
        bool EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep);

#ifdef DEBUG_USB_HOST
        void PrintEndpointDescriptor( const USB_ENDPOINT_DESCRIPTOR* ep_ptr );
#endif
public:
        USBH_MIDI(USB *p);
        // Misc functions
        operator bool() { return (bPollEnable); }
        uint16_t idVendor() { return vid; }
        uint16_t idProduct() { return pid; }
        // Methods for recieving and sending data
        uint8_t RecvData(uint16_t *bytes_rcvd, uint8_t *dataptr);
        uint8_t RecvData(uint8_t *outBuf, bool isRaw=false);
        inline uint8_t RecvRawData(uint8_t *outBuf) { return RecvData(outBuf, true); };
        uint8_t SendData(uint8_t *dataptr, uint8_t nCable=0);
        inline uint8_t SendRawData(uint16_t bytes_send, uint8_t *dataptr) { return pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, bytes_send, dataptr); };
        uint8_t lookupMsgSize(uint8_t midiMsg, uint8_t cin=0);
        uint8_t SendSysEx(uint8_t *dataptr, uint16_t datasize, uint8_t nCable=0);
        uint8_t extractSysExData(uint8_t *p, uint8_t *buf);
        // backward compatibility functions
        inline uint8_t RcvData(uint16_t *bytes_rcvd, uint8_t *dataptr) { return RecvData(bytes_rcvd, dataptr); };
        inline uint8_t RcvData(uint8_t *outBuf) { return RecvData(outBuf); };

        // USBDeviceConfig implementation
        virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        virtual uint8_t Release();
        virtual uint8_t GetAddress() { return bAddress; };

        void attachOnInit(void (*funcOnInit)(void)) {
                pFuncOnInit = funcOnInit;
        };

        void attachOnRelease(void (*funcOnRelease)(void)) {
                pFuncOnRelease = funcOnRelease;
        };
private:
        void (*pFuncOnInit)(void) = nullptr; // Pointer to function called in onInit()
        void (*pFuncOnRelease)(void) = nullptr; // Pointer to function called in onRelease()
};

#endif //_USBH_MIDI_H_
