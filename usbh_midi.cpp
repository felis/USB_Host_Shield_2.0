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

#include "usbh_midi.h"
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data

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

USBH_MIDI::USBH_MIDI(USB *p) :
pUsb(p),
bAddress(0),
bPollEnable(false),
readPtr(0) {
        // initialize endpoint data structures
        for(uint8_t i=0; i<MIDI_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr      = 0;
                epInfo[i].maxPktSize  = (i) ? 0 : 8;
                epInfo[i].bmNakPower  = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
        }
        // register in USB subsystem
        if (pUsb) {
                pUsb->RegisterDeviceClass(this);
        }
}

/* Connection initialization of an MIDI Device */
uint8_t USBH_MIDI::Init(uint8_t parent, uint8_t port, bool lowspeed)
{
        uint8_t    buf[sizeof (USB_DEVICE_DESCRIPTOR)];
        USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
        uint8_t    rcode;
        UsbDevice  *p = NULL;
        EpInfo     *oldep_ptr = NULL;
        uint8_t    num_of_conf;  // number of configurations
        uint8_t  bConfNum = 0;    // configuration number
        uint8_t  bNumEP = 1;      // total number of EP in the configuration

        USBTRACE("\rMIDI Init\r\n");
#ifdef DEBUG_USB_HOST
        Notify(PSTR("USBH_MIDI version "), 0x80), D_PrintHex((uint8_t) (USBH_MIDI_VERSION / 10000), 0x80), D_PrintHex((uint8_t) (USBH_MIDI_VERSION / 100 % 100), 0x80), D_PrintHex((uint8_t) (USBH_MIDI_VERSION % 100), 0x80), Notify(PSTR("\r\n"), 0x80);
#endif

        //for reconnect
        for(uint8_t i=epDataInIndex; i<=epDataOutIndex; i++) {
                epInfo[i].bmSndToggle = 0;
                epInfo[i].bmRcvToggle = 0;
                // If you want to retry if you get a NAK response when sending, enable the following:
                // epInfo[i].bmNakPower  = (i==epDataOutIndex) ? 10 : USB_NAK_NOWAIT;
        }

        // get memory address of USB device address pool
        AddressPool &addrPool = pUsb->GetAddressPool();

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

        // First Device Descriptor Request (Initially first 8 bytes)
        // https://techcommunity.microsoft.com/t5/microsoft-usb-blog/how-does-usb-stack-enumerate-a-device/ba-p/270685#_First_Device_Descriptor
        rcode = pUsb->getDevDescr( 0, 0, 8, (uint8_t*)buf );

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
        USBTRACE2("Addr:", bAddress);

        p->lowspeed = false;

        //get pointer to assigned address record
        p = addrPool.GetUsbDevicePtr(bAddress);
        if (!p) {
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
        }
        p->lowspeed = lowspeed;

        // Second Device Descriptor Request (Full)
        rcode = pUsb->getDevDescr( bAddress, 0, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t*)buf );
        if( rcode ){
                goto FailGetDevDescr;
        }
        vid = udd->idVendor;
        pid = udd->idProduct;
        num_of_conf = udd->bNumConfigurations;

        // Assign epInfo to epinfo pointer
        rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
        if (rcode) {
                USBTRACE("setEpInfoEntry failed");
                goto FailSetDevTblEntry;
        }

        USBTRACE("VID:"), D_PrintHex(vid, 0x80);
        USBTRACE(" PID:"), D_PrintHex(pid, 0x80);
        USBTRACE2(" #Conf:", num_of_conf);

        //Setup for well known vendor/device specific configuration
        bTransferTypeMask = bmUSB_TRANSFER_TYPE;
        setupDeviceSpecific();
        
        // STEP1: Check if attached device is a MIDI device and fill endpoint data structure
        USBTRACE("\r\nSTEP1: MIDI Start\r\n");
        for(uint8_t i = 0; i < num_of_conf; i++) {
                MidiDescParser midiDescParser(this, true);  // Check for MIDI device
                rcode = pUsb->getConfDescr(bAddress, 0, i, &midiDescParser);
                if(rcode) // Check error code
                        goto FailGetConfDescr;
                bNumEP += midiDescParser.getNumEPs();
                if(bNumEP > 1) {// All endpoints extracted
                        bConfNum = midiDescParser.getConfValue();
                        break;
                }
        }
        USBTRACE2("STEP1: MIDI,NumEP:", bNumEP);
        //Found the MIDI device?
        if( bNumEP == 1 ){  //Device not found.
                USBTRACE("MIDI not found.\r\nSTEP2: Attempts vendor specific bulk device\r\n");
                // STEP2: Check if attached device is a MIDI device and fill endpoint data structure
                for(uint8_t i = 0; i < num_of_conf; i++) {
                        MidiDescParser midiDescParser(this, false); // Allow all devices, vendor specific class with Bulk transfer
                        rcode = pUsb->getConfDescr(bAddress, 0, i, &midiDescParser);
                        if(rcode) // Check error code
                                goto FailGetConfDescr;
                        bNumEP += midiDescParser.getNumEPs();
                        if(bNumEP > 1) {// All endpoints extracted
                                bConfNum = midiDescParser.getConfValue();
                                break;
                        }
                }
                USBTRACE2("\r\nSTEP2: Vendor,NumEP:", bNumEP);
        }

        if( bNumEP < 2 ){  //Device not found.
                rcode = 0xff;
                goto FailGetConfDescr;
        }

        // Assign epInfo to epinfo pointer
        rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
        USBTRACE2("Conf:", bConfNum);
        USBTRACE2("EPin :", (uint8_t)(epInfo[epDataInIndex].epAddr + 0x80));
        USBTRACE2("EPout:", epInfo[epDataOutIndex].epAddr);

        // Set Configuration Value
        rcode = pUsb->setConf(bAddress, 0, bConfNum);
        if (rcode)
                goto FailSetConfDescr;

        bPollEnable = true;

        if(pFuncOnInit)
                pFuncOnInit(); // Call the user function

        USBTRACE("Init done.\r\n");
        return 0;
