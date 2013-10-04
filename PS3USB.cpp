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

#include "PS3USB.h"
// To enable serial debugging uncomment "#define DEBUG_USB_HOST" in message.h
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the PS3 Controllers

PS3USB::PS3USB(USB *p, uint8_t btadr5, uint8_t btadr4, uint8_t btadr3, uint8_t btadr2, uint8_t btadr1, uint8_t btadr0) :
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0), // device address - mandatory
bPollEnable(false) // don't start polling before dongle is connected
{
        for (uint8_t i = 0; i < PS3_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].epAttribs = 0;
                epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
        }

        if (pUsb) // register in USB subsystem
                pUsb->RegisterDeviceClass(this); //set devConfig[] entry

        my_bdaddr[5] = btadr5; // Change to your dongle's Bluetooth address instead
        my_bdaddr[4] = btadr4;
        my_bdaddr[3] = btadr3;
        my_bdaddr[2] = btadr2;
        my_bdaddr[1] = btadr1;
        my_bdaddr[0] = btadr0;
}

uint8_t PS3USB::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
        uint8_t rcode;
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;
        uint16_t PID;
        uint16_t VID;

        // get memory address of USB device address pool
        AddressPool &addrPool = pUsb->GetAddressPool();
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nPS3USB Init"), 0x80);
#endif
        // check if address has already been assigned to an instance
        if (bAddress) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nAddress in use"), 0x80);
