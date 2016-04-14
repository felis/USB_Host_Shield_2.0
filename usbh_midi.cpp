/*
 *******************************************************************************
 * USB-MIDI class driver for USB Host Shield 2.0 Library
 * Copyright (c) 2012-2016 Yuuichi Akagawa
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

#include "usbh_midi.h"
//////////////////////////
// MIDI MESAGES
// midi.org/techspecs/
//////////////////////////
// STATUS BYTES
// 0x8n == noteOff
// 0x9n == noteOn
// 0xAn == afterTouch
// 0xBn == controlChange
//    n == Channel(0x0-0xf)
//////////////////////////
//DATA BYTE 1
// note# == (0-127)
// or
// control# == (0-119)
//////////////////////////
// DATA BYTE 2
// velocity == (0-127)
// or
// controlVal == (0-127)
///////////////////////////////////////////////////////////////////////////////
// USB-MIDI Event Packets
// usb.org - Universal Serial Bus Device Class Definition for MIDI Devices 1.0
///////////////////////////////////////////////////////////////////////////////
//+-------------+-------------+-------------+-------------+
//|   Byte 0    |   Byte 1    |   Byte 2    |   Byte 3    |
//+------+------+-------------+-------------+-------------+
//|Cable | Code |             |             |             |
//|Number|Index |   MIDI_0    |   MIDI_1    |   MIDI_2    |
//|      |Number|             |             |             |
//|(4bit)|(4bit)|   (8bit)    |   (8bit)    |   (8bit)    |
//+------+------+-------------+-------------+-------------+
// CN == 0x0-0xf
//+-----+-----------+-------------------------------------------------------------------
//| CIN |MIDI_x size|Description
//+-----+-----------+-------------------------------------------------------------------
//| 0x0 | 1, 2 or 3 |Miscellaneous function codes. Reserved for future extensions.
//| 0x1 | 1, 2 or 3 |Cable events. Reserved for future expansion.
//| 0x2 |     2     |Two-byte System Common messages like MTC, SongSelect, etc.
//| 0x3 |     3     |Three-byte System Common messages like SPP, etc.
//| 0x4 |     3     |SysEx starts or continues
//| 0x5 |     1     |Single-byte System Common Message or SysEx ends with following single byte.
//| 0x6 |     2     |SysEx ends with following two bytes.
//| 0x7 |     3     |SysEx ends with following three bytes.
//| 0x8 |     3     |Note-off
//| 0x9 |     3     |Note-on
//| 0xA |     3     |Poly-KeyPress
//| 0xB |     3     |Control Change
//| 0xC |     2     |Program Change
//| 0xD |     2     |Channel Pressure
//| 0xE |     3     |PitchBend Change
//| 0xF |     1     |Single Byte
//+-----+-----------+-------------------------------------------------------------------

const uint8_t USBH_MIDI::epDataInIndex  = 1;
const uint8_t USBH_MIDI::epDataOutIndex = 2;
const uint8_t USBH_MIDI::epDataInIndexVSP  = 3;
const uint8_t USBH_MIDI::epDataOutIndexVSP = 4;

USBH_MIDI::USBH_MIDI(USB *p) :
pUsb(p),
bAddress(0),
bNumEP(1),
bPollEnable(false),
isMidiFound(false),
readPtr(0) {
        // initialize endpoint data structures
        for(uint8_t i=0; i<MIDI_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr        = 0;
                epInfo[i].maxPktSize  = (i) ? 0 : 8;
                epInfo[i].epAttribs     = 0;
//    epInfo[i].bmNakPower  = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
                epInfo[i].bmNakPower  = (i) ? USB_NAK_NOWAIT : 4;

        }
        // register in USB subsystem
        if (pUsb) {
                pUsb->RegisterDeviceClass(this);
        }
}

/* Connection initialization of an MIDI Device */
uint8_t USBH_MIDI::Init(uint8_t parent, uint8_t port, bool lowspeed)
{
        uint8_t    buf[DESC_BUFF_SIZE];
        USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
        uint8_t    rcode;
        UsbDevice  *p = NULL;
        EpInfo     *oldep_ptr = NULL;
        uint8_t    num_of_conf;  // number of configurations

        // get memory address of USB device address pool
        AddressPool &addrPool = pUsb->GetAddressPool();
#ifdef DEBUG_USB_HOST
        USBTRACE("\rMIDI Init\r\n");
#endif
        // check if address has already been assigned to an instance
        if (bAddress) {
                return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
        }
        // Get pointer to pseudo device with address 0 assigned
        p = addrPool.GetUsbDevicePtr(bAddress);
        if (!p) {
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
        }
        if (!p->epinfo) {
                return USB_ERROR_EPINFO_IS_NULL;
        }

        // Save old pointer to EP_RECORD of address 0
        oldep_ptr = p->epinfo;

        // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
        p->epinfo = epInfo;
        p->lowspeed = lowspeed;

        // Get device descriptor
        rcode = pUsb->getDevDescr( 0, 0, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t*)buf );
        vid = udd->idVendor;
        pid = udd->idProduct;
        // Restore p->epinfo
        p->epinfo = oldep_ptr;

        if( rcode ){
                goto FailGetDevDescr;
        }

        // Allocate new address according to device class
        bAddress = addrPool.AllocAddress(parent, false, port);
        if (!bAddress) {
                return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
        }

        // Extract Max Packet Size from device descriptor
        epInfo[0].maxPktSize = udd->bMaxPacketSize0;

        // Assign new address to the device
        rcode = pUsb->setAddr( 0, 0, bAddress );
        if (rcode) {
                p->lowspeed = false;
                addrPool.FreeAddress(bAddress);
                bAddress = 0;
                return rcode;
        }//if (rcode...
