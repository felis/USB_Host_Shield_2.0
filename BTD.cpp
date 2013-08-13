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

#include "BTD.h"
// To enable serial debugging uncomment "#define DEBUG_USB_HOST" in message.h
//#define EXTRADEBUG // Uncomment to get even more debugging data

const uint8_t BTD::BTD_CONTROL_PIPE = 0;
const uint8_t BTD::BTD_EVENT_PIPE = 1;
const uint8_t BTD::BTD_DATAIN_PIPE = 2;
const uint8_t BTD::BTD_DATAOUT_PIPE = 3;

BTD::BTD(USB *p) :
pUsb(p), // Pointer to USB class instance - mandatory
bAddress(0), // Device address - mandatory
bNumEP(1), // If config descriptor needs to be parsed
qNextPollTime(0), // Reset NextPollTime
bPollEnable(false) // Don't start polling before dongle is connected
{
        for (uint8_t i = 0; i < BTD_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].epAttribs = 0;
                epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
        }

        if (pUsb) // register in USB subsystem
                pUsb->RegisterDeviceClass(this); //set devConfig[] entry
}

uint8_t BTD::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
        uint8_t rcode;
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;
        uint8_t num_of_conf; // number of configurations
        uint16_t PID;
        uint16_t VID;

        // get memory address of USB device address pool
        AddressPool &addrPool = pUsb->GetAddressPool();
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nBTD Init"), 0x80);
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
        VID = ((USB_DEVICE_DESCRIPTOR*)buf)->idVendor;
        PID = ((USB_DEVICE_DESCRIPTOR*)buf)->idProduct;

        if (VID == PS3_VID && (PID == PS3_PID || PID == PS3NAVIGATION_PID || PID == PS3MOVE_PID)) {
                /* We only need the Control endpoint, so we don't have to initialize the other endpoints of device */
                rcode = pUsb->setConf(bAddress, epInfo[ BTD_CONTROL_PIPE ].epAddr, 1);
                if (rcode)
                        goto FailSetConfDescr;

#ifdef DEBUG_USB_HOST
                if (PID == PS3_PID || PID == PS3NAVIGATION_PID) {
                        if (PID == PS3_PID)
                                Notify(PSTR("\r\nDualshock 3 Controller Connected"), 0x80);
                        else // It must be a navigation controller
                                Notify(PSTR("\r\nNavigation Controller Connected"), 0x80);
                } else // It must be a Motion controller
                        Notify(PSTR("\r\nMotion Controller Connected"), 0x80);
#endif

                if (my_bdaddr[0] == 0x00 && my_bdaddr[1] == 0x00 && my_bdaddr[2] == 0x00 && my_bdaddr[3] == 0x00 && my_bdaddr[4] == 0x00 && my_bdaddr[5] == 0x00) {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nPlease plug in the dongle before trying to pair with the PS3 Controller\n\ror set the Bluetooth address in the constructor of the PS3BT class"), 0x80);
#endif
                } else {
                        if (PID == PS3_PID || PID == PS3NAVIGATION_PID)
                                setBdaddr(my_bdaddr); // Set internal Bluetooth address
                        else
                                setMoveBdaddr(my_bdaddr); // Set internal Bluetooth address
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nBluetooth Address was set to: "), 0x80);
                        for (int8_t i = 5; i > 0; i--) {
                                D_PrintHex<uint8_t > (my_bdaddr[i], 0x80);
                                Notify(PSTR(":"), 0x80);
                        }
                        D_PrintHex<uint8_t > (my_bdaddr[0], 0x80);
#endif
                }

                rcode = pUsb->setConf(bAddress, epInfo[ BTD_CONTROL_PIPE ].epAddr, 0); // Reset configuration value
                pUsb->setAddr(bAddress, 0, 0); // Reset address
                Release(); // Release device
                return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED; // Return
        } else {
                num_of_conf = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;

                // Check if attached device is a Bluetooth dongle and fill endpoint data structure
                // First interface in the configuration must have Bluetooth assigned Class/Subclass/Protocol
                // And 3 endpoints - interrupt-IN, bulk-IN, bulk-OUT, not necessarily in this order
                for (uint8_t i = 0; i < num_of_conf; i++) {
                        if (VID == IOGEAR_GBU521_VID && PID == IOGEAR_GBU521_PID) {
                                ConfigDescParser<USB_CLASS_VENDOR_SPECIFIC, WI_SUBCLASS_RF, WI_PROTOCOL_BT, CP_MASK_COMPARE_ALL> confDescrParser(this); // Needed for the IOGEAR GBU521
                                rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
                        } else {
                                ConfigDescParser<USB_CLASS_WIRELESS_CTRL, WI_SUBCLASS_RF, WI_PROTOCOL_BT, CP_MASK_COMPARE_ALL> confDescrParser(this);
                                rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
                        }
                        if (rcode) // Check error code
                                goto FailGetConfDescr;
                        if (bNumEP >= BTD_MAX_ENDPOINTS) // All endpoints extracted
                                break;
                }

                if (bNumEP < BTD_MAX_ENDPOINTS)
                        goto FailUnknownDevice;

                // Assign epInfo to epinfo pointer - this time all 3 endpoins
                rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);
                if (rcode)
                        goto FailSetDevTblEntry;

                delay(200); // Give time for address change

                // Set Configuration Value
                rcode = pUsb->setConf(bAddress, epInfo[ BTD_CONTROL_PIPE ].epAddr, bConfNum);
                if (rcode)
                        goto FailSetConfDescr;

                hci_num_reset_loops = 100; // only loop 100 times before trying to send the hci reset command
                hci_counter = 0;
                hci_state = HCI_INIT_STATE;
                watingForConnection = false;
                bPollEnable = true;

