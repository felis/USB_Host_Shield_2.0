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

#include "RFCOMM.h"
#define DEBUG // Uncomment to print data for debugging
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report sent to the Arduino

const uint8_t RFCOMM::BTD_EVENT_PIPE  = 1;			
const uint8_t RFCOMM::BTD_DATAIN_PIPE = 2;
const uint8_t RFCOMM::BTD_DATAOUT_PIPE = 3;

/*  
 * CRC (reversed crc) lookup table as calculated by the table generator in ETSI TS 101 369 V6.3.0.
 */
const uint8_t rfcomm_crc_table[256] PROGMEM = {    /* reversed, 8-bit, poly=0x07 */
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75, 0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69, 0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D, 0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51, 0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05, 0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19, 0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D, 0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21, 0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95, 0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89, 0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD, 0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1, 0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5, 0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9, 0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD, 0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1, 0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF
};

RFCOMM::RFCOMM(USB *p, const char* name, const char* pin):
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0), // device address - mandatory
bNumEP(1), // if config descriptor needs to be parsed
qNextPollTime(0),
bPollEnable(false) // don't start polling before dongle is connected
{
    for(uint8_t i=0; i<BTD_MAX_ENDPOINTS; i++)
	{
		epInfo[i].epAddr		= 0;
		epInfo[i].maxPktSize	= (i) ? 0 : 8;
		epInfo[i].epAttribs		= 0;        
        epInfo[i].bmNakPower    = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
	}
    
    if (pUsb) // register in USB subsystem
		pUsb->RegisterDeviceClass(this); //set devConfig[] entry
    
    btdName = name;
    btdPin = pin;    
}

