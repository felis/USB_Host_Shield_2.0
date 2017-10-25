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
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data

const uint8_t BTD::BTD_CONTROL_PIPE		= 0;
const uint8_t BTD::BTD_EVENT_PIPE		= 1;
const uint8_t BTD::BTD_DATAIN_PIPE		= 2;
const uint8_t BTD::BTD_DATAOUT_PIPE		= 3;

BTD::BTD(USB *p) : pUsb(p) {// Pointer to USB class instance - mandatory
	for (uint8_t i=0; i<BTD_NUM_SERVICES; i++) {
		btService[i] = NULL;
	}

	pairWithWii = false;
	PID = 0;
	VID = 0;

	Initialize(); // Set all variables, endpoint structs etc. to default values

	pUsb->RegisterDeviceClass(this); // Set devConfig[] entry
}





void BTD::Initialize() {
	uint16_t i;

	for (i=0; i<BTD_MAX_ENDPOINTS; i++) {
		epInfo[i].epAddr = 0;
		epInfo[i].maxPktSize = (i) ? 0 : 8;
		epInfo[i].bmSndToggle = 0;
		epInfo[i].bmRcvToggle = 0;
		epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
	}

	for (i=0; i<BTD_NUM_SERVICES; i++) {
		if (btService[i]) {
			btService[i]->Reset(); // Reset all Bluetooth services
		}
	}

	for (i=0; i<sizeof(my_bdaddr);		i++)	my_bdaddr[i]		= 0;
	for (i=0; i<sizeof(last_bdaddr);	i++)	last_bdaddr[i]		= 0;
	for (i=0; i<sizeof(remote_name);	i++)	remote_name[i]		= 0;
	for (i=0; i<sizeof(classOfDevice);	i++)	classOfDevice[i]	= 0;
	for (i=0; i<sizeof(hcibuf_in);		i++)	hcibuf_in[i]		= 0;
	for (i=0; i<sizeof(hcibuf_out);		i++)	hcibuf_out[i]		= 0;
	for (i=0; i<sizeof(l2c_in_raw);		i++)	l2c_in_raw[i]		= 0;
	for (i=0; i<sizeof(l2c_out_raw);	i++)	l2c_out_raw[i]		= 0;

	waitingForConnection	= false;
	l2capConnectionClaimed	= false;
	sdpConnectionClaimed	= false;
	rfcommConnectionClaimed	= false;
	hci_handle				= 0;
	hci_version				= 0;
	motionPlusInside		= false;
	wiiUProController		= false;
	bConfNum				= 0;
	checkRemoteName			= false;

	hci_state				= 0;
	hci_counter				= 0;
	hci_num_reset_loops		= 0;
	hci_event_flag			= 0;
	inquiry_counter			= 0;


	connectToWii			= false;
	incomingWii				= false;
	bAddress				= 0; // Clear device address
	bNumEP					= 1; // Must have to be reset to 1
	qNextPollTime			= 0; // Reset next poll time
	pollInterval			= 0;
	bPollEnable				= false; // Don't start polling before dongle is connected
}




uint8_t BTD::ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed) {
	const uint8_t constBufSize = sizeof (USB_DEVICE_DESCRIPTOR);
	uint8_t buf[constBufSize];
	USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
	uint8_t rcode;
	UsbDevice *p = NULL;
	EpInfo *oldep_ptr = NULL;

	Initialize(); // Set all variables, endpoint structs etc. to default values

	AddressPool &addrPool = pUsb->GetAddressPool(); // Get memory address of USB device address pool
	Notify(PSTR("\r\nBTD ConfigureDevice"), 0x80);

	if (bAddress) { // Check if address has already been assigned to an instance
		Notify(PSTR("\r\nAddress in use"), 0x80);
		return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
	}

	p = addrPool.GetUsbDevicePtr(0); // Get pointer to pseudo device with address 0 assigned
	if (!p) {
		Notify(PSTR("\r\nAddress not found"), 0x80);
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
	}

	if (!p->epinfo) {
		Notify(PSTR("\r\nepinfo is null"), 0x80);
		return USB_ERROR_EPINFO_IS_NULL;
	}

	oldep_ptr = p->epinfo; // Save old pointer to EP_RECORD of address 0
	p->epinfo = epInfo; // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
	p->lowspeed = lowspeed;
	rcode = pUsb->getDevDescr(0, 0, constBufSize, (uint8_t*)buf); // Get device descriptor - addr, ep, nbytes, data

	p->epinfo = oldep_ptr; // Restore p->epinfo

	if (rcode)
		goto FailGetDevDescr;

	bAddress = addrPool.AllocAddress(parent, false, port); // Allocate new address according to device class

	if (!bAddress) {
		Notify(PSTR("\r\nOut of address space"), 0x80);
		return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
	}

	epInfo[0].maxPktSize = udd->bMaxPacketSize0; // Extract Max Packet Size from device descriptor
	epInfo[1].epAddr = udd->bNumConfigurations; // Steal and abuse from epInfo structure to save memory

	VID = udd->idVendor;
	PID = udd->idProduct;

	return USB_ERROR_CONFIG_REQUIRES_ADDITIONAL_RESET;


FailGetDevDescr:
	NotifyFailGetDevDescr(rcode);
	if (rcode != hrJERR) {
		rcode = USB_ERROR_FailGetDevDescr;
	}
	Release();
	return rcode;
};