FailGetDevDescr:
FailSetDevTblEntry:
FailGetConfDescr:
FailSetConfDescr:
        Release();
        return rcode;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t USBH_MIDI::Release()
{
        if(pFuncOnRelease && bPollEnable)
                pFuncOnRelease(); // Call the user function

        pUsb->GetAddressPool().FreeAddress(bAddress);
        bAddress     = 0;
        bPollEnable  = false;
        readPtr      = 0;

        return 0;
}

/* Setup for well known vendor/device specific configuration */
void USBH_MIDI::setupDeviceSpecific()
{
        // Novation
        if( vid == 0x1235 ) {
                // LaunchPad and LaunchKey endpoint attribute is interrupt 
                // https://github.com/YuuichiAkagawa/USBH_MIDI/wiki/Novation-USB-Product-ID-List

                // LaunchPad: 0x20:S, 0x36:Mini, 0x51:Pro, 0x69:MK2
                if( pid == 0x20 || pid == 0x36 || pid == 0x51 || pid == 0x69 ) {
                        bTransferTypeMask = 2;
                        return;
                }

                // LaunchKey: 0x30-32,  0x35:Mini, 0x7B-0x7D:MK2, 0x0102,0x113-0x122:MiniMk3, 0x134-0x137:MK3
                if( (0x30 <= pid && pid <= 0x32) || pid == 0x35 || (0x7B <= pid && pid <= 0x7D) 
                  || pid == 0x102 || (0x113 <= pid && pid <= 0x122) || (0x134 <= pid && pid <= 0x137) ) {
                        bTransferTypeMask = 2;
                        return;
                }
        }
}

/* Receive data from MIDI device */
uint8_t USBH_MIDI::RecvData(uint16_t *bytes_rcvd, uint8_t *dataptr)
{
        *bytes_rcvd = (uint16_t)epInfo[epDataInIndex].maxPktSize;
        uint8_t  r = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, bytes_rcvd, dataptr);
#ifdef EXTRADEBUG
        if( r )
                USBTRACE2("inTransfer():", r);
#endif
        if( *bytes_rcvd < (MIDI_EVENT_PACKET_SIZE-4)){
                dataptr[*bytes_rcvd]     = '\0';
                dataptr[(*bytes_rcvd)+1] = '\0';
        }
        return r;
}