uint8_t RFCOMM::Init(uint8_t parent, uint8_t port, bool lowspeed)
{
    uint8_t	buf[sizeof(USB_DEVICE_DESCRIPTOR)];
	uint8_t	rcode;
	UsbDevice *p = NULL;
	EpInfo *oldep_ptr = NULL;  
    uint8_t	num_of_conf; // number of configurations
    
    // get memory address of USB device address pool
	AddressPool	&addrPool = pUsb->GetAddressPool();    
#ifdef EXTRADEBUG
	Notify(PSTR("\r\nRFCOMM Init"));
#endif
    // check if address has already been assigned to an instance
    if (bAddress) 
    {
#ifdef DEBUG
        Notify(PSTR("\r\nAddress in use"));
#endif
        return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
    }
    
    // Get pointer to pseudo device with address 0 assigned
    p = addrPool.GetUsbDevicePtr(0);
    
    if (!p) 
    {        
#ifdef DEBUG
	    Notify(PSTR("\r\nAddress not found"));
#endif
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }
    
    if (!p->epinfo) 
    {
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
    
    // Allocate new address according to device class
    bAddress = addrPool.AllocAddress(parent, false, port);
    
    if (!bAddress)
		return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
    
    // Extract Max Packet Size from device descriptor
    epInfo[0].maxPktSize = (uint8_t)((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0; 
    
    // Assign new address to the device
    rcode = pUsb->setAddr( 0, 0, bAddress );
    if (rcode) 
    {
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
    
    num_of_conf = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;

    // check if attached device is a Bluetooth dongle and fill endpoint data structure
    // first interface in the configuration must have Bluetooth assigned Class/Subclass/Protocol
    // and 3 endpoints - interrupt-IN, bulk-IN, bulk-OUT,
    // not necessarily in this order
    for (uint8_t i=0; i<num_of_conf; i++) {
        ConfigDescParser<USB_CLASS_WIRELESS_CTRL, WI_SUBCLASS_RF, WI_PROTOCOL_BT, CP_MASK_COMPARE_ALL> confDescrParser(this);
        rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
        if(rcode)
            goto FailGetConfDescr;
        if(bNumEP > 3) //all endpoints extracted
            break;
    }
    
    if (bNumEP < BTD_MAX_ENDPOINTS)
        goto FailUnknownDevice;
        
    // Assign epInfo to epinfo pointer - this time all 3 endpoins
    rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);
    if(rcode)
        goto FailSetDevTblEntry;        
        
    delay(200); // Give time for address change        
    
    // Set Configuration Value
    rcode = pUsb->setConf(bAddress, epInfo[ BTD_CONTROL_PIPE ].epAddr, bConfNum);
    if(rcode) 
        goto FailSetConf;        
    
    /* Set device cid for the SDP and RFCOMM channelse */
    sdp_dcid[0] = 0x50;//0x0050
    sdp_dcid[1] = 0x00;        
    rfcomm_dcid[0] = 0x51;//0x0051
    rfcomm_dcid[1] = 0x00;
    
    hci_num_reset_loops = 100; // only loop 100 times before trying to send the hci reset command
        
    hci_state = HCI_INIT_STATE;
    hci_counter = 0;        
    l2cap_sdp_state = L2CAP_SDP_WAIT;
    l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;
#ifdef DEBUG
    Notify(PSTR("\r\nBluetooth Dongle Initialized"));
#endif
        
    watingForConnection = false;
    bPollEnable = true;

    return 0; //successful configuration
    
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
FailGetConfDescr:
#ifdef DEBUG
    Notify(PSTR("\r\ngetConf:"));
#endif
    goto Fail;
FailSetConf:
#ifdef DEBUG
    Notify(PSTR("\r\nsetConf:"));
#endif
    goto Fail;
FailUnknownDevice:
#ifdef DEBUG
    Notify(PSTR("\r\nUnknown Device Connected:"));
#endif
    goto Fail;
Fail:
#ifdef DEBUG
    Notify(PSTR("\r\nBTD Init Failed, error code: "));
    Serial.print(rcode);                     
#endif    
    Release();
    return rcode;
}
/* Extracts interrupt-IN, bulk-IN, bulk-OUT endpoint information from config descriptor */
void RFCOMM::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) {
	//ErrorMessage<uint8_t>(PSTR("Conf.Val"),conf);
	//ErrorMessage<uint8_t>(PSTR("Iface Num"),iface);
	//ErrorMessage<uint8_t>(PSTR("Alt.Set"),alt);
	
	if(alt) // wrong interface - by BT spec, no alt setting 
        return;
    
    bConfNum = conf;    
	uint8_t index;
    
    if ((pep->bmAttributes & 0x03) == 3 && (pep->bEndpointAddress & 0x80) == 0x80) //Interrupt In endpoint found
		index = BTD_EVENT_PIPE;
    
    else {
        if ((pep->bmAttributes & 0x02) == 2) //bulk endpoint found 
			index = ((pep->bEndpointAddress & 0x80) == 0x80) ? BTD_DATAIN_PIPE : BTD_DATAOUT_PIPE;
        else
            return;
    }
    
    //Fill the rest of endpoint data structure  
    epInfo[index].epAddr		= (pep->bEndpointAddress & 0x0F);
    epInfo[index].maxPktSize	= (uint8_t)pep->wMaxPacketSize;  
#ifdef EXTRADEBUG
    PrintEndpointDescriptor(pep);    
#endif
    if(pollInterval < pep->bInterval) // Set the polling interval as the largest polling interval obtained from endpoints
        pollInterval = pep->bInterval;   
    bNumEP++;
    return;
}
void RFCOMM::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr) {
	Notify(PSTR("\r\nEndpoint descriptor:"));
	Notify(PSTR("\r\nLength:\t\t"));
	PrintHex<uint8_t>(ep_ptr->bLength);
	Notify(PSTR("\r\nType:\t\t"));
	PrintHex<uint8_t>(ep_ptr->bDescriptorType);
	Notify(PSTR("\r\nAddress:\t"));
	PrintHex<uint8_t>(ep_ptr->bEndpointAddress);
	Notify(PSTR("\r\nAttributes:\t"));
	PrintHex<uint8_t>(ep_ptr->bmAttributes);
	Notify(PSTR("\r\nMaxPktSize:\t"));
	PrintHex<uint16_t>(ep_ptr->wMaxPacketSize);
	Notify(PSTR("\r\nPoll Intrv:\t"));
	PrintHex<uint8_t>(ep_ptr->bInterval);
}

/* Performs a cleanup after failed Init() attempt */
uint8_t RFCOMM::Release() {
    connected = false;
	pUsb->GetAddressPool().FreeAddress(bAddress);    
	bAddress = 0;
    bPollEnable = false;
    bNumEP = 1; // must have to be reset to 1	
	return 0;
}
uint8_t RFCOMM::Poll() {    
	if (!bPollEnable)
		return 0;
    if (qNextPollTime <= millis()) { // Don't poll if shorter than polling interval
        qNextPollTime = millis() + pollInterval; // Set new poll time
        HCI_event_task(); // poll the HCI event pipe
        ACL_event_task(); // start polling the ACL input pipe too, though discard data until connected        
    }    
	return 0;
}

void RFCOMM::disconnect() { // Use this void to disconnect the RFCOMM Channel
    connected = false;    
    // First the two L2CAP channels has to be disconencted and then the HCI connection
    if(RFCOMMConnected)
        l2cap_disconnection_request(0x0A, rfcomm_dcid, rfcomm_scid);
    if(SDPConnected)
        l2cap_disconnection_request(0x0B, sdp_dcid, sdp_scid);            
    l2cap_sdp_state = L2CAP_DISCONNECT_RESPONSE;
}

void RFCOMM::HCI_event_task() {
    /* check the event pipe*/    
    uint16_t MAX_BUFFER_SIZE = BULK_MAXPKTSIZE; // Request more than 16 bytes anyway, the inTransfer routine will take care of this
    uint8_t rcode = pUsb->inTransfer(bAddress, epInfo[ BTD_EVENT_PIPE ].epAddr, &MAX_BUFFER_SIZE, hcibuf); // input on endpoint 1
    if(!rcode || rcode == hrNAK) // Check for errors
    {
        switch (hcibuf[0]) //switch on event type
        {
            case EV_COMMAND_COMPLETE:
                hci_event_flag |= HCI_FLAG_CMD_COMPLETE; // set command complete flag
                if (!hcibuf[5]) { // check if command succeeded                                       
                    if((hcibuf[3] == 0x09) && (hcibuf[4] == 0x10)) { // parameters from read local bluetooth address  
                        for (uint8_t i = 0; i < 6; i++) 
                            my_bdaddr[i] = hcibuf[6 + i];
                        hci_event_flag |= HCI_FLAG_READ_BDADDR;
                    }
                }
                break;
                
            case EV_COMMAND_STATUS:
                if(hcibuf[2]) // show status on serial if not OK 
                {    
#ifdef DEBUG
                    Notify(PSTR("\r\nHCI Command Failed: "));
                    PrintHex<uint8_t>(hcibuf[2]);
                    Notify(PSTR(" ")); 
                    PrintHex<uint8_t>(hcibuf[4]);
                    Notify(PSTR(" "));
                    PrintHex<uint8_t>(hcibuf[5]);                    
#endif
                }
                break;
                
            case EV_CONNECT_COMPLETE:
                if (!hcibuf[2]) { // check if connected OK
                    hci_handle = hcibuf[3] | hcibuf[4] << 8; //store the handle for the ACL connection
                    hci_event_flag |= HCI_FLAG_CONN_COMPLETE; // set connection complete flag
                }
                break;
                
            case EV_DISCONNECT_COMPLETE:            
                if (!hcibuf[2]) { // check if disconnected OK
                    hci_event_flag |= HCI_FLAG_DISCONN_COMPLETE; //set disconnect commend complete flag
                    hci_event_flag &= ~HCI_FLAG_CONN_COMPLETE; // clear connection complete flag
                }
                break;                              
                
            case EV_REMOTE_NAME_COMPLETE:
                if (!hcibuf[2]) { // check if reading is OK
                    for (uint8_t i = 0; i < 30; i++)
                        remote_name[i] = hcibuf[9 + i];  //store first 30 bytes
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
                hci_event_flag |= HCI_FLAG_INCOMING_REQUEST;
                break;
                
            case EV_PIN_CODE_REQUEST:
#ifdef DEBUG
                Notify(PSTR("\r\nBluetooth pin is set too: "));
                Serial.print(btdPin);
#endif
                hci_pin_code_request_reply(btdPin);                                   
                break;
                
            case EV_LINK_KEY_REQUEST:
#ifdef DEBUG
                Notify(PSTR("\r\nReceived Key Request"));
#endif
                hci_link_key_request_negative_reply();
                break;
                
            /* We will just ignore the following events */                
            case EV_NUM_COMPLETE_PKT:                
            case EV_ROLE_CHANGED:                
            case EV_PAGE_SCAN_REP_MODE:                
            case EV_LOOPBACK_COMMAND:                
            case EV_DATA_BUFFER_OVERFLOW:                
            case EV_CHANGE_CONNECTION_LINK:                
            case EV_AUTHENTICATION_COMPLETE:                
            case EV_MAX_SLOTS_CHANGE:
            case EV_QOS_SETUP_COMPLETE:
            case EV_LINK_KEY_NOTIFICATION:
            case EV_ENCRYPTION_CHANGE:
            case EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE:
                break;
                
            default:
#ifdef EXTRADEBUG
                if(hcibuf[0] != 0x00) {
                    Notify(PSTR("\r\nUnmanaged HCI Event: "));
                    PrintHex<uint8_t>(hcibuf[0]);
                }
#endif
                break;    
        } // switch
        HCI_task();
    }
    else {
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nHCI event error: "));
        PrintHex<uint8_t>(rcode);
#endif
    }
}

/* Poll Bluetooth and print result */
void RFCOMM::HCI_task() {
    switch (hci_state){
        case HCI_INIT_STATE:
            hci_counter++;
            if (hci_counter > hci_num_reset_loops) // wait until we have looped x times to clear any old events
            {  
                hci_reset();
                hci_state = HCI_RESET_STATE;
                hci_counter = 0;
            }
            break;
            
        case HCI_RESET_STATE:
            hci_counter++;
            if (hci_cmd_complete)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nHCI Reset complete"));
#endif
                hci_state = HCI_BDADDR_STATE;
                hci_read_bdaddr(); 
            }
            else if (hci_counter > hci_num_reset_loops) 
            {
                hci_num_reset_loops *= 10;
                if(hci_num_reset_loops > 2000)
                    hci_num_reset_loops = 2000;
#ifdef DEBUG
                Notify(PSTR("\r\nNo response to HCI Reset"));
#endif
                hci_state = HCI_INIT_STATE;
                hci_counter = 0;
            }
            break;
            
        case HCI_BDADDR_STATE:
            if (hci_read_bdaddr_complete)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nLocal Bluetooth Address: "));
                for(int8_t i = 5; i > 0;i--)
                {
                    PrintHex<uint8_t>(my_bdaddr[i]); 
                    Serial.print(":");
                }      
                PrintHex<uint8_t>(my_bdaddr[0]);
#endif
                hci_set_local_name(btdName);
                hci_state = HCI_SET_NAME_STATE;
            }
            break;
            
        case HCI_SET_NAME_STATE:
            if (hci_cmd_complete) {
#ifdef DEBUG               
                Notify(PSTR("\r\nThe name is set to: "));
                Serial.print(btdName);
#endif
                hci_state = HCI_SCANNING_STATE;                
            }
            break;
        case HCI_SCANNING_STATE:
#ifdef DEBUG
            Notify(PSTR("\r\nWait For Incoming Connection Request"));
#endif            
            hci_write_scan_enable();
            watingForConnection = true;
            hci_state = HCI_CONNECT_IN_STATE;
            break;
            
        case HCI_CONNECT_IN_STATE:
            if(hci_incoming_connect_request) {
                watingForConnection = false;
#ifdef DEBUG
                Notify(PSTR("\r\nIncoming Request"));                
#endif                
                hci_remote_name();
                hci_state = HCI_REMOTE_NAME_STATE;
            }
            break;     
            
        case HCI_REMOTE_NAME_STATE:
            if(hci_remote_name_complete) {
#ifdef DEBUG
                Notify(PSTR("\r\nRemote Name: "));
                for (uint8_t i = 0; i < 30; i++)        
                {
                    if(remote_name[i] == NULL)
                        break;
                    Serial.write(remote_name[i]);   
                }             
#endif
                hci_accept_connection();
                hci_state = HCI_CONNECTED_STATE;                                
            }      
            break;
            
        case HCI_CONNECTED_STATE:
            if (hci_connect_complete) {     
#ifdef DEBUG
                Notify(PSTR("\r\nConnected to Device: "));
                for(int8_t i = 5; i>0;i--)
                {
                    PrintHex<uint8_t>(disc_bdaddr[i]);
                    Serial.print(":");
                }      
                PrintHex<uint8_t>(disc_bdaddr[0]);
#endif
                hci_write_scan_disable(); // Only allow one connection
                hci_state = HCI_DISABLE_SCAN;
            }
            break;
            
        case HCI_DISABLE_SCAN:
            if (hci_cmd_complete) {                    
#ifdef DEBUG
                Notify(PSTR("\r\nScan Disabled"));
#endif
                l2cap_event_flag = 0;
                hci_state = HCI_DONE_STATE;
            }
            break;
            
        case HCI_DONE_STATE:
            if (hci_disconnect_complete)
                hci_state = HCI_DISCONNECT_STATE;
            break;
            
        case HCI_DISCONNECT_STATE:
            if (hci_disconnect_complete) {
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected from Device: "));                
                for(int8_t i = 5; i>0;i--)
                {
                    PrintHex<uint8_t>(disc_bdaddr[i]); 
                    Serial.print(":");
                }      
                PrintHex<uint8_t>(disc_bdaddr[0]);
#endif
                l2cap_event_flag = 0; // Clear all flags
                hci_event_flag = 0; // Clear all flags 
                
                //Reset all buffers                        
                for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++)
                    hcibuf[i] = 0;        
                for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++)
                    l2capinbuf[i] = 0;
                for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++)
                    l2capoutbuf[i] = 0;
                for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++)
                    rfcommbuf[i] = 0;
                
                l2cap_sdp_state = L2CAP_SDP_WAIT;                        
                l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;                                        
                hci_state = HCI_SCANNING_STATE;
            }
            break;
        default:
            break;
    }
}

