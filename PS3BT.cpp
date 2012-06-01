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

#include "PS3BT.h"
#define DEBUG // Uncomment to print data for debugging
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the PS3 Controllers

const uint8_t PS3BT::BTD_EVENT_PIPE  = 1;			
const uint8_t PS3BT::BTD_DATAIN_PIPE = 2;
const uint8_t PS3BT::BTD_DATAOUT_PIPE = 3;

prog_char OUTPUT_REPORT_BUFFER[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 
    0xff, 0x27, 0x10, 0x00, 0x32, 
    0xff, 0x27, 0x10, 0x00, 0x32, 
    0xff, 0x27, 0x10, 0x00, 0x32, 
    0xff, 0x27, 0x10, 0x00, 0x32, 
    0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

PS3BT::PS3BT(USB *p, uint8_t btadr5, uint8_t btadr4, uint8_t btadr3, uint8_t btadr2, uint8_t btadr1, uint8_t btadr0):
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0), // device address - mandatory
bNumEP(1), // if config descriptor needs to be parsed
qNextPollTime(0),
bPollEnable(false) // don't start polling before dongle is connected
{
    for(uint8_t i=0; i<PS3_MAX_ENDPOINTS; i++)
	{
		epInfo[i].epAddr		= 0;
		epInfo[i].maxPktSize	= (i) ? 0 : 8;
		epInfo[i].epAttribs		= 0;        
        epInfo[i].bmNakPower    = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
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

uint8_t PS3BT::Init(uint8_t parent, uint8_t port, bool lowspeed)
{
    uint8_t	buf[sizeof(USB_DEVICE_DESCRIPTOR)];
	uint8_t	rcode;
	UsbDevice *p = NULL;
	EpInfo *oldep_ptr = NULL;  
    uint8_t	num_of_conf; // number of configurations
    uint16_t PID;
    uint16_t VID;
    
    // get memory address of USB device address pool
	AddressPool	&addrPool = pUsb->GetAddressPool();    
#ifdef EXTRADEBUG
	Notify(PSTR("\r\nPS3BT Init"));
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
    VID = ((USB_DEVICE_DESCRIPTOR*)buf)->idVendor;
    PID = ((USB_DEVICE_DESCRIPTOR*)buf)->idProduct;
    
    if(VID == PS3_VID && (PID == PS3_PID ||  PID == PS3NAVIGATION_PID || PID == PS3MOVE_PID)) {                
        /* The application will work in reduced host mode, so we can save program and data
         memory space. After verifying the PID and VID we will use known values for the 
         configuration values for device, interface, endpoints and HID for the PS3 Controllers */
        
        /* Initialize data structures for endpoints of device */
        epInfo[ PS3_OUTPUT_PIPE ].epAddr = 0x02;    // PS3 output endpoint
        epInfo[ PS3_OUTPUT_PIPE ].epAttribs  = EP_INTERRUPT;
        epInfo[ PS3_OUTPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
        epInfo[ PS3_OUTPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
        epInfo[ PS3_OUTPUT_PIPE ].bmSndToggle = bmSNDTOG0;
        epInfo[ PS3_OUTPUT_PIPE ].bmRcvToggle = bmRCVTOG0;
        epInfo[ PS3_INPUT_PIPE ].epAddr = 0x01;    // PS3 report endpoint            
        epInfo[ PS3_INPUT_PIPE ].epAttribs  = EP_INTERRUPT;
        epInfo[ PS3_INPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
        epInfo[ PS3_INPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
        epInfo[ PS3_INPUT_PIPE ].bmSndToggle = bmSNDTOG0;
        epInfo[ PS3_INPUT_PIPE ].bmRcvToggle = bmRCVTOG0; 
        
        rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
        if( rcode ) 
            goto FailSetDevTblEntry;
        
        delay(200);//Give time for address change
        
        rcode = pUsb->setConf(bAddress, epInfo[ PS3_CONTROL_PIPE ].epAddr, 1);
        if( rcode ) 
            goto FailSetConf;
        
        if(PID == PS3_PID || PID == PS3NAVIGATION_PID)
        {            
            if(PID == PS3_PID) {                                      
#ifdef DEBUG
                Notify(PSTR("\r\nDualshock 3 Controller Connected"));    
#endif
            } else { // must be a navigation controller
#ifdef DEBUG
                Notify(PSTR("\r\nNavigation Controller Connected"));
#endif
            }
            /* Set internal bluetooth address */        
            setBdaddr(my_bdaddr);                     
        }
        else // must be a Motion controller
        {
#ifdef DEBUG
            Notify(PSTR("\r\nMotion Controller Connected"));            
#endif
            setMoveBdaddr(my_bdaddr); 
        }           
    }
    else
    {                                           
        //check if attached device is a Bluetooth dongle and fill endpoint data structure
        //first interface in the configuration must have Bluetooth assigned Class/Subclass/Protocol
        //and 3 endpoints - interrupt-IN, bulk-IN, bulk-OUT,
        //not necessarily in this order
        for (uint8_t i=0; i<num_of_conf; i++) {
            ConfigDescParser<USB_CLASS_WIRELESS_CTRL, WI_SUBCLASS_RF, WI_PROTOCOL_BT, CP_MASK_COMPARE_ALL> confDescrParser(this);
            rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
            if(rcode)
                goto FailGetConfDescr;
            if(bNumEP > 3) //all endpoints extracted
                break;
        } // for (uint8_t i=0; i<num_of_conf; i++...
        
        if (bNumEP < PS3_MAX_ENDPOINTS)
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
        
        //Needed for PS3 Dualshock Controller commands to work via bluetooth
        for (uint8_t i = 0; i < OUTPUT_REPORT_BUFFER_SIZE; i++)
            HIDBuffer[i + 2] = pgm_read_byte(&OUTPUT_REPORT_BUFFER[i]);//First two bytes reserved for report type and ID
        
        HIDBuffer[0] = 0x52;// HID BT Set_report (0x50) | Report Type (Output 0x02)
        HIDBuffer[1] = 0x01;// Report ID
        
        //Needed for PS3 Move Controller commands to work via bluetooth
        HIDMoveBuffer[0] = 0xA2;// HID BT DATA_request (0xA0) | Report Type (Output 0x02)            
        HIDMoveBuffer[1] = 0x02;// Report ID
        
        /* Set device cid for the control and intterrupt channelse - LSB */
        control_dcid[0] = 0x40;//0x0040
        control_dcid[1] = 0x00;        
        interrupt_dcid[0] = 0x41;//0x0041
        interrupt_dcid[1] = 0x00;
        
        hci_num_reset_loops = 100; // only loop 100 times before trying to send the hci reset command
        
        hci_state = HCI_INIT_STATE;
        hci_counter = 0;        
        l2cap_state = L2CAP_EV_WAIT;
#ifdef DEBUG
        Notify(PSTR("\r\nBluetooth Dongle Initialized"));
#endif
        
        watingForConnection = false;
        bPollEnable = true;
    }
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
    Notify(PSTR("\r\nUnknown Device Connected - VID: "));
    PrintHex<uint16_t>(VID);
    Notify(PSTR(" PID: "));
    PrintHex<uint16_t>(PID);
#endif
    goto Fail;
Fail:
#ifdef DEBUG
    Notify(PSTR("\r\nPS3 Init Failed, error code: "));
    Serial.print(rcode);                     
#endif    
    Release();
    return rcode;
}
/* Extracts interrupt-IN, bulk-IN, bulk-OUT endpoint information from config descriptor */
void PS3BT::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) 
{
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
void PS3BT::PrintEndpointDescriptor( const USB_ENDPOINT_DESCRIPTOR* ep_ptr )
{
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
uint8_t PS3BT::Release()
{
    PS3Connected = false;
    PS3MoveConnected = false;
    PS3NavigationConnected = false;
	pUsb->GetAddressPool().FreeAddress(bAddress);    
	bAddress = 0;
    bPollEnable = false;
    bNumEP = 1; // must have to be reset to 1	
	return 0;
}
uint8_t PS3BT::Poll()
{    
	if (!bPollEnable)
		return 0;
    if (qNextPollTime <= millis()) { // Don't poll if shorter than polling interval
        qNextPollTime = millis() + pollInterval; // Set new poll time
        HCI_event_task(); // poll the HCI event pipe
        ACL_event_task(); // start polling the ACL input pipe too, though discard data until connected        
    }    
	return 0;
}
void PS3BT::setBdaddr(uint8_t* BDADDR)
{    
    /* Set the internal bluetooth address */             
    uint8_t buf[8];            
    buf[0] = 0x01;
    buf[1] = 0x00;
    for (uint8_t i = 0; i < 6; i++)
        buf[i+2] = BDADDR[5 - i];//Copy into buffer, has to be written reversed
    
    //bmRequest = Host to device (0x00) | Class (0x20) | Interface (0x01) = 0x21, bRequest = Set Report (0x09), Report ID (0xF5), Report Type (Feature 0x03), interface (0x00), datalength, datalength, data)
    pUsb->ctrlReq(bAddress,epInfo[PS3_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0xF5, 0x03, 0x00, 8, 8, buf, NULL);      
#ifdef DEBUG
    Notify(PSTR("\r\nBluetooth Address was set to: "));
    for(int8_t i = 5; i > 0; i--)
    {
        PrintHex<uint8_t>(my_bdaddr[i]);
        Serial.print(":");
    }
    PrintHex<uint8_t>(my_bdaddr[0]);
#endif
    return;
}
void PS3BT::setMoveBdaddr(uint8_t* BDADDR)
{
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
    pUsb->ctrlReq(bAddress,epInfo[PS3_CONTROL_PIPE].epAddr, bmREQ_HID_OUT, HID_REQUEST_SET_REPORT, 0x05, 0x03, 0x00,11,11, buf, NULL);   
#ifdef DEBUG
    Notify(PSTR("\r\nBluetooth Address was set to: "));
    for(int8_t i = 5; i > 0; i--)
    {
        PrintHex<uint8_t>(my_bdaddr[i]);
        Serial.print(":");
    }
    PrintHex<uint8_t>(my_bdaddr[0]); 
#endif
	return;
}
bool PS3BT::getButton(Button b)
{
    if (l2capinbuf == NULL)
        return false;
    if(PS3MoveConnected) {
        if((l2capinbuf[((uint16_t)b >> 8)-1] & ((uint8_t)b & 0xff))) // All the buttons locations are shifted one back on the Move controller
            return true;
        else
            return false;
    } else {  
        if((l2capinbuf[(uint16_t)b >> 8] & ((uint8_t)b & 0xff)))
            return true;
        else
            return false;
    }
}
uint8_t PS3BT::getAnalogButton(AnalogButton a)
{
    if (l2capinbuf == NULL)
        return 0;
    return (uint8_t)(l2capinbuf[(uint16_t)a]);
}
uint8_t PS3BT::getAnalogHat(AnalogHat a)
{
    if (l2capinbuf == NULL)
        return 0;                        
    return (uint8_t)(l2capinbuf[(uint16_t)a]);            
}
int16_t PS3BT::getSensor(Sensor a)
{
    if (l2capinbuf == NULL)
        return 0;
    if (a == aX || a == aY || a == aZ || a == gZ)
        return ((l2capinbuf[(uint16_t)a] << 8) | l2capinbuf[(uint16_t)a + 1]);
    else if (a == mXmove || a == mYmove || a == mZmove) // These are all 12-bits long
    {        
        // Might not be correct, haven't tested it yet
        /*if (a == mXmove)
            return ((l2capinbuf[(uint16_t)a + 1] << 0x04) | (l2capinbuf[(uint16_t)a] << 0x0C));
        else if (a == mYmove)
            return ((l2capinbuf[(uint16_t)a + 1] & 0xF0) | (l2capinbuf[(uint16_t)a] << 0x08));
        else if (a == mZmove)
         return ((l2capinbuf[(uint16_t)a + 1] << 0x0F) | (l2capinbuf[(uint16_t)a] << 0x0C));
         */
        if (a == mXmove || a == mYmove)
            return (((l2capinbuf[(uint16_t)a] & 0x0F) << 8) | (l2capinbuf[(uint16_t)a + 1]));
        else // mZmove            
            return ((l2capinbuf[(uint16_t)a] << 4) | (l2capinbuf[(uint16_t)a + 1] >> 4));                
    }
    else if (a == tempMove) // The tempearature is 12 bits long too
        return ((l2capinbuf[(uint16_t)a] << 4) | ((l2capinbuf[(uint16_t)a + 1] & 0xF0) >> 4));
    else // aXmove, aYmove, aZmove, gXmove, gYmove and gZmove
        return (l2capinbuf[(uint16_t)a] | (l2capinbuf[(uint16_t)a + 1] << 8));
}
double PS3BT::getAngle(Angle a) {        
    double accXval;
    double accYval;
    double accZval;
    
    if(PS3Connected) {
        // Data for the Kionix KXPC4 used in the DualShock 3
        const double zeroG = 511.5; // 1.65/3.3*1023 (1,65V)
        accXval = -((double)getSensor(aX)-zeroG);
        accYval = -((double)getSensor(aY)-zeroG);
        accZval = -((double)getSensor(aZ)-zeroG);
    } else if(PS3MoveConnected) {        
        // It's a Kionix KXSC4 inside the Motion controller
        const uint16_t zeroG = 0x8000;                
        accXval = -(int16_t)(getSensor(aXmove)-zeroG);
        accYval = (int16_t)(getSensor(aYmove)-zeroG);
        accZval = (int16_t)(getSensor(aZmove)-zeroG);      
    }
    
    // Convert to 360 degrees resolution
    // atan2 outputs the value of -π to π (radians)
    // We are then converting it to 0 to 2π and then to degrees  
    if (a == Pitch) {        
        double angle = (atan2(accYval,accZval)+PI)*RAD_TO_DEG;
        return angle;
    } else {
        double angle = (atan2(accXval,accZval)+PI)*RAD_TO_DEG;
        return angle;
    }    
}
String PS3BT::getTemperature() {
    if(PS3MoveConnected) {
        int16_t input = getSensor(tempMove);    
        
        String output = String(input/100);
        output += ".";
        if(input%100 < 10)
            output += "0";
        output += String(input%100);
        
        return output;        
    }
}
bool PS3BT::getStatus(Status c)
{
    if (l2capinbuf == NULL)
        return false;
    if (l2capinbuf[(uint16_t)c >> 8] == ((uint8_t)c & 0xff))
        return true;
    return false;
}
String PS3BT::getStatusString()
{
    if (PS3Connected || PS3NavigationConnected)
    {
        char statusOutput[100];
        
        strcpy(statusOutput,"ConnectionStatus: ");
        
        if (getStatus(Plugged)) strcat(statusOutput,"Plugged");
        else if (getStatus(Unplugged)) strcat(statusOutput,"Unplugged");
        else strcat(statusOutput,"Error");
        
        
        strcat(statusOutput," - PowerRating: ");
        if (getStatus(Charging)) strcat(statusOutput,"Charging");
        else if (getStatus(NotCharging)) strcat(statusOutput,"Not Charging");
        else if (getStatus(Shutdown)) strcat(statusOutput,"Shutdown");
        else if (getStatus(Dying)) strcat(statusOutput,"Dying");
        else if (getStatus(Low)) strcat(statusOutput,"Low");
        else if (getStatus(High)) strcat(statusOutput,"High");
        else if (getStatus(Full)) strcat(statusOutput,"Full");
        else strcat(statusOutput,"Error");
        
        strcat(statusOutput," - WirelessStatus: ");
        
        if (getStatus(CableRumble)) strcat(statusOutput,"Cable - Rumble is on");
        else if (getStatus(Cable)) strcat(statusOutput,"Cable - Rumble is off");
        else if (getStatus(BluetoothRumble)) strcat(statusOutput,"Bluetooth - Rumble is on");
        else if (getStatus(Bluetooth)) strcat(statusOutput,"Bluetooth - Rumble is off");
        else strcat(statusOutput,"Error");
        
        return statusOutput;
        
    }
    else if(PS3MoveConnected)
    {
        char statusOutput[50];
        
        strcpy(statusOutput,"PowerRating: ");
        
        if (getStatus(MoveCharging)) strcat(statusOutput,"Charging");
        else if (getStatus(MoveNotCharging)) strcat(statusOutput,"Not Charging");
        else if (getStatus(MoveShutdown)) strcat(statusOutput,"Shutdown");
        else if (getStatus(MoveDying)) strcat(statusOutput,"Dying");
        else if (getStatus(MoveLow)) strcat(statusOutput,"Low");
        else if (getStatus(MoveHigh)) strcat(statusOutput,"High");
        else if (getStatus(MoveFull)) strcat(statusOutput,"Full");
        else strcat(statusOutput,"Error");
        
        return statusOutput;
    }
}
void PS3BT::disconnect()//Use this void to disconnect any of the controllers
{
    if (PS3Connected)
        PS3Connected = false;
    else if (PS3MoveConnected)
        PS3MoveConnected = false;
    else if (PS3NavigationConnected)
        PS3NavigationConnected = false;
    
    //First the HID interrupt channel has to be disconencted, then the HID control channel and finally the HCI connection
    l2cap_disconnection_request(0x0A, interrupt_dcid, interrupt_scid);            
    l2cap_state = L2CAP_EV_INTERRUPT_DISCONNECT;
}

void PS3BT::HCI_event_task()
{
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
                    if((hcibuf[3] == 0x01) && (hcibuf[4] == 0x10)) { // parameters from read local version information
                        hci_version = hcibuf[6]; // Check if it supports 2.0+EDR - see http://www.bluetooth.org/Technical/AssignedNumbers/hci.htm
                        hci_event_flag |= HCI_FLAG_READ_VERSION;
                    }                    
                    else if((hcibuf[3] == 0x09) && (hcibuf[4] == 0x10)) { // parameters from read local bluetooth address  
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
                    Serial.print(" "); 
                    PrintHex<uint8_t>(hcibuf[4]);
                    Serial.print(" ");
                    PrintHex<uint8_t>(hcibuf[5]);                    
#endif
                }
                break;
                
            case EV_CONNECT_COMPLETE:
                if (!hcibuf[2]) // check if connected OK
                { 
                    hci_handle = hcibuf[3] | hcibuf[4] << 8; //store the handle for the ACL connection
                    hci_event_flag |= HCI_FLAG_CONN_COMPLETE; // set connection complete flag
                }
                break;
                
            case EV_DISCONNECT_COMPLETE:            
                if (!hcibuf[2]) // check if disconnected OK
                {
                    hci_event_flag |= HCI_FLAG_DISCONN_COMPLETE; //set disconnect commend complete flag
                    hci_event_flag &= ~HCI_FLAG_CONN_COMPLETE; // clear connection complete flag
                }
                break;                              
                
            case EV_REMOTE_NAME_COMPLETE:
                if (!hcibuf[2]) // check if reading is OK
                {
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
                
            /* We will just ignore the following events */
                
            case EV_NUM_COMPLETE_PKT:
                break;
                
            case EV_ROLE_CHANGED:
                break;
                
            case EV_PAGE_SCAN_REP_MODE:
                break;
                
            case EV_LOOPBACK_COMMAND:
                break;
                
            case EV_DATA_BUFFER_OVERFLOW:
                break;
                
            case EV_CHANGE_CONNECTION_LINK:
                break;
                
            case EV_AUTHENTICATION_COMPLETE:
                break;
                
            default:
#ifdef EXTRADEBUG
                if(hcibuf[0] != 0x00)
                {
                    Notify(PSTR("\r\nUnmanaged Event: "));
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
void PS3BT::HCI_task()
{
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
                hci_read_local_version_information();
                hci_state = HCI_LOCAL_VERSION_STATE;                                
            }
            break;
            
        case HCI_LOCAL_VERSION_STATE:
            if (hci_read_version_complete) 
            {
#ifdef DEBUG
                if(hci_version < 3) {                    
                    Notify(PSTR("\r\nYour dongle may not support reading the analog buttons, sensors and status\r\nYour HCI Version is: "));
                    Serial.print(hci_version);
                    Notify(PSTR("\r\nBut should be at least 3\r\nThis means that it doesn't support Bluetooth Version 2.0+EDR")); 
                }
#endif
                hci_state = HCI_SCANNING_STATE;                
            }
            break;            
            
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
            if(hci_incoming_connect_request)
            {
                watingForConnection = false;
#ifdef DEBUG
                Notify(PSTR("\r\nIncoming Request"));                
#endif                
                hci_remote_name();
                hci_state = HCI_REMOTE_NAME_STATE;
            }
            break;     
            
        case HCI_REMOTE_NAME_STATE:
            if(hci_remote_name_complete)
            {
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
            if (hci_connect_complete)
            {     
#ifdef DEBUG
                Notify(PSTR("\r\nConnected to Device: "));
                for(int8_t i = 5; i>0;i--)
                {
                    PrintHex<uint8_t>(disc_bdaddr[i]);
                    Serial.print(":");
                }      
                PrintHex<uint8_t>(disc_bdaddr[0]);
#endif
                hci_write_scan_disable(); // Only allow one controller
                hci_state = HCI_DISABLE_SCAN;
            }
            break;
            
        case HCI_DISABLE_SCAN:
            if (hci_cmd_complete)
            {                    
#ifdef DEBUG
                Notify(PSTR("\r\nScan Disabled"));
#endif
                l2cap_event_flag = 0;
                l2cap_state = L2CAP_EV_CONTROL_SETUP;
                hci_state = HCI_DONE_STATE;
            }
            break;
            
        case HCI_DONE_STATE:
            if (hci_disconnect_complete)
                hci_state = HCI_DISCONNECT_STATE;
            break;
            
        case HCI_DISCONNECT_STATE:
            if (hci_disconnect_complete)
            {
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
                
                for (uint8_t i = 0; i < OUTPUT_REPORT_BUFFER_SIZE; i++)
                    HIDBuffer[i + 2] = pgm_read_byte(&OUTPUT_REPORT_BUFFER[i]); // First two bytes reserved for report type and ID    
                for (uint8_t i = 0; i < OUTPUT_REPORT_BUFFER_SIZE; i++)
                    HIDMoveBuffer[i + 2] = 0; // First two bytes reserved for report type and ID    
                
                l2cap_state = L2CAP_EV_WAIT;                        
                hci_state = HCI_SCANNING_STATE;
            }
            break;
        default:
            break;
    }
}

void PS3BT::ACL_event_task()
{  
    uint16_t MAX_BUFFER_SIZE = BULK_MAXPKTSIZE;    
    uint8_t rcode = pUsb->inTransfer(bAddress, epInfo[ BTD_DATAIN_PIPE ].epAddr, &MAX_BUFFER_SIZE, l2capinbuf); // input on endpoint 2
    if(!rcode || rcode == hrNAK) // Check for errors
    {    
        if (((l2capinbuf[0] | (l2capinbuf[1] << 8)) == (hci_handle | 0x2000)))//acl_handle_ok  
        {
            if ((l2capinbuf[6] | (l2capinbuf[7] << 8)) == 0x0001)//l2cap_control - Channel ID for ACL-U                                
            {
                /*
                 if (l2capinbuf[8] != 0x00)
                 {
                 Serial.print("\r\nL2CAP Signaling Command - 0x"); 
                 PrintHex<uint8_t>(l2capoutbuf[8]);                
                 }
                 */
                if (l2capinbuf[8] == L2CAP_CMD_COMMAND_REJECT)       
                {
#ifdef DEBUG
                    Notify(PSTR("\r\nL2CAP Command Rejected - Reason: "));
                    PrintHex<uint8_t>(l2capinbuf[13]);
                    Serial.print(" ");
                    PrintHex<uint8_t>(l2capinbuf[12]);
                    Serial.print(" Data: ");
                    PrintHex<uint8_t>(l2capinbuf[17]);
                    Serial.print(" ");
                    PrintHex<uint8_t>(l2capinbuf[16]);
                    Serial.print(" ");                
                    PrintHex<uint8_t>(l2capinbuf[15]);
                    Serial.print(" ");
                    PrintHex<uint8_t>(l2capinbuf[14]);     
#endif
                }
                else if (l2capinbuf[8] == L2CAP_CMD_CONNECTION_REQUEST)
                {                    
                    /*
                     Notify(PSTR("\r\nL2CAP Connection Request - PSM: "));                
                     PrintHex<uint8_t>(l2capinbuf[13]);
                     Serial.print(" ");
                     PrintHex<uint8_t>(l2capinbuf[12]);
                     Serial.print(" ");
                     
                     Notify(PSTR(" SCID: "));
                     PrintHex<uint8_t>(l2capinbuf[15]);
                     Serial.print(" ");
                     PrintHex<uint8_t>(l2capinbuf[14]);
                     
                     Notify(PSTR(" Identifier: "));
                     PrintHex<uint8_t>(l2capinbuf[9]);
                     */
                    if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == L2CAP_PSM_HID_CTRL)
                    {                    
                        identifier = l2capinbuf[9];
                        control_scid[0] = l2capinbuf[14];
                        control_scid[1] = l2capinbuf[15];
                        l2cap_event_flag |= L2CAP_EV_CONTROL_CONNECTION_REQUEST;
                    }
                    else if ((l2capinbuf[12] | (l2capinbuf[13] << 8)) == L2CAP_PSM_HID_INTR)
                    {
                        identifier = l2capinbuf[9];
                        interrupt_scid[0] = l2capinbuf[14];
                        interrupt_scid[1] = l2capinbuf[15];
                        l2cap_event_flag |= L2CAP_EV_INTERRUPT_CONNECTION_REQUEST;                                        
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_RESPONSE)
                {
                    if (l2capinbuf[12] == control_dcid[0] && l2capinbuf[13] == control_dcid[1])
                    {
                        if ((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000)//Success
                        {
                            //Serial.print("\r\nHID Control Configuration Complete");
                            l2cap_event_flag |= L2CAP_EV_CONTROL_CONFIG_SUCCESS;
                        }
                    }
                    else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1])
                    {
                        if ((l2capinbuf[16] | (l2capinbuf[17] << 8)) == 0x0000)//Success
                        {
                            //Serial.print("\r\nHID Interrupt Configuration Complete");
                            l2cap_event_flag |= L2CAP_EV_INTERRUPT_CONFIG_SUCCESS;
                        }
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_CONFIG_REQUEST)                
                {
                    if (l2capinbuf[12] == control_dcid[0] && l2capinbuf[13] == control_dcid[1])
                    {
                        //Serial.print("\r\nHID Control Configuration Request");
                        identifier = l2capinbuf[9]; 
                        l2cap_event_flag |= L2CAP_EV_CONTROL_CONFIG_REQUEST;          
                    }
                    else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1])
                    {
                        //Serial.print("\r\nHID Interrupt Configuration Request");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_EV_INTERRUPT_CONFIG_REQUEST;          
                    }
                }                                    
                else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_REQUEST)
                {
                    if (l2capinbuf[12] == control_dcid[0] && l2capinbuf[13] == control_dcid[1])
                    {
#ifdef DEBUG
                        Notify(PSTR("\r\nDisconnect Request: Control Channel"));
#endif
                        identifier = l2capinbuf[9];
                        l2cap_disconnection_response(identifier,control_dcid,control_scid);
                    }
                    else if (l2capinbuf[12] == interrupt_dcid[0] && l2capinbuf[13] == interrupt_dcid[1])  
                    {
#ifdef DEBUG
                        Notify(PSTR("\r\nDisconnect Request: Interrupt Channel"));                
#endif
                        identifier = l2capinbuf[9];
                        l2cap_disconnection_response(identifier,interrupt_dcid,interrupt_scid);
                    }
                }
                else if (l2capinbuf[8] == L2CAP_CMD_DISCONNECT_RESPONSE)
                {
                    if (l2capinbuf[12] == control_scid[0] && l2capinbuf[13] == control_scid[1])
                    {                                        
                        //Serial.print("\r\nDisconnect Response: Control Channel");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_EV_CONTROL_DISCONNECT_RESPONSE;
                    }
                    else if (l2capinbuf[12] == interrupt_scid[0] && l2capinbuf[13] == interrupt_scid[1])
                    {                                        
                        //Serial.print("\r\nDisconnect Response: Interrupt Channel");
                        identifier = l2capinbuf[9];
                        l2cap_event_flag |= L2CAP_EV_INTERRUPT_DISCONNECT_RESPONSE;                                        
                    }
                }                                     
            }
            else if (l2capinbuf[6] == interrupt_dcid[0] && l2capinbuf[7] == interrupt_dcid[1])//l2cap_interrupt
            {                                
                //Serial.print("\r\nL2CAP Interrupt");  
                if(PS3Connected || PS3MoveConnected || PS3NavigationConnected)
                {
                    readReport();
#ifdef PRINTREPORT
                    printReport(); //Uncomment "#define PRINTREPORT" to print the report send by the PS3 Controllers
#endif
                }
            }
            L2CAP_task();
        }        
    }
    else {
#ifdef EXTRADEBUG
        Notify(PSTR("\r\nACL data in error: "));
        PrintHex<uint8_t>(rcode);
#endif
    }
}
void PS3BT::L2CAP_task()
{    
    switch (l2cap_state)
    {
        case L2CAP_EV_WAIT:
            break;
        case L2CAP_EV_CONTROL_SETUP:
            if (l2cap_control_connection_request)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Control Incoming Connection Request"));
#endif
                l2cap_connection_response(identifier, control_dcid, control_scid, PENDING);          
                delay(1);
                l2cap_connection_response(identifier, control_dcid, control_scid, SUCCESSFUL);                        
                identifier++;
                delay(1);
                l2cap_config_request(identifier, control_scid);                   
                l2cap_state = L2CAP_EV_CONTROL_REQUEST;
            }
            break;
        case L2CAP_EV_CONTROL_REQUEST:
            if (l2cap_control_config_request)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Control Configuration Request"));
#endif
                l2cap_config_response(identifier, control_scid);                        
                l2cap_state = L2CAP_EV_CONTROL_SUCCESS;
            }
            break;
            
        case L2CAP_EV_CONTROL_SUCCESS:
            if (l2cap_control_config_success)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Control Successfully Configured"));
#endif
                l2cap_state = L2CAP_EV_INTERRUPT_SETUP;
            }
            break;
        case L2CAP_EV_INTERRUPT_SETUP:
            if (l2cap_interrupt_connection_request)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Interrupt Incoming Connection Request"));
#endif
                l2cap_connection_response(identifier, interrupt_dcid, interrupt_scid, PENDING);                        
                delay(1);
                l2cap_connection_response(identifier, interrupt_dcid, interrupt_scid, SUCCESSFUL);                        
                identifier++;
                delay(1);
                l2cap_config_request(identifier, interrupt_scid);                        
                
                l2cap_state = L2CAP_EV_INTERRUPT_REQUEST;
            }
            break;
        case L2CAP_EV_INTERRUPT_REQUEST:
            if (l2cap_interrupt_config_request)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Interrupt Configuration Request"));
#endif
                l2cap_config_response(identifier, interrupt_scid);                        
                l2cap_state = L2CAP_EV_INTERRUPT_SUCCESS;
            }
            break;
        case L2CAP_EV_INTERRUPT_SUCCESS:
            if (l2cap_interrupt_config_success)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nHID Interrupt Successfully Configured"));
#endif                
                if(remote_name[0] == 'M') { // First letter in Motion Controller ('M')      
                    for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++) // Reset l2cap in buffer as it sometimes read it as a button has been pressed
                        l2capinbuf[i] = 0;
                    ButtonState = 0;
                    OldButtonState = 0;       
                    
                    l2cap_state = L2CAP_EV_HID_PS3_LED;
                } else
                    l2cap_state = L2CAP_EV_HID_ENABLE_SIXAXIS;
                timer = millis();
            }
            break;
        case L2CAP_EV_HID_ENABLE_SIXAXIS:
            if(millis() - timer > 1000) { // loop 1 second before sending the command
                for (uint8_t i = 0; i < BULK_MAXPKTSIZE; i++) // Reset l2cap in buffer as it sometimes read it as a button has been pressed
                    l2capinbuf[i] = 0;
                ButtonState = 0;
                OldButtonState = 0;
                
                enable_sixaxis();                
                for (uint8_t i = 15; i < 19; i++)
                    l2capinbuf[i] = 0x7F; // Set the analog joystick values to center position                    
                l2cap_state = L2CAP_EV_HID_PS3_LED;
                timer = millis();
            }
            break;
            
        case L2CAP_EV_HID_PS3_LED: 
            if(millis() - timer > 1000) { // loop 1 second before sending the command
                if (remote_name[0] == 'P') { // First letter in PLAYSTATION(R)3 Controller ('P')
                    setLedOn(LED1);
#ifdef DEBUG
                    Notify(PSTR("\r\nDualshock 3 Controller Enabled\r\n"));
#endif     
                    PS3Connected = true;
                } else if (remote_name[0] == 'N') { // First letter in Navigation Controller ('N')
                    setLedOn(LED1); // This just turns LED constantly on, on the Navigation controller
#ifdef DEBUG
                    Notify(PSTR("\r\nNavigation Controller Enabled\r\n"));
#endif
                    PS3NavigationConnected = true;
                } else if(remote_name[0] == 'M') { // First letter in Motion Controller ('M')      
                    moveSetBulb(Red);
                    timerBulbRumble = millis();
#ifdef DEBUG
                    Notify(PSTR("\r\nMotion Controller Enabled\r\n"));
#endif
                    PS3MoveConnected = true;
                }
                l2cap_state = L2CAP_EV_L2CAP_DONE;                                         
            }                                    
            break;
            
        case L2CAP_EV_L2CAP_DONE:
            if (PS3MoveConnected)//The Bulb and rumble values, has to be send at aproximatly every 5th second for it to stay on
            {
                if (millis() - timerBulbRumble > 4000)//Send at least every 4th second
                {
                    HIDMove_Command(HIDMoveBuffer, HID_BUFFERSIZE);//The Bulb and rumble values, has to be written again and again, for it to stay turned on
                    timerBulbRumble = millis();
                }
            }
            break;
            
        case L2CAP_EV_INTERRUPT_DISCONNECT:
            if (l2cap_interrupt_disconnect_response)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected Interrupt Channel"));
#endif
                identifier++;
                l2cap_disconnection_request(identifier, control_dcid, control_scid);                                                
                l2cap_state = L2CAP_EV_CONTROL_DISCONNECT;
            }
            break;
            
        case L2CAP_EV_CONTROL_DISCONNECT:
            if (l2cap_control_disconnect_response)
            {
#ifdef DEBUG
                Notify(PSTR("\r\nDisconnected Control Channel"));
#endif
                hci_disconnect();
                l2cap_state = L2CAP_EV_L2CAP_DONE;
                hci_state = HCI_DISCONNECT_STATE;
            }
            break;
    }
}

