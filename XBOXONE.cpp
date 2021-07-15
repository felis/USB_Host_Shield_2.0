/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.
   Copyright (C) 2015 guruthree

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

 guruthree
 Web      :  https://github.com/guruthree/
 */

#include "XBOXONE.h"
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the Xbox ONE Controller

XBOXONE::XBOXONE(USB *p) :
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0), // device address - mandatory
bNumEP(1), // If config descriptor needs to be parsed
qNextPollTime(0), // Reset NextPollTime
pollInterval(0),
bPollEnable(false) { // don't start polling before dongle is connected
        for(uint8_t i = 0; i < XBOX_ONE_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].bmSndToggle = 0;
                epInfo[i].bmRcvToggle = 0;
                epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
        }

        if(pUsb) // register in USB subsystem
                pUsb->RegisterDeviceClass(this); //set devConfig[] entry
}

uint8_t XBOXONE::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
        USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
        uint8_t rcode;
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;
        uint16_t PID, VID;
        uint8_t num_of_conf; // Number of configurations

        // get memory address of USB device address pool
        AddressPool &addrPool = pUsb->GetAddressPool();
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nXBOXONE Init"), 0x80);
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

        if(!VIDPIDOK(VID, PID)) // Check VID
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

        num_of_conf = udd->bNumConfigurations; // Number of configurations

        USBTRACE2("NC:", num_of_conf);

        // Check if attached device is a Xbox One controller and fill endpoint data structure
        for(uint8_t i = 0; i < num_of_conf; i++) {
                ConfigDescParser<0, 0, 0, 0> confDescrParser(this); // Allow all devices, as we have already verified that it is a Xbox One controller from the VID and PID
                rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
                if(rcode) // Check error code
                        goto FailGetConfDescr;
                if(bNumEP >= XBOX_ONE_MAX_ENDPOINTS) // All endpoints extracted
                        break;
        }

        if(bNumEP < XBOX_ONE_MAX_ENDPOINTS)
                goto FailUnknownDevice;

        rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);
        if(rcode)
                goto FailSetDevTblEntry;

        delay(200); // Give time for address change

        rcode = pUsb->setConf(bAddress, epInfo[ XBOX_ONE_CONTROL_PIPE ].epAddr, bConfNum);
        if(rcode)
                goto FailSetConfDescr;

#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nXbox One Controller Connected\r\n"), 0x80);
#endif

        delay(200); // let things settle

        // Initialize the controller for input
        cmdCounter = 0; // Reset the counter used when sending out the commands
        uint8_t writeBuf[5];
        writeBuf[0] = 0x05;
        writeBuf[1] = 0x20;
        // Byte 2 is set in "XboxCommand"
        writeBuf[3] = 0x01;
        writeBuf[4] = 0x00;
        rcode = XboxCommand(writeBuf, 5);
        if (rcode)
                goto Fail;

        onInit();
        XboxOneConnected = true;
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
        Notify(PSTR("\r\nXbox One Init Failed, error code: "), 0x80);
        NotifyFail(rcode);
#endif
        Release();
        return rcode;
}

