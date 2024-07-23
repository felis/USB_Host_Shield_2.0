/* Copyright (C) 2013 Kristian Lauszus, TKJ Electronics. All rights reserved.

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

#include "XBOXOLD.h"
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the Xbox controller

/** Buttons on the controllers */
const uint8_t XBOXOLD_BUTTONS[] PROGMEM = {
        0x01, // UP
        0x08, // RIGHT
        0x02, // DOWN
        0x04, // LEFT

        0x20, // BACK
        0x10, // START
        0x40, // L3
        0x80, // R3

        // A, B, X, Y, BLACK, WHITE, L1, and R1 are analog buttons
        4, // BLACK
        5, // WHTIE
        6, // L1
        7, // R1

        1, // B
        0, // A
        2, // X
        3, // Y
};

XBOXOLD::XBOXOLD(USB *p) :
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0), // device address - mandatory
bNumEP(1), // If config descriptor needs to be parsed
qNextPollTime(0), // Reset NextPollTime
pollInterval(0),
bPollEnable(false) { // don't start polling before dongle is connected
        for(uint8_t i = 0; i < XBOX_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].bmSndToggle = 0;
                epInfo[i].bmRcvToggle = 0;
                epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
        }

        if(pUsb) // register in USB subsystem
                pUsb->RegisterDeviceClass(this); //set devConfig[] entry
}

uint8_t XBOXOLD::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
        USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
        uint8_t rcode;
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;
        uint16_t PID;
        uint16_t VID;
        uint8_t num_of_conf; // Number of configurations

        // get memory address of USB device address pool
        AddressPool &addrPool = pUsb->GetAddressPool();
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nXBOXUSB Init"), 0x80);
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

        if(!VIDPIDOK(VID, PID)) // Check if VID and PID match
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
        //delay(300); // Spec says you should wait at least 200ms

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

        /*
	   We better go and parse configuration values, as there are at least two kind of controllers that use different endpoints,
	   so using hardcoded values cause the usb host to miss all input reports from those controllers.

	   As an example:
           - 045e:0289 uses EP 1 for IN and EP 2 for OUT
           - but 045e:0202 uses both EP 2 for IN and OUT
	 */
        num_of_conf = udd->bNumConfigurations; // Number of configurations

        USBTRACE2("NC:", num_of_conf);

        // Check if attached device is a Xbox controller and fill endpoint data structure
        for(uint8_t i = 0; i < num_of_conf; i++) {
                ConfigDescParser<0, 0, 0, 0> confDescrParser(this); // Allow all devices, as we have already verified that it is a Xbox controller from the VID and PID
                rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
                if(rcode) // Check error code
                        goto FailGetConfDescr;
                if(bNumEP >= XBOX_MAX_ENDPOINTS) // All endpoints extracted
                        break;
        }

        if(bNumEP < XBOX_MAX_ENDPOINTS)
                goto FailUnknownDevice;

        rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);
        if(rcode)
                goto FailSetDevTblEntry;

        delay(200); // Give time for address change

        rcode = pUsb->setConf(bAddress, epInfo[ XBOX_CONTROL_PIPE ].epAddr, bConfNum);
        if(rcode)
                goto FailSetConfDescr;

#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nXbox Controller Connected\r\n"), 0x80);
#endif
        if(pFuncOnInit)
                pFuncOnInit(); // Call the user function
        XboxConnected = true;
        bPollEnable = true;
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

FailGetConfDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailGetConfDescr();
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
        Notify(PSTR("\r\nXbox Init Failed, error code: "), 0x80);
        NotifyFail(rcode);
#endif
        Release();
        return rcode;
}

/* Extracts endpoint information from config descriptor */
void XBOXOLD::EndpointXtract(uint8_t conf,
        uint8_t iface __attribute__((unused)),
        uint8_t alt __attribute__((unused)),
        uint8_t proto __attribute__((unused)),
        const USB_ENDPOINT_DESCRIPTOR *pep)
{
        bConfNum = conf;
        uint8_t index;

        if((pep->bmAttributes & bmUSB_TRANSFER_TYPE) == USB_TRANSFER_TYPE_INTERRUPT) { // Interrupt endpoint
                index = (pep->bEndpointAddress & 0x80) == 0x80 ? XBOX_INPUT_PIPE : XBOX_OUTPUT_PIPE; // Set the endpoint index
        } else
                return;

        // Fill the rest of endpoint data structure
        epInfo[index].epAddr = (pep->bEndpointAddress & 0x0F);
        epInfo[index].maxPktSize = (uint8_t)pep->wMaxPacketSize;
#ifdef EXTRADEBUG
        PrintEndpointDescriptor(pep);
#endif
        if(pollInterval < pep->bInterval) // Set the polling interval as the largest polling interval obtained from endpoints
                pollInterval = pep->bInterval;
        bNumEP++;
}