/************************************************************/
/*             HID Report (HCI ACL Packet)                  */
/************************************************************/
void PS3BT::readReport()
{                    
    if (l2capinbuf == NULL)
        return;
    if(l2capinbuf[8] == 0xA1)//HID_THDR_DATA_INPUT  
    {
        if(PS3Connected || PS3NavigationConnected)
            ButtonState = (uint32_t)(l2capinbuf[11] | ((uint16_t)l2capinbuf[12] << 8) | ((uint32_t)l2capinbuf[13] << 16));
        else if(PS3MoveConnected)
            ButtonState = (uint32_t)(l2capinbuf[10] | ((uint16_t)l2capinbuf[11] << 8) | ((uint32_t)l2capinbuf[12] << 16));
        
        //Notify(PSTR("\r\nButtonState");
        //PrintHex<uint32_t>(ButtonState);
        
        if(ButtonState != OldButtonState)
        {
            buttonChanged = true;            
            if(ButtonState != 0x00) {
                buttonPressed = true; 
                buttonReleased = false;
            } else {
                buttonPressed = false;
                buttonReleased = true;
            }
        }
        
        else
        {
            buttonChanged = false;
            buttonPressed = false;
            buttonReleased = false;
        }
        
        OldButtonState = ButtonState; 
    }
}  