/* Receive data from MIDI device */
uint8_t USBH_MIDI::RecvData(uint8_t *outBuf, bool isRaw)
{
        uint8_t rcode = 0;     //return code
        uint16_t  rcvd;

        if( bPollEnable == false ) return 0;

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
        uint8_t m;
        uint8_t cin = recvBuf[readPtr];
        if( isRaw == true ) {
                *(outBuf++) = cin;
        }
        readPtr++;
        *(outBuf++) = m = recvBuf[readPtr++];
        *(outBuf++) =     recvBuf[readPtr++];
        *(outBuf++) =     recvBuf[readPtr++];

        return getMsgSizeFromCin(cin & 0x0f);
}

/* Send data to MIDI device */
uint8_t USBH_MIDI::SendData(uint8_t *dataptr, uint8_t nCable)
{
        uint8_t buf[4];
        uint8_t status = dataptr[0];

        uint8_t cin =  convertStatus2Cin(status);
        if ( status == 0xf0 ) {
                // SysEx long message
                return SendSysEx(dataptr, countSysExDataSize(dataptr), nCable);
        }

        //Building USB-MIDI Event Packets
        buf[0] = (uint8_t)(nCable << 4) | cin;
        buf[1] = dataptr[0];

        uint8_t msglen = getMsgSizeFromCin(cin);
        switch(msglen) {
          //3 bytes message
          case 3 :
                buf[2] = dataptr[1];
                buf[3] = dataptr[2];
                break;

          //2 bytes message
          case 2 :
                buf[2] = dataptr[1];
                buf[3] = 0;
                break;

          //1 byte message
          case 1 :
                buf[2] = 0;
                buf[3] = 0;
                break;
          default :
                break;
        }
#ifdef EXTRADEBUG
        //Dump for raw USB-MIDI event packet
        Notify(PSTR("SendData():"), 0x80), D_PrintHex((buf[0]), 0x80), D_PrintHex((buf[1]), 0x80), D_PrintHex((buf[2]), 0x80), D_PrintHex((buf[3]), 0x80), Notify(PSTR("\r\n"), 0x80);
#endif
        return pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, 4, buf);
}

#ifdef DEBUG_USB_HOST
void USBH_MIDI::PrintEndpointDescriptor( const USB_ENDPOINT_DESCRIPTOR* ep_ptr )
{
        USBTRACE("Endpoint descriptor:\r\n");
        USBTRACE2(" Length:\t", ep_ptr->bLength);
        USBTRACE2(" Type:\t\t", ep_ptr->bDescriptorType);
        USBTRACE2(" Address:\t", ep_ptr->bEndpointAddress);
        USBTRACE2(" Attributes:\t", ep_ptr->bmAttributes);
        USBTRACE2(" MaxPktSize:\t", ep_ptr->wMaxPacketSize);
        USBTRACE2(" Poll Intrv:\t", ep_ptr->bInterval);
}
#endif

/* look up a MIDI message size from spec */
/*Return                                 */
/*  0 : undefined message                */
/*  0<: Vaild message size(1-3)          */
//uint8_t USBH_MIDI::lookupMsgSize(uint8_t midiMsg, uint8_t cin)
uint8_t USBH_MIDI::lookupMsgSize(uint8_t status, uint8_t cin)
{
        if( cin == 0 ){
                cin =  convertStatus2Cin(status);
        }
        return getMsgSizeFromCin(cin);
}

/* SysEx data size counter */
uint16_t USBH_MIDI::countSysExDataSize(uint8_t *dataptr)
{
        uint16_t c = 1;

        if( *dataptr != 0xf0 ){ //not SysEx
                return 0;
        }

        //Search terminator(0xf7)
        while(*dataptr != 0xf7) {
                dataptr++;
                c++;
                //Limiter (default: 256 bytes)
                if(c > MIDI_MAX_SYSEX_SIZE){
                        c = 0;
                        break;
                }
        }
        return c;
}