#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nBluetooth Dongle Initialized"), 0x80);
#endif
        }
        return 0; // Successful configuration

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
        pUsb->setAddr(bAddress, 0, 0); // Reset address
        rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
Fail:
#ifdef DEBUG_USB_HOST
        Notify(PSTR("\r\nBTD Init Failed, error code: "), 0x80);
        NotifyFail(rcode);
#endif
        Release();
        return rcode;
}

/* Extracts interrupt-IN, bulk-IN, bulk-OUT endpoint information from config descriptor */
void BTD::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) {
        //ErrorMessage<uint8_t>(PSTR("Conf.Val"),conf);
        //ErrorMessage<uint8_t>(PSTR("Iface Num"),iface);
        //ErrorMessage<uint8_t>(PSTR("Alt.Set"),alt);

        if (alt) // Wrong interface - by BT spec, no alt setting
                return;

        bConfNum = conf;
        uint8_t index;

        if ((pep->bmAttributes & 0x03) == 3 && (pep->bEndpointAddress & 0x80) == 0x80) { // Interrupt In endpoint found
                index = BTD_EVENT_PIPE;
                epInfo[index].bmNakPower = USB_NAK_NOWAIT;
        } else {
                if ((pep->bmAttributes & 0x02) == 2) // Bulk endpoint found
                        index = ((pep->bEndpointAddress & 0x80) == 0x80) ? BTD_DATAIN_PIPE : BTD_DATAOUT_PIPE;
                else
                        return;
        }

        // Fill the rest of endpoint data structure
        epInfo[index].epAddr = (pep->bEndpointAddress & 0x0F);
        epInfo[index].maxPktSize = (uint8_t)pep->wMaxPacketSize;
#ifdef EXTRADEBUG
        PrintEndpointDescriptor(pep);
#endif
        if (pollInterval < pep->bInterval) // Set the polling interval as the largest polling interval obtained from endpoints
                pollInterval = pep->bInterval;
        bNumEP++;
}