void PS3BT::printReport() //Uncomment "#define PRINTREPORT" to print the report send by the PS3 Controllers
{                 
    if (l2capinbuf == NULL)
        return;
    if(l2capinbuf[8] == 0xA1)//HID_THDR_DATA_INPUT  
    {
        for(uint8_t i = 10; i < 58;i++)
        {
            PrintHex<uint8_t>(l2capinbuf[i]);
            Serial.print(" ");
        }             
        Serial.println("");
    }
}

/************************************************************/
/*                    HCI Commands                        */
/************************************************************/

//perform HCI Command
void PS3BT::HCI_Command(uint8_t* data, uint16_t nbytes) 
{
    hci_event_flag &= ~HCI_FLAG_CMD_COMPLETE;
    pUsb->ctrlReq(bAddress, epInfo[ BTD_CONTROL_PIPE ].epAddr, bmREQ_HCI_OUT, 0x00, 0x00, 0x00 ,0x00, nbytes, nbytes, data, NULL);    
}

void PS3BT::hci_reset()
{
    hci_event_flag = 0; // clear all the flags
    hcibuf[0] = 0x03; // HCI OCF = 3
    hcibuf[1] = 0x03 << 2; // HCI OGF = 3
    hcibuf[2] = 0x00;
    HCI_Command(hcibuf, 3);
}
void PS3BT::hci_write_scan_enable()
{
    hci_event_flag &= ~HCI_FLAG_INCOMING_REQUEST;
    hcibuf[0] = 0x1A; // HCI OCF = 1A
    hcibuf[1] = 0x03 << 2; // HCI OGF = 3
    hcibuf[2] = 0x01;// parameter length = 1
    hcibuf[3] = 0x02;// Inquiry Scan disabled. Page Scan enabled.
    HCI_Command(hcibuf, 4);
}
void PS3BT::hci_write_scan_disable()
{
    hcibuf[0] = 0x1A; // HCI OCF = 1A
    hcibuf[1] = 0x03 << 2; // HCI OGF = 3
    hcibuf[2] = 0x01;// parameter length = 1
    hcibuf[3] = 0x00;// Inquiry Scan disabled. Page Scan disabled.
    HCI_Command(hcibuf, 4);
}
void PS3BT::hci_read_bdaddr()
{   
    hcibuf[0] = 0x09; // HCI OCF = 9
    hcibuf[1] = 0x04 << 2; // HCI OGF = 4
    hcibuf[2] = 0x00;
    HCI_Command(hcibuf, 3);
}                                   
void PS3BT::hci_read_local_version_information()
{
    hcibuf[0] = 0x01; // HCI OCF = 1
    hcibuf[1] = 0x04 << 2; // HCI OGF = 4
    hcibuf[2] = 0x00;
    HCI_Command(hcibuf, 3);    
}
void PS3BT::hci_accept_connection()
{    
    hcibuf[0] = 0x09; // HCI OCF = 9
    hcibuf[1] = 0x01 << 2; // HCI OGF = 1
    hcibuf[2] = 0x07; // parameter length 7
    hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
    hcibuf[4] = disc_bdaddr[1];
    hcibuf[5] = disc_bdaddr[2];
    hcibuf[6] = disc_bdaddr[3];
    hcibuf[7] = disc_bdaddr[4];
    hcibuf[8] = disc_bdaddr[5];
    hcibuf[9] = 0; //switch role to master
    
    HCI_Command(hcibuf, 10);
}
void PS3BT::hci_remote_name()
{
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
void PS3BT::hci_disconnect()
{
    hci_event_flag &= ~HCI_FLAG_DISCONN_COMPLETE;
    hcibuf[0] = 0x06; // HCI OCF = 6
    hcibuf[1]= 0x01 << 2; // HCI OGF = 1
    hcibuf[2] = 0x03; // parameter length =3
    hcibuf[3] = (uint8_t)(hci_handle & 0xFF);//connection handle - low byte
    hcibuf[4] = (uint8_t)((hci_handle >> 8) & 0x0F);//connection handle - high byte
    hcibuf[5] = 0x13; // reason
    
    HCI_Command(hcibuf, 6);
}
/************************************************************/
/*                    L2CAP Commands                        */
/************************************************************/
void PS3BT::L2CAP_Command(uint8_t* data, uint16_t nbytes)
{
    uint8_t buf[64];
    buf[0] = (uint8_t)(hci_handle & 0xff);    // HCI handle with PB,BC flag
    buf[1] = (uint8_t)(((hci_handle >> 8) & 0x0f) | 0x20);
    buf[2] = (uint8_t)((4 + nbytes) & 0xff);   // HCI ACL total data length
    buf[3] = (uint8_t)((4 + nbytes) >> 8);
    buf[4] = (uint8_t)(nbytes & 0xff);         // L2CAP header: Length
    buf[5] = (uint8_t)(nbytes >> 8);
    buf[6] = 0x01;  // L2CAP header: Channel ID
    buf[7] = 0x00;  // L2CAP Signalling channel over ACL-U logical link
    
    for (uint16_t i = 0; i < nbytes; i++)//L2CAP C-frame
        buf[8 + i] = data[i];        
    
    uint8_t rcode = pUsb->outTransfer(bAddress, epInfo[ BTD_DATAOUT_PIPE ].epAddr, (8 + nbytes), buf);
    if(rcode)
    {
#ifdef DEBUG
        Notify(PSTR("\r\nError sending message: 0x"));        
        PrintHex(rcode);
#endif        
    }        
}
void PS3BT::l2cap_connection_response(uint8_t rxid, uint8_t dcid[], uint8_t scid[], uint8_t result)
{            
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
void PS3BT::l2cap_config_request(uint8_t rxid, uint8_t dcid[])
{
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
void PS3BT::l2cap_config_response(uint8_t rxid, uint8_t scid[])
{            
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
void PS3BT::l2cap_disconnection_request(uint8_t rxid, uint8_t dcid[], uint8_t scid[])
{
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
void PS3BT::l2cap_disconnection_response(uint8_t rxid, uint8_t dcid[], uint8_t scid[])
{
    l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_RESPONSE;// Code
    l2capoutbuf[1] = rxid;// Identifier
    l2capoutbuf[2] = 0x04;// Length
    l2capoutbuf[3] = 0x00;
    l2capoutbuf[4] = scid[0];// Really Destination CID
    l2capoutbuf[5] = scid[1];
    l2capoutbuf[6] = dcid[0];// Really Source CID
    l2capoutbuf[7] = dcid[1];
    L2CAP_Command(l2capoutbuf, 8);
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
/*                    HID Commands                          */
/************************************************************/

//Playstation Sixaxis Dualshock and Navigation Controller commands
void PS3BT::HID_Command(uint8_t* data, uint16_t nbytes)
{
    uint8_t buf[64];
    buf[0] = (uint8_t)(hci_handle & 0xff);    // HCI handle with PB,BC flag
    buf[1] = (uint8_t)(((hci_handle >> 8) & 0x0f) | 0x20);
    buf[2] = (uint8_t)((4 + nbytes) & 0xff); // HCI ACL total data length
    buf[3] = (uint8_t)((4 + nbytes) >> 8);
    buf[4] = (uint8_t)(nbytes & 0xff); // L2CAP header: Length
    buf[5] = (uint8_t)(nbytes >> 8);
    buf[6] = control_scid[0];//Both the Navigation and Dualshock controller sends data via the controller channel
    buf[7] = control_scid[1];
    
    for (uint16_t i = 0; i < nbytes; i++)//L2CAP C-frame            
        buf[8 + i] = data[i];    
    
    if (millis() - timerHID <= 250)// Check if is has been more than 250ms since last command                
        delay((uint32_t)(250 - (millis() - timerHID)));//There have to be a delay between commands
    
    pUsb->outTransfer(bAddress, epInfo[ BTD_DATAOUT_PIPE ].epAddr, (8 + nbytes), buf);
    
    timerHID = millis();
}
void PS3BT::setAllOff()
{
    for (uint8_t i = 0; i < OUTPUT_REPORT_BUFFER_SIZE; i++)
        HIDBuffer[i + 2] = pgm_read_byte(&OUTPUT_REPORT_BUFFER[i]);//First two bytes reserved for report type and ID
    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);
}
void PS3BT::setRumbleOff()
{
    HIDBuffer[3] = 0x00;
    HIDBuffer[4] = 0x00;//low mode off
    HIDBuffer[5] = 0x00;
    HIDBuffer[6] = 0x00;//high mode off
    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);
}
void PS3BT::setRumbleOn(Rumble mode)
{
    /* Still not totally sure how it works, maybe something like this instead?
     * 3 - duration_right
     * 4 - power_right
     * 5 - duration_left
     * 6 - power_left
     */
    if ((mode & 0x30) > 0)
    {
        HIDBuffer[3] = 0xfe;
        HIDBuffer[5] = 0xfe;
        
        if (mode == RumbleHigh)
        {
            HIDBuffer[4] = 0;//low mode off
            HIDBuffer[6] = 0xff;//high mode on
        }
        else
        {
            HIDBuffer[4] = 0xff;//low mode on
            HIDBuffer[6] = 0;//high mode off
        }
        
        HID_Command(HIDBuffer, HID_BUFFERSIZE);
    }
}
void PS3BT::setLedOff(LED a)
{
    HIDBuffer[11] &= ~((uint8_t)(((uint16_t)a & 0x0f) << 1));    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);               
}
void PS3BT::setLedOn(LED a)
{
    HIDBuffer[11] |= (uint8_t)(((uint16_t)a & 0x0f) << 1);    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);            
}
void PS3BT::setLedToggle(LED a)
{
    HIDBuffer[11] ^= (uint8_t)(((uint16_t)a & 0x0f) << 1);    
    HID_Command(HIDBuffer, HID_BUFFERSIZE);            
}
void PS3BT::enable_sixaxis()//Command used to enable the Dualshock 3 and Navigation controller to send data via USB
{
    uint8_t cmd_buf[6];
    cmd_buf[0] = 0x53;// HID BT Set_report (0x50) | Report Type (Feature 0x03)
    cmd_buf[1] = 0xF4;// Report ID
    cmd_buf[2] = 0x42;// Special PS3 Controller enable commands
    cmd_buf[3] = 0x03;
    cmd_buf[4] = 0x00;
    cmd_buf[5] = 0x00;
    
    HID_Command(cmd_buf, 6);
}

//Playstation Move Controller commands
void PS3BT::HIDMove_Command(uint8_t* data,uint16_t nbytes)
{
    uint8_t buf[64];
    buf[0] = (uint8_t)(hci_handle & 0xff);    // HCI handle with PB,BC flag
    buf[1] = (uint8_t)(((hci_handle >> 8) & 0x0f) | 0x20);
    buf[2] = (uint8_t)((4 + nbytes) & 0xff); // HCI ACL total data length
    buf[3] = (uint8_t)((4 + nbytes) >> 8);
    buf[4] = (uint8_t)(nbytes & 0xff); // L2CAP header: Length
    buf[5] = (uint8_t)(nbytes >> 8);
    buf[6] = interrupt_scid[0];//The Move controller sends it's data via the intterrupt channel
    buf[7] = interrupt_scid[1];
    
    for (uint16_t i = 0; i < nbytes; i++)//L2CAP C-frame            
        buf[8 + i] = data[i];    
    
    if (millis() - timerHID <= 250)// Check if is has been less than 200ms since last command                            
        delay((uint32_t)(250 - (millis() - timerHID)));//There have to be a delay between commands
    
    pUsb->outTransfer(bAddress, epInfo[ BTD_DATAOUT_PIPE ].epAddr, (8 + nbytes), buf);
    
    timerHID = millis();
}
void PS3BT::moveSetBulb(uint8_t r, uint8_t g, uint8_t b)//Use this to set the Color using RGB values
{            
    //set the Bulb's values into the write buffer            
    HIDMoveBuffer[3] = r;
    HIDMoveBuffer[4] = g;
    HIDMoveBuffer[5] = b;
    
    HIDMove_Command(HIDMoveBuffer, HID_BUFFERSIZE);   
}
void PS3BT::moveSetBulb(Colors color)//Use this to set the Color using the predefined colors in "enums.h"
{
    moveSetBulb((uint8_t)(color >> 16),(uint8_t)(color >> 8),(uint8_t)(color));
}
void PS3BT::moveSetRumble(uint8_t rumble)
{
#ifdef DEBUG
    if(rumble < 64 && rumble != 0) // The rumble value has to at least 64, or approximately 25% (64/255*100)
        Notify(PSTR("\r\nThe rumble value has to at least 64, or approximately 25%"));
#endif
    //set the rumble value into the write buffer
    HIDMoveBuffer[7] = rumble;
    
    HIDMove_Command(HIDMoveBuffer, HID_BUFFERSIZE);            
}