uint8_t BTD::Init(uint8_t parent, uint8_t port, bool lowspeed) {
	uint8_t rcode;
	uint8_t num_of_conf = epInfo[1].epAddr; // Number of configurations
	epInfo[1].epAddr = 0;

	AddressPool &addrPool = pUsb->GetAddressPool();
	Notify(PSTR("\r\nBTD Init"), 0x80);
	UsbDevice *p = addrPool.GetUsbDevicePtr(bAddress); // Get pointer to assigned address record

	if (!p) {
		Notify(PSTR("\r\nAddress not found"), 0x80);
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
	}

	delay(300); // Assign new address to the device

	rcode = pUsb->setAddr(0, 0, bAddress); // Assign new address to the device
	if (rcode) {
		Notify(PSTR("\r\nsetAddr: "), 0x80);
		D_PrintHex<uint8_t > (rcode, 0x80);
		p->lowspeed = false;
		goto Fail;
	}
	Notify(PSTR("\r\nAddr: "), 0x80);
	D_PrintHex<uint8_t > (bAddress, 0x80);

	p->lowspeed = false;

	p = addrPool.GetUsbDevicePtr(bAddress); // Get pointer to assigned address record
	if (!p) {
		Notify(PSTR("\r\nAddress not found"), 0x80);
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
	}

	p->lowspeed = lowspeed;

	rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo); // Assign epInfo to epinfo pointer - only EP0 is known
	if (rcode) goto FailSetDevTblEntry;


	// Check if attached device is a Bluetooth dongle and fill endpoint data structure
	// First interface in the configuration must have Bluetooth assigned Class/Subclass/Protocol
	// And 3 endpoints - interrupt-IN, bulk-IN, bulk-OUT, not necessarily in this order
	for (uint8_t i=0; i<num_of_conf; i++) {
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

	// Set Configuration Value
	rcode = pUsb->setConf(bAddress, epInfo[ BTD_CONTROL_PIPE ].epAddr, bConfNum);
	if (rcode)
		goto FailSetConfDescr;

	hci_num_reset_loops = 100; // only loop 100 times before trying to send the hci reset command
	hci_counter = 0;
	hci_state = HCI_INIT_STATE;
	Notify("\r\nHCI_INIT_STATE");
	waitingForConnection = false;
	bPollEnable = true;

	Notify(PSTR("\r\nBluetooth Dongle Initialized"), 0x80);
	return 0; // Successful configuration


	/* Diagnostic messages */
FailSetDevTblEntry:
	NotifyFailSetDevTblEntry();
	goto Fail;

FailGetConfDescr:
	NotifyFailGetConfDescr();
	goto Fail;

FailSetConfDescr:
	NotifyFailSetConfDescr();
	goto Fail;

FailUnknownDevice:
	NotifyFailUnknownDevice(VID, PID);
	pUsb->setAddr(bAddress, 0, 0); // Reset address
	rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

Fail:
	Notify(PSTR("\r\nBTD Init Failed, error code: "), 0x80);
	NotifyFail(rcode);
	Release();
	return rcode;
}







/* Extracts interrupt-IN, bulk-IN, bulk-OUT endpoint information from config descriptor */
void BTD::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) {
	if (alt) return;

	bConfNum = conf;
	uint8_t index;

	if ((pep->bmAttributes & bmUSB_TRANSFER_TYPE) == USB_TRANSFER_TYPE_INTERRUPT && (pep->bEndpointAddress & 0x80) == 0x80) { // Interrupt In endpoint found
		index = BTD_EVENT_PIPE;
		epInfo[index].bmNakPower = USB_NAK_NOWAIT;

	} else if ((pep->bmAttributes & bmUSB_TRANSFER_TYPE) == USB_TRANSFER_TYPE_BULK) {// Bulk endpoint found
		index = ((pep->bEndpointAddress & 0x80) == 0x80) ? BTD_DATAIN_PIPE : BTD_DATAOUT_PIPE;

	} else {
		return;
	}

	// Fill the rest of endpoint data structure
	epInfo[index].epAddr = (pep->bEndpointAddress & 0x0F);
	epInfo[index].maxPktSize = (uint8_t)pep->wMaxPacketSize;
	PrintEndpointDescriptor(pep);
	if (pollInterval < pep->bInterval) { // Set the polling interval as the largest polling interval obtained from endpoints
		pollInterval = pep->bInterval;
	}
	bNumEP++;
}






void BTD::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr) {
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
}





/* Performs a cleanup after failed Init() attempt */
uint8_t BTD::Release() {
	Initialize(); // Set all variables, endpoint structs etc. to default values
	pUsb->GetAddressPool().FreeAddress(bAddress);
	return 0;
}






uint8_t BTD::Poll() {
	if (bPollEnable) {

		if (qNextPollTime >= pollInterval) {
			qNextPollTime = 0;
			HCI_event_task();		// Poll the HCI event pipe
			HCI_task();				// HCI state machine
			ACL_event_task();		// Poll the ACL input pipe too
		}
	}

	return USBDeviceConfig::Poll();
}






void BTD::disconnect() {
	for (uint8_t i=0; i<BTD_NUM_SERVICES; i++) {
		if (btService[i]) {
			btService[i]->disconnect();
		}
	}
};