void RFCOMM::ACL_event_task()
{
    uint16_t MAX_BUFFER_SIZE = BULK_MAXPKTSIZE;    
    uint8_t rcode = pUsb->inTransfer(bAddress, epInfo[ BTD_DATAIN_PIPE ].epAddr, &MAX_BUFFER_SIZE, l2capinbuf); // input on endpoint 2
    if(!rcode) { // Check for errors
        if (((l2capinbuf[0] | (l2capinbuf[1] << 8)) == (hci_handle | 0x2000))) { //acl_handle_ok  
            if ((l2capinbuf[6] | (l2capinbuf[7] << 8)) == 0x0001) { //l2cap_control - Channel ID for ACL-U
                /*
                 if (l2capinbuf[8] != 0x00)
                 {
                 Serial.print("\r\nL2CAP Signaling Command - 0x"); 
                 PrintHex<uint8_t>(l2capoutbuf[8]);                
                 }
                 */
                if (l2capinbuf[8] == L2CAP_CMD_COMMAND_REJECT) {
#ifdef DEBUG
                    Notify(PSTR("\r\nL2CAP Command Rejected - Reason: "));
                    PrintHex<uint8_t>(l2capinbuf[13]);
                    Notify(PSTR(" "));
                    PrintHex<uint8_t>(l2capinbuf[12]);
                    Notify(PSTR(" Data: "));
                    PrintHex<uint8_t>(l2capinbuf[17]);
                    Notify(PSTR(" "));
                    PrintHex<uint8_t>(l2capinbuf[16]);
                    Notify(PSTR(" "));                
                    PrintHex<uint8_t>(l2capinbuf[15]);
                    Notify(PSTR(" "));
                    PrintHex<uint8_t>(l2capinbuf[14]);     
#endif
                }
                else if (l2capinbuf[8] == L2CAP_CMD_CONNECTION_REQUEST) {     
#ifdef EXTRADEBUG
                    Notify(PSTR("\r\nL2CAP Connection Request - PSM: "));                
                    PrintHex<uint8_t>(l2capinbuf[13]);
                    Notify(PSTR(" "));
                    PrintHex<uint8_t>(l2capinbuf[12]);
                    Notify(PSTR(" "));
                     
                    Notify(PSTR(" SCID: "));
                    PrintHex<uint8_t>(l2capinbuf[15]);
                    Notify(PSTR(" "));
                    PrintHex<uint8_t>(l2capinbuf[14]);
                     
                    Notify(PSTR(" Identifier: "));
                    PrintHex<uint8_t>(l2capinbuf[9]);
#endif                    
                    if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == SDP_PSM) {
                        identifier = l2capinbuf[9];
                        sdp_scid[0] = l2capinbuf[14];
                        sdp_scid[1] = l2capinbuf[15];
                        l2cap_event_flag |= L2CAP_FLAG_CONNECTION_SDP_REQUEST;

                    }
                    if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == RFCOMM_PSM) {
                        identifier = l2capinbuf[9];
                        rfcomm_scid[0] = l2capinbuf[14];
                        rfcomm_scid[1] = l2capinbuf[15];
                        l2cap_event_flag |= L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST;
                        
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_RESPONSE) {
                    if (l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
                        if ((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) { //Success
                            //Serial.print("\r\nSDP Configuration Complete");
                            l2cap_event_flag |= L2CAP_FLAG_CONFIG_SDP_SUCCESS;
                        }
                    }
                    else if (l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
                        if ((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000) { //Success
                            //Serial.print("\r\nRFCOMM Configuration Complete");
                            l2cap_event_flag |= L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS;
                        }
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_REQUEST) {
                    if (l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
                        //Serial.print("\r\nSDP Configuration Request");
                        identifier = l2capinbuf[9]; 
                        l2cap_event_flag |= L2CAP_FLAG_CONFIG_SDP_REQUEST;          
                    }
                    else if (l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
                        //Serial.print("\r\nRFCOMM Configuration Request");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_FLAG_CONFIG_RFCOMM_REQUEST;          
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_REQUEST) {
                    if (l2capinbuf[12] == sdp_dcid[0] && l2capinbuf[13] == sdp_dcid[1]) {
                        //Notify(PSTR("\r\nDisconnect Request: SDP Channel"));
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_SDP_REQUEST;                      
                    }
                    else if (l2capinbuf[12] == rfcomm_dcid[0] && l2capinbuf[13] == rfcomm_dcid[1]) {
                        //Notify(PSTR("\r\nDisconnect Request: RFCOMM Channel"));
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST;
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_RESPONSE) {
                    if (l2capinbuf[12] == sdp_scid[0] && l2capinbuf[13] == sdp_scid[1]) {                                        
                        //Serial.print("\r\nDisconnect Response: SDP Channel");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_RESPONSE;
                    }
                    else if (l2capinbuf[12] == rfcomm_scid[0] && l2capinbuf[13] == rfcomm_scid[1]) {                                        
                        //Serial.print("\r\nDisconnect Response: RFCOMM Channel");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_FLAG_DISCONNECT_RESPONSE;                                        
                    }
                } 
                else if (l2capinbuf[8] == L2CAP_CMD_INFORMATION_REQUEST) {
#ifdef DEBUG
                    Notify(PSTR("\r\nInformation request"));
#endif
                    identifier = l2capinbuf[9];
                    l2cap_information_response(identifier,l2capinbuf[12],l2capinbuf[13]);                    
                } 
                else {
#ifdef EXTRADEBUG
                    Notify(PSTR("\r\nL2CAP Unknown Command: "));
#endif
                    PrintHex<uint8_t>(l2capinbuf[8]);
                }
            }
            else if (l2capinbuf[6] == sdp_dcid[0] && l2capinbuf[7] == sdp_dcid[1]) { // SDP                                
                if(l2capinbuf[8] == SDP_SERVICE_SEARCH_ATTRIBUTE_REQUEST_PDU) {          
                    /*
                    Serial.print("\r\nUUID: 0x");
                    Serial.print(l2capinbuf[16],HEX);
                    Serial.print(" ");
                    Serial.print(l2capinbuf[17],HEX);
                    */
                    if ((l2capinbuf[16] << 8 | l2capinbuf[17]) == SERIALPORT_UUID) {
                        if(firstMessage) {
                            serialPortResponse1(l2capinbuf[9],l2capinbuf[10]);
                            firstMessage = false;
                        } else {
                            serialPortResponse2(l2capinbuf[9],l2capinbuf[10]); // Serialport continuation state
                            firstMessage = true;
                        }
                    } else if ((l2capinbuf[16] << 8 | l2capinbuf[17]) == L2CAP_UUID) {
                        if(firstMessage) {
                            l2capResponse1(l2capinbuf[9],l2capinbuf[10]);
                            firstMessage = false;
                        } else {
                            l2capResponse2(l2capinbuf[9],l2capinbuf[10]); // L2CAP continuation state
                            firstMessage = true;
                        }
                    } else
                        serviceNotSupported(l2capinbuf[9],l2capinbuf[10]); // The service is not supported                        
                }
            }
            else if (l2capinbuf[6] == rfcomm_dcid[0] && l2capinbuf[7] == rfcomm_dcid[1]) { // RFCOMM                
                rfcommChannel = l2capinbuf[8] & 0xF8;
                rfcommDirection = l2capinbuf[8] & 0x04;
                rfcommCommandResponse = l2capinbuf[8] & 0x02;                
                rfcommChannelType = l2capinbuf[9] & 0xEF;
                rfcommPfBit = l2capinbuf[9] & 0x10;       
                
                if(rfcommChannel>>3 != 0x00)
                    rfcommChannelPermanent = rfcommChannel;
                
#ifdef EXTRADEBUG
                Notify(PSTR("\r\nRFCOMM Channel: "));
                Serial.print(rfcommChannel>>3,HEX);
                Notify(PSTR(" Direction: "));
                Serial.print(rfcommDirection>>2,HEX);
                Notify(PSTR(" CommandResponse: "));
                Serial.print(rfcommCommandResponse>>1,HEX);                 
                Notify(PSTR(" ChannelType: "));
                Serial.print(rfcommChannelType,HEX); 
                Notify(PSTR(" PF_BIT: "));
                Serial.print(rfcommPfBit,HEX);                  
#endif  
            
                if (rfcommChannelType == RFCOMM_DISC) {
#ifdef DEBUG
                    Notify(PSTR("\r\nReceived Disconnect RFCOMM Command on channel: "));
                    Serial.print(rfcommChannel>>3,HEX);
#endif
                    connected = false;
                    sendRfcomm(rfcommChannel,rfcommDirection,rfcommCommandResponse,RFCOMM_UA,rfcommPfBit,rfcommbuf,0x00); // UA Command
                }
                if(connected) {
                    readReport();                    
#ifdef PRINTREPORT
                    printReport(); //Uncomment "#define PRINTREPORT" to print the report send to the Arduino via Bluetooth
#endif
                } else {                    
                    if(rfcommChannelType == RFCOMM_SABM) { // SABM Command - this is sent twice: once for channel 0 and then for the channel to establish
#ifdef DEBUG
                        Notify(PSTR("\r\nReceived SABM Command"));          
#endif                        
                        sendRfcomm(rfcommChannel,rfcommDirection,rfcommCommandResponse,RFCOMM_UA,rfcommPfBit,rfcommbuf,0x00); // UA Command
                    } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_PN_CMD) { // UIH Parameter Negotiation Command
#ifdef DEBUG
                        Notify(PSTR("\r\nReceived UIH Parameter Negotiation Command"));                      
#endif
                        rfcommbuf[0] = BT_RFCOMM_PN_RSP; // UIH Parameter Negotiation Response
                        rfcommbuf[1] = l2capinbuf[12]; // Length and shiftet like so: length << 1 | 1
                        rfcommbuf[2] = l2capinbuf[13]; // Channel: channel << 1 | 1
                        rfcommbuf[3] = 0xE0; // Pre difined for Bluetooth, see 5.5.3 of TS 07.10 Adaption for RFCOMM
                        rfcommbuf[4] = 0x00; // Priority
                        rfcommbuf[5] = 0x00; // Timer                           
                        rfcommbuf[6] = 0x40; // Max Fram Size LSB - we will just set this to 64
                        rfcommbuf[7] = 0x00; // Max Fram Size MSB                            
                        rfcommbuf[8] = 0x00; // MaxRatransm.
                        rfcommbuf[9] = 0x00; // Number of Frames                    
                        sendRfcomm(rfcommChannel,rfcommDirection,0,RFCOMM_UIH,rfcommPfBit,rfcommbuf,0x0A);                        
                    } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_MSC_CMD) { // UIH Modem Status Command
#ifdef DEBUG
                        Notify(PSTR("\r\nSend UIH Modem Status Response"));
#endif
                        rfcommbuf[0] = BT_RFCOMM_MSC_RSP; // UIH Modem Status Response
                        rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                        rfcommbuf[2] = l2capinbuf[13]; // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                        rfcommbuf[3] = l2capinbuf[14];
                        sendRfcomm(rfcommChannel,rfcommDirection,0,RFCOMM_UIH,rfcommPfBit,rfcommbuf,0x04);
                        
                        delay(1);                        
#ifdef DEBUG
                        Notify(PSTR("\r\nSend UIH Modem Status Command"));
#endif
                        rfcommbuf[0] = BT_RFCOMM_MSC_CMD; // UIH Modem Status Command
                        rfcommbuf[1] = 2 << 1 | 1; // Length and shiftet like so: length << 1 | 1
                        rfcommbuf[2] = l2capinbuf[13]; // Channel: (1 << 0) | (1 << 1) | (0 << 2) | (channel << 3)
                        rfcommbuf[3] = 0x8D; // Can receive frames (YES), Ready to Communicate (YES), Ready to Receive (YES), Incomig Call (NO), Data is Value (YES) 
                        
                        sendRfcomm(rfcommChannel,rfcommDirection,0,RFCOMM_UIH,rfcommPfBit,rfcommbuf,0x04);
                    } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_MSC_RSP) { // UIH Modem Status Response
                        if(!creditSent) {
#ifdef DEBUG
                            Notify(PSTR("\r\nSend UIH Command with credit"));   
#endif
                            sendRfcommCredit(rfcommChannelPermanent,rfcommDirection,0,RFCOMM_UIH,0x10,0xFF); // 255 credit
                            creditSent = true;
                            timer = millis();
                            waitForLastCommand = true;
                        }                                      
                    } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[10] == 0x01) { // UIH Command with credit
#ifdef DEBUG
                        Notify(PSTR("\r\nReceived UIH Command with credit"));                  
#endif                                              
                    } else if(rfcommChannelType == RFCOMM_UIH && l2capinbuf[11] == BT_RFCOMM_RPN_CMD) { // UIH Remote Port Negotiation Command
#ifdef DEBUG
                        Notify(PSTR("\r\nReceived UIH Remote Port Negotiation Command"));                   
#endif
                        rfcommbuf[0] = BT_RFCOMM_RPN_RSP; // Command
                        rfcommbuf[1] = l2capinbuf[12]; // Length and shiftet like so: length << 1 | 1
                        rfcommbuf[2] = l2capinbuf[13]; // Channel: channel << 1 | 1
                        rfcommbuf[3] = l2capinbuf[14]; // Pre difined for Bluetooth, see 5.5.3 of TS 07.10 Adaption for RFCOMM
                        rfcommbuf[4] = l2capinbuf[15]; // Priority
                        rfcommbuf[5] = l2capinbuf[16]; // Timer                           
                        rfcommbuf[6] = l2capinbuf[17]; // Max Fram Size LSB
                        rfcommbuf[7] = l2capinbuf[18]; // Max Fram Size MSB                            
                        rfcommbuf[8] = l2capinbuf[19]; // MaxRatransm.
                        rfcommbuf[9] = l2capinbuf[20]; // Number of Frames                    
                        sendRfcomm(rfcommChannel,rfcommDirection,0,RFCOMM_UIH,rfcommPfBit,rfcommbuf,0x0A); // UIH Remote Port Negotiation Response                    
#ifdef DEBUG
                        Notify(PSTR("\r\nRFCOMM Connection is now established\r\n"));                   
#endif                  
                        waitForLastCommand = false;
                        creditSent = false;
                        connected = true; // The RFCOMM channel is now established
                    } else if(rfcommChannelType != RFCOMM_DISC) {
#ifdef DEBUG
                        Notify(PSTR("\r\nUnsupported RFCOMM Data - ChannelType: "));
                        PrintHex<uint8_t>(rfcommChannelType);
                        Notify(PSTR(" Command: "));
                        PrintHex<uint8_t>(l2capinbuf[11]);
#endif                        
                    }
                }
            } else {
#ifdef EXTRADEBUG
                Notify(PSTR("\r\nUnsupported L2CAP Data - Channel ID: "));
                PrintHex<uint8_t>(l2capinbuf[7]);         
                Notify(PSTR(" "));
                PrintHex<uint8_t>(l2capinbuf[6]);
#endif                
            }
            SDP_task();
            RFCOMM_task();
        }        
    }
    else if (rcode != hrNAK) {
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nACL data in error: "));
        PrintHex<uint8_t>(rcode);
#endif
    }
    if((millis() - timer) > 100 && waitForLastCommand) { // We will only wait 100ms and see if the UIH Remote Port Negotiation Command is send, as some deviced don't send it
#ifdef DEBUG
        Notify(PSTR("\r\nRFCOMM Connection is now established - Automatic\r\n"));                   
#endif
        creditSent = false;
        waitForLastCommand = false;
        connected = true; // The RFCOMM channel is now established            
    }
}
void RFCOMM::SDP_task() {
    switch (l2cap_sdp_state)
    {
        case L2CAP_SDP_WAIT:
            if (l2cap_connection_request_sdp_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nSDP Incoming Connection Request"));
#endif
                l2cap_connection_response(identifier, sdp_dcid, sdp_scid, PENDING);          
                delay(1);
                l2cap_connection_response(identifier, sdp_dcid, sdp_scid, SUCCESSFUL);                        
                identifier++;
                delay(1);
                l2cap_config_request(identifier, sdp_scid);                   
                l2cap_sdp_state = L2CAP_SDP_REQUEST; 
            }            
            break;
        case L2CAP_SDP_REQUEST:
            if (l2cap_config_request_sdp_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nSDP Configuration Request"));
#endif                
                l2cap_config_response(identifier, sdp_scid);                        
                l2cap_sdp_state = L2CAP_SDP_SUCCESS;
            }
            break;
        case L2CAP_SDP_SUCCESS:
            if (l2cap_config_success_sdp_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nSDP Successfully Configured"));
#endif
                firstMessage = true; // Reset bool
                SDPConnected = true;
                l2cap_sdp_state = L2CAP_SDP_DONE;
            }
            break;
        case L2CAP_SDP_DONE:
            if(l2cap_disconnect_request_sdp_flag) {
                SDPConnected = false;
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected SDP Channel"));
#endif
                l2cap_disconnection_response(identifier,sdp_dcid,sdp_scid);
                l2cap_sdp_state = L2CAP_SDP_WAIT;
            } 
            break;            
        case L2CAP_DISCONNECT_RESPONSE: // This is for both disconnection response from the RFCOMM and SDP channel if they were connected
            if (l2cap_disconnect_response_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected L2CAP Connection"));
#endif
                RFCOMMConnected = false;
                SDPConnected = false;
                hci_disconnect();
                l2cap_sdp_state = L2CAP_SDP_WAIT;
                l2cap_sdp_state = L2CAP_RFCOMM_WAIT;
                hci_state = HCI_DISCONNECT_STATE;
            }
            break;    
    }
}
void RFCOMM::RFCOMM_task()
{         
    switch (l2cap_rfcomm_state)
    {
        case L2CAP_RFCOMM_WAIT:            
            if(l2cap_connection_request_rfcomm_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nRFCOMM Incoming Connection Request"));
#endif
                l2cap_connection_response(identifier, rfcomm_dcid, rfcomm_scid, PENDING);                        
                delay(1);
                l2cap_connection_response(identifier, rfcomm_dcid, rfcomm_scid, SUCCESSFUL);                        
                identifier++;
                delay(1);
                l2cap_config_request(identifier, rfcomm_scid);                        
                l2cap_rfcomm_state = L2CAP_RFCOMM_REQUEST;                
            } 
            break;                    
        case L2CAP_RFCOMM_REQUEST:
            if (l2cap_config_request_rfcomm_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nRFCOMM Configuration Request"));
#endif
                l2cap_config_response(identifier, rfcomm_scid);                        
                l2cap_rfcomm_state = L2CAP_RFCOMM_SUCCESS;
            }
            break;
        case L2CAP_RFCOMM_SUCCESS:
            if (l2cap_config_success_rfcomm_flag) {
#ifdef DEBUG
                Notify(PSTR("\r\nRFCOMM Successfully Configured"));
#endif                
                rfcommAvailable = 0; // Reset number of bytes available
                bytesReceived = 0; // Reset number of bytes received
                RFCOMMConnected = true;
                l2cap_rfcomm_state = L2CAP_RFCOMM_DONE;
            }
            break;            
        case L2CAP_RFCOMM_DONE:
            if(l2cap_disconnect_request_rfcomm_flag) {
                RFCOMMConnected = false;
                connected = false;
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected RFCOMM Channel"));
#endif
                l2cap_disconnection_response(identifier,rfcomm_dcid,rfcomm_scid);
                l2cap_rfcomm_state = L2CAP_RFCOMM_WAIT;
            }
            break;                    
    }
}