/* Send SysEx message to MIDI device */
uint8_t USBH_MIDI::SendSysEx(uint8_t *dataptr, uint16_t datasize, uint8_t nCable)
{
        uint8_t buf[MIDI_EVENT_PACKET_SIZE];
        uint8_t rc = 0;
        uint16_t n = datasize;
        uint8_t wptr = 0;
        uint8_t maxpkt = epInfo[epDataInIndex].maxPktSize;

        USBTRACE("SendSysEx:\r\t");
        USBTRACE2(" Length:\t", datasize);
#ifdef EXTRADEBUG
        uint16_t pktSize = (n+2)/3;   //Calculate total USB MIDI packet size
        USBTRACE2(" Total pktSize:\t", pktSize);
#endif

        while(n > 0) {
                //Byte 0
                buf[wptr] = (nCable << 4) | 0x4;             //x4 SysEx starts or continues

                switch ( n ) {
                    case 1 :
                        buf[wptr++] = (nCable << 4) | 0x5;   //x5 SysEx ends with following single byte.
                        buf[wptr++] = *(dataptr++);
                        buf[wptr++] = 0x00;
                        buf[wptr++] = 0x00;
                        n = 0;
                        break;
                    case 2 :
                        buf[wptr++] = (nCable << 4) | 0x6;   //x6 SysEx ends with following two bytes.
                        buf[wptr++] = *(dataptr++);
                        buf[wptr++] = *(dataptr++);
                        buf[wptr++] = 0x00;
                        n = 0;
                        break;
                    case 3 :
                        buf[wptr]   = (nCable << 4) | 0x7;   //x7 SysEx ends with following three bytes.
                        // fall through
                    default :
                        wptr++;
                        buf[wptr++] = *(dataptr++);
                        buf[wptr++] = *(dataptr++);
                        buf[wptr++] = *(dataptr++);
                        n = n - 3;
                        break;
                }

                if( wptr >= maxpkt || n == 0 ){ //Reach a maxPktSize or data end.
                        USBTRACE2(" wptr:\t", wptr);
                        if( (rc = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, wptr, buf)) != 0 ){
                                break;
                        }
                        wptr = 0;  //rewind write pointer
                }
        }
        return(rc);
}

uint8_t USBH_MIDI::extractSysExData(uint8_t *p, uint8_t *buf)
{
        uint8_t rc = 0;
        uint8_t cin = *(p) & 0x0f;

        //SysEx message?
        if( (cin & 0xc) != 4 ) return rc;

        switch(cin) {
            case 4:
            case 7:
                *buf++ = *(p+1);
                *buf++ = *(p+2);
                *buf++ = *(p+3);
                rc = 3;
                break;
            case 6:
                *buf++ = *(p+1);
                *buf++ = *(p+2);
                rc = 2;
                break;
            case 5:
                *buf++ = *(p+1);
                rc = 1;
                break;
            default:
                break;
        }
        return(rc);
}

// Configuration Descriptor Parser
// Copied from confdescparser.h and modifiy.
MidiDescParser::MidiDescParser(UsbMidiConfigXtracter *xtractor, bool modeMidi) :
theXtractor(xtractor),
stateParseDescr(0),
dscrLen(0),
dscrType(0),
nEPs(0),
isMidiSearch(modeMidi){
        theBuffer.pValue = varBuffer;
        valParser.Initialize(&theBuffer);
        theSkipper.Initialize(&theBuffer);
}
void MidiDescParser::Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset __attribute__((unused))) {
        uint16_t cntdn = (uint16_t)len;
        uint8_t *p = (uint8_t*)pbuf;

        while(cntdn)
                if(!ParseDescriptor(&p, &cntdn))
                        return;
}

