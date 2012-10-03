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

#include "XBOXUSB.h"
#define DEBUG // Uncomment to print data for debugging
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the Xbox 360 Controller

XBOXUSB::XBOXUSB(USB *p):
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

uint8_t XBOXUSB::Init(uint8_t parent, uint8_t port, bool lowspeed) {
    uint8_t	buf[sizeof(USB_DEVICE_DESCRIPTOR)];
	uint8_t	rcode;
	UsbDevice *p = NULL;
	EpInfo *oldep_ptr = NULL;
    uint16_t PID;
    uint16_t VID;
    
    // get memory address of USB device address pool
	AddressPool	&addrPool = pUsb->GetAddressPool();    
#ifdef EXTRADEBUG
	Notify(PSTR("\r\nXBOXUSB Init"));
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
    
    if(VID != XBOX_VID && VID != MADCATZ_VID) // We just check if it's a xbox controller using the Vendor ID
        goto FailUnknownDevice;
    if(PID == XBOX_WIRELESS_PID) {
#ifdef DEBUG
        Notify(PSTR("\r\nYou have plugged in a wireless Xbox 360 controller - it doesn't support USB communication"));
#endif
        goto FailUnknownDevice;
    }
    else if(PID == XBOX_WIRELESS_RECEIVER_PID || PID == XBOX_WIRELESS_RECEIVER_THIRD_PARTY_PID) {
#ifdef DEBUG
        Notify(PSTR("\r\nThis library only supports Xbox 360 controllers via USB"));
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
       configuration values for device, interface, endpoints and HID for the XBOX360 Controllers */
        
    /* Initialize data structures for endpoints of device */
    epInfo[ XBOX_INPUT_PIPE ].epAddr = 0x01;    // XBOX 360 report endpoint
    epInfo[ XBOX_INPUT_PIPE ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_INPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_INPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_INPUT_PIPE ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_INPUT_PIPE ].bmRcvToggle = bmRCVTOG0;
    epInfo[ XBOX_OUTPUT_PIPE ].epAddr = 0x02;    // XBOX 360 output endpoint
    epInfo[ XBOX_OUTPUT_PIPE ].epAttribs  = EP_INTERRUPT;
    epInfo[ XBOX_OUTPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
    epInfo[ XBOX_OUTPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
    epInfo[ XBOX_OUTPUT_PIPE ].bmSndToggle = bmSNDTOG0;
    epInfo[ XBOX_OUTPUT_PIPE ].bmRcvToggle = bmRCVTOG0;
        
    rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
    if( rcode )
        goto FailSetDevTblEntry;
        
    delay(200);//Give time for address change
        
    rcode = pUsb->setConf(bAddress, epInfo[ XBOX_CONTROL_PIPE ].epAddr, 1);
    if( rcode )
        goto FailSetConf;        

#ifdef DEBUG
    Notify(PSTR("\r\nXbox 360 Controller Connected"));
#endif                         
    setLedMode(ROTATING);
    Xbox360Connected = true;        

    bPollEnable = true;
    Notify(PSTR("\r\n"));
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
uint8_t XBOXUSB::Release() {
    Xbox360Connected = false;
	pUsb->GetAddressPool().FreeAddress(bAddress);    
	bAddress = 0;
    bPollEnable = false;
	return 0;
}
uint8_t XBOXUSB::Poll() {    
	if (!bPollEnable)
		return 0;
    uint16_t BUFFER_SIZE = EP_MAXPKTSIZE;
    pUsb->inTransfer(bAddress, epInfo[ XBOX_INPUT_PIPE ].epAddr, &BUFFER_SIZE, readBuf); // input on endpoint 1
    readReport();
#ifdef PRINTREPORT
    printReport(); // Uncomment "#define PRINTREPORT" to print the report send by the Xbox 360 Controller
#endif
	return 0;
}

void XBOXUSB::readReport() {              
    if (readBuf == NULL)
        return;
    if(readBuf[0] != 0x00 || readBuf[1] != 0x14) { // Check if it's the correct report - the controller also sends different status reports
        return;
    }

    ButtonState = (uint32_t)(readBuf[2] | ((uint16_t)readBuf[3] << 8) | ((uint32_t)readBuf[4] << 16) | ((uint32_t)readBuf[5] << 24));
    
    //Notify(PSTR("\r\nButtonState");
    //PrintHex<uint32_t>(ButtonState);
    
    if(ButtonState != OldButtonState) {
        buttonChanged = true;            
        if(ButtonState != 0x00) {
            buttonPressed = true; 
            buttonReleased = false;
        } else {
            buttonPressed = false;
            buttonReleased = true;
        }
    } else {
        buttonChanged = false;
        buttonPressed = false;
        buttonReleased = false;
    }
    
    OldButtonState = ButtonState; 
}  

void XBOXUSB::printReport() { //Uncomment "#define PRINTREPORT" to print the report send by the Xbox 360 Controller
    if (readBuf == NULL)
        return;
    for(uint8_t i = 0; i < XBOX_REPORT_BUFFER_SIZE;i++) {
        PrintHex<uint8_t>(readBuf[i]);
        Serial.print(" ");
    }             
    Serial.println("");
}

uint8_t XBOXUSB::getButton(Button b) {
    if (readBuf == NULL)
        return false;
    if(b == L2 || b == R2) { // These are analog buttons
        return (uint8_t)(readBuf[(uint8_t)b]);
    }
    else {
        if ((readBuf[(uint16_t)b >> 8] & ((uint8_t)b & 0xff)) > 0)
            return 1;
        else
            return 0;
    }
}
int16_t XBOXUSB::getAnalogHat(AnalogHat a) {
    if (readBuf == NULL)
        return 0;
    return (int16_t)(readBuf[(uint8_t)a+1] << 8 | readBuf[(uint8_t)a]);            
}

/* Playstation Sixaxis Dualshock and Navigation Controller commands */
void XBOXUSB::XboxCommand(uint8_t* data, uint16_t nbytes) {
    //bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set Report (0x09), Report ID (0x00), Report Type (Output 0x02), interface (0x00), datalength, datalength, data)
    pUsb->ctrlReq(bAddress,epInfo[XBOX_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0x00, 0x02, 0x00, nbytes, nbytes, data, NULL);
}
void XBOXUSB::setLedOn(LED l) {
    if(l == ALL) // All LEDs can't be on a the same time
        return;
    writeBuf[0] = 0x01;
    writeBuf[1] = 0x03;
    writeBuf[2] = (uint8_t)l;
    writeBuf[2] += 4;
    
    XboxCommand(writeBuf, 3);            
}
void XBOXUSB::setLedOff() {
    writeBuf[0] = 0x01;
    writeBuf[1] = 0x03;
    writeBuf[2] = 0x00;
    
    XboxCommand(writeBuf, 3);
}
void XBOXUSB::setLedBlink(LED l) {
    writeBuf[0] = 0x01;
    writeBuf[1] = 0x03;
    writeBuf[2] = (uint8_t)l;
    
    XboxCommand(writeBuf, 3);            
}
void XBOXUSB::setLedMode(LEDMode lm) { // This function is used to do some speciel LED stuff the controller supports
    writeBuf[0] = 0x01;
    writeBuf[1] = 0x03;
    writeBuf[2] = (uint8_t)lm;
    
    XboxCommand(writeBuf, 3);
}
void XBOXUSB::setRumbleOn(uint8_t lValue, uint8_t rValue) {
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x08;
    writeBuf[2] = 0x00;
    writeBuf[3] = lValue; // big weight
    writeBuf[4] = rValue; // small weight
    writeBuf[5] = 0x00;
    writeBuf[6] = 0x00;
    writeBuf[7] = 0x00;
    
    XboxCommand(writeBuf, 8);
}