void BTD::HCI_event_task() {
	uint16_t	length	= sizeof(hcibuf_in);
	uint8_t		rcode	= pUsb->inTransfer(
							bAddress,
							epInfo[BTD_EVENT_PIPE].epAddr,
							&length,
							hcibuf_in,
							pollInterval
						);

	if (!length) return;

#ifdef EXTRADEBUG
	if (length  &&  hcibuf_in[0] != 0x13) {
		Serial.print("\r\n");
		char debugz[10] = {0,0,0,0,0, 0,0,0,0,0};
		sprintf(debugz, "%d : ", length);
		Serial.print(debugz);

		for (uint16_t xx=0; xx<length; xx++) {
			sprintf(debugz, "%02x", hcibuf_in[xx]);
			Serial.print(debugz);
		}
	}
#endif

	if (!rcode || rcode == hrNAK) { // Check for errors
		switch(hcibuf_in[0]) { // Switch on event type




			case EV_COMMAND_COMPLETE:
				Notify("\r\nEV_COMMAND_COMPLETE");
				if (!hcibuf_in[5]) { // Check if command succeeded
					hci_set_flag(HCI_FLAG_CMD_COMPLETE); // Set command complete flag
					if ((hcibuf_in[3] == 0x01) && (hcibuf_in[4] == 0x10)) { // Parameters from read local version information
						hci_version = hcibuf_in[6]; // Used to check if it supports 2.0+EDR - see http://www.bluetooth.org/Technical/AssignedNumbers/hci.htm
						hci_set_flag(HCI_FLAG_READ_VERSION);
					} else if ((hcibuf_in[3] == 0x09) && (hcibuf_in[4] == 0x10)) { // Parameters from read local bluetooth address
						for (uint8_t i=0; i<6; i++)
							my_bdaddr[i] = hcibuf_in[6 + i];
						hci_set_flag(HCI_FLAG_READ_BDADDR);
					}
				}
				break;





			case EV_COMMAND_STATUS:
				Notify("\r\nEV_COMMAND_STATUS");
				if (hcibuf_in[2] == 0x0B) {
					Notify(PSTR("\r\nHCI Attempting Re-Connect"), 0x80);

					delay(50);
					hci_connect();
				} else if (hcibuf_in[2]) { // Show status on serial if not OK
					Notify(PSTR("\r\nHCI Command Failed: "), 0x80);
					D_PrintHex<uint8_t > (hcibuf_in[2], 0x80);
				}
				break;





			case EV_INQUIRY_COMPLETE:
				Notify("\r\nEV_INQUIRY_COMPLETE");
				if (inquiry_counter >= 5 && (pairWithWii)) {
					inquiry_counter = 0;

					if (pairWithWii){
						Notify(PSTR("\r\nCouldn't find Wii Remote"), 0x80);
					} else{
						Notify(PSTR("\r\nCouldn't find HID device"), 0x80);
					}

					pairWithWiiRemote(false);
				}
				inquiry_counter++;
				break;





			case EV_INQUIRY_RESULT:
				Notify("\r\nEV_INQUIRY_RESULT");
				if (hcibuf_in[2]) { // Check that there is more than zero responses
					Notify(PSTR("\r\nNumber of responses: "), 0x80);
					Notify(hcibuf_in[2], 0x80);

					for (uint8_t i=0; i<hcibuf_in[2]; i++) {
						uint8_t offset = 8 * hcibuf_in[2] + 3 * i;

						for (uint8_t j = 0; j < 3; j++) {
							classOfDevice[j] = hcibuf_in[j + 4 + offset];
						}

						Notify(PSTR("\r\nClass of device: "), 0x80);
						D_PrintHex<uint8_t > (classOfDevice[2], 0x80);
						Notify(PSTR(" "), 0x80);
						D_PrintHex<uint8_t > (classOfDevice[1], 0x80);
						Notify(PSTR(" "), 0x80);
						D_PrintHex<uint8_t > (classOfDevice[0], 0x80);

						if (pairWithWii && classOfDevice[2] == 0x00 && (classOfDevice[1] & 0x05) && (classOfDevice[0] & 0x0C)) { // See http://wiibrew.org/wiki/Wiimote#SDP_information
							checkRemoteName = true; // Check remote name to distinguish between the different controllers

							for (uint8_t j = 0; j < 6; j++) {
								last_bdaddr[j] = hcibuf_in[j + 3 + 6 * i];
							}

							hci_set_flag(HCI_FLAG_DEVICE_FOUND);
							break;
						}
					}
				}
				break;





			case EV_CONNECT_COMPLETE:
				Notify("\r\nEV_CONNECT_COMPLETE");
				hci_set_flag(HCI_FLAG_CONNECT_EVENT);

				if (!hcibuf_in[2]) { // Check if connected OK
					Notify(PSTR("\r\nConnection established"), 0x80);
					hci_handle = hcibuf_in[3] | ((hcibuf_in[4] & 0x0F) << 8); // Store the handle for the ACL connection
					hci_set_flag(HCI_FLAG_CONNECT_COMPLETE); // Set connection complete flag

				} else {
					hci_state = HCI_CHECK_DEVICE_SERVICE;
					Notify("\r\nHCI_CHECK_DEVICE_SERVICE");
					Notify(PSTR("\r\nConnection Failed: "), 0x80);
					D_PrintHex<uint8_t > (hcibuf_in[2], 0x80);
				}
				break;





			case EV_DISCONNECT_COMPLETE:
				Notify("\r\nEV_DISCONNECT_COMPLETE");
				if (!hcibuf_in[2]) { // Check if disconnected OK
					hci_set_flag(HCI_FLAG_DISCONNECT_COMPLETE); // Set disconnect command complete flag
					hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE); // Clear connection complete flag
				}
				break;





			case EV_REMOTE_NAME_COMPLETE:
				Notify("\r\nEV_REMOTE_NAME_COMPLETE");
				if (!hcibuf_in[2]) { // Check if reading is OK
					for (int i=0; i<min((int16_t)sizeof(remote_name)-1, length-9); i++) {
						remote_name[i] = hcibuf_in[9 + i];
						if (remote_name[i] == '\0'){ // End of string
							break;
						}
					}
					remote_name[sizeof(remote_name)-1] = '\0';
					hci_set_flag(HCI_FLAG_REMOTE_NAME_COMPLETE);
				}
				break;





			case EV_INCOMING_CONNECT:
				Notify("\r\nEV_INCOMING_CONNECT");

				for (uint8_t i=0; i<6; i++) {
					last_bdaddr[i] = hcibuf_in[i + 2];
				}

				for (uint8_t i=0; i<3; i++) {
					classOfDevice[i] = hcibuf_in[i + 8];
				}

				Notify(PSTR("\r\nClass of device: "), 0x80);
				D_PrintHex<uint8_t > (classOfDevice[2], 0x80);
				Notify(PSTR(" "), 0x80);
				D_PrintHex<uint8_t > (classOfDevice[1], 0x80);
				Notify(PSTR(" "), 0x80);
				D_PrintHex<uint8_t > (classOfDevice[0], 0x80);

				hci_set_flag(HCI_FLAG_INCOMING_REQUEST);
				break;





			case EV_PIN_CODE_REQUEST:
				Notify("\r\nEV_PIN_CODE_REQUEST");
				if (pairWithWii) {
					Notify(PSTR("\r\nPairing with Wii Remote"), 0x80);
					hci_pin_code_request_reply();
				} else {
					Notify(PSTR("\r\nNo pin was set"), 0x80);
					hci_pin_code_negative_request_reply();
				}
				break;





			case EV_LINK_KEY_REQUEST:
				Notify("\r\nEV_LINK_KEY_REQUEST");
				Notify(PSTR("\r\nReceived Key Request"), 0x80);
				hci_link_key_request_negative_reply();
				break;





			case EV_AUTHENTICATION_COMPLETE:
				Notify("\r\nEV_AUTHENTICATION_COMPLETE");
				if (!hcibuf_in[2]) { // Check if pairing was successful
					if (pairWithWii && !connectToWii) {
						Notify(PSTR("\r\nPairing successful with Wii Remote"), 0x80);
						connectToWii = true; // Used to indicate to the Wii service, that it should connect to this device
					}
				} else {
					Notify(PSTR("\r\nPairing Failed: "), 0x80);
					D_PrintHex<uint8_t > (hcibuf_in[2], 0x80);
					hci_disconnect(hci_handle);
					hci_state = HCI_DISCONNECT_STATE;
					Notify("\r\nHCI_DISCONNECT_STATE");
				}
				break;





				/* We will just ignore the following events */
			case EV_ROLE_CHANGED:
					Notify("\r\nEV_ROLE_CHANGED");
					break;

			case EV_PAGE_SCAN_REP_MODE:
					Notify("\r\nEV_PAGE_SCAN_REP_MODE");
					break;

			case EV_LOOPBACK_COMMAND:
					Notify("\r\nEV_LOOPBACK_COMMAND");
					break;

			case EV_DATA_BUFFER_OVERFLOW:
					Notify("\r\nEV_DATA_BUFFER_OVERFLOW");
					break;

			case EV_CHANGE_CONNECTION_LINK:
					Notify("\r\nEV_CHANGE_CONNECTION_LINK");
					break;

			case EV_MAX_SLOTS_CHANGE:
					Notify("\r\nEV_MAX_SLOTS_CHANGE");
					break;

			case EV_QOS_SETUP_COMPLETE:
					Notify("\r\nEV_QOS_SETUP_COMPLETE");
					break;

			case EV_LINK_KEY_NOTIFICATION:
					Notify("\r\nEV_LINK_KEY_NOTIFICATION");
					break;

			case EV_ENCRYPTION_CHANGE:
					Notify("\r\nEV_ENCRYPTION_CHANGE");
					break;

			case EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE:
					Notify("\r\nEV_READ_REMOTE_VERSION_INFORMATION_COMPLETE");
					break;

			case EV_NUM_COMPLETE_PKT:
				break;



			default:
				if (hcibuf_in[0] != 0x00) {
					Notify(PSTR("\r\nUnmanaged HCI Event: "), 0x80);
					D_PrintHex<uint8_t > (hcibuf_in[0], 0x80);
				}
				break;
		} // Switch
	} else {
		Notify(PSTR("\r\nHCI event error: "), 0x80);
		D_PrintHex<uint8_t > (rcode, 0x80);
	}
}