#ifdef DEBUG_USB_HOST
        USBTRACE2("Addr:", bAddress);
#endif
        p->lowspeed = false;

        //get pointer to assigned address record
        p = addrPool.GetUsbDevicePtr(bAddress);
        if (!p) {
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
        }
        p->lowspeed = lowspeed;

        num_of_conf = udd->bNumConfigurations;

        // Assign epInfo to epinfo pointer
        rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
        if (rcode) {
#ifdef DEBUG_USB_HOST
                USBTRACE("setEpInfoEntry failed");
#endif
                goto FailSetDevTblEntry;
        }
#ifdef DEBUG_USB_HOST
        USBTRACE2("NC:", num_of_conf);
#endif
        for (uint8_t i=0; i<num_of_conf; i++) {
                parseConfigDescr(bAddress, i);
                if (bNumEP > 1)
                        break;
        } // for
#ifdef DEBUG_USB_HOST
        USBTRACE2("NumEP:", bNumEP);
#endif
        if( bConfNum == 0 ){  //Device not found.
                goto FailGetConfDescr;
        }

        if( !isMidiFound ){ //MIDI Device not found. Try first Bulk transfer device
                epInfo[epDataInIndex].epAddr      = epInfo[epDataInIndexVSP].epAddr;
                epInfo[epDataInIndex].maxPktSize  = epInfo[epDataInIndexVSP].maxPktSize;
                epInfo[epDataOutIndex].epAddr     = epInfo[epDataOutIndexVSP].epAddr;
                epInfo[epDataOutIndex].maxPktSize = epInfo[epDataOutIndexVSP].maxPktSize;
        }

        // Assign epInfo to epinfo pointer
        rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);
#ifdef DEBUG_USB_HOST
        USBTRACE2("Conf:", bConfNum);
#endif
        // Set Configuration Value
        rcode = pUsb->setConf(bAddress, 0, bConfNum);
        if (rcode) {
                goto FailSetConfDescr;
        }
#ifdef DEBUG_USB_HOST
        USBTRACE("Init done.");
#endif
        bPollEnable = true;
        return 0;
FailGetDevDescr:
FailSetDevTblEntry:
FailGetConfDescr:
FailSetConfDescr:
        Release();
        return rcode;
}