/************************************************************/
/*                     RFCOMM Report                        */
/************************************************************/
void RFCOMM::readReport() {
    if(rfcommChannelType != RFCOMM_UIH || rfcommChannel != rfcommChannelPermanent)
        return;
    uint8_t length = l2capinbuf[10] >> 1; // Get length
    if(rfcommAvailable + length > 256)
        return; // Return if the buffer would be full

    uint8_t offset = l2capinbuf[4]-length-4; // See if there is credit
    
    for(uint8_t i = 0; i < length; i++)
        rfcommDataBuffer[rfcommAvailable+i] = l2capinbuf[11+i+offset];    
    rfcommAvailable += length;
    bytesReceived += length;
    if(bytesReceived > 200) {
        bytesReceived = 0;
        sendRfcommCredit(rfcommChannelPermanent,rfcommDirection,0,RFCOMM_UIH,0x10,0xFF); // Send 255 more credit
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nSent more credit"));
#endif
    }
        
#ifdef EXTRADEBUG
    Notify(PSTR("\r\nRFCOMM Data Available: "));
    Serial.print(rfcommAvailable);
    if (offset) {
        Notify(PSTR(" - Credit: 0x"));
        Serial.print(l2capinbuf[11],HEX);
    }
#endif
}
void RFCOMM::printReport() { //Uncomment "#define PRINTREPORT" to print the report send to the Arduino
    if(rfcommChannelType != RFCOMM_UIH || rfcommChannel != rfcommChannelPermanent)
        return;
    uint8_t length = l2capinbuf[10] >> 1; // Get length        
    uint8_t offset = l2capinbuf[4]-length-4; // See if there is credit        
    for(uint8_t i = 0; i < length; i++)
        Serial.write(l2capinbuf[i+11+offset]);
}