void BTD::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr) {
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
uint8_t BTD::Release() {
        for (uint8_t i = 0; i < BTD_NUMSERVICES; i++) {
                if (btService[i])
                        btService[i]->Reset(); // Reset all Bluetooth services
        }

        pUsb->GetAddressPool().FreeAddress(bAddress);
        bAddress = 0;
        bPollEnable = false;
        bNumEP = 1; // must have to be reset to 1
        return 0;
}

uint8_t BTD::Poll() {
        if (!bPollEnable)
                return 0;
        if (qNextPollTime <= millis()) { // Don't poll if shorter than polling interval
                qNextPollTime = millis() + pollInterval; // Set new poll time
                HCI_event_task(); // poll the HCI event pipe
                ACL_event_task(); // start polling the ACL input pipe too, though discard data until connected
        }
        return 0;
}

void BTD::HCI_event_task() {
        /* check the event pipe*/
        uint16_t MAX_BUFFER_SIZE = BULK_MAXPKTSIZE; // Request more than 16 bytes anyway, the inTransfer routine will take care of this
        uint8_t rcode = pUsb->inTransfer(bAddress, epInfo[ BTD_EVENT_PIPE ].epAddr, &MAX_BUFFER_SIZE, hcibuf); // input on endpoint 1
        if (!rcode || rcode == hrNAK) // Check for errors
        {
                switch (hcibuf[0]) //switch on event type
                {
                        case EV_COMMAND_COMPLETE:
                                if (!hcibuf[5]) { // Check if command succeeded
                                        hci_event_flag |= HCI_FLAG_CMD_COMPLETE; // set command complete flag
                                        if ((hcibuf[3] == 0x01) && (hcibuf[4] == 0x10)) { // parameters from read local version information
                                                hci_version = hcibuf[6]; // Used to check if it supports 2.0+EDR - see http://www.bluetooth.org/Technical/AssignedNumbers/hci.htm
                                                hci_event_flag |= HCI_FLAG_READ_VERSION;
                                        } else if ((hcibuf[3] == 0x09) && (hcibuf[4] == 0x10)) { // parameters from read local bluetooth address
                                                for (uint8_t i = 0; i < 6; i++)
                                                        my_bdaddr[i] = hcibuf[6 + i];
                                                hci_event_flag |= HCI_FLAG_READ_BDADDR;
                                        }
                                }
                                break;

                        case EV_COMMAND_STATUS:
                                if (hcibuf[2]) { // show status on serial if not OK
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nHCI Command Failed: "), 0x80);
                                        D_PrintHex<uint8_t > (hcibuf[2], 0x80);
                                        Notify(PSTR(" "), 0x80);
                                        D_PrintHex<uint8_t > (hcibuf[4], 0x80);
                                        Notify(PSTR(" "), 0x80);
                                        D_PrintHex<uint8_t > (hcibuf[5], 0x80);
#endif
                                }
                                break;

                        case EV_INQUIRY_COMPLETE:
                                if (inquiry_counter >= 5) {
                                        inquiry_counter = 0;
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nCouldn't find Wiimote"), 0x80);
#endif
                                        connectToWii = false;
                                        pairWithWii = false;
                                        hci_state = HCI_SCANNING_STATE;
                                }
                                inquiry_counter++;
                                break;

                        case EV_INQUIRY_RESULT:
                                if (hcibuf[2]) { // Check that there is more than zero responses
#ifdef EXTRADEBUG
                                        Notify(PSTR("\r\nNumber of responses: "), 0x80);
                                        Notify(hcibuf[2], 0x80);
#endif
                                        for (uint8_t i = 0; i < hcibuf[2]; i++) {
                                                if ((hcibuf[4 + 8 * hcibuf[2] + 3 * i] == 0x04 && hcibuf[5 + 8 * hcibuf[2] + 3 * i] == 0x25 && hcibuf[6 + 8 * hcibuf[2] + 3 * i] == 0x00) || (hcibuf[4 + 8 * hcibuf[2] + 3 * i] == 0x08 && hcibuf[5 + 8 * hcibuf[2] + 3 * i] == 0x05 && hcibuf[6 + 8 * hcibuf[2] + 3 * i] == 0x00)) { // See http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html and http://wiibrew.org/wiki/Wiimote#SDP_information
                                                        if (hcibuf[4 + 8 * hcibuf[2] + 3 * i] == 0x08) // Check if it's the new Wiimote with motion plus inside that was detected
                                                                motionPlusInside = true;
                                                        else
                                                                motionPlusInside = false;
                                                        disc_bdaddr[0] = hcibuf[3 + 6 * i];
                                                        disc_bdaddr[1] = hcibuf[4 + 6 * i];
                                                        disc_bdaddr[2] = hcibuf[5 + 6 * i];
                                                        disc_bdaddr[3] = hcibuf[6 + 6 * i];
                                                        disc_bdaddr[4] = hcibuf[7 + 6 * i];
                                                        disc_bdaddr[5] = hcibuf[8 + 6 * i];
                                                        hci_event_flag |= HCI_FLAG_WII_FOUND;
                                                        break;
                                                }
#ifdef EXTRADEBUG
                                                else {
                                                        Notify(PSTR("\r\nClass of device: "), 0x80);
                                                        D_PrintHex<uint8_t > (hcibuf[6 + 8 * hcibuf[2] + 3 * i], 0x80);
                                                        Notify(PSTR(" "), 0x80);
                                                        D_PrintHex<uint8_t > (hcibuf[5 + 8 * hcibuf[2] + 3 * i], 0x80);
                                                        Notify(PSTR(" "), 0x80);
                                                        D_PrintHex<uint8_t > (hcibuf[4 + 8 * hcibuf[2] + 3 * i], 0x80);
                                                }
#endif
                                        }
                                }
                                break;

                        case EV_CONNECT_COMPLETE:
                                hci_event_flag |= HCI_FLAG_CONNECT_EVENT;
                                if (!hcibuf[2]) { // check if connected OK
                                        hci_handle = hcibuf[3] | ((hcibuf[4] & 0x0F) << 8); // store the handle for the ACL connection
                                        hci_event_flag |= HCI_FLAG_CONN_COMPLETE; // set connection complete flag
                                }
#ifdef EXTRADEBUG
                                else {
                                        Notify(PSTR("\r\nConnection Failed"), 0x80);
                                        hci_state = HCI_CHECK_WII_SERVICE;
                                }
#endif
                                break;

                        case EV_DISCONNECT_COMPLETE:
                                if (!hcibuf[2]) { // check if disconnected OK
                                        hci_event_flag |= HCI_FLAG_DISCONN_COMPLETE; // set disconnect command complete flag
                                        hci_event_flag &= ~HCI_FLAG_CONN_COMPLETE; // clear connection complete flag
                                }
                                break;

                        case EV_REMOTE_NAME_COMPLETE:
                                if (!hcibuf[2]) { // check if reading is OK
                                        for (uint8_t i = 0; i < min(sizeof (remote_name), sizeof (hcibuf) - 9); i++)
                                                remote_name[i] = hcibuf[9 + i];
                                        hci_event_flag |= HCI_FLAG_REMOTE_NAME_COMPLETE;
                                }
                                break;

                        case EV_INCOMING_CONNECT:
                                disc_bdaddr[0] = hcibuf[2];
                                disc_bdaddr[1] = hcibuf[3];
                                disc_bdaddr[2] = hcibuf[4];
                                disc_bdaddr[3] = hcibuf[5];
                                disc_bdaddr[4] = hcibuf[6];
                                disc_bdaddr[5] = hcibuf[7];
#ifdef EXTRADEBUG
                                Notify(PSTR("\r\nClass of device: "), 0x80);
                                D_PrintHex<uint8_t > (hcibuf[10], 0x80);
                                Notify(PSTR(" "), 0x80);
                                D_PrintHex<uint8_t > (hcibuf[9], 0x80);
                                Notify(PSTR(" "), 0x80);
                                D_PrintHex<uint8_t > (hcibuf[8], 0x80);
#endif
                                hci_event_flag |= HCI_FLAG_INCOMING_REQUEST;
                                break;

                        case EV_PIN_CODE_REQUEST:
                                if (pairWithWii) {
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nPairing with wiimote"), 0x80);
#endif
                                        hci_pin_code_request_reply();
                                } else if (btdPin != NULL) {
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nBluetooth pin is set too: "), 0x80);
                                        NotifyStr(btdPin, 0x80);
#endif
                                        hci_pin_code_request_reply();
                                } else {
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nNo pin was set"), 0x80);
#endif
                                        hci_pin_code_negative_request_reply();
                                }
                                break;

                        case EV_LINK_KEY_REQUEST:
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nReceived Key Request"), 0x80);
#endif
                                hci_link_key_request_negative_reply();
                                break;

                        case EV_AUTHENTICATION_COMPLETE:
                                if (pairWithWii && !connectToWii) {
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nPairing successful"), 0x80);
#endif
                                        connectToWii = true; // Only send the ACL data to the Wii service
                                }
                                break;
                                /* We will just ignore the following events */
                        case EV_NUM_COMPLETE_PKT:
                        case EV_ROLE_CHANGED:
                        case EV_PAGE_SCAN_REP_MODE:
                        case EV_LOOPBACK_COMMAND:
                        case EV_DATA_BUFFER_OVERFLOW:
                        case EV_CHANGE_CONNECTION_LINK:
                        case EV_MAX_SLOTS_CHANGE:
                        case EV_QOS_SETUP_COMPLETE:
                        case EV_LINK_KEY_NOTIFICATION:
                        case EV_ENCRYPTION_CHANGE:
                        case EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE:
                                break;
#ifdef EXTRADEBUG
                        default:
                                if (hcibuf[0] != 0x00) {
                                        Notify(PSTR("\r\nUnmanaged HCI Event: "), 0x80);
                                        D_PrintHex<uint8_t > (hcibuf[0], 0x80);
                                }
                                break;
#endif
                } // switch
        }
#ifdef EXTRADEBUG
        else {
                Notify(PSTR("\r\nHCI event error: "), 0x80);
                D_PrintHex<uint8_t > (rcode, 0x80);
        }
#endif
        HCI_task();
}

/* Poll Bluetooth and print result */
void BTD::HCI_task() {
        switch (hci_state) {
                case HCI_INIT_STATE:
                        hci_counter++;
                        if (hci_counter > hci_num_reset_loops) { // wait until we have looped x times to clear any old events
                                hci_reset();
                                hci_state = HCI_RESET_STATE;
                                hci_counter = 0;
                        }
                        break;

                case HCI_RESET_STATE:
                        hci_counter++;
                        if (hci_cmd_complete) {
                                hci_counter = 0;
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nHCI Reset complete"), 0x80);
#endif
                                hci_state = HCI_CLASS_STATE;
                                hci_write_class_of_device();
                        } else if (hci_counter > hci_num_reset_loops) {
                                hci_num_reset_loops *= 10;
                                if (hci_num_reset_loops > 2000)
                                        hci_num_reset_loops = 2000;
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nNo response to HCI Reset"), 0x80);
#endif
                                hci_state = HCI_INIT_STATE;
                                hci_counter = 0;
                        }
                        break;

                case HCI_CLASS_STATE:
                        if (hci_cmd_complete) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nWrite class of device"), 0x80);
#endif
                                hci_state = HCI_BDADDR_STATE;
                                hci_read_bdaddr();
                        }
                        break;

                case HCI_BDADDR_STATE:
                        if (hci_read_bdaddr_complete) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nLocal Bluetooth Address: "), 0x80);
                                for (int8_t i = 5; i > 0; i--) {
                                        D_PrintHex<uint8_t > (my_bdaddr[i], 0x80);
                                        Notify(PSTR(":"), 0x80);
                                }
                                D_PrintHex<uint8_t > (my_bdaddr[0], 0x80);
#endif
                                hci_read_local_version_information();
                                hci_state = HCI_LOCAL_VERSION_STATE;
                        }
                        break;

                case HCI_LOCAL_VERSION_STATE: // The local version is used by the PS3BT class
                        if (hci_read_version_complete) {
                                if (btdName != NULL) {
                                        hci_set_local_name(btdName);
                                        hci_state = HCI_SET_NAME_STATE;
                                } else
                                        hci_state = HCI_CHECK_WII_SERVICE;
                        }
                        break;

                case HCI_SET_NAME_STATE:
                        if (hci_cmd_complete) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nThe name is set to: "), 0x80);
                                NotifyStr(btdName, 0x80);
#endif
                                hci_state = HCI_CHECK_WII_SERVICE;
                        }
                        break;

                case HCI_CHECK_WII_SERVICE:
                        if (pairWithWii) { // Check if it should try to connect to a wiimote
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nStarting inquiry\r\nPress 1 & 2 on the Wiimote\r\nOr press sync if you are using a Wii U Pro Controller"), 0x80);
#endif
                                hci_inquiry();
                                hci_state = HCI_INQUIRY_STATE;
                        } else
                                hci_state = HCI_SCANNING_STATE; // Don't try to connect to a Wiimote
                        break;

                case HCI_INQUIRY_STATE:
                        if (hci_wii_found) {
                                hci_inquiry_cancel(); // Stop inquiry
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nWiimote found"), 0x80);
                                Notify(PSTR("\r\nNow just create the instance like so:"), 0x80);
                                Notify(PSTR("\r\nWII Wii(&Btd);"), 0x80);
                                Notify(PSTR("\r\nAnd then press any button on the Wiimote"), 0x80);
#endif
                                if (motionPlusInside) {
                                        hci_remote_name(); // We need to know the name to distinguish between a Wiimote and a Wii U Pro Controller
                                        hci_state = HCI_REMOTE_NAME_STATE;
                                } else
                                        hci_state = HCI_CONNECT_WII_STATE;
                        }
                        break;

                case HCI_CONNECT_WII_STATE:
                        if (hci_cmd_complete) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nConnecting to Wiimote"), 0x80);