#endif
                return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
        }

        // Get pointer to pseudo device with address 0 assigned
        p = addrPool.GetUsbDevicePtr(0);

        if (!p) {
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nAddress not found"), 0x80);
#endif
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
        }

        if (!p->epinfo) {
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

        if (rcode)
                goto FailGetDevDescr;

        VID = ((USB_DEVICE_DESCRIPTOR*)buf)->idVendor;
        PID = ((USB_DEVICE_DESCRIPTOR*)buf)->idProduct;

        if (VID != PS3_VID || (PID != PS3_PID && PID != PS3NAVIGATION_PID && PID != PS3MOVE_PID))
                goto FailUnknownDevice;

        // Allocate new address according to device class
        bAddress = addrPool.AllocAddress(parent, false, port);

        if (!bAddress)
                return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

        // Extract Max Packet Size from device descriptor
        epInfo[0].maxPktSize = (uint8_t)((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0;

        // Assign new address to the device
        rcode = pUsb->setAddr(0, 0, bAddress);
        if (rcode) {
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
        delay(300); // Spec says you should wait at least 200ms
        
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
           memory space. After verifying the PID and VID we will use known values for the
           configuration values for device, interface, endpoints and HID for the PS3 Controllers */

        /* Initialize data structures for endpoints of device */
        epInfo[ PS3_OUTPUT_PIPE ].epAddr = 0x02; // PS3 output endpoint
        epInfo[ PS3_OUTPUT_PIPE ].epAttribs = EP_INTERRUPT;
        epInfo[ PS3_OUTPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
        epInfo[ PS3_OUTPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
        epInfo[ PS3_OUTPUT_PIPE ].bmSndToggle = bmSNDTOG0;
        epInfo[ PS3_OUTPUT_PIPE ].bmRcvToggle = bmRCVTOG0;
        epInfo[ PS3_INPUT_PIPE ].epAddr = 0x01; // PS3 report endpoint
        epInfo[ PS3_INPUT_PIPE ].epAttribs = EP_INTERRUPT;
        epInfo[ PS3_INPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
        epInfo[ PS3_INPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
        epInfo[ PS3_INPUT_PIPE ].bmSndToggle = bmSNDTOG0;
        epInfo[ PS3_INPUT_PIPE ].bmRcvToggle = bmRCVTOG0;

        rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
        if (rcode)
                goto FailSetDevTblEntry;

        delay(200); //Give time for address change

        rcode = pUsb->setConf(bAddress, epInfo[ PS3_CONTROL_PIPE ].epAddr, 1);
        if (rcode)
                goto FailSetConfDescr;

        if (PID == PS3_PID || PID == PS3NAVIGATION_PID) {
                if (PID == PS3_PID) {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nDualshock 3 Controller Connected"), 0x80);
#endif
                        PS3Connected = true;
                } else { // must be a navigation controller
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nNavigation Controller Connected"), 0x80);
#endif
                        PS3NavigationConnected = true;
                }
                enable_sixaxis(); // The PS3 controller needs a special command before it starts sending data

                // Needed for PS3 Dualshock and Navigation commands to work
                for (uint8_t i = 0; i < PS3_REPORT_BUFFER_SIZE; i++)
                        writeBuf[i] = pgm_read_byte(&PS3_REPORT_BUFFER[i]);

                for (uint8_t i = 6; i < 10; i++)
                        readBuf[i] = 0x7F; // Set the analog joystick values to center position
        } else { // must be a Motion controller
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nMotion Controller Connected"), 0x80);
#endif
                PS3MoveConnected = true;
                writeBuf[0] = 0x02; // Set report ID, this is needed for Move commands to work
        }
        if (my_bdaddr[0] != 0x00 || my_bdaddr[1] != 0x00 || my_bdaddr[2] != 0x00 || my_bdaddr[3] != 0x00 || my_bdaddr[4] != 0x00 || my_bdaddr[5] != 0x00) {
                if (PS3MoveConnected)
                        setMoveBdaddr(my_bdaddr); // Set internal Bluetooth address
                else
                        setBdaddr(my_bdaddr); // Set internal Bluetooth address

#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nBluetooth Address was set to: "), 0x80);
                for (int8_t i = 5; i > 0; i--) {
                        D_PrintHex<uint8_t > (my_bdaddr[i], 0x80);
                        Notify(PSTR(":"), 0x80);
                }
                D_PrintHex<uint8_t > (my_bdaddr[0], 0x80);
#endif
        }
        onInit();

        bPollEnable = true;
        Notify(PSTR("\r\n"), 0x80);
        timer = millis();
        return 0; // successful configuration

        /* diagnostic messages */
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
        Notify(PSTR("\r\nPS3 Init Failed, error code: "), 0x80);
        NotifyFail(rcode);
#endif
        Release();
        return rcode;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t PS3USB::Release() {
        PS3Connected = false;
        PS3MoveConnected = false;
        PS3NavigationConnected = false;
        pUsb->GetAddressPool().FreeAddress(bAddress);
        bAddress = 0;
        bPollEnable = false;
        return 0;
}

uint8_t PS3USB::Poll() {
        if (!bPollEnable)
                return 0;

        if (PS3Connected || PS3NavigationConnected) {
                uint16_t BUFFER_SIZE = EP_MAXPKTSIZE;
                pUsb->inTransfer(bAddress, epInfo[ PS3_INPUT_PIPE ].epAddr, &BUFFER_SIZE, readBuf); // input on endpoint 1
                if (millis() - timer > 100) { // Loop 100ms before processing data
                        readReport();
#ifdef PRINTREPORT
                        printReport(); // Uncomment "#define PRINTREPORT" to print the report send by the PS3 Controllers
#endif
                }
        } else if (PS3MoveConnected) { // One can only set the color of the bulb, set the rumble, set and get the bluetooth address and calibrate the magnetometer via USB
                if (millis() - timer > 4000) // Send at least every 4th second
                {
                        Move_Command(writeBuf, MOVE_REPORT_BUFFER_SIZE); // The Bulb and rumble values, has to be written again and again, for it to stay turned on
                        timer = millis();
                }
        }
        return 0;
}

void PS3USB::readReport() {
        if (readBuf == NULL)
                return;

        ButtonState = (uint32_t)(readBuf[2] | ((uint16_t)readBuf[3] << 8) | ((uint32_t)readBuf[4] << 16));

        //Notify(PSTR("\r\nButtonState", 0x80);
        //PrintHex<uint32_t>(ButtonState, 0x80);

        if (ButtonState != OldButtonState) {
                ButtonClickState = ButtonState & ~OldButtonState; // Update click state variable
                OldButtonState = ButtonState;
        }
}

void PS3USB::printReport() { //Uncomment "#define PRINTREPORT" to print the report send by the PS3 Controllers
#ifdef PRINTREPORT
        if (readBuf == NULL)
                return;
        for (uint8_t i = 0; i < PS3_REPORT_BUFFER_SIZE; i++) {
                D_PrintHex<uint8_t > (readBuf[i], 0x80);
                Notify(PSTR(" "), 0x80);
        }
        Notify(PSTR("\r\n"), 0x80);
#endif
}

bool PS3USB::getButtonPress(Button b) {
        return (ButtonState & pgm_read_dword(&BUTTONS[(uint8_t)b]));
}

bool PS3USB::getButtonClick(Button b) {
        uint32_t button = pgm_read_dword(&BUTTONS[(uint8_t)b]);
        bool click = (ButtonClickState & button);
        ButtonClickState &= ~button; // clear "click" event
        return click;
}

uint8_t PS3USB::getAnalogButton(Button a) {
        if (readBuf == NULL)
                return 0;
        return (uint8_t)(readBuf[(pgm_read_byte(&ANALOGBUTTONS[(uint8_t)a])) - 9]);
}

uint8_t PS3USB::getAnalogHat(AnalogHat a) {
        if (readBuf == NULL)
                return 0;
        return (uint8_t)(readBuf[((uint8_t)a + 6)]);
}

uint16_t PS3USB::getSensor(Sensor a) {
        if (readBuf == NULL)
                return 0;
        return ((readBuf[((uint16_t)a) - 9] << 8) | readBuf[((uint16_t)a + 1) - 9]);
}

double PS3USB::getAngle(Angle a) {
        if (PS3Connected) {
                double accXval;
                double accYval;
                double accZval;

                // Data for the Kionix KXPC4 used in the DualShock 3
                const double zeroG = 511.5; // 1.65/3.3*1023 (1,65V)
                accXval = -((double)getSensor(aX) - zeroG);
                accYval = -((double)getSensor(aY) - zeroG);
                accZval = -((double)getSensor(aZ) - zeroG);

                // Convert to 360 degrees resolution
                // atan2 outputs the value of -π to π (radians)
                // We are then converting it to 0 to 2π and then to degrees
                if (a == Pitch) {
                        double angle = (atan2(accYval, accZval) + PI) * RAD_TO_DEG;
                        return angle;
                } else {
                        double angle = (atan2(accXval, accZval) + PI) * RAD_TO_DEG;
                        return angle;
                }
        } else
                return 0;
}

bool PS3USB::getStatus(Status c) {
        if (readBuf == NULL)
                return false;
        if (readBuf[((uint16_t)c >> 8) - 9] == ((uint8_t)c & 0xff))
                return true;
        return false;
}

String PS3USB::getStatusString() {
        if (PS3Connected || PS3NavigationConnected) {
                char statusOutput[100];

                strcpy(statusOutput, "ConnectionStatus: ");

                if (getStatus(Plugged)) strcat(statusOutput, "Plugged");
                else if (getStatus(Unplugged)) strcat(statusOutput, "Unplugged");
                else strcat(statusOutput, "Error");


                strcat(statusOutput, " - PowerRating: ");

                if (getStatus(Charging)) strcat(statusOutput, "Charging");
                else if (getStatus(NotCharging)) strcat(statusOutput, "Not Charging");
                else if (getStatus(Shutdown)) strcat(statusOutput, "Shutdown");
                else if (getStatus(Dying)) strcat(statusOutput, "Dying");
                else if (getStatus(Low)) strcat(statusOutput, "Low");
                else if (getStatus(High)) strcat(statusOutput, "High");
                else if (getStatus(Full)) strcat(statusOutput, "Full");
                else strcat(statusOutput, "Error");

                strcat(statusOutput, " - WirelessStatus: ");

                if (getStatus(CableRumble)) strcat(statusOutput, "Cable - Rumble is on");
                else if (getStatus(Cable)) strcat(statusOutput, "Cable - Rumble is off");
                else if (getStatus(BluetoothRumble)) strcat(statusOutput, "Bluetooth - Rumble is on");
                else if (getStatus(Bluetooth)) strcat(statusOutput, "Bluetooth - Rumble is off");
                else strcat(statusOutput, "Error");

                return statusOutput;
        } else
                return "Error";
}

/* Playstation Sixaxis Dualshock and Navigation Controller commands */
void PS3USB::PS3_Command(uint8_t* data, uint16_t nbytes) {
        //bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set Report (0x09), Report ID (0x01), Report Type (Output 0x02), interface (0x00), datalength, datalength, data)
        pUsb->ctrlReq(bAddress, epInfo[PS3_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0x01, 0x02, 0x00, nbytes, nbytes, data, NULL);
}

void PS3USB::setAllOff() {
        for (uint8_t i = 0; i < PS3_REPORT_BUFFER_SIZE; i++)
                writeBuf[i] = pgm_read_byte(&PS3_REPORT_BUFFER[i]); // Reset buffer

        PS3_Command(writeBuf, PS3_REPORT_BUFFER_SIZE);
}

void PS3USB::setRumbleOff() {
        writeBuf[1] = 0x00;
        writeBuf[2] = 0x00; //low mode off
        writeBuf[3] = 0x00;
        writeBuf[4] = 0x00; //high mode off

        PS3_Command(writeBuf, PS3_REPORT_BUFFER_SIZE);
}

void PS3USB::setRumbleOn(Rumble mode) {
        if ((mode & 0x30) > 0x00) {
                uint8_t power[2] = {0xff, 0x00}; // Defaults to RumbleLow
                if (mode == RumbleHigh) {
                        power[0] = 0x00;
                        power[1] = 0xff;
                }
                setRumbleOn(0xfe, power[0], 0xfe, power[1]);
        }
}

void PS3USB::setRumbleOn(uint8_t rightDuration, uint8_t rightPower, uint8_t leftDuration, uint8_t leftPower) {
        writeBuf[1] = rightDuration;
        writeBuf[2] = rightPower;
        writeBuf[3] = leftDuration;
        writeBuf[4] = leftPower;
        PS3_Command(writeBuf, PS3_REPORT_BUFFER_SIZE);
}

void PS3USB::setLedRaw(uint8_t value) {
        writeBuf[9] = value << 1;
        PS3_Command(writeBuf, PS3_REPORT_BUFFER_SIZE);
}

void PS3USB::setLedOff(LED a) {
        writeBuf[9] &= ~((uint8_t)((pgm_read_byte(&LEDS[(uint8_t)a]) & 0x0f) << 1));
        PS3_Command(writeBuf, PS3_REPORT_BUFFER_SIZE);
}

void PS3USB::setLedOn(LED a) {
        writeBuf[9] |= (uint8_t)((pgm_read_byte(&LEDS[(uint8_t)a]) & 0x0f) << 1);
        PS3_Command(writeBuf, PS3_REPORT_BUFFER_SIZE);
}

void PS3USB::setLedToggle(LED a) {
        writeBuf[9] ^= (uint8_t)((pgm_read_byte(&LEDS[(uint8_t)a]) & 0x0f) << 1);
        PS3_Command(writeBuf, PS3_REPORT_BUFFER_SIZE);
}

void PS3USB::setBdaddr(uint8_t* BDADDR) {
        /* Set the internal bluetooth address */
        uint8_t buf[8];
        buf[0] = 0x01;
        buf[1] = 0x00;
        for (uint8_t i = 0; i < 6; i++)
                buf[i + 2] = BDADDR[5 - i]; //Copy into buffer, has to be written reversed

        //bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set Report (0x09), Report ID (0xF5), Report Type (Feature 0x03), interface (0x00), datalength, datalength, data)
        pUsb->ctrlReq(bAddress, epInfo[PS3_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0xF5, 0x03, 0x00, 8, 8, buf, NULL);
}

void PS3USB::enable_sixaxis() { //Command used to enable the Dualshock 3 and Navigation controller to send data via USB
        uint8_t cmd_buf[4];
        cmd_buf[0] = 0x42; // Special PS3 Controller enable commands
        cmd_buf[1] = 0x0c;
        cmd_buf[2] = 0x00;
        cmd_buf[3] = 0x00;

        //bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set Report (0x09), Report ID (0xF4), Report Type (Feature 0x03), interface (0x00), datalength, datalength, data)
        pUsb->ctrlReq(bAddress, epInfo[PS3_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0xF4, 0x03, 0x00, 4, 4, cmd_buf, NULL);
}

/* Playstation Move Controller commands */
void PS3USB::Move_Command(uint8_t* data, uint16_t nbytes) {
        pUsb->outTransfer(bAddress, epInfo[ PS3_OUTPUT_PIPE ].epAddr, nbytes, data);
}

void PS3USB::moveSetBulb(uint8_t r, uint8_t g, uint8_t b) { //Use this to set the Color using RGB values
        // set the Bulb's values into the write buffer
        writeBuf[2] = r;
        writeBuf[3] = g;
        writeBuf[4] = b;

        Move_Command(writeBuf, MOVE_REPORT_BUFFER_SIZE);
}

void PS3USB::moveSetBulb(Colors color) { //Use this to set the Color using the predefined colors in "enums.h"
        moveSetBulb((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color));
}

void PS3USB::moveSetRumble(uint8_t rumble) {
#ifdef DEBUG_USB_HOST
        if (rumble < 64 && rumble != 0) // The rumble value has to at least 64, or approximately 25% (64/255*100)
                Notify(PSTR("\r\nThe rumble value has to at least 64, or approximately 25%"), 0x80);
#endif
        //set the rumble value into the write buffer
        writeBuf[6] = rumble;

        Move_Command(writeBuf, MOVE_REPORT_BUFFER_SIZE);
}

void PS3USB::setMoveBdaddr(uint8_t* BDADDR) {
        /* Set the internal bluetooth address */
        uint8_t buf[11];
        buf[0] = 0x05;
        buf[7] = 0x10;
        buf[8] = 0x01;
        buf[9] = 0x02;
        buf[10] = 0x12;

        for (uint8_t i = 0; i < 6; i++)
                buf[i + 1] = BDADDR[i];

        //bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set Report (0x09), Report ID (0x05), Report Type (Feature 0x03), interface (0x00), datalength, datalength, data)
        pUsb->ctrlReq(bAddress, epInfo[PS3_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0x05, 0x03, 0x00, 11, 11, buf, NULL);
}

void PS3USB::onInit() {
        if (pFuncOnInit)
                pFuncOnInit(); // Call the user function
        else {
                if (PS3MoveConnected)
                        moveSetBulb(Red);
                else // Dualshock 3 or Navigation controller
                        setLedOn(LED1);
        }
}