/************************************************************/
/*                    HCI Commands                        */
/************************************************************/
void RFCOMM::HCI_Command(uint8_t* data, uint16_t nbytes) {
    hci_event_flag &= ~HCI_FLAG_CMD_COMPLETE;
    pUsb->ctrlReq(bAddress, epInfo[ BTD_CONTROL_PIPE ].epAddr, bmREQ_HCI_OUT, 0x00, 0x00, 0x00 ,0x00, nbytes, nbytes, data, NULL);    
}
void RFCOMM::hci_reset() {
    hci_event_flag = 0; // clear all the flags
    hcibuf[0] = 0x03; // HCI OCF = 3
    hcibuf[1] = 0x03 << 2; // HCI OGF = 3
    hcibuf[2] = 0x00;
    HCI_Command(hcibuf, 3);
}
void RFCOMM::hci_write_scan_enable() {
    hci_event_flag &= ~HCI_FLAG_INCOMING_REQUEST;
    hcibuf[0] = 0x1A; // HCI OCF = 1A
    hcibuf[1] = 0x03 << 2; // HCI OGF = 3
    hcibuf[2] = 0x01; // parameter length = 1
    hcibuf[3] = 0x03; // Inquiry Scan enabled. Page Scan enabled.
    HCI_Command(hcibuf, 4);
}
void RFCOMM::hci_write_scan_disable() {
    hcibuf[0] = 0x1A; // HCI OCF = 1A
    hcibuf[1] = 0x03 << 2; // HCI OGF = 3
    hcibuf[2] = 0x01; // parameter length = 1
    hcibuf[3] = 0x00; // Inquiry Scan disabled. Page Scan disabled.
    HCI_Command(hcibuf, 4);
}
void RFCOMM::hci_read_bdaddr() {   
    hcibuf[0] = 0x09; // HCI OCF = 9
    hcibuf[1] = 0x04 << 2; // HCI OGF = 4
    hcibuf[2] = 0x00;
    HCI_Command(hcibuf, 3);
}
void RFCOMM::hci_accept_connection() {    
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
void RFCOMM::hci_remote_name() {
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
void RFCOMM::hci_set_local_name(const char* name) {
    hcibuf[0] = 0x13; // HCI OCF = 13
    hcibuf[1] = 0x03 << 2; // HCI OGF = 3
    hcibuf[2] = strlen(name)+1; // parameter length = the length of the string
    uint8_t i;
    for(i = 0; i < strlen(name); i++)
        hcibuf[i+3] = name[i];
    hcibuf[i+3] = 0x00; // End of string

    HCI_Command(hcibuf, 4+strlen(name));
}
void RFCOMM::hci_pin_code_request_reply(const char* key) {    
    hcibuf[0] = 0x0D; // HCI OCF = 0D
    hcibuf[1] = 0x01 << 2; // HCI OGF = 1
    hcibuf[2] = 0x17; // parameter length 23
    hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
    hcibuf[4] = disc_bdaddr[1];
    hcibuf[5] = disc_bdaddr[2];
    hcibuf[6] = disc_bdaddr[3];
    hcibuf[7] = disc_bdaddr[4];
    hcibuf[8] = disc_bdaddr[5];
    hcibuf[9] = strlen(key); // Length of key
    uint8_t i;
    for(i = 0; i < strlen(key); i++) // The maximum size of the key is 16
        hcibuf[i+10] = key[i];
    for(;i < 16; i++)
        hcibuf[i+10] = 0x00; // The rest should be 0
    
    HCI_Command(hcibuf, 26);
}
void RFCOMM::hci_link_key_request_negative_reply() {
    hcibuf[0] = 0x0C; // HCI OCF = 0C
    hcibuf[1] = 0x01 << 2; // HCI OGF = 1
    hcibuf[2] = 0x06; // parameter length 7
    hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
    hcibuf[4] = disc_bdaddr[1];
    hcibuf[5] = disc_bdaddr[2];
    hcibuf[6] = disc_bdaddr[3];
    hcibuf[7] = disc_bdaddr[4];
    hcibuf[8] = disc_bdaddr[5];
    
    HCI_Command(hcibuf, 9);
    
}
void RFCOMM::hci_disconnect() {
    hci_event_flag &= ~HCI_FLAG_DISCONN_COMPLETE;
    hcibuf[0] = 0x06; // HCI OCF = 6
    hcibuf[1] = 0x01 << 2; // HCI OGF = 1
    hcibuf[2] = 0x03; // parameter length = 3
    hcibuf[3] = (uint8_t)(hci_handle & 0xFF);//connection handle - low byte
    hcibuf[4] = (uint8_t)((hci_handle >> 8) & 0x0F);//connection handle - high byte
    hcibuf[5] = 0x13; // reason
    
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
void RFCOMM::L2CAP_Command(uint8_t* data, uint8_t nbytes, uint8_t channelLow, uint8_t channelHigh) {
    uint8_t buf[256];
    buf[0] = (uint8_t)(hci_handle & 0xff);    // HCI handle with PB,BC flag
    buf[1] = (uint8_t)(((hci_handle >> 8) & 0x0f) | 0x20);
    buf[2] = (uint8_t)((4 + nbytes) & 0xff);   // HCI ACL total data length
    buf[3] = (uint8_t)((4 + nbytes) >> 8);
    buf[4] = (uint8_t)(nbytes & 0xff);         // L2CAP header: Length
    buf[5] = (uint8_t)(nbytes >> 8);
    buf[6] = channelLow;
    buf[7] = channelHigh;
    
    for (uint16_t i = 0; i < nbytes; i++)//L2CAP C-frame
        buf[8 + i] = data[i];        
    
    uint8_t rcode = pUsb->outTransfer(bAddress, epInfo[ BTD_DATAOUT_PIPE ].epAddr, (8 + nbytes), buf);
    if(rcode) {
#ifdef DEBUG
        Notify(PSTR("\r\nError sending L2CAP message: 0x"));        
        PrintHex<uint8_t>(rcode);
        Notify(PSTR(" - Channel ID: "));
        Serial.print(channelHigh);
        Notify(PSTR(" "));
        Serial.print(channelHigh);
#endif        
    }        
}
void RFCOMM::l2cap_connection_response(uint8_t rxid, uint8_t* dcid, uint8_t* scid, uint8_t result) {            
    l2capoutbuf[0] = L2CAP_CMD_CONNECTION_RESPONSE;// Code
    l2capoutbuf[1] = rxid;// Identifier
    l2capoutbuf[2] = 0x08;// Length
    l2capoutbuf[3] = 0x00;
    l2capoutbuf[4] = dcid[0];// Destination CID
    l2capoutbuf[5] = dcid[1];
    l2capoutbuf[6] = scid[0];// Source CID
    l2capoutbuf[7] = scid[1];
    l2capoutbuf[8] = result;// Result: Pending or Success
    l2capoutbuf[9] = 0x00;
    l2capoutbuf[10] = 0x00;// No further information
    l2capoutbuf[11] = 0x00;
    
    L2CAP_Command(l2capoutbuf, 12);            
}        
void RFCOMM::l2cap_config_request(uint8_t rxid, uint8_t* dcid) {
    l2capoutbuf[0] = L2CAP_CMD_CONFIG_REQUEST;// Code
    l2capoutbuf[1] = rxid;// Identifier
    l2capoutbuf[2] = 0x08;// Length
    l2capoutbuf[3] = 0x00;
    l2capoutbuf[4] = dcid[0];// Destination CID
    l2capoutbuf[5] = dcid[1];
    l2capoutbuf[6] = 0x00;// Flags
    l2capoutbuf[7] = 0x00;
    l2capoutbuf[8] = 0x01;// Config Opt: type = MTU (Maximum Transmission Unit) - Hint
    l2capoutbuf[9] = 0x02;// Config Opt: length            
    l2capoutbuf[10] = 0xFF;// MTU
    l2capoutbuf[11] = 0xFF;
    
    L2CAP_Command(l2capoutbuf, 12);
}
void RFCOMM::l2cap_config_response(uint8_t rxid, uint8_t* scid) {            
    l2capoutbuf[0] = L2CAP_CMD_CONFIG_RESPONSE;// Code
    l2capoutbuf[1] = rxid;// Identifier
    l2capoutbuf[2] = 0x0A;// Length
    l2capoutbuf[3] = 0x00;
    l2capoutbuf[4] = scid[0];// Source CID
    l2capoutbuf[5] = scid[1];
    l2capoutbuf[6] = 0x00;// Flag
    l2capoutbuf[7] = 0x00;
    l2capoutbuf[8] = 0x00;// Result
    l2capoutbuf[9] = 0x00;
    l2capoutbuf[10] = 0x01;// Config
    l2capoutbuf[11] = 0x02;
    l2capoutbuf[12] = 0xA0;
    l2capoutbuf[13] = 0x02;
    
    L2CAP_Command(l2capoutbuf, 14);
}
void RFCOMM::l2cap_disconnection_request(uint8_t rxid, uint8_t* dcid, uint8_t* scid) {
    l2cap_event_flag = 0; // Reset flags
    l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_REQUEST;// Code
    l2capoutbuf[1] = rxid;// Identifier
    l2capoutbuf[2] = 0x04;// Length
    l2capoutbuf[3] = 0x00;
    l2capoutbuf[4] = scid[0];// Really Destination CID
    l2capoutbuf[5] = scid[1];
    l2capoutbuf[6] = dcid[0];// Really Source CID
    l2capoutbuf[7] = dcid[1];
    L2CAP_Command(l2capoutbuf, 8);
}
void RFCOMM::l2cap_disconnection_response(uint8_t rxid, uint8_t* dcid, uint8_t* scid) {
    l2cap_event_flag = 0; // Reset flags
    l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_RESPONSE;// Code
    l2capoutbuf[1] = rxid;// Identifier
    l2capoutbuf[2] = 0x04;// Length
    l2capoutbuf[3] = 0x00;
    l2capoutbuf[4] = dcid[0];
    l2capoutbuf[5] = dcid[1];
    l2capoutbuf[6] = scid[0];
    l2capoutbuf[7] = scid[1];
    L2CAP_Command(l2capoutbuf, 8);
}
void RFCOMM::l2cap_information_response(uint8_t rxid, uint8_t infoTypeLow, uint8_t infoTypeHigh) {
    l2capoutbuf[0] = L2CAP_CMD_INFORMATION_RESPONSE;// Code
    l2capoutbuf[1] = rxid;// Identifier
    l2capoutbuf[2] = 0x08;// Length
    l2capoutbuf[3] = 0x00;
    l2capoutbuf[4] = infoTypeLow;
    l2capoutbuf[5] = infoTypeHigh;
    l2capoutbuf[6] = 0x00; // Result = success
    l2capoutbuf[7] = 0x00; // Result = success
    l2capoutbuf[8] = 0x00;
    l2capoutbuf[9] = 0x00;
    l2capoutbuf[10] = 0x00;
    l2capoutbuf[11] = 0x00;
    L2CAP_Command(l2capoutbuf, 12);
}

/************************************************************/
/*                    SDP Commands                          */
/************************************************************/
void RFCOMM::SDP_Command(uint8_t* data, uint8_t nbytes) { // See page 223 in the Bluetooth specs
    L2CAP_Command(data,nbytes,sdp_scid[0],sdp_scid[1]);
}
void RFCOMM::serviceNotSupported(uint8_t transactionIDHigh, uint8_t transactionIDLow) { // See page 235 in the Bluetooth specs
    l2capoutbuf[0] = SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_PDU;
    l2capoutbuf[1] = transactionIDHigh;
    l2capoutbuf[2] = transactionIDLow;
    l2capoutbuf[3] = 0x00; // Parameter Length
    l2capoutbuf[4] = 0x05; // Parameter Length
    l2capoutbuf[5] = 0x00; // AttributeListsByteCount
    l2capoutbuf[6] = 0x02; // AttributeListsByteCount
    
    /* Attribute ID/Value Sequence: */
    l2capoutbuf[7] = 0x35;
    l2capoutbuf[8] = 0x00;
    l2capoutbuf[9] = 0x00;
    
    SDP_Command(l2capoutbuf,10);
}
void RFCOMM::serialPortResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
    l2capoutbuf[0] = SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_PDU;
    l2capoutbuf[1] = transactionIDHigh;
    l2capoutbuf[2] = transactionIDLow;
    l2capoutbuf[3] = 0x00; // Parameter Length
    l2capoutbuf[4] = 0x2B; // Parameter Length
    l2capoutbuf[5] = 0x00; // AttributeListsByteCount
    l2capoutbuf[6] = 0x26; // AttributeListsByteCount

    /* Attribute ID/Value Sequence: */
    l2capoutbuf[7] = 0x36;
    l2capoutbuf[8] = 0x00;
    l2capoutbuf[9] = 0x3C;
    l2capoutbuf[10] = 0x36;
    l2capoutbuf[11] = 0x00;
    
    l2capoutbuf[12] = 0x39;
    l2capoutbuf[13] = 0x09;
    l2capoutbuf[14] = 0x00;
    l2capoutbuf[15] = 0x00;
    l2capoutbuf[16] = 0x0A;
    l2capoutbuf[17] = 0x00;
    l2capoutbuf[18] = 0x01;
    l2capoutbuf[19] = 0x00;
    l2capoutbuf[20] = 0x06;
    l2capoutbuf[21] = 0x09;
    l2capoutbuf[22] = 0x00;
    l2capoutbuf[23] = 0x01;
    l2capoutbuf[24] = 0x35;
    l2capoutbuf[25] = 0x03;
    l2capoutbuf[26] = 0x19;
    l2capoutbuf[27] = 0x11;
    
    l2capoutbuf[28] = 0x01;
    l2capoutbuf[29] = 0x09;
    l2capoutbuf[30] = 0x00;
    l2capoutbuf[31] = 0x04;
    l2capoutbuf[32] = 0x35;
    l2capoutbuf[33] = 0x0C;
    l2capoutbuf[34] = 0x35;
    l2capoutbuf[35] = 0x03;
    l2capoutbuf[36] = 0x19;
    l2capoutbuf[37] = 0x01;
    l2capoutbuf[38] = 0x00;
    l2capoutbuf[39] = 0x35;
    l2capoutbuf[40] = 0x05;
    l2capoutbuf[41] = 0x19;
    l2capoutbuf[42] = 0x00;
    l2capoutbuf[43] = 0x03;
    
    l2capoutbuf[44] = 0x08;
    l2capoutbuf[45] = 0x02; // Two more bytes?
    l2capoutbuf[46] = 0x00; // 19 more bytes to come
    l2capoutbuf[47] = 0x19; 
    
    SDP_Command(l2capoutbuf,48);    
}
void RFCOMM::serialPortResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
    l2capoutbuf[0] = SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_PDU;
    l2capoutbuf[1] = transactionIDHigh;
    l2capoutbuf[2] = transactionIDLow;
    l2capoutbuf[3] = 0x00; // Parameter Length
    l2capoutbuf[4] = 0x1C; // Parameter Length
    l2capoutbuf[5] = 0x00; // AttributeListsByteCount
    l2capoutbuf[6] = 0x19; // AttributeListsByteCount
    
    /* Attribute ID/Value Sequence: */
    l2capoutbuf[7] = 0x01;
    l2capoutbuf[8] = 0x09;
    l2capoutbuf[9] = 0x00;
    l2capoutbuf[10] = 0x06;
    l2capoutbuf[11] = 0x35;
    
    l2capoutbuf[12] = 0x09;
    l2capoutbuf[13] = 0x09;
    l2capoutbuf[14] = 0x65;
    l2capoutbuf[15] = 0x6E;
    l2capoutbuf[16] = 0x09;
    l2capoutbuf[17] = 0x00;
    l2capoutbuf[18] = 0x6A;
    l2capoutbuf[19] = 0x09;
    l2capoutbuf[20] = 0x01;
    l2capoutbuf[21] = 0x00;
    l2capoutbuf[22] = 0x09;
    l2capoutbuf[23] = 0x01;
    l2capoutbuf[24] = 0x00;
    l2capoutbuf[25] = 0x25;

    l2capoutbuf[26] = 0x05; // Name length 
    l2capoutbuf[27] = 'T';
    l2capoutbuf[28] = 'K';
    l2capoutbuf[29] = 'J';
    l2capoutbuf[30] = 'S';
    l2capoutbuf[31] = 'P';
    l2capoutbuf[32] = 0x00;        

    SDP_Command(l2capoutbuf,33);
}
void RFCOMM::l2capResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
    serialPortResponse1(transactionIDHigh,transactionIDLow); // These has to send all the supported functions, since it only supports virtual serialport it just sends the message again
}
void RFCOMM::l2capResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow) {
    serialPortResponse2(transactionIDHigh,transactionIDLow); // Same data as serialPortResponse2  
}
/************************************************************/
/*                    RFCOMM Commands                       */
/************************************************************/
void RFCOMM::RFCOMM_Command(uint8_t* data, uint8_t nbytes) {
    L2CAP_Command(data,nbytes,rfcomm_scid[0],rfcomm_scid[1]);
}