#endif
                                hci_connect();
                                hci_state = HCI_CONNECTED_WII_STATE;
                        }
                        break;

                case HCI_CONNECTED_WII_STATE:
                        if (hci_connect_event) {
                                if (hci_connect_complete) {
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nConnected to Wiimote"), 0x80);
#endif
                                        hci_authentication_request(); // This will start the pairing with the wiimote
                                        hci_state = HCI_SCANNING_STATE;
                                } else {
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nTrying to connect one more time..."), 0x80);
#endif
                                        hci_connect(); // Try to connect one more time
                                }
                        }
                        break;

                case HCI_SCANNING_STATE:
                        if (!connectToWii && !pairWithWii) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nWait For Incoming Connection Request"), 0x80);
#endif
                                hci_write_scan_enable();
                                watingForConnection = true;
                                hci_state = HCI_CONNECT_IN_STATE;
                        }
                        break;

                case HCI_CONNECT_IN_STATE:
                        if (hci_incoming_connect_request) {
                                watingForConnection = false;
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nIncoming Connection Request"), 0x80);
#endif
                                hci_remote_name();
                                hci_state = HCI_REMOTE_NAME_STATE;
                        } else if (hci_disconnect_complete)
                                hci_state = HCI_DISCONNECT_STATE;
                        break;

                case HCI_REMOTE_NAME_STATE:
                        if (hci_remote_name_complete) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nRemote Name: "), 0x80);
                                for (uint8_t i = 0; i < 30; i++) {
                                        if (remote_name[i] == NULL)
                                                break;
                                        Notifyc(remote_name[i], 0x80);
                                }