/* get and parse config descriptor */
void USBH_MIDI::parseConfigDescr( byte addr, byte conf )
{
        uint8_t buf[ DESC_BUFF_SIZE ];
        uint8_t* buf_ptr = buf;
        byte rcode;
        byte descr_length;
        byte descr_type;
        unsigned int total_length;
        USB_ENDPOINT_DESCRIPTOR *epDesc;
        boolean isMidi = false;

        // get configuration descriptor (get descriptor size only)
        rcode = pUsb->getConfDescr( addr, 0, 4, conf, buf );
        if( rcode ){
                return;
        }
        total_length = buf[2] | ((int)buf[3] << 8);
        if( total_length > DESC_BUFF_SIZE ) {    //check if total length is larger than buffer
                total_length = DESC_BUFF_SIZE;
        }

        // get configuration descriptor (all)
        rcode = pUsb->getConfDescr( addr, 0, total_length, conf, buf ); //get the whole descriptor
        if( rcode ){
                return;
        }

        //parsing descriptors
        while( buf_ptr < buf + total_length ) {
                descr_length = *( buf_ptr );
                descr_type   = *( buf_ptr + 1 );
                switch( descr_type ) {
                  case USB_DESCRIPTOR_CONFIGURATION :
                        bConfNum = buf_ptr[5];
                        break;
                  case  USB_DESCRIPTOR_INTERFACE :
                        if( buf_ptr[5] == USB_CLASS_AUDIO && buf_ptr[6] == USB_SUBCLASS_MIDISTREAMING ) {  //p[5]; bInterfaceClass = 1(Audio), p[6]; bInterfaceSubClass = 3(MIDI Streaming)
                                isMidiFound = true; //MIDI device found.
                                isMidi      = true;
                        }else{
#ifdef DEBUG_USB_HOST
                                USBTRACE("No MIDI Device\n");
#endif
                                isMidi = false;
                        }
                        break;
                  case USB_DESCRIPTOR_ENDPOINT :
                        epDesc = (USB_ENDPOINT_DESCRIPTOR *)buf_ptr;
                        if ((epDesc->bmAttributes & 0x02) == 2) {//bulk
                                uint8_t index;
                                if( isMidi )
                                        index = ((epDesc->bEndpointAddress & 0x80) == 0x80) ? epDataInIndex : epDataOutIndex;
                                else
                                        index = ((epDesc->bEndpointAddress & 0x80) == 0x80) ? epDataInIndexVSP : epDataOutIndexVSP;
                                epInfo[index].epAddr     = (epDesc->bEndpointAddress & 0x0F);
                                epInfo[index].maxPktSize = (uint8_t)epDesc->wMaxPacketSize;
                                bNumEP ++;
#ifdef DEBUG_USB_HOST
                                PrintEndpointDescriptor(epDesc);
#endif
                        }
                        break;
                  default:
                        break;
                }//switch( descr_type
                buf_ptr += descr_length;    //advance buffer pointer
        }//while( buf_ptr <=...
}

/* Performs a cleanup after failed Init() attempt */
uint8_t USBH_MIDI::Release()
{
        pUsb->GetAddressPool().FreeAddress(bAddress);
        bNumEP       = 1;               //must have to be reset to 1
        bAddress     = 0;
        bPollEnable  = false;
        readPtr      = 0;
        return 0;
}

/* Receive data from MIDI device */
uint8_t USBH_MIDI::RecvData(uint16_t *bytes_rcvd, uint8_t *dataptr)
{
        *bytes_rcvd = (uint16_t)epInfo[epDataInIndex].maxPktSize;
        uint8_t  r = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, bytes_rcvd, dataptr);

        if( *bytes_rcvd < (MIDI_EVENT_PACKET_SIZE-4)){
                dataptr[*bytes_rcvd]     = '\0';
                dataptr[(*bytes_rcvd)+1] = '\0';
        }
        return r;
}

/* Receive data from MIDI device */
uint8_t USBH_MIDI::RecvData(uint8_t *outBuf)
{
        byte rcode = 0;     //return code
        uint16_t  rcvd;

        if( bPollEnable == false ) return false;

        //Checking unprocessed message in buffer.
        if( readPtr != 0 && readPtr < MIDI_EVENT_PACKET_SIZE ){
                if(recvBuf[readPtr] == 0 && recvBuf[readPtr+1] == 0) {
                        //no unprocessed message left in the buffer.
                }else{
                        goto RecvData_return_from_buffer;
                }
        }

        readPtr = 0;
        rcode = RecvData( &rcvd, recvBuf);
        if( rcode != 0 ) {
                return 0;
        }

        //if all data is zero, no valid data received.
        if( recvBuf[0] == 0 && recvBuf[1] == 0 && recvBuf[2] == 0 && recvBuf[3] == 0 ) {
                return 0;
        }

RecvData_return_from_buffer:
        readPtr++;
        outBuf[0] = recvBuf[readPtr];
        readPtr++;
        outBuf[1] = recvBuf[readPtr];
        readPtr++;
        outBuf[2] = recvBuf[readPtr];
        readPtr++;
        return lookupMsgSize(outBuf[0]);
}

/* Send data to MIDI device */
uint8_t USBH_MIDI::SendData(uint8_t *dataptr, byte nCable)
{
        byte buf[4];
        byte msg;

        msg = dataptr[0];
        // SysEx long message ?
        if( msg == 0xf0 )
        {
                return SendSysEx(dataptr, countSysExDataSize(dataptr), nCable);
        }

        buf[0] = (nCable << 4) | (msg >> 4);
        if( msg < 0xf0 ) msg = msg & 0xf0;


        //Building USB-MIDI Event Packets
        buf[1] = dataptr[0];
        buf[2] = dataptr[1];
        buf[3] = dataptr[2];

        switch(lookupMsgSize(msg)) {
          //3 bytes message
          case 3 :
                if(msg == 0xf2) {//system common message(SPP)
                        buf[0] = (nCable << 4) | 3;
                }
                break;

          //2 bytes message
          case 2 :
                if(msg == 0xf1 || msg == 0xf3) {//system common message(MTC/SongSelect)
                        buf[0] = (nCable << 4) | 2;
                }
                buf[3] = 0;
                break;

          //1 bytes message
          case 1 :
          default :
                buf[2] = 0;
                buf[3] = 0;
                break;
        }
        return pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, 4, buf);
}