void RFCOMM::sendRfcomm(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t* data, uint8_t length) {
    l2capoutbuf[0] = channel | direction | CR | extendAddress; // RFCOMM Address
    l2capoutbuf[1] = channelType | pfBit; // RFCOMM Control
    l2capoutbuf[2] = length << 1 | 0x01; // Length and format (allways 0x01 bytes format)
    uint8_t i = 0;
    for(; i < length; i++)
        l2capoutbuf[i+3] = data[i];
    l2capoutbuf[i+3] = calcFcs(l2capoutbuf);
#ifdef EXTRADEBUG
    Notify(PSTR(" - RFCOMM Data: "));
    for(i = 0; i < length+4; i++) {
        Serial.print(l2capoutbuf[i],HEX);
        Notify(PSTR(" "));
    }
#endif    
    RFCOMM_Command(l2capoutbuf,length+4);
}                     

void RFCOMM::sendRfcommCredit(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t credit) {
    l2capoutbuf[0] = channel | direction | CR | extendAddress; // RFCOMM Address
    l2capoutbuf[1] = channelType | pfBit; // RFCOMM Control
    l2capoutbuf[2] = 0x01; // Length = 0
    l2capoutbuf[3] = credit; // Credit
    l2capoutbuf[4] = calcFcs(l2capoutbuf);                                                    
#ifdef EXTRADEBUG
    Notify(PSTR(" - RFCOMM Credit Data: "));
    for(uint8_t i = 0; i < 5; i++) {
        Serial.print(l2capoutbuf[i],HEX);
        Notify(PSTR(" "));
    }
#endif
    RFCOMM_Command(l2capoutbuf,5);    
}