/* Poll Bluetooth and print result */
void BTD::HCI_task() {
	switch(hci_state) {




		case HCI_INIT_STATE:
			hci_counter++;
			if (hci_counter > hci_num_reset_loops) { // wait until we have looped x times to clear any old events
				hci_reset();
				hci_state = HCI_RESET_STATE;
				Notify("\r\nHCI_RESET_STATE");
				hci_counter = 0;
			}
			break;





		case HCI_RESET_STATE:
			hci_counter++;
			if (hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
				hci_counter = 0;
				Notify(PSTR("\r\nHCI Reset complete"), 0x80);
				hci_state = HCI_CLASS_STATE;
				Notify("\r\nHCI_CLASS_STATE");
				hci_write_class_of_device();

			} else if (hci_counter > hci_num_reset_loops) {
				hci_num_reset_loops *= 10;
				if (hci_num_reset_loops > 2000) {
					hci_num_reset_loops = 2000;
				}
				Notify(PSTR("\r\nNo response to HCI Reset"), 0x80);
				hci_state = HCI_INIT_STATE;
				Notify("\r\nHCI_INIT_STATE");
				hci_counter = 0;
			}
			break;





		case HCI_CLASS_STATE:
			if (hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
				Notify(PSTR("\r\nWrite class of device"), 0x80);
				hci_state = HCI_BDADDR_STATE;
				Notify("\r\nHCI_BDADDR_STATE");
				hci_read_bdaddr();
			}
			break;





		case HCI_BDADDR_STATE:
			if (hci_check_flag(HCI_FLAG_READ_BDADDR)) {

				Notify(PSTR("\r\nLocal Bluetooth Address: "), 0x80);
				for (int8_t i=5; i>0; i--) {
					D_PrintHex<uint8_t > (my_bdaddr[i], 0x80);
					Notify(PSTR(":"), 0x80);
				}
				D_PrintHex<uint8_t > (my_bdaddr[0], 0x80);

				hci_read_local_version_information();
				hci_state = HCI_CHECK_DEVICE_SERVICE;
				Notify("\r\nnHCI_CHECK_DEVICE_SERVICE");
			}
			break;






		case HCI_CHECK_DEVICE_SERVICE:
			if (pairWithWii) {
				Notify(PSTR("\r\nStarting inquiry - PRESS THE SYNC BUTTON Y0"), 0x80);
				hci_inquiry();
				hci_state = HCI_INQUIRY_STATE;
				Notify("\r\nHCI_INQUIRY_STATE");
			} else {
				hci_state = HCI_SCANNING_STATE; // Don't try to connect to a Wii Remote
				Notify("\r\nHCI_SCANNING_STATE");
			}
			break;





		case HCI_INQUIRY_STATE:
			if (hci_check_flag(HCI_FLAG_DEVICE_FOUND)) {
				hci_inquiry_cancel(); // Stop inquiry
				Notify(PSTR("\r\nWii Remote found"), 0x80);
				if (checkRemoteName) {
					hci_remote_name(); // We need to know the name to distinguish between the Wii Remote, the new Wii Remote with Motion Plus inside, a Wii U Pro Controller and a Wii Balance Board
					hci_state = HCI_REMOTE_NAME_STATE;
					Notify("\r\nHCI_REMOTE_NAME_STATE");
				} else {
					hci_state = HCI_CONNECT_DEVICE_STATE;
					Notify("\r\nHCI_CONNECT_DEVICE_STATE");
				}
			}
			break;





		case HCI_CONNECT_DEVICE_STATE:
			if (hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
				Notify(PSTR("\r\nConnecting to Wii Remote"), 0x80);
				checkRemoteName = false;
				hci_connect();
				hci_state = HCI_CONNECTED_DEVICE_STATE;
				Notify("\r\nHCI_CONNECTED_DEVICE_STATE");
			}
			break;





		case HCI_CONNECTED_DEVICE_STATE:
			if (hci_check_flag(HCI_FLAG_CONNECT_EVENT)) {
				if (hci_check_flag(HCI_FLAG_CONNECT_COMPLETE)) {
					Notify(PSTR("\r\nConnected to Wii Remote"), 0x80);
					hci_authentication_request(); // This will start the pairing with the Wii Remote
					hci_state = HCI_CHECK_DEVICE_SERVICE;
					Notify("\r\nHCI_CHECK_DEVICE_SERVICE");
				} else {
					Notify(PSTR("\r\nTrying to connect one more time..."), 0x80);
					hci_connect(); // Try to connect one more time
				}
			}
			break;





		case HCI_SCANNING_STATE:
			if (!connectToWii && !pairWithWii) {
				Notify(PSTR("\r\nWait For Incoming Connection Request"), 0x80);
				hci_write_scan_enable();
				hci_counter = 0;
				waitingForConnection = true;
				hci_state = HCI_CONNECT_IN_STATE;
				Notify("\r\nHCI_CONNECT_IN_STATE");
			}
			break;





		case HCI_CONNECT_IN_STATE:
			if (hci_check_flag(HCI_FLAG_INCOMING_REQUEST)) {
				waitingForConnection = false;
				Notify(PSTR("\r\nIncoming Connection Request"), 0x80);
				hci_remote_name();
				hci_counter = 0;
				hci_state = HCI_REMOTE_NAME_STATE;
				Notify("\r\nHCI_REMOTE_NAME_STATE");
			} else if (hci_check_flag(HCI_FLAG_DISCONNECT_COMPLETE)) {
				hci_counter = 0;
				hci_state = HCI_DISCONNECT_STATE;
				Notify("\r\nHCI_DISCONNECT_STATE");
			}
			break;





		case HCI_REMOTE_NAME_STATE:
			if (hci_check_flag(HCI_FLAG_REMOTE_NAME_COMPLETE)) {
				Notify(PSTR("\r\nRemote Name: "), 0x80);
				for (uint8_t i=0; i<strlen(remote_name); i++) {
					Notifyc(remote_name[i], 0x80);
				}

				if (strncmp((const char*)remote_name, "Nintendo", 8) == 0) {
					incomingWii = true;
					motionPlusInside = false;
					wiiUProController = false;
					Notify(PSTR("\r\nWii Remote is connecting"), 0x80);

					if (strncmp((const char*)remote_name, "Nintendo RVL-CNT-01-TR", 22) == 0) {
						Notify(PSTR(" with Motion Plus Inside"), 0x80);
						motionPlusInside = true;

					} else if (strncmp((const char*)remote_name, "Nintendo RVL-CNT-01-UC", 22) == 0) {
						Notify(PSTR(" - Wii U Pro Controller"), 0x80);
						wiiUProController = motionPlusInside = true;

					} else if (strncmp((const char*)remote_name, "Nintendo RVL-WBC-01", 19) == 0) {
						Notify(PSTR(" - Wii Balance Board"), 0x80);
					}
				}
				if (pairWithWii && checkRemoteName) {
					hci_state = HCI_CONNECT_DEVICE_STATE;
					Notify("\r\nHCI_CONNECT_DEVICE_STATE");
				} else {
					hci_accept_connection();
					hci_state = HCI_CONNECTED_STATE;
					Notify("\r\nHCI_CONNECTED_STATE");
				}
			}
			break;





		case HCI_CONNECTED_STATE:
			if (hci_check_flag(HCI_FLAG_CONNECT_COMPLETE)) {
				Notify(PSTR("\r\nConnected to Device: "), 0x80);
				for (int8_t i=5; i>0; i--) {
					D_PrintHex<uint8_t > (last_bdaddr[i], 0x80);
					Notify(PSTR(":"), 0x80);
				}
				D_PrintHex<uint8_t > (last_bdaddr[0], 0x80);

				// Clear these flags for a new connection
				l2capConnectionClaimed = false;
				sdpConnectionClaimed = false;
				rfcommConnectionClaimed = false;

				hci_event_flag = 0;
				hci_state = HCI_DONE_STATE;
				Notify("\r\nHCI_DONE_STATE");
			}
			break;





		case HCI_DONE_STATE:
			hci_counter++;
			if (hci_counter > 1000) { // Wait until we have looped 1000 times to make sure that the L2CAP connection has been started
				hci_counter = 0;
				hci_state = HCI_CHECK_DEVICE_SERVICE;
				Notify("\r\nHCI_CHECK_DEVICE_SERVICE");
			}
			break;





		case HCI_DISCONNECT_STATE:
			if (hci_check_flag(HCI_FLAG_DISCONNECT_COMPLETE)) {
				Notify(PSTR("\r\nHCI Disconnected from Device"), 0x80);

				// Reset all buffers
				memset(hcibuf_in,	0, sizeof(hcibuf_in));
				memset(l2c_in_raw,	0, sizeof(l2c_in_raw));

				hci_event_flag	= 0; // Clear all flags
				connectToWii	= false;
				incomingWii		= false;
				checkRemoteName	= false;

				hci_state = HCI_CHECK_DEVICE_SERVICE;
				Notify("\r\nHCI_CHECK_DEVICE_SERVICE");
			}
			break;

	}
}