void XBOXOLD::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr
    __attribute__((unused)))
{
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nEndpoint descriptor:"), 0x80);
        Notify(PSTR("\r\nLength:\t\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bLength, 0x80);
        Notify(PSTR("\r\nType:\t\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bDescriptorType, 0x80);
        Notify(PSTR("\r\nAddress:\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bEndpointAddress, 0x80);
        Notify(PSTR("\r\nAttributes:\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bmAttributes, 0x80);
        Notify(PSTR("\r\nMaxPktSize:\t"), 0x80);
        D_PrintHex<uint16_t > (ep_ptr->wMaxPacketSize, 0x80);
        Notify(PSTR("\r\nPoll Intrv:\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bInterval, 0x80);
#endif
}

/* Performs a cleanup after failed Init() attempt */
uint8_t XBOXOLD::Release() {
        XboxConnected = false;
        pUsb->GetAddressPool().FreeAddress(bAddress);
        bAddress = 0;
        bNumEP = 1; // Must have to be reset to 1
        qNextPollTime = 0; // Reset next poll time
        pollInterval = 0;
        bPollEnable = false;
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nXbox Controller Disconnected\r\n"), 0x80);
#endif
        return 0;
}

uint8_t XBOXOLD::Poll() {
        uint8_t rcode = 0;

        if(!bPollEnable)
                return 0;

        if((int32_t)((uint32_t)millis() - qNextPollTime) >= 0L) { // Do not poll if shorter than polling interval
                qNextPollTime = (uint32_t)millis() + pollInterval; // Set new poll time
                uint16_t length =  (uint16_t)epInfo[ XBOX_INPUT_PIPE ].maxPktSize; // Read the maximum packet size from the endpoint
                uint8_t rcode = pUsb->inTransfer(bAddress, epInfo[ XBOX_INPUT_PIPE ].epAddr, &length, readBuf, pollInterval);
                if(!rcode) {
                        readReport();
#ifdef PRINTREPORT // Uncomment "#define PRINTREPORT" to print the report send by the Xbox ONE Controller
                        printReport(length);
#endif
                }
#ifdef DEBUG_USB_HOST
                else if(rcode != hrNAK) { // Not a matter of no update to send
                        Notify(PSTR("\r\nXbox Poll Failed, error code: "), 0x80);
                        NotifyFail(rcode);
                }
#endif
        }
        return rcode;
}

void XBOXOLD::readReport() {
        ButtonState = readBuf[2];

        for(uint8_t i = 0; i < sizeof (buttonValues); i++)
                buttonValues[i] = readBuf[i + 4]; // A, B, X, Y, BLACK, WHITE, L1, and R1

        hatValue[LeftHatX] = (int16_t)(((uint16_t)readBuf[13] << 8) | readBuf[12]);
        hatValue[LeftHatY] = (int16_t)(((uint16_t)readBuf[15] << 8) | readBuf[14]);
        hatValue[RightHatX] = (int16_t)(((uint16_t)readBuf[17] << 8) | readBuf[16]);
        hatValue[RightHatY] = (int16_t)(((uint16_t)readBuf[19] << 8) | readBuf[18]);

        //Notify(PSTR("\r\nButtonState"), 0x80);
        //PrintHex<uint8_t>(ButtonState, 0x80);

        if(ButtonState != OldButtonState || memcmp(buttonValues, oldButtonValues, sizeof (buttonValues)) != 0) {
                ButtonClickState = ButtonState & ~OldButtonState; // Update click state variable
                OldButtonState = ButtonState;

                for(uint8_t i = 0; i < sizeof (buttonValues); i++) {
                        if(oldButtonValues[i] == 0 && buttonValues[i] != 0)
                                buttonClicked[i] = true; // Update A, B, X, Y, BLACK, WHITE, L1, and R1 click state
                        oldButtonValues[i] = buttonValues[i];
                }
        }
}

void XBOXOLD::printReport(uint16_t length __attribute__((unused))) { //Uncomment "#define PRINTREPORT" to print the report send by the Xbox controller
#ifdef PRINTREPORT
        if(readBuf == NULL || !length)
                return;
        for(uint8_t i = 0; i < length; i++) {
                D_PrintHex<uint8_t > (readBuf[i], 0x80);
                Notify(PSTR(" "), 0x80);
        }
        Notify(PSTR("\r\n"), 0x80);
#endif
}

int8_t XBOXOLD::getAnalogIndex(ButtonEnum b) {
        // For legacy reasons these mapping indices not match up,
        // as the original code uses L1/R1 for the triggers and
        // L2/R2 for the white/black buttons. To fix these new enums
        // we have to transpose the keys before passing them through
        // the button index function
        switch (b) {
        case(LT): b = L1; break;  // normally L2
        case(RT): b = R1; break;  // normally R2
        case(LB): b = WHITE; break;  // normally L1
        case(RB): b = BLACK; break;  // normally R1
        default: break;
        }

        // A, B, X, Y, BLACK, WHITE, L1, and R1 are analog buttons
        const int8_t index = ButtonIndex(b);

        switch (index) {
        case ButtonIndex(A):
        case ButtonIndex(B):
        case ButtonIndex(X):
        case ButtonIndex(Y):
        case ButtonIndex(BLACK):
        case ButtonIndex(WHITE):
        case ButtonIndex(L1):
        case ButtonIndex(R1):
                return index;
        default: break;
        }

        return -1;
}

int8_t XBOXOLD::getDigitalIndex(ButtonEnum b) {
        // UP, DOWN, LEFT, RIGHT, START, BACK, L3, and R3 are digital buttons
        const int8_t index = ButtonIndex(b);

        switch (index) {
        case ButtonIndex(UP):
        case ButtonIndex(DOWN):
        case ButtonIndex(LEFT):
        case ButtonIndex(RIGHT):
        case ButtonIndex(START):
        case ButtonIndex(BACK):
        case ButtonIndex(L3):
        case ButtonIndex(R3):
                return index;
        default: break;
        }

        return -1;
}

uint8_t XBOXOLD::getButtonPress(ButtonEnum b) {
        const int8_t analogIndex = getAnalogIndex(b);
        if (analogIndex >= 0) {
                const uint8_t buttonIndex = pgm_read_byte(&XBOXOLD_BUTTONS[analogIndex]);
                return buttonValues[buttonIndex];
        }
        const int8_t digitalIndex = getDigitalIndex(b);
        if (digitalIndex >= 0) {
                const uint8_t buttonMask = pgm_read_byte(&XBOXOLD_BUTTONS[digitalIndex]);
                return (ButtonState & buttonMask);
        }
        return 0;
}

bool XBOXOLD::getButtonClick(ButtonEnum b) {
        const int8_t analogIndex = getAnalogIndex(b);
        if (analogIndex >= 0) {
                const uint8_t buttonIndex = pgm_read_byte(&XBOXOLD_BUTTONS[analogIndex]);
                if (buttonClicked[buttonIndex]) {
                        buttonClicked[buttonIndex] = false;
                        return true;
                }
                return false;
        }
        const int8_t digitalIndex = getDigitalIndex(b);
        if (digitalIndex >= 0) {
                const uint8_t mask = pgm_read_byte(&XBOXOLD_BUTTONS[digitalIndex]);
                const bool click = (ButtonClickState & mask);
                ButtonClickState &= ~mask;
                return click;
        }
        return 0;
}

int16_t XBOXOLD::getAnalogHat(AnalogHatEnum a) {
        return hatValue[a];
}

/* Xbox Controller commands */
void XBOXOLD::XboxCommand(uint8_t* data, uint16_t nbytes) {
        //bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set Report (0x09), Report ID (0x00), Report Type (Output 0x02), interface (0x00), datalength, datalength, data)
        pUsb->ctrlReq(bAddress, epInfo[XBOX_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0x00, 0x02, 0x00, nbytes, nbytes, data, NULL);
}

void XBOXOLD::setRumbleOn(uint8_t lValue, uint8_t rValue) {
        uint8_t writeBuf[6];

        writeBuf[0] = 0x00;
        writeBuf[1] = 0x06;
        writeBuf[2] = 0x00;
        writeBuf[3] = rValue; // small weight
        writeBuf[4] = 0x00;
        writeBuf[5] = lValue; // big weight

        XboxCommand(writeBuf, 6);
}