#endif
                                if (strncmp((const char*)remote_name, "Nintendo", 8) == 0) {
                                        incomingWii = true;
#ifdef DEBUG_USB_HOST
                                        Notify(PSTR("\r\nWiimote is connecting"), 0x80);
#endif
                                        if (strncmp((const char*)remote_name, "Nintendo RVL-CNT-01-TR", 22) == 0) {
#ifdef DEBUG_USB_HOST
                                                Notify(PSTR(" with Motion Plus Inside"), 0x80);
#endif
                                                motionPlusInside = true;
                                        } else if (strncmp((const char*)remote_name, "Nintendo RVL-CNT-01-UC", 22) == 0) {
#ifdef DEBUG_USB_HOST
                                                Notify(PSTR(" - Wii U Pro Controller"), 0x80);
#endif
                                                motionPlusInside = true;
                                                wiiUProController = true;
                                        } else {
                                                motionPlusInside = false;
                                                wiiUProController = false;
                                        }
                                }
                                if (pairWithWii && motionPlusInside)
                                        hci_state = HCI_CONNECT_WII_STATE;
                                else {
                                        hci_accept_connection();
                                        hci_state = HCI_CONNECTED_STATE;
                                }
                        }
                        break;

                case HCI_CONNECTED_STATE:
                        if (hci_connect_complete) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nConnected to Device: "), 0x80);
                                for (int8_t i = 5; i > 0; i--) {
                                        D_PrintHex<uint8_t > (disc_bdaddr[i], 0x80);
                                        Notify(PSTR(":"), 0x80);
                                }
                                D_PrintHex<uint8_t > (disc_bdaddr[0], 0x80);
#endif
                                // Clear these flags for a new connection
                                l2capConnectionClaimed = false;
                                sdpConnectionClaimed = false;
                                rfcommConnectionClaimed = false;

                                hci_event_flag = 0;
                                hci_state = HCI_DONE_STATE;
                        }
                        break;

                case HCI_DONE_STATE:
                        hci_counter++;
                        if (hci_counter > 1000) { // Wait until we have looped 1000 times to make sure that the L2CAP connection has been started
                                hci_counter = 0;
                                hci_state = HCI_SCANNING_STATE;
                        }
                        break;

                case HCI_DISCONNECT_STATE:
                        if (hci_disconnect_complete) {
#ifdef DEBUG_USB_HOST
                                Notify(PSTR("\r\nHCI Disconnected from Device"), 0x80);
#endif
                                hci_event_flag = 0; // Clear all flags

                                // Reset all buffers
                                for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++)
                                        hcibuf[i] = 0;
                                for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++)
                                        l2capinbuf[i] = 0;

                                hci_state = HCI_SCANNING_STATE;
                        }
                        break;
                default:
                        break;
        }
}

void BTD::ACL_event_task() {
        uint16_t MAX_BUFFER_SIZE = BULK_MAXPKTSIZE;
        uint8_t rcode = pUsb->inTransfer(bAddress, epInfo[ BTD_DATAIN_PIPE ].epAddr, &MAX_BUFFER_SIZE, l2capinbuf); // input on endpoint 2
        if (!rcode) { // Check for errors
                for (uint8_t i = 0; i < BTD_NUMSERVICES; i++)
                        if (btService[i])
                                btService[i]->ACLData(l2capinbuf);
        }
#ifdef EXTRADEBUG
        else if (rcode != hrNAK) {
                Notify(PSTR("\r\nACL data in error: "), 0x80);
                D_PrintHex<uint8_t > (rcode, 0x80);
        }
#endif
        for (uint8_t i = 0; i < BTD_NUMSERVICES; i++)
                if (btService[i])
                        btService[i]->Run();
}