void BTD::ACL_event_task() {
	uint16_t	length	= sizeof(l2c_in_raw);
	uint8_t		rcode	= pUsb->inTransfer(
							bAddress,
							epInfo[BTD_DATAIN_PIPE].epAddr,
							&length,
							l2c_in_raw,
							pollInterval
						);

	if (!rcode  &&  length) { // Check for errors
		for (uint8_t i=0; i<BTD_NUM_SERVICES; i++) {
			if (btService[i]) btService[i]->ACLData(l2c_in_raw);
		}
	}

	else if (rcode != hrNAK) {
		Notify(PSTR("\r\nACL data in error: "), 0x80);
		D_PrintHex<uint8_t > (rcode, 0x80);
	}

	for (uint8_t i=0; i<BTD_NUM_SERVICES; i++) {
		if (btService[i]) btService[i]->Run();
	}
}






/************************************************************/
/*                    HCI Commands                          */
/************************************************************/
void BTD::HCI_Command(uint8_t* data, uint16_t nbytes) {
	hci_clear_flag(HCI_FLAG_CMD_COMPLETE);

	pUsb->ctrlReq(
		bAddress,
		epInfo[ BTD_CONTROL_PIPE ].epAddr,
		bmREQ_HCI_OUT,
		0x00,
		0x00,
		0x00,
		0x00,
		nbytes,
		nbytes,
		data,
		NULL
	);
}