#ifdef DEBUG_USB_HOST
void USBH_MIDI::PrintEndpointDescriptor( const USB_ENDPOINT_DESCRIPTOR* ep_ptr )
{
        Notify(PSTR("Endpoint descriptor:"), 0x80);
        Notify(PSTR("\r\nLength:\t\t"), 0x80);
        PrintHex<uint8_t>(ep_ptr->bLength, 0x80);
        Notify(PSTR("\r\nType:\t\t"), 0x80);
        PrintHex<uint8_t>(ep_ptr->bDescriptorType, 0x80);
        Notify(PSTR("\r\nAddress:\t"), 0x80);
        PrintHex<uint8_t>(ep_ptr->bEndpointAddress, 0x80);
        Notify(PSTR("\r\nAttributes:\t"), 0x80);
        PrintHex<uint8_t>(ep_ptr->bmAttributes, 0x80);
        Notify(PSTR("\r\nMaxPktSize:\t"), 0x80);
        PrintHex<uint16_t>(ep_ptr->wMaxPacketSize, 0x80);
        Notify(PSTR("\r\nPoll Intrv:\t"), 0x80);
        PrintHex<uint8_t>(ep_ptr->bInterval, 0x80);
        Notify(PSTR("\r\n"), 0x80);
}
#endif

/* look up a MIDI message size from spec */
/*Return                                 */
/*  0 : undefined message                */
/*  0<: Vaild message size(1-3)          */
uint8_t USBH_MIDI::lookupMsgSize(uint8_t midiMsg)
{
        uint8_t msgSize = 0;

        if( midiMsg < 0xf0 ) midiMsg &= 0xf0;
        switch(midiMsg) {
          //3 bytes messages
          case 0xf2 : //system common message(SPP)
          case 0x80 : //Note off
          case 0x90 : //Note on
          case 0xa0 : //Poly KeyPress
          case 0xb0 : //Control Change
          case 0xe0 : //PitchBend Change
                msgSize = 3;
                break;

          //2 bytes messages
          case 0xf1 : //system common message(MTC)
          case 0xf3 : //system common message(SongSelect)
          case 0xc0 : //Program Change
          case 0xd0 : //Channel Pressure
                msgSize = 2;
                break;

          //1 bytes messages
          case 0xf8 : //system realtime message
          case 0xf9 : //system realtime message
          case 0xfa : //system realtime message
          case 0xfb : //system realtime message
          case 0xfc : //system realtime message
          case 0xfe : //system realtime message
          case 0xff : //system realtime message
                msgSize = 1;
                break;

          //undefine messages
          default :
                break;
        }
        return msgSize;
}

/* SysEx data size counter */
unsigned int USBH_MIDI::countSysExDataSize(uint8_t *dataptr)
{
        unsigned int c = 1;

        if( *dataptr != 0xf0 ){ //not SysEx
                return 0;
        }

        //Search terminator(0xf7)
        while(*dataptr != 0xf7)
        {
                dataptr++;
                c++;

                //Limiter (upto 256 bytes)
                if(c > 256){
                        c = 0;
                        break;
                }
        }
        return c;
}

/* Send SysEx message to MIDI device */
uint8_t USBH_MIDI::SendSysEx(uint8_t *dataptr, unsigned int datasize, byte nCable)
{
        byte buf[4];
        uint8_t rc;
        unsigned int n = datasize;

        while(n > 0) {
                //Byte 0
                buf[0] = (nCable << 4) | 0x4;           //x4 SysEx starts or continues

                switch ( n ) {
                  case 1 :
                        buf[0] = (nCable << 4) | 0x5;   //x5 SysEx ends with following single byte.
                        buf[1] = *(dataptr++);
                        buf[2] = 0x00;
                        buf[3] = 0x00;
                        n = n - 1;
                        break;
                  case 2 :
                        buf[0] = (nCable << 4) | 0x6;   //x6 SysEx ends with following two bytes.
                        buf[1] = *(dataptr++);
                        buf[2] = *(dataptr++);
                        buf[3] = 0x00;
                        n = n - 2;
                        break;
                  case 3 :
                        buf[0] = (nCable << 4) | 0x7;   //x7 SysEx ends with following three bytes.
                  default :
                        buf[1] = *(dataptr++);
                        buf[2] = *(dataptr++);
                        buf[3] = *(dataptr++);
                        n = n - 3;
                        break;
                }
                rc = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, 4, buf);
                if(rc != 0)
                 break;
        }
        return(rc);
}