bool MidiDescParser::ParseDescriptor(uint8_t **pp, uint16_t *pcntdn) {
        USB_CONFIGURATION_DESCRIPTOR* ucd = reinterpret_cast<USB_CONFIGURATION_DESCRIPTOR*>(varBuffer);
        USB_INTERFACE_DESCRIPTOR* uid = reinterpret_cast<USB_INTERFACE_DESCRIPTOR*>(varBuffer);
        switch(stateParseDescr) {
                case 0:
                        theBuffer.valueSize = 2;
                        valParser.Initialize(&theBuffer);
                        stateParseDescr = 1;
                        // fall through
                case 1:
                        if(!valParser.Parse(pp, pcntdn))
                                return false;
                        dscrLen = *((uint8_t*)theBuffer.pValue);
                        dscrType = *((uint8_t*)theBuffer.pValue + 1);
                        stateParseDescr = 2;
                        // fall through
                case 2:
                        // This is a sort of hack. Assuming that two bytes are all ready in the buffer
                        //      the pointer is positioned two bytes ahead in order for the rest of descriptor
                        //      to be read right after the size and the type fields.
                        // This should be used carefully. varBuffer should be used directly to handle data
                        //      in the buffer.
                        theBuffer.pValue = varBuffer + 2;
                        stateParseDescr = 3;
                        // fall through
                case 3:
                        switch(dscrType) {
                                case USB_DESCRIPTOR_INTERFACE:
                                        isGoodInterface = false;
                                        break;
                                case USB_DESCRIPTOR_CONFIGURATION:
                                case USB_DESCRIPTOR_ENDPOINT:
                                case HID_DESCRIPTOR_HID:
                                        break;
                        }
                        theBuffer.valueSize = dscrLen - 2;
                        valParser.Initialize(&theBuffer);
                        stateParseDescr = 4;
                        // fall through
                case 4:
                        switch(dscrType) {
                                case USB_DESCRIPTOR_CONFIGURATION:
                                        if(!valParser.Parse(pp, pcntdn))
                                                return false;
                                        confValue = ucd->bConfigurationValue;
                                        break;
                                case USB_DESCRIPTOR_INTERFACE:
                                        if(!valParser.Parse(pp, pcntdn))
                                                return false;
                                        USBTRACE("Interface descriptor:\r\n");
                                        USBTRACE2(" Inf#:\t\t", uid->bInterfaceNumber);
                                        USBTRACE2(" Alt:\t\t", uid->bAlternateSetting);
                                        USBTRACE2(" EPs:\t\t", uid->bNumEndpoints);
                                        USBTRACE2(" IntCl:\t\t", uid->bInterfaceClass);
                                        USBTRACE2(" IntSubcl:\t", uid->bInterfaceSubClass);
                                        USBTRACE2(" Protocol:\t", uid->bInterfaceProtocol);
                                        // MIDI check mode ?
                                        if( isMidiSearch ){ //true: MIDI Streaming, false: ALL
                                                if( uid->bInterfaceClass == USB_CLASS_AUDIO && uid->bInterfaceSubClass == USB_SUBCLASS_MIDISTREAMING ) {
                                                        // MIDI found.
                                                        USBTRACE("+MIDI found\r\n\r\n");
                                                }else{
                                                        USBTRACE("-MIDI not found\r\n\r\n");
                                                        break;
                                                }
                                        }
                                        isGoodInterface = true;
                                        // Initialize the counter if no two endpoints can be found in one interface.
                                        if(nEPs < 2)
                                                // reset endpoint counter
                                                nEPs = 0;
                                        break;
                                case USB_DESCRIPTOR_ENDPOINT:
                                        if(!valParser.Parse(pp, pcntdn))
                                                return false;
                                        if(isGoodInterface && nEPs < 2){
                                                USBTRACE(">Extracting endpoint\r\n");
                                                if( theXtractor->EndpointXtract(confValue, 0, 0, 0, (USB_ENDPOINT_DESCRIPTOR*)varBuffer) ) 
                                                        nEPs++;
                                        }
                                        break;

                                default:
                                        if(!theSkipper.Skip(pp, pcntdn, dscrLen - 2))
                                                return false;
                        }
                        theBuffer.pValue = varBuffer;
                        stateParseDescr = 0;
        }
        return true;
}

/* Extracts endpoint information from config descriptor */
bool USBH_MIDI::EndpointXtract(uint8_t conf __attribute__((unused)),
        uint8_t iface __attribute__((unused)),
        uint8_t alt __attribute__((unused)),
        uint8_t proto __attribute__((unused)),
        const USB_ENDPOINT_DESCRIPTOR *pep)
{
        uint8_t index;

#ifdef DEBUG_USB_HOST
        PrintEndpointDescriptor(pep);
#endif
        // Is the endpoint transfer type bulk?
        if((pep->bmAttributes & bTransferTypeMask) == USB_TRANSFER_TYPE_BULK) {
                USBTRACE("+valid EP found.\r\n");
                index = (pep->bEndpointAddress & 0x80) == 0x80 ? epDataInIndex : epDataOutIndex;
        } else {
                USBTRACE("-No valid EP found.\r\n");
                return false;
        } 

        // Fill the rest of endpoint data structure
        epInfo[index].epAddr = (pep->bEndpointAddress & 0x0F);
        // The maximum packet size for the USB Host Shield 2.0 library is 64 bytes.
        if(pep->wMaxPacketSize > MIDI_EVENT_PACKET_SIZE) {
                epInfo[index].maxPktSize = MIDI_EVENT_PACKET_SIZE;
        } else {
                epInfo[index].maxPktSize = (uint8_t)pep->wMaxPacketSize;
        }

        return true;
}