void BTD::hci_reset() {
	hci_event_flag = 0; // Clear all the flags
	hcibuf_out[0] = 0x03; // HCI OCF = 3
	hcibuf_out[1] = 0x03 << 2; // HCI OGF = 3
	hcibuf_out[2] = 0x00;

	HCI_Command(hcibuf_out, 3);
}




void BTD::hci_write_scan_enable() {
	hci_clear_flag(HCI_FLAG_INCOMING_REQUEST);
	hcibuf_out[0] = 0x1A; // HCI OCF = 1A
	hcibuf_out[1] = 0x03 << 2; // HCI OGF = 3
	hcibuf_out[2] = 0x01; // parameter length = 1
	hcibuf_out[3] = 0x03; // Inquiry Scan enabled. Page Scan enabled.

	HCI_Command(hcibuf_out, 4);
}




void BTD::hci_write_scan_disable() {
	hcibuf_out[0] = 0x1A; // HCI OCF = 1A
	hcibuf_out[1] = 0x03 << 2; // HCI OGF = 3
	hcibuf_out[2] = 0x01; // parameter length = 1
	hcibuf_out[3] = 0x00; // Inquiry Scan disabled. Page Scan disabled.

	HCI_Command(hcibuf_out, 4);
}




void BTD::hci_read_bdaddr() {
	hci_clear_flag(HCI_FLAG_READ_BDADDR);
	hcibuf_out[0] = 0x09; // HCI OCF = 9
	hcibuf_out[1] = 0x04 << 2; // HCI OGF = 4
	hcibuf_out[2] = 0x00;

	HCI_Command(hcibuf_out, 3);
}




void BTD::hci_read_local_version_information() {
	hci_clear_flag(HCI_FLAG_READ_VERSION);
	hcibuf_out[0] = 0x01; // HCI OCF = 1
	hcibuf_out[1] = 0x04 << 2; // HCI OGF = 4
	hcibuf_out[2] = 0x00;

	HCI_Command(hcibuf_out, 3);
}




void BTD::hci_accept_connection() {
	hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE);
	hcibuf_out[0] = 0x09; // HCI OCF = 9
	hcibuf_out[1] = 0x01 << 2; // HCI OGF = 1
	hcibuf_out[2] = 0x07; // parameter length 7
	hcibuf_out[3] = last_bdaddr[0]; // 6 octet bdaddr
	hcibuf_out[4] = last_bdaddr[1];
	hcibuf_out[5] = last_bdaddr[2];
	hcibuf_out[6] = last_bdaddr[3];
	hcibuf_out[7] = last_bdaddr[4];
	hcibuf_out[8] = last_bdaddr[5];
	hcibuf_out[9] = 0x00; // Switch role to master

	HCI_Command(hcibuf_out, 10);
}




void BTD::hci_remote_name() {
	hci_clear_flag(HCI_FLAG_REMOTE_NAME_COMPLETE);
	hcibuf_out[0] = 0x19; // HCI OCF = 19
	hcibuf_out[1] = 0x01 << 2; // HCI OGF = 1
	hcibuf_out[2] = 0x0A; // parameter length = 10
	hcibuf_out[3] = last_bdaddr[0]; // 6 octet bdaddr
	hcibuf_out[4] = last_bdaddr[1];
	hcibuf_out[5] = last_bdaddr[2];
	hcibuf_out[6] = last_bdaddr[3];
	hcibuf_out[7] = last_bdaddr[4];
	hcibuf_out[8] = last_bdaddr[5];
	hcibuf_out[9] = 0x01; // Page Scan Repetition Mode
	hcibuf_out[10] = 0x00; // Reserved
	hcibuf_out[11] = 0x00; // Clock offset - low byte
	hcibuf_out[12] = 0x00; // Clock offset - high byte

	HCI_Command(hcibuf_out, 13);
}




void BTD::hci_set_local_name(const char* name) {
	const uint16_t len = strlen(name);

	hcibuf_out[0] = 0x13; // HCI OCF = 13
	hcibuf_out[1] = 0x03 << 2; // HCI OGF = 3
	hcibuf_out[2] = len+1; // parameter length = the length of the string + end byte

	uint16_t i=0;
	for (; i<len; i++) {
		hcibuf_out[i+3] = name[i];
	}

	hcibuf_out[i+3] = '\0'; // End of string

	HCI_Command(hcibuf_out, len+4);
}