/************************************************************/
/*                    HCI Commands                        */

/************************************************************/
void BTD::HCI_Command(uint8_t* data, uint16_t nbytes) {
        hci_event_flag &= ~HCI_FLAG_CMD_COMPLETE;
        pUsb->ctrlReq(bAddress, epInfo[ BTD_CONTROL_PIPE ].epAddr, bmREQ_HCI_OUT, 0x00, 0x00, 0x00, 0x00, nbytes, nbytes, data, NULL);
}

void BTD::hci_reset() {
        hci_event_flag = 0; // Clear all the flags
        hcibuf[0] = 0x03; // HCI OCF = 3
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x00;

        HCI_Command(hcibuf, 3);
}

void BTD::hci_write_scan_enable() {
        hci_event_flag &= ~HCI_FLAG_INCOMING_REQUEST;
        hcibuf[0] = 0x1A; // HCI OCF = 1A
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x01; // parameter length = 1
        if (btdName != NULL)
                hcibuf[3] = 0x03; // Inquiry Scan enabled. Page Scan enabled.
        else
                hcibuf[3] = 0x02; // Inquiry Scan disabled. Page Scan enabled.

        HCI_Command(hcibuf, 4);
}

void BTD::hci_write_scan_disable() {
        hcibuf[0] = 0x1A; // HCI OCF = 1A
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x01; // parameter length = 1
        hcibuf[3] = 0x00; // Inquiry Scan disabled. Page Scan disabled.

        HCI_Command(hcibuf, 4);
}

void BTD::hci_read_bdaddr() {
        hcibuf[0] = 0x09; // HCI OCF = 9
        hcibuf[1] = 0x04 << 2; // HCI OGF = 4
        hcibuf[2] = 0x00;

        HCI_Command(hcibuf, 3);
}

void BTD::hci_read_local_version_information() {
        hcibuf[0] = 0x01; // HCI OCF = 1
        hcibuf[1] = 0x04 << 2; // HCI OGF = 4
        hcibuf[2] = 0x00;

        HCI_Command(hcibuf, 3);
}

void BTD::hci_accept_connection() {
        hci_event_flag &= ~HCI_FLAG_CONN_COMPLETE;
        hcibuf[0] = 0x09; // HCI OCF = 9
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x07; // parameter length 7
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        hcibuf[9] = 0x00; //switch role to master

        HCI_Command(hcibuf, 10);
}

void BTD::hci_remote_name() {
        hci_event_flag &= ~HCI_FLAG_REMOTE_NAME_COMPLETE;
        hcibuf[0] = 0x19; // HCI OCF = 19
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x0A; // parameter length = 10
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        hcibuf[9] = 0x01; //Page Scan Repetition Mode
        hcibuf[10] = 0x00; //Reserved
        hcibuf[11] = 0x00; //Clock offset - low byte
        hcibuf[12] = 0x00; //Clock offset - high byte

        HCI_Command(hcibuf, 13);
}

void BTD::hci_set_local_name(const char* name) {
        hcibuf[0] = 0x13; // HCI OCF = 13
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = strlen(name) + 1; // parameter length = the length of the string + end byte
        uint8_t i;
        for (i = 0; i < strlen(name); i++)
                hcibuf[i + 3] = name[i];
        hcibuf[i + 3] = 0x00; // End of string

        HCI_Command(hcibuf, 4 + strlen(name));
}

void BTD::hci_inquiry() {
        hci_event_flag &= ~HCI_FLAG_WII_FOUND;
        hcibuf[0] = 0x01;
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x05; // Parameter Total Length = 5
        hcibuf[3] = 0x33; // LAP: Genera/Unlimited Inquiry Access Code (GIAC = 0x9E8B33) - see https://www.bluetooth.org/Technical/AssignedNumbers/baseband.htm
        hcibuf[4] = 0x8B;
        hcibuf[5] = 0x9E;
        hcibuf[6] = 0x30; // Inquiry time = 61.44 sec (maximum)
        hcibuf[7] = 0x0A; // 10 number of responses

        HCI_Command(hcibuf, 8);
}

void BTD::hci_inquiry_cancel() {
        hcibuf[0] = 0x02;
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x00; // Parameter Total Length = 0

        HCI_Command(hcibuf, 3);
}

void BTD::hci_connect() {
        hci_event_flag &= ~(HCI_FLAG_CONN_COMPLETE | HCI_FLAG_CONNECT_EVENT);
        hcibuf[0] = 0x05;
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x0D; // parameter Total Length = 13
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        hcibuf[9] = 0x18; // DM1 or DH1 may be used
        hcibuf[10] = 0xCC; // DM3, DH3, DM5, DH5 may be used
        hcibuf[11] = 0x01; // Page repetition mode R1
        hcibuf[12] = 0x00; // Reserved
        hcibuf[13] = 0x00; // Clock offset
        hcibuf[14] = 0x00; // Invalid clock offset
        hcibuf[15] = 0x00; // Do not allow role switch

        HCI_Command(hcibuf, 16);
}