/* CRC on 2 bytes */
uint8_t RFCOMM::__crc(uint8_t* data) {
    return(pgm_read_byte(&rfcomm_crc_table[pgm_read_byte(&rfcomm_crc_table[0xff ^ data[0]]) ^ data[1]]));
}

/* Calculate FCS - we never actually check if the host sends correct FCS to the Arduino */
uint8_t RFCOMM::calcFcs(uint8_t *data) {
    if((data[1] & 0xEF) == RFCOMM_UIH)
        return (0xff - __crc(data)); // FCS on 2 bytes
    else
        return (0xff - pgm_read_byte(&rfcomm_crc_table[__crc(data) ^ data[2]])); // FCS on 3 bytes
}

/* Serial commands */
void RFCOMM::print(const char* data) {
    rfcommbuf[0] = rfcommChannelPermanent | 0 | 0 | extendAddress;; // RFCOMM Address
    rfcommbuf[1] = RFCOMM_UIH; // RFCOMM Control                                                                
    rfcommbuf[2] = strlen(data) << 1 | 1; // Length
    uint8_t i = 0;
    for(; i < strlen(data); i++)
        rfcommbuf[i+3] = data[i];                                                                
    rfcommbuf[i+3] = calcFcs(rfcommbuf);                            
    
    RFCOMM_Command(rfcommbuf,strlen(data)+4);
}
void RFCOMM::print(uint8_t data) {
    print(&data,1);
}
void RFCOMM::print(uint8_t* array, uint8_t length) {
    rfcommbuf[0] = rfcommChannelPermanent | 0 | 0 | extendAddress;; // RFCOMM Address
    rfcommbuf[1] = RFCOMM_UIH; // RFCOMM Control
    rfcommbuf[2] = length << 1 | 1; // Length
    uint8_t i = 0;
    for(; i < length; i++)
        rfcommbuf[i+3] = array[i];
    rfcommbuf[i+3] = calcFcs(rfcommbuf);
    
    RFCOMM_Command(rfcommbuf,length+4);
}
void RFCOMM::print(const __FlashStringHelper *ifsh) {
    const char PROGMEM *p = (const char PROGMEM *)ifsh;
    size_t size = 0;
    while (1) { // Calculate the size of the string
        uint8_t c = pgm_read_byte(p+size);
        if (c == 0)
            break;
        size++;
    }
    uint8_t buf[size];
    
    for(uint8_t i = 0; i < size; i++)
        buf[i] = pgm_read_byte(p++);
    
    print(buf,size);
}