void BTD::hci_inquiry(bool general) {
	hci_clear_flag(HCI_FLAG_DEVICE_FOUND);

	// LAP: GIAC/LIAC - General/Limited Inquery Access Code
	const uint8_t access = (general ? 0x33 : 0x00);

	HCI_COMMAND(
		0x01,
		0x01 << 2,		// HCI OGF = 1
		0x05,			// Parameter Total Length = 5
		access,
		0x8B,
		0x9E,
		0x30,			// Inquiry time = 61.44 sec (maximum)
		0x0A			// 10 number of responses
	);
}




void BTD::hci_inquiry_cancel() {
	HCI_COMMAND(
		0x02,
		0x01 << 2,		// HCI OGF = 1
		0x00			// Parameter Total Length = 0
	);
}




void BTD::hci_connect(uint8_t *bdaddr) {
	hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE | HCI_FLAG_CONNECT_EVENT);

	HCI_COMMAND(
		0x05,
		0x01 << 2,		// HCI OGF = 1
		0x0D,			// parameter Total Length = 13
		bdaddr[0],		// 6 octet bdaddr (LSB)
		bdaddr[1],
		bdaddr[2],
		bdaddr[3],
		bdaddr[4],
		bdaddr[5],
		0x18,			// DM1 or DH1 may be used
		0xCC,			// DM3, DH3, DM5, DH5 may be used
		0x01,			// Page repetition mode R1
		0x00,			// Reserved
		0x00,			// Clock offset
		0x00,			// Invalid clock offset
		0x00			// Do not allow role switch
	);
}




void BTD::hci_pin_code_request_reply() {
	hcibuf_out[0] = 0x0D; // HCI OCF = 0D
	hcibuf_out[1] = 0x01 << 2; // HCI OGF = 1
	hcibuf_out[2] = 0x17; // parameter length 23
	hcibuf_out[3] = last_bdaddr[0]; // 6 octet bdaddr
	hcibuf_out[4] = last_bdaddr[1];
	hcibuf_out[5] = last_bdaddr[2];
	hcibuf_out[6] = last_bdaddr[3];
	hcibuf_out[7] = last_bdaddr[4];
	hcibuf_out[8] = last_bdaddr[5];

	hcibuf_out[9] = 6; // Pin length is the length of the Bluetooth address

	Notify(PSTR("\r\nPairing with Wii controller via SYNC"), 0x80);

	for (uint8_t i=0; i<6; i++) {
		hcibuf_out[10 + i] = my_bdaddr[i]; // The pin is the Bluetooth dongles Bluetooth address backwards
	}

	for (uint8_t i = 16; i<26; i++) {
		hcibuf_out[i] = 0x00; // The rest should be 0
	}

	HCI_Command(hcibuf_out, 26);
}




void BTD::hci_pin_code_negative_request_reply() {
	hcibuf_out[0] = 0x0E; // HCI OCF = 0E
	hcibuf_out[1] = 0x01 << 2; // HCI OGF = 1
	hcibuf_out[2] = 0x06; // parameter length 6
	hcibuf_out[3] = last_bdaddr[0]; // 6 octet bdaddr
	hcibuf_out[4] = last_bdaddr[1];
	hcibuf_out[5] = last_bdaddr[2];
	hcibuf_out[6] = last_bdaddr[3];
	hcibuf_out[7] = last_bdaddr[4];
	hcibuf_out[8] = last_bdaddr[5];

	HCI_Command(hcibuf_out, 9);
}




void BTD::hci_link_key_request_negative_reply() {
	hcibuf_out[0] = 0x0C; // HCI OCF = 0C
	hcibuf_out[1] = 0x01 << 2; // HCI OGF = 1
	hcibuf_out[2] = 0x06; // parameter length 6
	hcibuf_out[3] = last_bdaddr[0]; // 6 octet bdaddr
	hcibuf_out[4] = last_bdaddr[1];
	hcibuf_out[5] = last_bdaddr[2];
	hcibuf_out[6] = last_bdaddr[3];
	hcibuf_out[7] = last_bdaddr[4];
	hcibuf_out[8] = last_bdaddr[5];

	HCI_Command(hcibuf_out, 9);
}




void BTD::hci_authentication_request() {
	hcibuf_out[0] = 0x11; // HCI OCF = 11
	hcibuf_out[1] = 0x01 << 2; // HCI OGF = 1
	hcibuf_out[2] = 0x02; // parameter length = 2
	hcibuf_out[3] = (uint8_t)(hci_handle & 0xFF); //connection handle - low byte
	hcibuf_out[4] = (uint8_t)((hci_handle >> 8) & 0x0F); //connection handle - high byte

	HCI_Command(hcibuf_out, 5);
}




void BTD::hci_disconnect(uint16_t handle) { // This is called by the different services
	hci_clear_flag(HCI_FLAG_DISCONNECT_COMPLETE);
	hcibuf_out[0] = 0x06; // HCI OCF = 6
	hcibuf_out[1] = 0x01 << 2; // HCI OGF = 1
	hcibuf_out[2] = 0x03; // parameter length = 3
	hcibuf_out[3] = (uint8_t)(handle & 0xFF); //connection handle - low byte
	hcibuf_out[4] = (uint8_t)((handle >> 8) & 0x0F); //connection handle - high byte
	hcibuf_out[5] = 0x13; // reason

	HCI_Command(hcibuf_out, 6);
}