void BTD::hci_pin_code_request_reply() {
        hcibuf[0] = 0x0D; // HCI OCF = 0D
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x17; // parameter length 23
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        if (pairWithWii) {
                hcibuf[9] = 6; // Pin length is the length of the Bluetooth address
                if (wiiUProController) {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nParing with Wii U Pro Controller"), 0x80);
#endif
                        for (uint8_t i = 0; i < 6; i++)
                                hcibuf[10 + i] = my_bdaddr[i]; // The pin is the Bluetooth dongles Bluetooth address backwards
                } else {
                        for (uint8_t i = 0; i < 6; i++)
                                hcibuf[10 + i] = disc_bdaddr[i]; // The pin is the Wiimote's Bluetooth address backwards
                }
                for (uint8_t i = 16; i < 26; i++)
                        hcibuf[i] = 0x00; // The rest should be 0
        } else {
                hcibuf[9] = strlen(btdPin); // Length of pin
                uint8_t i;
                for (i = 0; i < strlen(btdPin); i++) // The maximum size of the pin is 16
                        hcibuf[i + 10] = btdPin[i];
                for (; i < 16; i++)
                        hcibuf[i + 10] = 0x00; // The rest should be 0
        }

        HCI_Command(hcibuf, 26);
}

void BTD::hci_pin_code_negative_request_reply() {
        hcibuf[0] = 0x0E; // HCI OCF = 0E
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x06; // parameter length 6
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];

        HCI_Command(hcibuf, 9);
}

void BTD::hci_link_key_request_negative_reply() {
        hcibuf[0] = 0x0C; // HCI OCF = 0C
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x06; // parameter length 6
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];

        HCI_Command(hcibuf, 9);
}

void BTD::hci_authentication_request() {
        hcibuf[0] = 0x11; // HCI OCF = 11
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x02; // parameter length = 2
        hcibuf[3] = (uint8_t)(hci_handle & 0xFF); //connection handle - low byte
        hcibuf[4] = (uint8_t)((hci_handle >> 8) & 0x0F); //connection handle - high byte

        HCI_Command(hcibuf, 5);
}

void BTD::hci_disconnect(uint16_t handle) { // This is called by the different services
        hci_event_flag &= ~HCI_FLAG_DISCONN_COMPLETE;
        hcibuf[0] = 0x06; // HCI OCF = 6
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x03; // parameter length = 3
        hcibuf[3] = (uint8_t)(handle & 0xFF); //connection handle - low byte
        hcibuf[4] = (uint8_t)((handle >> 8) & 0x0F); //connection handle - high byte
        hcibuf[5] = 0x13; // reason

        HCI_Command(hcibuf, 6);
}

void BTD::hci_write_class_of_device() { // See http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html
        hcibuf[0] = 0x24; // HCI OCF = 3
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x03; // parameter length = 3
        hcibuf[3] = 0x04; // Robot
        hcibuf[4] = 0x08; // Toy
        hcibuf[5] = 0x00;

        HCI_Command(hcibuf, 6);
}
/*******************************************************************
 *                                                                 *
 *                        HCI ACL Data Packet                      *
 *                                                                 *
 *   buf[0]          buf[1]          buf[2]          buf[3]
 *   0       4       8    11 12      16              24            31 MSB
 *  .-+-+-+-+-+-+-+-|-+-+-+-|-+-|-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *  |      HCI Handle       |PB |BC |       Data Total Length       |   HCI ACL Data Packet
 *  .-+-+-+-+-+-+-+-|-+-+-+-|-+-|-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *
 *   buf[4]          buf[5]          buf[6]          buf[7]
 *   0               8               16                            31 MSB
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *  |            Length             |          Channel ID           |   Basic L2CAP header
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *
 *   buf[8]          buf[9]          buf[10]         buf[11]
 *   0               8               16                            31 MSB
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *  |     Code      |  Identifier   |            Length             |   Control frame (C-frame)
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.   (signaling packet format)
 */
/************************************************************/
/*                    L2CAP Commands                        */

/************************************************************/
void BTD::L2CAP_Command(uint16_t handle, uint8_t* data, uint8_t nbytes, uint8_t channelLow, uint8_t channelHigh) {
        uint8_t buf[8 + nbytes];
        buf[0] = (uint8_t)(handle & 0xff); // HCI handle with PB,BC flag
        buf[1] = (uint8_t)(((handle >> 8) & 0x0f) | 0x20);
        buf[2] = (uint8_t)((4 + nbytes) & 0xff); // HCI ACL total data length
        buf[3] = (uint8_t)((4 + nbytes) >> 8);
        buf[4] = (uint8_t)(nbytes & 0xff); // L2CAP header: Length
        buf[5] = (uint8_t)(nbytes >> 8);
        buf[6] = channelLow;
        buf[7] = channelHigh;

        for (uint16_t i = 0; i < nbytes; i++) // L2CAP C-frame
                buf[8 + i] = data[i];

        uint8_t rcode = pUsb->outTransfer(bAddress, epInfo[ BTD_DATAOUT_PIPE ].epAddr, (8 + nbytes), buf);
        if (rcode) {
                delay(100); // This small delay prevents it from overflowing if it fails
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nError sending L2CAP message: 0x"), 0x80);
                D_PrintHex<uint8_t > (rcode, 0x80);
                Notify(PSTR(" - Channel ID: "), 0x80);
                D_PrintHex<uint8_t > (channelHigh, 0x80);
                Notify(PSTR(" "), 0x80);
                D_PrintHex<uint8_t > (channelLow, 0x80);
#endif
        }
}