void RFCOMM::println(const char* data) {
    char output[strlen(data)+2];
    strcpy(output,data);
    strcat(output,"\r\n");
    print(output);
}
void RFCOMM::println(uint8_t data) {
    uint8_t buf[3] = {data, '\r', '\n'};
    print(buf,3);
}
void RFCOMM::println(uint8_t* array, uint8_t length) {
    uint8_t buf[length+2];
    memcpy(buf,array,length);
    buf[length] = '\r';
    buf[length+1] = '\n';    
    print(buf,length+2);
}
void RFCOMM::println(const __FlashStringHelper *ifsh) {
    const char PROGMEM *p = (const char PROGMEM *)ifsh;    
    size_t size = 0;
    while (1) { // Calculate the size of the string
        uint8_t c = pgm_read_byte(p+size);
        if (c == 0)
            break;
        size++;
    }
    uint8_t buf[size+2];
    
    for(uint8_t i = 0; i < size; i++)
        buf[i] = pgm_read_byte(p++);
    
    buf[size] = '\r';
    buf[size+1] = '\n';
    print(buf,size+2);
}

uint8_t RFCOMM::read() {
    uint8_t output = rfcommDataBuffer[0];
    for(uint8_t i = 1; i < rfcommAvailable; i++)
        rfcommDataBuffer[i-1] = rfcommDataBuffer[i]; // Shift the buffer one left
    rfcommAvailable--;
    return output;
}