void BTD::hci_write_class_of_device() { // See http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html
	hcibuf_out[0] = 0x24; // HCI OCF = 24
	hcibuf_out[1] = 0x03 << 2; // HCI OGF = 3
	hcibuf_out[2] = 0x03; // parameter length = 3
	hcibuf_out[3] = 0x04; // Robot
	hcibuf_out[4] = 0x08; // Toy
	hcibuf_out[5] = 0x00;

	HCI_Command(hcibuf_out, 6);
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


void BTD::l2cap_raw(uint16_t handle, uint16_t channel) {
	l2c_out.hci_handle	= handle | 0x2000;
	l2c_out.hci_length	= l2c_out.l2c_length + 4;
	l2c_out.l2c_channel	= channel;

#ifndef EXTRADEBUG

	pUsb->outTransfer(
		bAddress,
		epInfo[BTD_DATAOUT_PIPE].epAddr,
		l2c_out.hci_length + 4,
		l2c_out_raw
	);

#else //EXTRADEBUG

	Serial.print("\r\n L2CAP COMMAND : ");
	char output[4] = {0,0,0,0};
	for (int i=0; i<l2c_out.hci_length+4; i++) {
		sprintf(output, "%02x:", l2c_out_raw[i]);
		Serial.print(output);
	}

	uint8_t rcode = pUsb->outTransfer(
		bAddress,
		epInfo[BTD_DATAOUT_PIPE].epAddr,
		l2c_out.hci_length + 4,
		l2c_out_raw
	);

	if (rcode) {
		delay(100); // This small delay prevents it from overflowing if it fails
		Notify(PSTR("\r\nError sending L2CAP message: 0x"), 0x80);
		D_PrintHex<uint8_t > (rcode, 0x80);
		Notify(PSTR(" - Channel ID: "), 0x80);
		D_PrintHex<uint16_t > (l2c_out.l2c_channel, 0x80);
	}
#endif //EXTRADEBUG
}




void BTD::L2CAP_Command(uint16_t handle, const void *data, uint8_t nbytes, uint16_t channel) {
	l2c_out.l2c_length	= nbytes;

	const uint8_t *data_ptr = (const uint8_t*) data;

	for (uint16_t i=0; i<nbytes; i++) { // L2CAP C-frame
		l2c_out.l2c_payload[i] = data_ptr[i];
	}

	l2cap_raw(handle, channel);
}




void BTD::l2cap_connection_request(uint16_t handle, uint8_t rxid, uint16_t scid, uint16_t psm) {
	l2c_out.l2c_length		= 8;
	l2c_out.l2c_command		= L2CAP_CMD_CONNECTION_REQUEST;
	l2c_out.identifier		= rxid;
	l2c_out.data_length		= l2c_out.l2c_length - 4;
	l2c_out.hid_control		= psm;
	l2c_out.hid_channel		= scid;
	l2cap_raw(handle);
}




void BTD::l2cap_connection_response(uint16_t handle, uint8_t rxid, uint16_t dcid, uint16_t scid, uint8_t result) {
	l2c_out.l2c_length		= 12;
	l2c_out.l2c_command		= L2CAP_CMD_CONNECTION_RESPONSE;
	l2c_out.identifier		= rxid;
	l2c_out.data_length		= l2c_out.l2c_length - 4;
	l2c_out.hid_control		= dcid;
	l2c_out.hid_channel		= scid;
	l2c_out.hid_result		= result;
	l2c_out.hid_status		= 0x0000;
	l2cap_raw(handle);
}




void BTD::l2cap_config_request(uint16_t handle, uint8_t rxid, uint16_t dcid) {
	l2c_out.l2c_length		= 12;
	l2c_out.l2c_command		= L2CAP_CMD_CONFIG_REQUEST;
	l2c_out.identifier		= rxid;
	l2c_out.data_length		= l2c_out.l2c_length - 4;
	l2c_out.hid_control		= dcid;
	l2c_out.hid_flags		= 0x0000;
	l2c_out.hid_result		= 0x0201;
	l2c_out.hid_status		= 0xFFFF;
	l2cap_raw(handle);
//		0x01, // Config Opt: type = MTU (Maximum Transmission Unit) - Hint
//		0x02, // Config Opt: length
}




void BTD::l2cap_config_response(uint16_t handle, uint8_t rxid, uint16_t scid) {
	l2c_out.l2c_length		= 14;
	l2c_out.l2c_command		= L2CAP_CMD_CONFIG_RESPONSE;
	l2c_out.identifier		= rxid;
	l2c_out.data_length		= l2c_out.l2c_length - 4;
	l2c_out.hid_control		= scid;
	l2c_out.hid_flags		= 0x0000;
	l2c_out.hid_result		= 0x0000;
	l2c_out.hid_status		= 0x0201;
	l2c_out.hid_config		= 0x02A0;
	l2cap_raw(handle);
}




void BTD::l2cap_disconnection_request(uint16_t handle, uint8_t rxid, uint16_b dcid, uint16_b scid) {
	L2CAP_COMMAND(
		handle,
		L2CAP_CMD_DISCONNECT_REQUEST, // Code
		rxid, // Identifier
		0x04, // Length
		0x00,
		dcid.byte_0,
		dcid.byte_1,
		scid.byte_0,
		scid.byte_1,
	);
}




void BTD::l2cap_disconnection_response(uint16_t handle, uint8_t rxid, uint16_b dcid, uint16_b scid) {
	L2CAP_COMMAND(
		handle,
		L2CAP_CMD_DISCONNECT_RESPONSE, // Code
		rxid, // Identifier
		0x04, // Length
		0x00,
		dcid.byte_0,
		dcid.byte_1,
		scid.byte_0,
		scid.byte_1,
	);
}




void BTD::l2cap_information_response(uint16_t handle, uint8_t rxid, uint8_t infoTypeLow, uint8_t infoTypeHigh) {
	L2CAP_COMMAND(
		handle,
		L2CAP_CMD_INFORMATION_RESPONSE, // Code
		rxid, // Identifier
		0x08, // Length
		0x00,
		infoTypeLow,
		infoTypeHigh,
		0x00, // Result = success
		0x00, // Result = success
		0x00,
		0x00,
		0x00,
		0x00,
	);
}