void BTD::l2cap_connection_request(uint16_t handle, uint8_t rxid, uint8_t* scid, uint16_t psm) {
        l2capoutbuf[0] = L2CAP_CMD_CONNECTION_REQUEST; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x04; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = (uint8_t)(psm & 0xff); // PSM
        l2capoutbuf[5] = (uint8_t)(psm >> 8);
        l2capoutbuf[6] = scid[0]; // Source CID
        l2capoutbuf[7] = scid[1];

        L2CAP_Command(handle, l2capoutbuf, 8);
}

void BTD::l2cap_connection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid, uint8_t result) {
        l2capoutbuf[0] = L2CAP_CMD_CONNECTION_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x08; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0]; // Destination CID
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = scid[0]; // Source CID
        l2capoutbuf[7] = scid[1];
        l2capoutbuf[8] = result; // Result: Pending or Success
        l2capoutbuf[9] = 0x00;
        l2capoutbuf[10] = 0x00; // No further information
        l2capoutbuf[11] = 0x00;

        L2CAP_Command(handle, l2capoutbuf, 12);
}

void BTD::l2cap_config_request(uint16_t handle, uint8_t rxid, uint8_t* dcid) {
        l2capoutbuf[0] = L2CAP_CMD_CONFIG_REQUEST; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x08; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0]; // Destination CID
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = 0x00; // Flags
        l2capoutbuf[7] = 0x00;
        l2capoutbuf[8] = 0x01; // Config Opt: type = MTU (Maximum Transmission Unit) - Hint
        l2capoutbuf[9] = 0x02; // Config Opt: length
        l2capoutbuf[10] = 0xFF; // MTU
        l2capoutbuf[11] = 0xFF;

        L2CAP_Command(handle, l2capoutbuf, 12);
}

void BTD::l2cap_config_response(uint16_t handle, uint8_t rxid, uint8_t* scid) {
        l2capoutbuf[0] = L2CAP_CMD_CONFIG_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x0A; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = scid[0]; // Source CID
        l2capoutbuf[5] = scid[1];
        l2capoutbuf[6] = 0x00; // Flag
        l2capoutbuf[7] = 0x00;
        l2capoutbuf[8] = 0x00; // Result
        l2capoutbuf[9] = 0x00;
        l2capoutbuf[10] = 0x01; // Config
        l2capoutbuf[11] = 0x02;
        l2capoutbuf[12] = 0xA0;
        l2capoutbuf[13] = 0x02;

        L2CAP_Command(handle, l2capoutbuf, 14);
}

void BTD::l2cap_disconnection_request(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid) {
        l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_REQUEST; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x04; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0];
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = scid[0];
        l2capoutbuf[7] = scid[1];

        L2CAP_Command(handle, l2capoutbuf, 8);
}

void BTD::l2cap_disconnection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid) {
        l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x04; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0];
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = scid[0];
        l2capoutbuf[7] = scid[1];

        L2CAP_Command(handle, l2capoutbuf, 8);
}

void BTD::l2cap_information_response(uint16_t handle, uint8_t rxid, uint8_t infoTypeLow, uint8_t infoTypeHigh) {
        l2capoutbuf[0] = L2CAP_CMD_INFORMATION_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x08; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = infoTypeLow;
        l2capoutbuf[5] = infoTypeHigh;
        l2capoutbuf[6] = 0x00; // Result = success
        l2capoutbuf[7] = 0x00; // Result = success
        l2capoutbuf[8] = 0x00;
        l2capoutbuf[9] = 0x00;
        l2capoutbuf[10] = 0x00;
        l2capoutbuf[11] = 0x00;

        L2CAP_Command(handle, l2capoutbuf, 12);
}

/* PS3 Commands - only set Bluetooth address is implemented in this library */
void BTD::setBdaddr(uint8_t* bdaddr) {
        /* Set the internal Bluetooth address */
        uint8_t buf[8];
        buf[0] = 0x01;
        buf[1] = 0x00;

        for (uint8_t i = 0; i < 6; i++)
                buf[i + 2] = bdaddr[5 - i]; // Copy into buffer, has to be written reversed

        // bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set Report (0x09), Report ID (0xF5), Report Type (Feature 0x03), interface (0x00), datalength, datalength, data)
        pUsb->ctrlReq(bAddress, epInfo[BTD_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0xF5, 0x03, 0x00, 8, 8, buf, NULL);
}

void BTD::setMoveBdaddr(uint8_t* bdaddr) {
        /* Set the internal Bluetooth address */
        uint8_t buf[11];
        buf[0] = 0x05;
        buf[7] = 0x10;
        buf[8] = 0x01;
        buf[9] = 0x02;
        buf[10] = 0x12;

        for (uint8_t i = 0; i < 6; i++)
                buf[i + 1] = bdaddr[i];

        // bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set Report (0x09), Report ID (0x05), Report Type (Feature 0x03), interface (0x00), datalength, datalength, data)
        pUsb->ctrlReq(bAddress, epInfo[BTD_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0x05, 0x03, 0x00, 11, 11, buf, NULL);
}