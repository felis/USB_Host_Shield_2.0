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

#include "XBOXRECV.h"
#define DEBUG // Uncomment to print data for debugging
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the Xbox 360 Controller

XBOXRECV::XBOXRECV(USB *p):
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0), // device address - mandatory
bPollEnable(false) { // don't start polling before dongle is connected
    for(uint8_t i=0; i<XBOX_MAX_ENDPOINTS; i++) {
		epInfo[i].epAddr		= 0;
		epInfo[i].maxPktSize	= (i) ? 0 : 8;
		epInfo[i].epAttribs		= 0;        
        epInfo[i].bmNakPower    = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
	}
    
    if (pUsb) // register in USB subsystem
		pUsb->RegisterDeviceClass(this); //set devConfig[] entry
}

uint8_t XBOXRECV::Init(uint8_t parent, uint8_t port, bool lowspeed) {
    uint8_t	buf[sizeof(USB_DEVICE_DESCRIPTOR)];
	uint8_t	rcode;
	UsbDevice *p = NULL;
	EpInfo *oldep_ptr = NULL;
    uint16_t PID;
    uint16_t VID;
    
    // get memory address of USB device address pool
	AddressPool	&addrPool = pUsb->GetAddressPool();    
#ifdef EXTRADEBUG
	Notify(PSTR("\r\nXBOXRECV Init"));
#endif
    // check if address has already been assigned to an instance
    if (bAddress) {
#ifdef DEBUG
        Notify(PSTR("\r\nAddress in use"));
#endif
        return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
    }
    
    // Get pointer to pseudo device with address 0 assigned
    p = addrPool.GetUsbDevicePtr(0);
    
    if (!p) {        
#ifdef DEBUG
	    Notify(PSTR("\r\nAddress not found"));
#endif
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }
    
    if (!p->epinfo) {
#ifdef DEBUG
        Notify(PSTR("\r\nepinfo is null"));
#endif
        return USB_ERROR_EPINFO_IS_NULL;
    }
    
    // Save old pointer to EP_RECORD of address 0
    oldep_ptr = p->epinfo;
    
    // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
    p->epinfo = epInfo;
    
    p->lowspeed = lowspeed;
    
    // Get device descriptor
    rcode = pUsb->getDevDescr(0, 0, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t*)buf);// Get device descriptor - addr, ep, nbytes, data
    // Restore p->epinfo
    p->epinfo = oldep_ptr;
    
    if(rcode)
        goto FailGetDevDescr;
    
    VID = ((USB_DEVICE_DESCRIPTOR*)buf)->idVendor;
    PID = ((USB_DEVICE_DESCRIPTOR*)buf)->idProduct;
    
    if(VID != XBOX_VID && VID != MADCATZ_VID) // We just check if it's a xbox receiver using the Vendor ID
        goto FailUnknownDevice;
    else if(PID != XBOX_WIRELESS_RECEIVER_PID && PID != XBOX_WIRELESS_RECEIVER_THIRD_PARTY_PID) {
#ifdef DEBUG
        Notify(PSTR("\r\nYou'll need a wireless receiver for this libary to work"));
#endif
        goto FailUnknownDevice;
    }        
    
    // Allocate new address according to device class
    bAddress = addrPool.AllocAddress(parent, false, port);
    
    if (!bAddress)
		return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
    
    // Extract Max Packet Size from device descriptor
    epInfo[0].maxPktSize = (uint8_t)((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0; 
    
    // Assign new address to the device
    rcode = pUsb->setAddr( 0, 0, bAddress );
    if (rcode) {
        p->lowspeed = false;
        addrPool.FreeAddress(bAddress);
        bAddress = 0;
#ifdef DEBUG
        Notify(PSTR("\r\nsetAddr: "));
#endif
        PrintHex<uint8_t>(rcode);
        return rcode;
    }
#ifdef EXTRADEBUG
    Notify(PSTR("\r\nAddr: "));
    PrintHex<uint8_t>(bAddress);
#endif
    p->lowspeed = false;
    
    //get pointer to assigned address record
    p = addrPool.GetUsbDevicePtr(bAddress);
    if (!p) 
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    
    p->lowspeed = lowspeed;        
    
    // Assign epInfo to epinfo pointer - only EP0 is known
    rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
    if (rcode)
        goto FailSetDevTblEntry;
            
    /* The application will work in reduced host mode, so we can save program and data
       memory space. After verifying the VID we will use known values for the
       configuration values for device, interface, endpoints and HID for the XBOX360 Wireless receiver */
        
    /* Initialize data structures for endpoints of device */
    epInfo[ XBOX_INPUT_PIPE_1 ].epAddr = 0x01;    // XBOX 360 report endpoint - poll interval 1ms
    epInfo[ XBOX_INPUT_PIPE_1 ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_INPUT_PIPE_1 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_INPUT_PIPE_1 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_INPUT_PIPE_1 ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_INPUT_PIPE_1 ].bmRcvToggle = bmRCVTOG0;
    epInfo[ XBOX_OUTPUT_PIPE_1 ].epAddr = 0x01;    // XBOX 360 output endpoint - poll interval 8ms
    epInfo[ XBOX_OUTPUT_PIPE_1 ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_OUTPUT_PIPE_1 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_OUTPUT_PIPE_1 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_OUTPUT_PIPE_1 ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_OUTPUT_PIPE_1 ].bmRcvToggle = bmRCVTOG0;
    
    epInfo[ XBOX_INPUT_PIPE_2 ].epAddr = 0x03;    // XBOX 360 report endpoint - poll interval 1ms
    epInfo[ XBOX_INPUT_PIPE_2 ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_INPUT_PIPE_2 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_INPUT_PIPE_2 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_INPUT_PIPE_2 ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_INPUT_PIPE_2 ].bmRcvToggle = bmRCVTOG0;
    epInfo[ XBOX_OUTPUT_PIPE_2 ].epAddr = 0x03;    // XBOX 360 output endpoint - poll interval 8ms
    epInfo[ XBOX_OUTPUT_PIPE_2 ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_OUTPUT_PIPE_2 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_OUTPUT_PIPE_2 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_OUTPUT_PIPE_2 ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_OUTPUT_PIPE_2 ].bmRcvToggle = bmRCVTOG0;
    
    epInfo[ XBOX_INPUT_PIPE_3 ].epAddr = 0x05;    // XBOX 360 report endpoint - poll interval 1ms
    epInfo[ XBOX_INPUT_PIPE_3 ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_INPUT_PIPE_3 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_INPUT_PIPE_3 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_INPUT_PIPE_3 ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_INPUT_PIPE_3 ].bmRcvToggle = bmRCVTOG0;
    epInfo[ XBOX_OUTPUT_PIPE_3 ].epAddr = 0x05;    // XBOX 360 output endpoint - poll interval 8ms
    epInfo[ XBOX_OUTPUT_PIPE_3 ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_OUTPUT_PIPE_3 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_OUTPUT_PIPE_3 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_OUTPUT_PIPE_3 ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_OUTPUT_PIPE_3 ].bmRcvToggle = bmRCVTOG0;
    
    epInfo[ XBOX_INPUT_PIPE_4 ].epAddr = 0x07;    // XBOX 360 report endpoint - poll interval 1ms
    epInfo[ XBOX_INPUT_PIPE_4 ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_INPUT_PIPE_4 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_INPUT_PIPE_4 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_INPUT_PIPE_4 ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_INPUT_PIPE_4 ].bmRcvToggle = bmRCVTOG0;
    epInfo[ XBOX_OUTPUT_PIPE_4 ].epAddr = 0x07;    // XBOX 360 output endpoint - poll interval 8ms
    epInfo[ XBOX_OUTPUT_PIPE_4 ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_OUTPUT_PIPE_4 ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_OUTPUT_PIPE_4 ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_OUTPUT_PIPE_4 ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_OUTPUT_PIPE_4 ].bmRcvToggle = bmRCVTOG0;
        
    rcode = pUsb->setEpInfoEntry(bAddress, 9, epInfo);
    if( rcode )
        goto FailSetDevTblEntry;
        
    delay(200);//Give time for address change
        
    rcode = pUsb->setConf(bAddress, epInfo[ XBOX_CONTROL_PIPE ].epAddr, 1);
    if( rcode )
        goto FailSetConf;        

#ifdef DEBUG
    Notify(PSTR("\r\nXbox Wireless Receiver Connected\r\n"));
#endif
    XboxReceiverConnected = true;
    bPollEnable = true;
    return 0; // successful configuration
    
    /* diagnostic messages */  
FailGetDevDescr:
#ifdef DEBUG
    Notify(PSTR("\r\ngetDevDescr:"));
#endif
    goto Fail;    
FailSetDevTblEntry:
#ifdef DEBUG
    Notify(PSTR("\r\nsetDevTblEn:"));
#endif
    goto Fail;
FailSetConf:
#ifdef DEBUG
    Notify(PSTR("\r\nsetConf:"));
#endif
    goto Fail; 
FailUnknownDevice:
#ifdef DEBUG
    Notify(PSTR("\r\nUnknown Device Connected - VID: "));
    PrintHex<uint16_t>(VID);
    Notify(PSTR(" PID: "));
    PrintHex<uint16_t>(PID);
#endif
    rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
    goto Fail;
Fail:
#ifdef DEBUG
    Notify(PSTR("\r\nXbox 360 Init Failed, error code: "));
    Serial.print(rcode,HEX);
#endif    
    Release();
    return rcode;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t XBOXRECV::Release() {
    XboxReceiverConnected = false;
    for(uint8_t i=0;i<4;i++)
        Xbox360Connected[i] = false;
	pUsb->GetAddressPool().FreeAddress(bAddress);    
	bAddress = 0;
    bPollEnable = false;
	return 0;
}
uint8_t XBOXRECV::Poll() {    
	if (!bPollEnable)
		return 0;
    uint16_t BUFFER_SIZE = EP_MAXPKTSIZE;
    for(uint8_t i=0;i<4;i++) {
        switch (i) {
            case 0:
                pUsb->inTransfer(bAddress, epInfo[ XBOX_INPUT_PIPE_1 ].epAddr, &BUFFER_SIZE, readBuf);
                break;
            case 1:
                pUsb->inTransfer(bAddress, epInfo[ XBOX_INPUT_PIPE_2 ].epAddr, &BUFFER_SIZE, readBuf);
                break;
            case 2:
                pUsb->inTransfer(bAddress, epInfo[ XBOX_INPUT_PIPE_3 ].epAddr, &BUFFER_SIZE, readBuf);
                break;
            case 3:
                pUsb->inTransfer(bAddress, epInfo[ XBOX_INPUT_PIPE_4 ].epAddr, &BUFFER_SIZE, readBuf);
                break;                
            default:
                break;
        }
        if(BUFFER_SIZE > 0) {
#ifdef EXTRADEBUG
            Notify(PSTR("Bytes Received: "));
            Serial.print(BUFFER_SIZE);
            Notify(PSTR("\r\n"));
#endif
            readReport(i);
#ifdef PRINTREPORT
            printReport(i,BUFFER_SIZE); // Uncomment "#define PRINTREPORT" to print the report send by the Xbox 360 Controller
#endif            
        }
    }
	return 0;
}

void XBOXRECV::readReport(uint8_t controller) {
    if (readBuf == NULL)
        return;
    if(readBuf[0] == 0x08) { // This report is send when a controller is connected and disconnected
        Xbox360Connected[controller] = (bool)(readBuf[1] == 0x80);
#ifdef DEBUG
        Notify(PSTR("Controller "));
        Serial.print(controller);
#endif
        if(Xbox360Connected[controller]) {
#ifdef DEBUG
            Notify(PSTR(": connected\r\n"));
#endif
            switch (controller) {
                case 0:
                    setLedOn(controller,LED1);
                    break;
                case 1:
                    setLedOn(controller,LED2);
                    break;
                case 2:
                    setLedOn(controller,LED3);
                    break;
                case 3:
                    setLedOn(controller,LED4);
                    break;
            }
        }
#ifdef DEBUG
        else
            Notify(PSTR(": disconnected\r\n"));
#endif
        return;
    }
    if(readBuf[1] != 0x01) // Check if it's the correct report - the receiver also sends different status reports
        return;
    
    Xbox360Connected[controller] = true; // A controller must be connected if it's sending data

    ButtonState[controller] = (uint32_t)(readBuf[9] | ((uint16_t)readBuf[8] << 8) | ((uint32_t)readBuf[7] << 16) | ((uint32_t)readBuf[6] << 24));
    
    hatValue[controller][0] = (int16_t)(((uint16_t)readBuf[11] << 8) | readBuf[10]);
    hatValue[controller][1] = (int16_t)(((uint16_t)readBuf[13] << 8) | readBuf[12]);
    hatValue[controller][2] = (int16_t)(((uint16_t)readBuf[15] << 8) | readBuf[14]);
    hatValue[controller][3] = (int16_t)(((uint16_t)readBuf[17] << 8) | readBuf[16]);
    
    //Notify(PSTR("\r\nButtonState: "));
    //PrintHex<uint32_t>(ButtonState[controller]);
    
    if(ButtonState[controller] != OldButtonState[controller]) {
        ButtonClickState[controller] = (ButtonState[controller] >> 16) & ((~OldButtonState[controller]) >> 16); // Update click state variable, but don't include the two trigger buttons L2 and R2
        if(((uint8_t)OldButtonState[controller]) == 0 && ((uint8_t)ButtonState[controller]) != 0) // The L2 and R2 buttons are special as they are analog buttons
            R2Clicked = true;
        if((uint8_t)(OldButtonState[controller] >> 8) == 0 && (uint8_t)(ButtonState[controller] >> 8) != 0)
            L2Clicked = true;
        OldButtonState[controller] = ButtonState[controller];
    }
}

void XBOXRECV::printReport(uint8_t controller, uint8_t nBytes) { //Uncomment "#define PRINTREPORT" to print the report send by the Xbox 360 Controller
#if defined(DEBUG) || defined(PRINTREPORT)
    if (readBuf == NULL)
        return;
    Notify(PSTR("Controller "));
    Serial.print(controller);
    Notify(PSTR(": "));
    for(uint8_t i = 0; i < nBytes;i++) {
        PrintHex<uint8_t>(readBuf[i]);
        Serial.print(" ");
    }
    Serial.println();
#endif
}
uint8_t XBOXRECV::getButtonPress(uint8_t controller, Button b) {
    if(b == L2) // These are analog buttons
        return (uint8_t)(ButtonState[controller] >> 8);
    else if(b == R2)
        return (uint8_t)ButtonState[controller];
    return (ButtonState[controller] & ((uint32_t)b << 16));
}
bool XBOXRECV::getButtonClick(uint8_t controller, Button b) {
    if(b == L2) {
        if(L2Clicked) {
            L2Clicked = false;
            return true;
        }
        return false;
    }
    else if(b== R2) {
        if(R2Clicked) {
            R2Clicked = false;
            return true;
        }
        return false;
    }
    bool click = (ButtonClickState[controller] & (uint16_t)b);
    ButtonClickState[controller] &= ~((uint16_t)b);  // clear "click" event
    return click;
}
int16_t XBOXRECV::getAnalogHat(uint8_t controller, AnalogHat a) {
    return hatValue[controller][a];
}

void XBOXRECV::XboxCommand(uint8_t controller, uint8_t* data, uint16_t nbytes) {
    uint8_t rcode;
    switch (controller) {
        case 0:
            rcode = pUsb->outTransfer(bAddress, epInfo[ XBOX_OUTPUT_PIPE_1 ].epAddr, nbytes, data);
            break;
        case 1:
            rcode = pUsb->outTransfer(bAddress, epInfo[ XBOX_OUTPUT_PIPE_2 ].epAddr, nbytes, data);
            break;
        case 2:
            rcode = pUsb->outTransfer(bAddress, epInfo[ XBOX_OUTPUT_PIPE_3 ].epAddr, nbytes, data);
            break;
        case 3:
            rcode = pUsb->outTransfer(bAddress, epInfo[ XBOX_OUTPUT_PIPE_4 ].epAddr, nbytes, data);
            break;
        default:
            break;
    }
#ifdef EXTRADEBUG
    if(rcode)
        Notify(PSTR("Error sending Xbox message\r\n"));
#endif
}
void XBOXRECV::setLedRaw(uint8_t controller, uint8_t value) {
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x00;
    writeBuf[2] = 0x08;
    writeBuf[3] = value+0x40;
    writeBuf[4] = 0x00;
    writeBuf[5] = 0x00;
    writeBuf[6] = 0x00;
    writeBuf[7] = 0x00;
    writeBuf[8] = 0x00;
    writeBuf[9] = 0x00;
    writeBuf[10] = 0x00;
    writeBuf[11] = 0x00;
    
    for(uint8_t i=0;i<10;i++) { // This small hack is needed for some reason as the controller doesn't always respond to the command
        XboxCommand(controller, writeBuf, 12);
        delay(1);
    }
}
void XBOXRECV::setLedOn(uint8_t controller, LED l) {
    if(l == ALL) // All LEDs can't be on a the same time
        return;
    setLedRaw(controller,((uint8_t)l)+4);
}
void XBOXRECV::setLedBlink(uint8_t controller, LED l) {
    setLedRaw(controller,(uint8_t)l);
}
void XBOXRECV::setLedMode(uint8_t controller, LEDMode lm) { // This function is used to do some speciel LED stuff the controller supports
    setLedRaw(controller,(uint8_t)lm);
}
void XBOXRECV::setRumbleOn(uint8_t controller, uint8_t lValue, uint8_t rValue) {
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x01;
    writeBuf[2] = 0x0f;
    writeBuf[3] = 0xc0;
    writeBuf[4] = 0x00;
    writeBuf[5] = lValue; // big weight
    writeBuf[6] = rValue; // small weight
    writeBuf[7] = 0x00;
    writeBuf[8] = 0x00;
    writeBuf[9] = 0x00;
    writeBuf[10] = 0x00;
    writeBuf[11] = 0x00;
    
    XboxCommand(controller, writeBuf, 12);
}