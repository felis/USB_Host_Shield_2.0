#include "usbhub.h"

USBHub::USBHub(USB *p) : 
		pUsb(p), 
		bAddress(0), 
		bNbrPorts(0), 
		bInitState(0), 
//		bPortResetCounter(1),
		qNextPollTime(0),
		bPollEnable(false)
{
	epInfo[0].epAddr		= 0;
	epInfo[0].MaxPktSize	= 8;
	epInfo[0].sndToggle		= bmSNDTOG0;   //set DATA0/1 toggles to 0
	epInfo[0].rcvToggle		= bmRCVTOG0;

	epInfo[1].epAddr		= 1;
	epInfo[1].MaxPktSize	= 1;
	epInfo[1].Interval		= 0xff;
	epInfo[1].sndToggle		= bmSNDTOG0;   //set DATA0/1 toggles to 0
	epInfo[1].rcvToggle		= bmRCVTOG0;

	if (pUsb)
		pUsb->RegisterDeviceClass(this);
}

uint8_t USBHub::Init(uint8_t parent, uint8_t port)
{
	uint8_t		buf[32];
	uint8_t		rcode;
	UsbDevice	*p = NULL;
	EP_RECORD	*oldep_ptr = NULL;
	uint8_t		len = 0;
	uint16_t	cd_len = 0;

	AddressPool	&addrPool = pUsb->GetAddressPool();

	switch (bInitState)
	{
	case 0:
		Serial.println("Init");

		if (bAddress)
			return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;

		// Get pointer to pseudo device with address 0 assigned
		p = addrPool.GetUsbDevicePtr(0);

		if (!p)
			return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

		if (!p->epinfo)
		{
			Serial.println("epinfo");
			return USB_ERROR_EPINFO_IS_NULL;
		}

		// Save old pointer to EP_RECORD of address 0
		oldep_ptr = p->epinfo;

		// Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
		p->epinfo = epInfo;

		// Get device descriptor
		rcode = pUsb->getDevDescr( 0, 0, 8, (uint8_t*)buf );

		if  (!rcode)
			len = (buf[0] > 32) ? 32 : buf[0];

		if( rcode ) 
		{
			// Restore p->epinfo
			p->epinfo = oldep_ptr;

			Serial.println("getDevDesc:");
			return rcode;
		}

		// Extract device class from device descriptor
		// If device class is not a hub return
		if (((USB_DEVICE_DESCRIPTOR*)buf)->bDeviceClass != 0x09)
			return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

		// Allocate new address according to device class
		bAddress = addrPool.AllocAddress(parent, (((USB_DEVICE_DESCRIPTOR*)buf)->bDeviceClass == 0x09) ? true : false, port);

		if (!bAddress)
			return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

		// Extract Max Packet Size from the device descriptor
		epInfo[0].MaxPktSize = ((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0; 

		// Assign new address to the device
		rcode = pUsb->setAddr( 0, 0, bAddress );

		if (rcode)
		{
			// Restore p->epinfo
			p->epinfo = oldep_ptr;
			addrPool.FreeAddress(bAddress);
			bAddress = 0;
			Serial.print("setAddr:");
			Serial.println(rcode, HEX);
			return rcode;
		}

		Serial.print("Addr:");
		Serial.println(bAddress, HEX);

		// Restore p->epinfo
		p->epinfo = oldep_ptr;

		if (len)
			rcode = pUsb->getDevDescr( bAddress, 0, len, (uint8_t*)buf );

		if(rcode) 
			goto FailGetDevDescr;

		// Assign epInfo to epinfo pointer
		rcode = pUsb->setDevTableEntry(bAddress, epInfo);

		if (rcode)
			goto FailSetDevTblEntry;

		bInitState = 1;

	case 1:
		// Get hub descriptor
		rcode = pUsb->GetHubDescriptor(bAddress, 0, 0, 8, buf);

		if (rcode)
			goto FailGetHubDescr;

		// Save number of ports for future use
		bNbrPorts = ((HubDescriptor*)buf)->bNbrPorts;

		bInitState = 2;

	case 2:
		// Read configuration Descriptor in Order To Obtain Proper Configuration Value
		rcode = pUsb->getConfDescr(bAddress, 0, 8, 0, buf);

		if (!rcode)
		{
			cd_len = ((USB_CONFIGURATION_DESCRIPTOR*)buf)->wTotalLength;
			rcode = pUsb->getConfDescr(bAddress, 0, cd_len, 0, buf);
		}
		if (rcode)
			goto FailGetConfDescr;

		// The following code is of no practical use in real life applications.
		// It only intended for the usb protocol sniffer to properly parse hub-class requests.
		{
			uint8_t buf2[24];

			rcode = pUsb->getConfDescr(bAddress, 0, buf[0], 0, buf2);

			if (rcode)
				goto FailGetConfDescr;
		}

		Serial.print("Conf val:");
		Serial.println(buf[5], HEX);

		// Set Configuration Value
		rcode = pUsb->setConf(bAddress, 0, buf[5]);

		if (rcode)
			goto FailSetConfDescr;

		Serial.println("Hub configured");

		bInitState = 3;

	case 3:
		// Power on all ports
		for (uint8_t j=1; j<=bNbrPorts; j++)
			pUsb->HubPortPowerOn(bAddress, j);

		Serial.println("Ports powered");

		bPollEnable = true;
		bInitState = 0;
	}
	bInitState = 0;
	return 0;

FailGetDevDescr:
	Serial.print("getDevDescr:");
	goto Fail;

FailSetDevTblEntry:
	Serial.print("setDevTblEn:");
	goto Fail;

FailGetHubDescr:
	Serial.print("getHub:");
	goto Fail;

FailGetConfDescr:
	Serial.print("getConf:");
	goto Fail;

FailSetConfDescr:
	Serial.print("setConf:");
	goto Fail;

FailGetPortStatus:
	Serial.print("GetPortStatus:");
	goto Fail;

Fail:
	Serial.println(rcode, HEX);
	return rcode;
}

uint8_t USBHub::Release()
{
	pUsb->GetAddressPool().FreeAddress(bAddress);

	bAddress			= 0;
	bNbrPorts			= 0;
//	bPortResetCounter	= 0;
	qNextPollTime		= 0;
	bPollEnable			= false;
	return 0;
}

uint8_t USBHub::Poll()
{
	uint8_t rcode = 0;

	if (!bPollEnable)
		return 0;

	if (qNextPollTime <= millis())
	{
		Serial.print("Poll:");
		Serial.println(bAddress, HEX);

		rcode = GetHubStatus(bAddress);
		if (rcode)
		{
			Serial.print("HubStatus:");
			Serial.println(rcode,HEX);
		}
		qNextPollTime = millis() + 100;
	}
	return rcode;
}


//bmHUB_PORT_STATUS_C_PORT_CONNECTION		
//bmHUB_PORT_STATUS_C_PORT_ENABLE			
//bmHUB_PORT_STATUS_C_PORT_SUSPEND		
//bmHUB_PORT_STATUS_C_PORT_OVER_CURRENT	
//bmHUB_PORT_STATUS_C_PORT_RESET			

uint8_t USBHub::GetHubStatus(uint8_t addr)
{
	uint8_t		rcode;
	uint8_t		buf[8];

//  uint8_t inTransfer( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* data, unsigned int nak_limit = USB_NAK_LIMIT );
	//Serial.println(devtable[1].epinfo->epAddr, HEX);

	rcode = pUsb->inTransfer(addr, 1, 1, buf, 1);

	if (rcode)
	{
		Serial.print("inTransfer:");
		Serial.println(rcode, HEX);
		return rcode;
	}
	Serial.print("Int:");
	Serial.println(buf[0],HEX);

	//return 0;

	if (buf[0] & 0x01)		// Hub Status Change
	{
		pUsb->PrintHubStatus(addr);
		//rcode = GetHubStatus(1, 0, 1, 4, buf);
		//if (rcode)
		//{
		//	Serial.print("GetHubStatus Error");
		//	Serial.println(rcode, HEX);
		//	return rcode;
		//}
	}
	for (uint8_t port=1,mask=0x02; port<8; mask<<=1, port++)
	{
		if (buf[0] & mask)
		{
			HubEvent	evt;
			evt.bmEvent = 0;

			rcode = pUsb->GetPortStatus(addr, 0, port, 4, evt.evtBuff);

			if (rcode)
			{
				Serial.print("GetPortStatus err:");
				Serial.println(rcode, HEX);
				Serial.print("on port:");
				Serial.println(port, DEC);
				continue;
			}
			HubPortStatusChange(addr, port, evt);
		}
	} // for
	return 0;
}

void USBHub::HubPortStatusChange(uint8_t addr, uint8_t port, HubEvent &evt)
{
	Serial.print("Prt:");
	Serial.print(port,HEX);
	
	Serial.print("\tEvt:");
	Serial.println(evt.bmEvent,HEX);
	Serial.print("\tSt:");
	Serial.println(evt.bmStatus,HEX);
	Serial.print("\tCh:");
	Serial.println(evt.bmChange,HEX);


	//Serial.print("Con:");
	//Serial.println(bmHUB_PORT_EVENT_CONNECT, HEX);

	//Serial.print("Rc:");
	//Serial.println(bmHUB_PORT_EVENT_RESET_COMPLETE, HEX);

	PrintHubPortStatus(pUsb, addr, port, true);

	switch (evt.bmEvent)
	{
	//case (bmHUB_PORT_STATE_DISABLED  | bmHUB_PORT_STATUS_C_PORT_CONNECTION  | bmHUB_PORT_STATUS_C_PORT_SUSPEND):
	//	Serial.println("DIS");
	//	pUsb->HubClearPortFeatures(addr, port, HUB_FEATURE_PORT_ENABLE | HUB_FEATURE_C_PORT_CONNECTION);
	//	pUsb->HubPortReset(addr, port);
	//	break;

	// Device connected event
	case bmHUB_PORT_EVENT_CONNECT:
		Serial.println("CON");
		pUsb->HubClearPortFeatures(addr, port, HUB_FEATURE_C_PORT_ENABLE);
		pUsb->HubClearPortFeatures(addr, port, HUB_FEATURE_C_PORT_CONNECTION);
		pUsb->HubPortReset(addr, port);
		break;

	// Reset complete event
	case bmHUB_PORT_EVENT_RESET_COMPLETE:
		Serial.println("RCMPL");
		pUsb->HubClearPortFeatures(addr, port, HUB_FEATURE_C_PORT_RESET | HUB_FEATURE_C_PORT_CONNECTION);

		// Check if device is a low-speed device
		if (evt.bmStatus & bmHUB_PORT_STATUS_PORT_LOW_SPEED)
		{
			UsbDevice *p = pUsb->GetAddressPool().GetUsbDevicePtr(addr);

			if (p)
				p->lowspeed = true;
		}

		delay(50);

		pUsb->Configuring(addr, port);
		break;

	// Suspended or resuming state
	case bmHUB_PORT_STATE_SUSPENDED:
		Serial.println("SUSP");
		break;

	// Resume complete event
	case (bmHUB_PORT_STATE_ENABLED | HUB_FEATURE_C_PORT_SUSPEND):
		break;

	//case bmHUB_PORT_STATE_RESUMING:
	//	break;

	} // switch (evt.bmEvent)
}


void PrintHubPortStatus(USB *usbptr, uint8_t addr, uint8_t port, bool print_changes)
{
	uint8_t		rcode = 0;
	HubEvent	evt;

	rcode = usbptr->GetPortStatus(addr, 0, port, 4, evt.evtBuff);

	if (rcode)
	{
		Serial.println("ERROR!!!");
		return;
	}
	Serial.print("\r\nPort ");
	Serial.println(port, DEC);

	Serial.println("Status");
	Serial.print("CONNECTION:\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_CONNECTION) > 0, DEC);
	Serial.print("ENABLE:\t\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_ENABLE) > 0, DEC);
	Serial.print("SUSPEND:\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_SUSPEND) > 0, DEC);
	Serial.print("OVER_CURRENT:\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_OVER_CURRENT) > 0, DEC);	
	Serial.print("RESET:\t\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_RESET) > 0, DEC);
	Serial.print("POWER:\t\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_POWER) > 0, DEC);
	Serial.print("LOW_SPEED:\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_LOW_SPEED) > 0, DEC);
	Serial.print("HIGH_SPEED:\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_HIGH_SPEED) > 0, DEC);
	Serial.print("TEST:\t\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_TEST) > 0, DEC);	
	Serial.print("INDICATOR:\t");
	Serial.println((evt.bmStatus & bmHUB_PORT_STATUS_PORT_INDICATOR) > 0, DEC);

	if (!print_changes)
		return;

	Serial.println("\nChange");
	Serial.print("CONNECTION:\t");
	Serial.println((evt.bmChange & bmHUB_PORT_STATUS_C_PORT_CONNECTION) > 0, DEC);
	Serial.print("ENABLE:\t\t");
	Serial.println((evt.bmChange & bmHUB_PORT_STATUS_C_PORT_ENABLE) > 0, DEC);
	Serial.print("SUSPEND:\t");
	Serial.println((evt.bmChange & bmHUB_PORT_STATUS_C_PORT_SUSPEND) > 0, DEC);
	Serial.print("OVER_CURRENT:\t");
	Serial.println((evt.bmChange & bmHUB_PORT_STATUS_C_PORT_OVER_CURRENT) > 0, DEC);	
	Serial.print("RESET:\t\t");
	Serial.println((evt.bmChange & bmHUB_PORT_STATUS_C_PORT_RESET) > 0, DEC);
}