/* Extracts endpoint information from config descriptor */
void XBOXONE::EndpointXtract(uint8_t conf,
        uint8_t iface __attribute__((unused)),
        uint8_t alt __attribute__((unused)),
        uint8_t proto __attribute__((unused)),
        const USB_ENDPOINT_DESCRIPTOR *pep)
{
        
    bConfNum = conf;
        uint8_t index;

        if((pep->bmAttributes & bmUSB_TRANSFER_TYPE) == USB_TRANSFER_TYPE_INTERRUPT) { // Interrupt endpoint
                index = (pep->bEndpointAddress & 0x80) == 0x80 ? XBOX_ONE_INPUT_PIPE : XBOX_ONE_OUTPUT_PIPE; // Set the endpoint index
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

void XBOXONE::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr
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
uint8_t XBOXONE::Release() {
        XboxOneConnected = false;
        pUsb->GetAddressPool().FreeAddress(bAddress);
        bAddress = 0; // Clear device address
        bNumEP = 1; // Must have to be reset to 1
        qNextPollTime = 0; // Reset next poll time
        pollInterval = 0;
        bPollEnable = false;
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nXbox One Controller Disconnected\r\n"), 0x80);
#endif
        return 0;
}

uint8_t XBOXONE::Poll() {
        uint8_t rcode = 0;

        if(!bPollEnable)
                return 0;

        if((int32_t)((uint32_t)millis() - qNextPollTime) >= 0L) { // Do not poll if shorter than polling interval
                qNextPollTime = (uint32_t)millis() + pollInterval; // Set new poll time
                uint16_t length =  (uint16_t)epInfo[ XBOX_ONE_INPUT_PIPE ].maxPktSize; // Read the maximum packet size from the endpoint
                uint8_t rcode = pUsb->inTransfer(bAddress, epInfo[ XBOX_ONE_INPUT_PIPE ].epAddr, &length, readBuf, pollInterval);
                if(!rcode) {
                        readReport();
#ifdef PRINTREPORT // Uncomment "#define PRINTREPORT" to print the report send by the Xbox ONE Controller
                        for(uint8_t i = 0; i < length; i++) {
                                D_PrintHex<uint8_t > (readBuf[i], 0x80);
                                Notify(PSTR(" "), 0x80);
                        }
                        Notify(PSTR("\r\n"), 0x80);
#endif
                }
#ifdef DEBUG_USB_HOST
                else if(rcode != hrNAK) { // Not a matter of no update to send
                        Notify(PSTR("\r\nXbox One Poll Failed, error code: "), 0x80);
                        NotifyFail(rcode);
                }
#endif
    }
    return rcode;
}

void XBOXONE::readReport() {
        if(readBuf[0] == 0x07) {
                // The XBOX button has a separate message
                if(readBuf[4] == 1)
                        ButtonState |= pgm_read_word(&XBOX_BUTTONS[ButtonIndex(XBOX)]);
                else
                        ButtonState &= ~pgm_read_word(&XBOX_BUTTONS[ButtonIndex(XBOX)]);

                if(ButtonState != OldButtonState) {
                    ButtonClickState = ButtonState & ~OldButtonState; // Update click state variable
                    OldButtonState = ButtonState;
                }
        }
        if(readBuf[0] != 0x20) { // Check if it's the correct report, otherwise return - the controller also sends different status reports
#ifdef EXTRADEBUG
                Notify(PSTR("\r\nXbox Poll: "), 0x80);
                D_PrintHex<uint8_t > (readBuf[0], 0x80); // 0x03 is a heart beat report!
#endif
                return;
        }

        uint16_t xbox = ButtonState & pgm_read_word(&XBOX_BUTTONS[ButtonIndex(XBOX)]); // Since the XBOX button is separate, save it and add it back in
        // xbox button from before, dpad, abxy, start/back, sync, stick click, shoulder buttons
        ButtonState = xbox | (((uint16_t)readBuf[5] & 0xF) << 8) | (readBuf[4] & 0xF0)  | (((uint16_t)readBuf[4] & 0x0C) << 10) | ((readBuf[4] & 0x01) << 3) | (((uint16_t)readBuf[5] & 0xC0) << 8) | ((readBuf[5] & 0x30) >> 4);

        triggerValue[0] = (uint16_t)(((uint16_t)readBuf[7] << 8) | readBuf[6]);
        triggerValue[1] = (uint16_t)(((uint16_t)readBuf[9] << 8) | readBuf[8]);

        hatValue[LeftHatX] = (int16_t)(((uint16_t)readBuf[11] << 8) | readBuf[10]);
        hatValue[LeftHatY] = (int16_t)(((uint16_t)readBuf[13] << 8) | readBuf[12]);
        hatValue[RightHatX] = (int16_t)(((uint16_t)readBuf[15] << 8) | readBuf[14]);
        hatValue[RightHatY] = (int16_t)(((uint16_t)readBuf[17] << 8) | readBuf[16]);

        // Read and store share button separately
        const bool newShare = (readBuf[22] & 0x01) ? 1 : 0;
        shareClicked = ((sharePressed != newShare) && newShare) ? 1 : 0;
        sharePressed = newShare;

        //Notify(PSTR("\r\nButtonState"), 0x80);
        //PrintHex<uint16_t>(ButtonState, 0x80);

        if(ButtonState != OldButtonState) {
                ButtonClickState = ButtonState & ~OldButtonState; // Update click state variable
                OldButtonState = ButtonState;
        }

        // Handle click detection for triggers
        if(triggerValue[0] != 0 && triggerValueOld[0] == 0)
                L2Clicked = true;
        triggerValueOld[0] = triggerValue[0];
        if(triggerValue[1] != 0 && triggerValueOld[1] == 0)
                R2Clicked = true;
        triggerValueOld[1] = triggerValue[1];
}

uint16_t XBOXONE::getButtonPress(ButtonEnum b) {
        // special handling for 'SHARE' button due to index collision with 'BACK',
        // since the 'SHARE' value originally came from the PS4 controller and
        // the 'SHARE' button was added to Xbox later with the Series S/X controllers
        if (b == SHARE) return sharePressed;

        const int8_t index = getButtonIndexXbox(b); if (index < 0) return 0;
        if(index == ButtonIndex(L2)) // These are analog buttons
                return triggerValue[0];
        else if(index == ButtonIndex(R2))
                return triggerValue[1];
        return (bool)(ButtonState & ((uint16_t)pgm_read_word(&XBOX_BUTTONS[index])));
}

bool XBOXONE::getButtonClick(ButtonEnum b) {
        // special handling for 'SHARE' button, ibid the above
        if (b == SHARE) {
                if (shareClicked) {
                        shareClicked = false;
                        return true;
                }
                return false;
        }

        const int8_t index = getButtonIndexXbox(b); if (index < 0) return 0;
        if(index == ButtonIndex(L2)) {
                if(L2Clicked) {
                        L2Clicked = false;
                        return true;
                }
                return false;
        } else if(index == ButtonIndex(R2)) {
                if(R2Clicked) {
                        R2Clicked = false;
                        return true;
                }
                return false;
        }
        uint16_t button = pgm_read_word(&XBOX_BUTTONS[index]);
        bool click = (ButtonClickState & button);
        ButtonClickState &= ~button; // Clear "click" event
        return click;
}

int16_t XBOXONE::getAnalogHat(AnalogHatEnum a) {
        return hatValue[a];
}

/* Xbox Controller commands */
uint8_t XBOXONE::XboxCommand(uint8_t* data, uint16_t nbytes) {
        data[2] = cmdCounter++; // Increment the output command counter
        uint8_t rcode = pUsb->outTransfer(bAddress, epInfo[ XBOX_ONE_OUTPUT_PIPE ].epAddr, nbytes, data);
#ifdef DEBUG_USB_HOST
        if(rcode) {
                Notify(PSTR("\r\nXboxCommand failed. Return: "), 0x80);
                D_PrintHex<uint8_t > (rcode, 0x80);
        }
#endif
        return rcode;
}

// The Xbox One packets are described at: https://github.com/quantus/xbox-one-controller-protocol
void XBOXONE::onInit() {
        // A short buzz to show the controller is active
        uint8_t writeBuf[13];

        // Activate rumble
        writeBuf[0] = 0x09;
        writeBuf[1] = 0x00;
        // Byte 2 is set in "XboxCommand"

        // Single rumble effect
        writeBuf[3] = 0x09; // Substructure (what substructure rest of this packet has)
        writeBuf[4] = 0x00; // Mode
        writeBuf[5] = 0x0F; // Rumble mask (what motors are activated) (0000 lT rT L R)
        writeBuf[6] = 0x04; // lT force
        writeBuf[7] = 0x04; // rT force
        writeBuf[8] = 0x20; // L force
        writeBuf[9] = 0x20; // R force
        writeBuf[10] = 0x80; // Length of pulse
        writeBuf[11] = 0x00; // Off period
        writeBuf[12] = 0x00; // Repeat count
        XboxCommand(writeBuf, 13);

        if(pFuncOnInit)
                pFuncOnInit(); // Call the user function
}

void XBOXONE::setRumbleOff() {
        uint8_t writeBuf[13];

        // Activate rumble
        writeBuf[0] = 0x09;
        writeBuf[1] = 0x00;
        // Byte 2 is set in "XboxCommand"

        // Continuous rumble effect
        writeBuf[3] = 0x09; // Substructure (what substructure rest of this packet has)
        writeBuf[4] = 0x00; // Mode
        writeBuf[5] = 0x0F; // Rumble mask (what motors are activated) (0000 lT rT L R)
        writeBuf[6] = 0x00; // lT force
        writeBuf[7] = 0x00; // rT force
        writeBuf[8] = 0x00; // L force
        writeBuf[9] = 0x00; // R force
        writeBuf[10] = 0x00; // On period
        writeBuf[11] = 0x00; // Off period
        writeBuf[12] = 0x00; // Repeat count
        XboxCommand(writeBuf, 13);
}

void XBOXONE::setRumbleOn(uint8_t leftTrigger, uint8_t rightTrigger, uint8_t leftMotor, uint8_t rightMotor) {
        uint8_t writeBuf[13];

        // Activate rumble
        writeBuf[0] = 0x09;
        writeBuf[1] = 0x00;
        // Byte 2 is set in "XboxCommand"

        // Continuous rumble effect
        writeBuf[3] = 0x09; // Substructure (what substructure rest of this packet has)
        writeBuf[4] = 0x00; // Mode
        writeBuf[5] = 0x0F; // Rumble mask (what motors are activated) (0000 lT rT L R)
        writeBuf[6] = leftTrigger; // lT force
        writeBuf[7] = rightTrigger; // rT force
        writeBuf[8] = leftMotor; // L force
        writeBuf[9] = rightMotor; // R force
        writeBuf[10] = 0xFF; // On period
        writeBuf[11] = 0x00; // Off period
        writeBuf[12] = 0xFF; // Repeat count
        XboxCommand(writeBuf, 13);
}
