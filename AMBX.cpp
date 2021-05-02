/* Copyright (C) 2021 Aran Vink. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Aran Vink
  e-mail   :  aranvink@gmail.com
 */

#include "AMBX.h"
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data

AMBX::AMBX(USB *p) :
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0) // device address - mandatory
{
        for(uint8_t i = 0; i < AMBX_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].bmSndToggle = 0;
                epInfo[i].bmRcvToggle = 0;
                epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
        }

        if(pUsb) // register in USB subsystem
                pUsb->RegisterDeviceClass(this); //set devConfig[] entry
}

uint8_t AMBX::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
        USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
        uint8_t rcode;
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;
        uint16_t PID;
        uint16_t VID;

        // get memory address of USB device address pool
        AddressPool &addrPool = pUsb->GetAddressPool();
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nAMBX Init"), 0x80);
#endif
        // check if address has already been assigned to an instance
        if(bAddress) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nAddress in use"), 0x80);
#endif
                return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
        }

        // Get pointer to pseudo device with address 0 assigned
        p = addrPool.GetUsbDevicePtr(0);

        if(!p) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nAddress not found"), 0x80);
#endif
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
        }

        if(!p->epinfo) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nepinfo is null"), 0x80);
#endif
                return USB_ERROR_EPINFO_IS_NULL;
        }

        // Save old pointer to EP_RECORD of address 0
        oldep_ptr = p->epinfo;

        // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
        p->epinfo = epInfo;

        p->lowspeed = lowspeed;

        // Get device descriptor
        rcode = pUsb->getDevDescr(0, 0, sizeof (USB_DEVICE_DESCRIPTOR), (uint8_t*)buf); // Get device descriptor - addr, ep, nbytes, data
        // Restore p->epinfo
        p->epinfo = oldep_ptr;

        if(rcode)
                goto FailGetDevDescr;

        VID = udd->idVendor;
        PID = udd->idProduct;

        if(VID != AMBX_VID || (PID != AMBX_PID))
                goto FailUnknownDevice;

        // Allocate new address according to device class
        bAddress = addrPool.AllocAddress(parent, false, port);

        if(!bAddress)
                return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

        // Extract Max Packet Size from device descriptor
        epInfo[0].maxPktSize = udd->bMaxPacketSize0;

        // Assign new address to the device
        rcode = pUsb->setAddr(0, 0, bAddress);
        if(rcode) {
                p->lowspeed = false;
                addrPool.FreeAddress(bAddress);
                bAddress = 0;
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nsetAddr: "), 0x80);
                D_PrintHex<uint8_t > (rcode, 0x80);
#endif
                return rcode;
        }
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nAddr: "), 0x80);
        D_PrintHex<uint8_t > (bAddress, 0x80);
#endif

        p->lowspeed = false;

        //get pointer to assigned address record
        p = addrPool.GetUsbDevicePtr(bAddress);
        if(!p)
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

        p->lowspeed = lowspeed;

        // Assign epInfo to epinfo pointer - only EP0 is known
        rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
        if(rcode)
                goto FailSetDevTblEntry;


        /* The application will work in reduced host mode, so we can save program and data
           memory space. After verifying the PID and VID we will use known values for the
           configuration values for device, interface, endpoints for the AMBX Controller */

        /* Initialize data structures for endpoints of device */
        epInfo[ AMBX_OUTPUT_PIPE ].epAddr = AMBX_ENDPOINT_OUT; // AMBX output endpoint
        epInfo[ AMBX_OUTPUT_PIPE ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
        epInfo[ AMBX_OUTPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
        epInfo[ AMBX_OUTPUT_PIPE ].maxPktSize = AMBX_EP_MAXPKTSIZE;
        epInfo[ AMBX_OUTPUT_PIPE ].bmSndToggle = 0;
        epInfo[ AMBX_OUTPUT_PIPE ].bmRcvToggle = 0;

        rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
        if(rcode)
                goto FailSetDevTblEntry;

        delay(200); //Give time for address change

        //For some reason this is need to make it work
        rcode = pUsb->setConf(bAddress, epInfo[ AMBX_CONTROL_PIPE ].epAddr, 1);
        if(rcode)
                goto FailSetConfDescr;

        if(PID == AMBX_PID || PID) {
                AMBXConnected = true;
        }
        onInit();

        Notify(PSTR("\r\n"), 0x80);
        return 0; // Successful configuration

        /* Diagnostic messages */
FailGetDevDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailGetDevDescr();
        goto Fail;
#endif

FailSetDevTblEntry:
#ifdef DEBUG_USB_HOST
        NotifyFailSetDevTblEntry();
        goto Fail;
#endif

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailSetConfDescr();
#endif
        goto Fail;

FailUnknownDevice:
#ifdef DEBUG_USB_HOST
        NotifyFailUnknownDevice(VID, PID);
#endif
        rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

Fail:
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nAMBX Init Failed, error code: "), 0x80);
        NotifyFail(rcode);
#endif
        Release();
        return rcode;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t AMBX::Release() {
        AMBXConnected = false;
        pUsb->GetAddressPool().FreeAddress(bAddress);
        bAddress = 0;
        return 0;
}

uint8_t AMBX::Poll() {
        return 0;
}

void AMBX::Light_Command(uint8_t *data, uint16_t nbytes) {
        #ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nLight command "), 0x80);
        #endif
        pUsb->outTransfer(bAddress, epInfo[ AMBX_OUTPUT_PIPE ].epAddr, nbytes, data);
}

void AMBX::setLight(uint8_t ambx_light, uint8_t r, uint8_t g, uint8_t b) {
        writeBuf[0] = AMBX_PREFIX_COMMAND;
        writeBuf[1] = ambx_light;
        writeBuf[2] = AMBX_SET_COLOR_COMMAND;
        writeBuf[3] = r;
        writeBuf[4] = g;
        writeBuf[5] = b;
        Light_Command(writeBuf, AMBX_LIGHT_COMMAND_BUFFER_SIZE);
}

void AMBX::setLight(AmbxLightsEnum ambx_light, AmbxColorsEnum color) { // Use this to set the Light with Color using the predefined in "AMBXEnums.h"
        setLight(ambx_light, (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color));
}

void AMBX::setAllLights(AmbxColorsEnum color) { // Use this to set the Color using the predefined colors in "AMBXEnums.h"
        setLight(Sidelight_left, (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color));
        setLight(Sidelight_right, (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color));
        setLight(Wallwasher_center, (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color));
        setLight(Wallwasher_left, (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color));
        setLight(Wallwasher_right, (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color));
}

void AMBX::onInit() {
        #ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nOnInit execute "), 0x80);
        #endif
        if(pFuncOnInit)
                pFuncOnInit(); // Call the user function
}
