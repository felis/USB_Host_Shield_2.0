#include "usbhub.h"

USBHub::USBHub(USB *p) : 
		pUsb(p), 
		bAddress(0), 
		bNbrPorts(0), 
		bInitState(0), 
		bPortResetCounter(1),
		qNextPollTime(0),
		bPollEnable(false)
{
	epInfo[0].epAddr		= 0;
	epInfo[0].MaxPktSize	= 8;
	epInfo[0].sndToggle	= bmSNDTOG0;   //set DATA0/1 toggles to 0
	epInfo[0].rcvToggle	= bmRCVTOG0;

	epInfo[1].epAddr		= 1;
	epInfo[1].MaxPktSize	= 1;
	epInfo[1].Interval		= 0xff;
	epInfo[1].sndToggle	= bmSNDTOG0;   //set DATA0/1 toggles to 0
	epInfo[1].rcvToggle	= bmRCVTOG0;

	if (pUsb)
		pUsb->RegisterDeviceClass(this);
}

uint8_t USBHub::Init(uint8_t addr)
{
	uint8_t	buf[8];
	uint8_t	rcode;

	switch (bInitState)
	{
	case 0:
		Serial.println("Init");

		bAddress = addr;

		rcode = pUsb->getDevDescr( addr, 0, 8, (uint8_t*)buf );

		if(rcode) 
			goto FailGetDevDescr;

		// Return if the device is not a hub
		if (((USB_DEVICE_DESCRIPTOR*)buf)->bDeviceClass != 0x09)
			return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

		rcode = pUsb->setDevTableEntry(addr, epInfo);

		if (rcode)
			goto FailSetDevTblEntry;

		bInitState = 1;

	case 1:
		// Get hub descriptor
		rcode = pUsb->GetHubDescriptor(addr, 0, 0, 8, buf);

		if (rcode)
			goto FailGetHubDescr;

		// Save number of ports for future use
		bNbrPorts = ((HubDescriptor*)buf)->bNbrPorts;
		
		bInitState = 2;

	case 2:
		// Read configuration Descriptor in Order To Obtain Proper Configuration Value
		rcode = pUsb->getConfDescr(addr, 0, 8, 0, buf);

		if (rcode)
			goto FailGetConfDescr;

		Serial.print("Conf val:");
		Serial.println(buf[5], HEX);

		// Set Configuration Value
		rcode = pUsb->setConf(addr, 0, buf[5]);

		if (rcode)
			goto FailSetConfDescr;

		Serial.println("Hub configured");

		bInitState = 3;

	case 3:
		// Power on all ports
		for (uint8_t j=1; j<=bNbrPorts; j++)
			pUsb->HubPortPowerOn(addr, j);

		Serial.println("Ports powered");

		bPollEnable = true;
		bInitState = 0;
//		bInitState = 4;

	//case 4:
	//	Serial.print("RC:");
	//	Serial.println(bPortResetCounter, DEC);

	//	for (; bPortResetCounter<=bNbrPorts; bPortResetCounter++)
	//	{
	//		HubEvent	evt;
	//		rcode = pUsb->GetPortStatus(addr, 0, bPortResetCounter, 4, evt.evtBuff);

	//		if (rcode) 
	//			goto FailGetPortStatus;

	//		Serial.print("RC:");
	//		Serial.println(bPortResetCounter, DEC);
	//		Serial.print("\tSt:");
	//		Serial.print(evt.bmStatus, BIN);
	//		Serial.print("\tCh:");
	//		Serial.println(evt.bmChange, BIN);

	//		// Initiate reset only if there is a device plugged into the port
	//		if (evt.bmStatus & bmHUB_PORT_STATUS_PORT_CONNECTION)
	//		{
	//			Serial.print("Con:");
	//			Serial.println(bPortResetCounter, DEC);

	//			pUsb->HubPortReset(addr, bPortResetCounter);
	//			bPollEnable = true;
	//			
	//			if (bPortResetCounter < bNbrPorts)
	//				bPortResetCounter ++;

	//			return USB_DEV_CONFIG_ERROR_DEVICE_INIT_INCOMPLETE;
	//		}
	//	} // for
//		bPortResetCounter = 1;
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

uint8_t USBHub::Release(uint8_t addr)
{
	bAddress			= 0;
	bNbrPorts			= 0;
	bPortResetCounter	= 0;
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
		Serial.println("Poll");

		rcode = GetHubStatus(bAddress);
		if (rcode)
		{
			Serial.print("HubStatus:");
			Serial.println(rcode,HEX);
		}
		qNextPollTime = millis() + 10;
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

	Serial.print("Rc:");
	Serial.println(bmHUB_PORT_EVENT_RESET_COMPLETE, HEX);

	PrintHubPortStatus(pUsb, addr, port);

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

		{
			uint8_t new_addr;

			delay(20);

			// Begin addressing
			pUsb->Addressing(&new_addr);
		}

		//uint8_t new_addr = addrPool.AllocAddress(addr, true, port);
		//rcode = pUsb->setAddr(0, 0, new_addr);
		//if (rcode)
		//{
		//	Serial.print("ERR:");
		//	Serial.println(rcode, HEX);
		//}
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


void PrintHubPortStatus(USB *usbptr, uint8_t addr, uint8_t port)
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


#if 0
        case USB_STATE_ADDRESSING:
			Serial.println("ADRESSING");
			{
			UsbDevice *p = addrPool.GetUsbDevicePtr(0);

			if (p)
			{
				uint8_t addr = addrPool.AllocAddress(0, (p->devclass == 0x09) ? true : false);

				if (addr)
				{
					Serial.print("Addr:");
					Serial.println(addr,HEX);

					UsbDevice *p0 = p;

                    rcode = setAddr( 0, 0, addr );

					if (rcode)
					{
						Serial.println(rcode, HEX);
						break;
					}
					p = addrPool.GetUsbDevicePtr(addr);

					usb_task_state = USB_STATE_CONFIGURING;

				} // if (addr)
			} // if (p)
			}
   //         for( i = 1; i < USB_NUMDEVICES; i++ ) 
			//{
   //             if ( devtable[ i ].epinfo == NULL ) 
			//	{
   //                 devtable[ i ].epinfo = devtable[ 0 ].epinfo;        //set correct MaxPktSize
   //                                                                     //temporary record
   //                                                                     //until plugged with real device endpoint structure
			//		devtable[ i ].devclass = devtable[ 0 ].devclass;

   //                 rcode = setAddr( 0, 0, i );

   //                 if( rcode == 0 ) 
			//		{
   //                     tmpaddr = i;

			//			if (devtable[i].devclass == USB_CLASS_HUB)
			//			{
			//				Serial.println("USB_CLASS_HUB");

			//				uint8_t		buf[8];

			//				rcode = GetHubDescriptor(i, 0, 0, 8, buf);

			//				if (rcode == 0)
			//				{
			//					// Increment number of hubs found
			//					numHubs ++;

			//					// Insert hub address into a first available hub array entry
			//					for (uint8_t j=0; j<HUB_MAX_HUBS; j++)
			//					{
			//						if (hubs[j].bAddress == 0)
			//						{
			//							hubs[j].bAddress	= i;
			//							hubs[j].bNbrPorts	= ((HubDescriptor*)buf)->bNbrPorts;
			//							break;
			//						}
			//					} // for
			//					
			//				} // if (rcode == 0)
			//			}
			//			usb_task_state = USB_STATE_CONFIGURING;
   //                 }
   //                 else 
			//		{
   //                     usb_error = USB_STATE_ADDRESSING;          //set address error
   //                     usb_task_state = USB_STATE_ERROR;
   //                 }
   //                 break;  //break if address assigned or error occured during address assignment attempt                      

   //             } // if( devtable[ i ].epinfo == NULL ) 
   //         }   //for( i = 1; i < USB_NUMDEVICES; i++)

            if( usb_task_state == USB_STATE_ADDRESSING )			//no vacant place in devtable
			{     
                usb_error = 0xfe;
                usb_task_state = USB_STATE_ERROR;
            }
            break;
        case USB_STATE_CONFIGURING:
			Serial.print("CONFIGURING...");

			//// Hub Configuration Code
			//if (devtable[1].devclass == USB_CLASS_HUB)
			//{
			//	Serial.println("HUB");

			//	// Read configuration Descriptor in Order To Obtain Proper Configuration Value
			//	uint8_t	buf[8];
			//	rcode = getConfDescr(1, 0, 8, 0, buf);

			//	if (rcode)
			//	{
			//		Serial.print("getConfDescr:");
			//		Serial.println(rcode, HEX);
			//	}

			//	Serial.print("Conf val:");
			//	Serial.println(buf[5], HEX);

			//	// Set Configuration Value
			//	rcode = setConf(1, 0, buf[5]);

			//	if (rcode)
			//	{
			//		Serial.print("setConf:");
			//		Serial.println(rcode, HEX);
			//	}

			//	Serial.println("Hub configured");

			//	// Enter Port Configuring State
			//	usb_task_state = USB_STATE_HUB_PORT_POWERED_OFF;
			//	//usb_task_state = USB_STATE_HUB_PORT_DISABLED;
			//	break;

			//} //if (devtable[i].devclass == USB_CLASS_HUB)
			usb_task_state = USB_STATE_RUNNING;
			Serial.println("Configured");
            break;
#endif