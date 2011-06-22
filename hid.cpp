/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
*/
#include "hid.h"

//const uint16_t	HID::maxHidInterfaces		= 3;		
//const uint16_t	HID::maxEpPerInterface		= 2;		
const uint8_t	HID::epInterruptInIndex		= 1;	
const uint8_t	HID::epInterruptOutIndex	= 2;

HID::HID(USB *p) : 
		pUsb(p), 
		bAddress(0), 
		qNextPollTime(0),
		bPollEnable(false)
{
	Initialize();

	if (pUsb)
		pUsb->RegisterDeviceClass(this);
}

uint16_t HID::GetHidClassDescrLen(uint8_t type, uint8_t num)
{
	for (uint8_t i=0, n=0; i<HID_MAX_HID_CLASS_DESCRIPTORS; i++)
	{
		if (descrInfo[i].bDescrType == type)
		{
			if (n == num)
				return descrInfo[i].wDescriptorLength;
			n ++;
		}
	}
	return 0;
}

void HID::Initialize()
{
	for (uint8_t i=0; i<HID_MAX_HID_CLASS_DESCRIPTORS; i++)
	{
		descrInfo[i].bDescrType			= 0;
		descrInfo[i].wDescriptorLength	= 0;
	}
	for (uint8_t i=0; i<maxHidInterfaces; i++)
	{
		hidInterfaces[i].bmInterface	= 0;
		hidInterfaces[i].bmProtocol		= 0;

		for (uint8_t j=0; j<maxEpPerInterface; j++)
			hidInterfaces[i].epIndex[j] = 0;
	}
	for(uint8_t i=0; i<totalEndpoints; i++)
	{
		epInfo[i].epAddr		= 0;
		epInfo[i].maxPktSize	= (i) ? 0 : 8;
		epInfo[i].epAttribs		= 0;

		if (!i)
			epInfo[i].bmNakPower	= USB_NAK_MAX_POWER;
	}
	bNumEP		= 1;
	bNumIface	= 0;
	bConfNum	= 0;
}

uint8_t HID::Init(uint8_t parent, uint8_t port, bool lowspeed)
{
	const uint8_t constBufSize = sizeof(USB_DEVICE_DESCRIPTOR);

	uint8_t		buf[constBufSize];
	uint8_t		rcode;
	UsbDevice	*p = NULL;
	EpInfo		*oldep_ptr = NULL;
	uint8_t		len = 0;
	uint16_t	cd_len = 0;

	uint8_t		num_of_conf;	// number of configurations
	uint8_t		num_of_intf;	// number of interfaces

	AddressPool	&addrPool = pUsb->GetAddressPool();

	USBTRACE("HID Init\r\n");

	if (bAddress)
		return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;

	// Get pointer to pseudo device with address 0 assigned
	p = addrPool.GetUsbDevicePtr(0);

	if (!p)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

	if (!p->epinfo)
	{
		USBTRACE("epinfo\r\n");
		return USB_ERROR_EPINFO_IS_NULL;
	}

	// Save old pointer to EP_RECORD of address 0
	oldep_ptr = p->epinfo;

	// Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
	p->epinfo = epInfo;

	p->lowspeed = lowspeed;

	// Get device descriptor
	rcode = pUsb->getDevDescr( 0, 0, 8, (uint8_t*)buf );

	if  (!rcode)
		len = (buf[0] > constBufSize) ? constBufSize : buf[0];

	if( rcode ) 
	{
		// Restore p->epinfo
		p->epinfo = oldep_ptr;

		goto FailGetDevDescr;
	}

	// Restore p->epinfo
	p->epinfo = oldep_ptr;

	// Allocate new address according to device class
	bAddress = addrPool.AllocAddress(parent, false, port);

	if (!bAddress)
		return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

	// Extract Max Packet Size from the device descriptor
	epInfo[0].maxPktSize = (uint8_t)((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0; 

	// Assign new address to the device
	rcode = pUsb->setAddr( 0, 0, bAddress );

	if (rcode)
	{
		p->lowspeed = false;
		addrPool.FreeAddress(bAddress);
		bAddress = 0;
		USBTRACE2("setAddr:",rcode);
		return rcode;
	}

	USBTRACE2("Addr:", bAddress);

	p->lowspeed = false;

	p = addrPool.GetUsbDevicePtr(bAddress);

	if (!p)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

	p->lowspeed = lowspeed;

	if (len)
		rcode = pUsb->getDevDescr( bAddress, 0, len, (uint8_t*)buf );

	if(rcode) 
		goto FailGetDevDescr;

	num_of_conf = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;

	// Assign epInfo to epinfo pointer
	rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);

	if (rcode)
		goto FailSetDevTblEntry;

	USBTRACE2("NC:", num_of_conf);

	for (uint8_t i=0; i<num_of_conf; i++)
	{
		HexDumper<USBReadParser, uint16_t, uint16_t>		HexDump;
		ConfigDescParser<USB_CLASS_HID, 0, 0, 
			CP_MASK_COMPARE_CLASS>							confDescrParser(this);

		rcode = pUsb->getConfDescr(bAddress, 0, i, &HexDump);
		rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
		
		if (bNumEP > 1)
			break;
	} // for
	
	if (bNumEP < 2)
		return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

	// Assign epInfo to epinfo pointer
	rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);

	USBTRACE2("\r\nCnf:", bConfNum);

	// Set Configuration Value
	rcode = pUsb->setConf(bAddress, 0, bConfNum);

	if (rcode)
		goto FailSetConfDescr;

	USBTRACE("HID configured\r\n");

	bPollEnable = true;
	return 0;

FailGetDevDescr:
	USBTRACE("getDevDescr:");
	goto Fail;

FailSetDevTblEntry:
	USBTRACE("setDevTblEn:");
	goto Fail;

FailGetConfDescr:
	USBTRACE("getConf:");
	goto Fail;

FailSetConfDescr:
	USBTRACE("setConf:");
	goto Fail;

Fail:
	Serial.println(rcode, HEX);
	Release();
	return rcode;
}

HID::HIDInterface* HID::FindInterface(uint8_t iface, uint8_t alt, uint8_t proto)
{
	for (uint8_t i=0; i<bNumIface && i<maxHidInterfaces; i++)
		if (hidInterfaces[i].bmInterface == iface && hidInterfaces[i].bmAltSet == alt
			&& hidInterfaces[i].bmProtocol == proto)
			return hidInterfaces + i;
	return NULL;
}

void HID::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) 
{
	// If the first configuration satisfies, the others are not concidered.
	if (bNumEP > 1 && conf != bConfNum)
		return;

	ErrorMessage<uint8_t>(PSTR("\r\nConf.Val"), conf);
	ErrorMessage<uint8_t>(PSTR("Iface Num"), iface);
	ErrorMessage<uint8_t>(PSTR("Alt.Set"), alt);

	bConfNum = conf;

	HIDInterface	*piface = FindInterface(iface, alt, proto);
	
	// Fill in interface structure in case of new interface 
	if (!piface)
	{
		piface = hidInterfaces + bNumIface;
		piface->bmInterface = iface;
		piface->bmAltSet	= alt;
		piface->bmProtocol	= proto;
		bNumIface ++;
	}
	
	uint8_t index;

	if ((pep->bmAttributes & 0x03) == 3 && (pep->bEndpointAddress & 0x80) == 0x80)
	{
		USBTRACE("I8\r\n");
		index = epInterruptInIndex;
	}
	else
	{
		USBTRACE("I0\r\n");
		index = epInterruptOutIndex;
	}

	// Fill in the endpoint info structure
	epInfo[index].epAddr		= (pep->bEndpointAddress & 0x0F);
	epInfo[index].maxPktSize	= (uint8_t)pep->wMaxPacketSize;
	epInfo[index].epAttribs		= 0;

	// Fill in the endpoint index list
	piface->epIndex[index] = (pep->bEndpointAddress & 0x0F);

	bNumEP ++;

	PrintEndpointDescriptor(pep);
}


uint8_t HID::Release()
{
	pUsb->GetAddressPool().FreeAddress(bAddress);

	bNumEP				= 1;
	bAddress			= 0;
	qNextPollTime		= 0;
	bPollEnable			= false;
	return 0;
}

uint8_t HID::Poll()
{
	uint8_t rcode = 0;

	if (!bPollEnable)
		return 0;

	if (qNextPollTime <= millis())
	{
		qNextPollTime = millis() + 100;

		const uint8_t const_buff_len = 16;
		uint8_t buf[const_buff_len];

		HexDumper<USBReadParser, uint16_t, uint16_t>    Hex;
		uint16_t	read = (uint16_t)const_buff_len;

		uint8_t rcode = pUsb->inTransfer(bAddress, epInfo[epInterruptInIndex].epAddr, &read, buf);

		if (rcode && rcode != hrNAK)
		{
			USBTRACE2("Poll:", rcode);
			return rcode;
		}
		for (uint8_t i=0; i<read; i++)
			PrintHex<uint8_t>(buf[i]);
	}
	return rcode;
}

//get HID report descriptor 
uint8_t HID::getReportDescr( uint8_t ep, USBReadParser *parser ) 
{
	const uint8_t	constBufLen = 64;
	uint8_t			buf[constBufLen];

	//HexDumper<USBReadParser, uint16_t, uint16_t>		HexDump;

	//return( pUsb->ctrlReq( bAddress, ep, /*bmREQ_HIDREPORT*/0x81, USB_REQUEST_GET_DESCRIPTOR, 0x00, 
	//	HID_DESCRIPTOR_REPORT, 0x0000, constBufLen, constBufLen, buf, NULL ));

	uint8_t rcode = pUsb->ctrlReq( bAddress, ep, /*bmREQ_HIDREPORT*/0x81, USB_REQUEST_GET_DESCRIPTOR, 0x00, 
		HID_DESCRIPTOR_REPORT, 0x0000, 0x142, constBufLen, buf, (USBReadParser*)parser );

	//return ((rcode != hrSTALL) ? rcode : 0);
	return rcode;
}
//uint8_t HID::getHidDescr( uint8_t ep, uint16_t nbytes, uint8_t* dataptr ) 
//{ 
//    return( pUsb->ctrlReq( bAddress, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0x00, HID_DESCRIPTOR_HID, 0x0000, nbytes, dataptr ));
//}
uint8_t HID::setReport( uint8_t ep, uint16_t nbytes, uint8_t iface, uint8_t report_type, uint8_t report_id, uint8_t* dataptr ) 
{
    return( pUsb->ctrlReq( bAddress, ep, bmREQ_HIDOUT, HID_REQUEST_SET_REPORT, report_id, report_type, iface, nbytes, nbytes, dataptr, NULL ));
}
uint8_t HID::getReport( uint8_t ep, uint16_t nbytes, uint8_t iface, uint8_t report_type, uint8_t report_id, uint8_t* dataptr ) 
{ 
	return( pUsb->ctrlReq( bAddress, ep, bmREQ_HIDIN, HID_REQUEST_GET_REPORT, report_id, report_type, iface, nbytes, nbytes, dataptr, NULL ));
}
uint8_t HID::getIdle( uint8_t ep, uint8_t iface, uint8_t reportID, uint8_t* dataptr ) 
{
    return( pUsb->ctrlReq( bAddress, ep, bmREQ_HIDIN, HID_REQUEST_GET_IDLE, reportID, 0, iface, 0x0001, 0x0001, dataptr, NULL ));    
}
uint8_t HID::setIdle( uint8_t ep, uint8_t iface, uint8_t reportID, uint8_t duration ) 
{
    return( pUsb->ctrlReq( bAddress, ep, bmREQ_HIDOUT, HID_REQUEST_SET_IDLE, reportID, duration, iface, 0x0000, 0x0000, NULL, NULL ));
}
uint8_t HID::setProto( uint8_t ep, uint8_t iface, uint8_t protocol ) 
{
	return( pUsb->ctrlReq( bAddress, ep, bmREQ_HIDOUT, HID_REQUEST_SET_PROTOCOL, protocol, 0x00, iface, 0x0000, 0x0000, NULL, NULL ));
}
uint8_t HID::getProto( uint8_t ep, uint8_t iface, uint8_t* dataptr ) 
{
	return( pUsb->ctrlReq( bAddress, ep, bmREQ_HIDIN, HID_REQUEST_GET_PROTOCOL, 0x00, 0x00, iface, 0x0001, 0x0001, dataptr, NULL ));        
}

void ReportDescParser::Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset)
{
	uint16_t	cntdn	= (uint16_t)len;
	uint8_t		*p		= (uint8_t*)pbuf; 

	while(cntdn)
	{
		ParseItem(&p, &cntdn);

		//if (ParseItem(&p, &cntdn))
		//	return;
	}
}

uint8_t ReportDescParser::ParseItem(uint8_t **pp, uint16_t *pcntdn)
{
	uint8_t	ret = enErrorSuccess;

	switch (itemParseState)
	{
	case 0:
		//if (**pp == HID_LONG_ITEM_PREFIX)
		//{
		//	*pp		++;
		//	pcntdn	--;

		//	if (!(*pcntdn))
		//		return enErrorIncomplete;

		//	itemSize = **pp + 3; // bDataSize + sizeof(bLongItemTag) + sizeof(bDataSize) + 1
		//}
		//else
		{
			uint8_t size	= ((**pp) & DATA_SIZE_MASK);
			itemPrefix		= (**pp);
			itemSize		= 1 + ((size == DATA_SIZE_4) ? 4 : size);

			//USBTRACE2("Sz1:", size);
			//Serial.print("\r\nSz:");
			//Serial.println(itemSize,DEC);

			switch (itemPrefix & (TYPE_MASK | TAG_MASK))
			{
			case (TYPE_GLOBAL | TAG_GLOBAL_PUSH):
				Notify(PSTR("\r\nPush"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_POP):
				Notify(PSTR("\r\nPop"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_USAGEPAGE):
				Notify(PSTR("\r\nUsage Page"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_LOGICALMIN):
				Notify(PSTR("\r\nLogical Min"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_LOGICALMAX):
				Notify(PSTR("\r\nLogical Max"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_PHYSMIN):
				Notify(PSTR("\r\nPhysical Min"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_PHYSMAX):
				Notify(PSTR("\r\nPhysical Max"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_UNITEXP):
				Notify(PSTR("\r\nUnit Exp"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_UNIT):
				Notify(PSTR("\r\nUnit"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_REPORTSIZE):
				Notify(PSTR("\r\nReport Size"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_REPORTCOUNT):
				Notify(PSTR("\r\nReport Count"));
				break;
			case (TYPE_GLOBAL | TAG_GLOBAL_REPORTID):
				Notify(PSTR("\r\nReport Id"));
				break;
			case (TYPE_LOCAL | TAG_LOCAL_USAGE):
				Notify(PSTR("\r\nUsage"));
				break;
			case (TYPE_LOCAL | TAG_LOCAL_USAGEMIN):
				Notify(PSTR("\r\nUsage Min"));
				break;
			case (TYPE_LOCAL | TAG_LOCAL_USAGEMAX):
				Notify(PSTR("\r\nUsage Max"));
				break;
			case (TYPE_MAIN | TAG_MAIN_COLLECTION):
				Notify(PSTR("\r\nCollection"));
				break;
			case (TYPE_MAIN | TAG_MAIN_ENDCOLLECTION):
				Notify(PSTR("\r\nEnd Collection"));
				break;
			case (TYPE_MAIN | TAG_MAIN_INPUT):
				Notify(PSTR("\r\nInput"));
				break;
			case (TYPE_MAIN | TAG_MAIN_OUTPUT):
				Notify(PSTR("\r\nOutput"));
				break;
			case (TYPE_MAIN | TAG_MAIN_FEATURE):
				Notify(PSTR("\r\nFeature"));
				break;
			} // switch (**pp & (TYPE_MASK | TAG_MASK))
		}
		(*pp)		++;
		(*pcntdn)	--;
		itemSize	--;
		itemParseState	= 1;

		if (!itemSize)
			break;

		if (!pcntdn)
			return enErrorIncomplete;
	case 1:
        theBuffer.valueSize = itemSize;
		valParser.Initialize(&theBuffer);
		itemParseState	= 2;
	case 2:
		if (!valParser.Parse(pp, pcntdn))
            return enErrorIncomplete;
		itemParseState	= 3;
	case 3:
		{
		uint8_t data = *((uint8_t*)varBuffer);

		switch (itemPrefix & (TYPE_MASK | TAG_MASK))
		{
		case (TYPE_LOCAL  | TAG_LOCAL_USAGE):
			if (pfUsage)
				if (theBuffer.valueSize > 1)
					pfUsage(*((uint16_t*)varBuffer));
				else
					pfUsage(data);
		case (TYPE_GLOBAL | TAG_GLOBAL_LOGICALMIN):
		case (TYPE_GLOBAL | TAG_GLOBAL_LOGICALMAX):
		case (TYPE_GLOBAL | TAG_GLOBAL_PHYSMIN):
		case (TYPE_GLOBAL | TAG_GLOBAL_PHYSMAX):
		case (TYPE_GLOBAL | TAG_GLOBAL_REPORTSIZE):
		case (TYPE_GLOBAL | TAG_GLOBAL_REPORTCOUNT):
		case (TYPE_GLOBAL | TAG_GLOBAL_REPORTID):
		case (TYPE_LOCAL  | TAG_LOCAL_USAGEMIN):
		case (TYPE_LOCAL  | TAG_LOCAL_USAGEMAX):
		case (TYPE_GLOBAL | TAG_GLOBAL_UNITEXP):
		case (TYPE_GLOBAL | TAG_GLOBAL_UNIT):
			Notify(PSTR("("));
			for (uint8_t i=0; i<theBuffer.valueSize; i++)
				PrintHex<uint8_t>(data);
			Notify(PSTR(")"));
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_PUSH):
		case (TYPE_GLOBAL | TAG_GLOBAL_POP):
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_USAGEPAGE):
			SetUsagePage(data);
			PrintUsagePage(data);

			Notify(PSTR("("));
			for (uint8_t i=0; i<theBuffer.valueSize; i++)
				PrintHex<uint8_t>(data);
			Notify(PSTR(")"));
			break;
		case (TYPE_MAIN   | TAG_MAIN_COLLECTION):
		case (TYPE_MAIN   | TAG_MAIN_ENDCOLLECTION):
			switch (data)
			{
			case 0x00:
				Notify(PSTR(" Physical"));
				break;
			case 0x01:
				Notify(PSTR(" Application"));
				break;
			case 0x02:
				Notify(PSTR(" Logical"));
				break;
			case 0x03:
				Notify(PSTR(" Report"));
				break;
			case 0x04:
				Notify(PSTR(" Named Array"));
				break;
			case 0x05:
				Notify(PSTR(" Usage Switch"));
				break;
			case 0x06:
				Notify(PSTR(" Usage Modifier"));
				break;
			default:
				Notify(PSTR(" Vendor Defined("));
				PrintHex<uint8_t>(data);
				Notify(PSTR(")"));
			}
			break;
		case (TYPE_MAIN   | TAG_MAIN_INPUT):
		case (TYPE_MAIN   | TAG_MAIN_OUTPUT):
		case (TYPE_MAIN   | TAG_MAIN_FEATURE):
			Notify(PSTR("("));
			PrintBin<uint8_t>(data);
			Notify(PSTR(")"));
			break;
		} // switch (**pp & (TYPE_MASK | TAG_MASK))
		}
		itemParseState	= 4;
	case 4:
		if (itemSize > 1 && !theSkipper.Skip(pp, pcntdn, itemSize))
			return enErrorIncomplete;
	} // switch (itemParseState)
	itemParseState	= 0;
	return enErrorSuccess;
}

const char *usagePageTitles0[]	PROGMEM = 
{
	pstrUsagePageGenericDesktopControls	,
	pstrUsagePageSimulationControls		,
	pstrUsagePageVRControls				,
	pstrUsagePageSportControls			,
	pstrUsagePageGameControls			,
	pstrUsagePageGenericDeviceControls	,
	pstrUsagePageKeyboardKeypad			,
	pstrUsagePageLEDs					,
	pstrUsagePageButton					,
	pstrUsagePageOrdinal				,	
	pstrUsagePageTelephone				,
	pstrUsagePageConsumer				,
	pstrUsagePageDigitizer				,
	pstrUsagePagePID					,
	pstrUsagePageUnicode					
};
const char *usagePageTitles1[]	PROGMEM = 
{
	pstrUsagePageBarCodeScanner			,
	pstrUsagePageScale					,
	pstrUsagePageMSRDevices				,
	pstrUsagePagePointOfSale			,	
	pstrUsagePageCameraControl			,
	pstrUsagePageArcade					
};

ReportDescParser::UsagePageFunc		ReportDescParser::usagePageFunctions[] /*PROGMEM*/ =
{
	&ReportDescParser::PrintGenericDesktopPageUsage,
	&ReportDescParser::PrintSimulationControlsPageUsage,
	&ReportDescParser::PrintVRControlsPageUsage,
	&ReportDescParser::PrintSportsControlsPageUsage,
	&ReportDescParser::PrintGameControlsPageUsage,
	&ReportDescParser::PrintGenericDeviceControlsPageUsage,
	NULL,		// Keyboard/Keypad
	&ReportDescParser::PrintLEDPageUsage,
	NULL,		// Button
	NULL,		// Ordinal
	&ReportDescParser::PrintTelephonyPageUsage,
	&ReportDescParser::PrintConsumerPageUsage,
	&ReportDescParser::PrintDigitizerPageUsage,
	NULL,		// Reserved
	NULL,		// PID
	NULL		// Unicode
};

void ReportDescParser::SetUsagePage(uint16_t page)
{
	pfUsage = NULL;

	if (page > 0x00 && page < 0x11)
		pfUsage = /*(UsagePageFunc)pgm_read_word*/(usagePageFunctions[page - 1]);
	//else if (page > 0x7f && page < 0x84)
	//	Notify(pstrUsagePageMonitor);
	//else if (page > 0x83 && page < 0x8c)
	//	Notify(pstrUsagePagePower);
	//else if (page > 0x8b && page < 0x92)
	//	Notify((char*)pgm_read_word(&usagePageTitles1[page - 0x8c]));
	//else if (page > 0xfeff && page <= 0xffff)
	//	Notify(pstrUsagePageVendorDefined);
	else
		switch (page)
		{
		case 0x14:
			pfUsage = &ReportDescParser::PrintAlphanumDisplayPageUsage;
			break;
		case 0x40:
			pfUsage = &ReportDescParser::PrintMedicalInstrumentPageUsage;
			break;
		}
}

void ReportDescParser::PrintUsagePage(uint16_t page)
{
	Notify(pstrSpace);

	if (page > 0x00 && page < 0x11)
		Notify((char*)pgm_read_word(&usagePageTitles0[page - 1]));
	else if (page > 0x7f && page < 0x84)
		Notify(pstrUsagePageMonitor);
	else if (page > 0x83 && page < 0x8c)
		Notify(pstrUsagePagePower);
	else if (page > 0x8b && page < 0x92)
		Notify((char*)pgm_read_word(&usagePageTitles1[page - 0x8c]));
	else if (page > 0xfeff && page <= 0xffff)
		Notify(pstrUsagePageVendorDefined);
	else
		switch (page)
		{
		case 0x14:
			Notify(pstrUsagePageAlphaNumericDisplay);
			break;
		case 0x40:
			Notify(pstrUsagePageMedicalInstruments);
			break;
		default:
			Notify(pstrUsagePageUndefined);
		}
}

const char *genDesktopTitles0[] PROGMEM =
{
	pstrUsagePointer					,
	pstrUsageMouse						,
	pstrUsageJoystick					,
	pstrUsageGamePad					,	
	pstrUsageKeyboard					,
	pstrUsageKeypad						,
	pstrUsageMultiAxisController		,	
	pstrUsageTabletPCSystemControls		

};
const char *genDesktopTitles1[] PROGMEM =
{
	pstrUsageX							,
	pstrUsageY							,
	pstrUsageZ							,
	pstrUsageRx							,
	pstrUsageRy							,
	pstrUsageRz							,
	pstrUsageSlider						,
	pstrUsageDial						,
	pstrUsageWheel						,
	pstrUsageHatSwitch					,
	pstrUsageCountedBuffer				,
	pstrUsageByteCount					,
	pstrUsageMotionWakeup				,
	pstrUsageStart						,
	pstrUsageSelect						,
	pstrUsagePageReserved				,
	pstrUsageVx							,
	pstrUsageVy							,
	pstrUsageVz							,
	pstrUsageVbrx						,
	pstrUsageVbry						,
	pstrUsageVbrz						,
	pstrUsageVno						,	
	pstrUsageFeatureNotification		,	
	pstrUsageResolutionMultiplier		
};
const char *genDesktopTitles2[] PROGMEM =
{
	pstrUsageSystemControl		,
	pstrUsageSystemPowerDown	,	
	pstrUsageSystemSleep		,	
	pstrUsageSystemWakeup		,
	pstrUsageSystemContextMenu	,
	pstrUsageSystemMainMenu		,
	pstrUsageSystemAppMenu		,
	pstrUsageSystemMenuHelp		,
	pstrUsageSystemMenuExit		,
	pstrUsageSystemMenuSelect	,
	pstrUsageSystemMenuRight	,	
	pstrUsageSystemMenuLeft		,
	pstrUsageSystemMenuUp		,
	pstrUsageSystemMenuDown		,
	pstrUsageSystemColdRestart	,
	pstrUsageSystemWarmRestart	,
	pstrUsageDPadUp				,
	pstrUsageDPadDown			,
	pstrUsageDPadRight			,
	pstrUsageDPadLeft			
};
const char *genDesktopTitles3[] PROGMEM =
{
	pstrUsageSystemDock				,
	pstrUsageSystemUndock			,
	pstrUsageSystemSetup			,	
	pstrUsageSystemBreak			,	
	pstrUsageSystemDebuggerBreak	,	
	pstrUsageApplicationBreak		,
	pstrUsageApplicationDebuggerBreak,
	pstrUsageSystemSpeakerMute		,
	pstrUsageSystemHibernate			
};
const char *genDesktopTitles4[] PROGMEM =
{
	pstrUsageSystemDisplayInvert		,	
	pstrUsageSystemDisplayInternal		,
	pstrUsageSystemDisplayExternal		,
	pstrUsageSystemDisplayBoth			,
	pstrUsageSystemDisplayDual			,
	pstrUsageSystemDisplayToggleIntExt	,
	pstrUsageSystemDisplaySwapPriSec	,
	pstrUsageSystemDisplayLCDAutoscale	
};

void ReportDescParser::PrintGenericDesktopPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x0a)
		Notify((char*)pgm_read_word(&genDesktopTitles0[usage - 1]));
	else if (usage > 0x2f && usage < 0x49)
		Notify((char*)pgm_read_word(&genDesktopTitles1[usage - 0x30]));
	else if (usage > 0x7f && usage < 0x94)
		Notify((char*)pgm_read_word(&genDesktopTitles2[usage - 0x80]));
	else if (usage > 0x9f && usage < 0xa9)
		Notify((char*)pgm_read_word(&genDesktopTitles3[usage - 0xa0]));
	else if (usage > 0xaf && usage < 0xb8)
		Notify((char*)pgm_read_word(&genDesktopTitles4[usage - 0xb0]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *simuTitles0[] PROGMEM = 
{
	pstrUsageFlightSimulationDevice		,
	pstrUsageAutomobileSimulationDevice	,
	pstrUsageTankSimulationDevice		,
	pstrUsageSpaceshipSimulationDevice	,
	pstrUsageSubmarineSimulationDevice	,
	pstrUsageSailingSimulationDevice	,	
	pstrUsageMotocicleSimulationDevice	,
	pstrUsageSportsSimulationDevice		,
	pstrUsageAirplaneSimulationDevice	,
	pstrUsageHelicopterSimulationDevice	,
	pstrUsageMagicCarpetSimulationDevice,
	pstrUsageBicycleSimulationDevice		
};
const char *simuTitles1[] PROGMEM = 
{
	pstrUsageFlightControlStick			,
	pstrUsageFlightStick				,	
	pstrUsageCyclicControl				,
	pstrUsageCyclicTrim					,
	pstrUsageFlightYoke					,
	pstrUsageTrackControl				
};
const char *simuTitles2[] PROGMEM = 
{
	pstrUsageAileron					,	
	pstrUsageAileronTrim				,	
	pstrUsageAntiTorqueControl			,
	pstrUsageAutopilotEnable			,	
	pstrUsageChaffRelease				,
	pstrUsageCollectiveControl			,
	pstrUsageDiveBrake					,
	pstrUsageElectronicCountermeasures	,
	pstrUsageElevator					,
	pstrUsageElevatorTrim				,
	pstrUsageRudder						,
	pstrUsageThrottle					,
	pstrUsageFlightCommunications		,
	pstrUsageFlareRelease				,
	pstrUsageLandingGear				,	
	pstrUsageToeBrake					,
	pstrUsageTrigger					,	
	pstrUsageWeaponsArm					,
	pstrUsageWeaponsSelect				,
	pstrUsageWingFlaps					,	
	pstrUsageAccelerator				,	
	pstrUsageBrake						,
	pstrUsageClutch						,
	pstrUsageShifter					,	
	pstrUsageSteering					,
	pstrUsageTurretDirection			,	
	pstrUsageBarrelElevation			,	
	pstrUsageDivePlane					,
	pstrUsageBallast					,	
	pstrUsageBicycleCrank				,
	pstrUsageHandleBars					,
	pstrUsageFrontBrake					,
	pstrUsageRearBrake					
};

void ReportDescParser::PrintSimulationControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x0d)
		Notify((char*)pgm_read_word(&simuTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x26)
		Notify((char*)pgm_read_word(&simuTitles1[usage - 0x20]));
	else if (usage > 0xaf && usage < 0xd1)
		Notify((char*)pgm_read_word(&simuTitles2[usage - 0xb0]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *vrTitles0[]	PROGMEM = 
{
	pstrUsageBelt				,
	pstrUsageBodySuit			,
	pstrUsageFlexor				,
	pstrUsageGlove				,
	pstrUsageHeadTracker		,	
	pstrUsageHeadMountedDisplay	,
	pstrUsageHandTracker		,	
	pstrUsageOculometer			,
	pstrUsageVest				,
	pstrUsageAnimatronicDevice	
};
const char *vrTitles1[]	PROGMEM = 
{
	pstrUsageStereoEnable	,
	pstrUsageDisplayEnable	
};

void ReportDescParser::PrintVRControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x0b)
		Notify((char*)pgm_read_word(&vrTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x22)
		Notify((char*)pgm_read_word(&vrTitles1[usage - 0x20]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *sportsCtrlTitles0[]	PROGMEM = 
{
	pstrUsageBaseballBat				,	
	pstrUsageGolfClub					,
	pstrUsageRowingMachine				,
	pstrUsageTreadmill					
};
const char *sportsCtrlTitles1[]	PROGMEM = 
{
	pstrUsageOar						,	
	pstrUsageSlope						,
	pstrUsageRate						,
	pstrUsageStickSpeed					,
	pstrUsageStickFaceAngle				,
	pstrUsageStickHeelToe				,
	pstrUsageStickFollowThough			,
	pstrUsageStickTempo					,
	pstrUsageStickType					,
	pstrUsageStickHeight					
};
const char *sportsCtrlTitles2[]	PROGMEM = 
{
	pstrUsagePutter						,
	pstrUsage1Iron						,
	pstrUsage2Iron						,
	pstrUsage3Iron						,
	pstrUsage4Iron						,
	pstrUsage5Iron						,
	pstrUsage6Iron						,
	pstrUsage7Iron						,
	pstrUsage8Iron						,
	pstrUsage9Iron						,
	pstrUsage10Iron						,
	pstrUsage11Iron						,
	pstrUsageSandWedge					,
	pstrUsageLoftWedge					,
	pstrUsagePowerWedge					,
	pstrUsage1Wood						,
	pstrUsage3Wood						,
	pstrUsage5Wood						,
	pstrUsage7Wood						,
	pstrUsage9Wood						
};

void ReportDescParser::PrintSportsControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x05)
		Notify((char*)pgm_read_word(&sportsCtrlTitles0[usage - 1]));
	else if (usage > 0x2f && usage < 0x3a)
		Notify((char*)pgm_read_word(&sportsCtrlTitles1[usage - 0x30]));
	else if (usage > 0x4f && usage < 0x64)
		Notify((char*)pgm_read_word(&sportsCtrlTitles2[usage - 0x50]));
	else
		Notify(pstrUsagePageUndefined);
}




const char *gameTitles0[] PROGMEM =
{
	pstrUsage3DGameController		,
	pstrUsagePinballDevice			,
	pstrUsageGunDevice				
};
const char *gameTitles1[] PROGMEM =
{
	pstrUsagePointOfView			,	
	pstrUsageTurnRightLeft			,
	pstrUsagePitchForwardBackward	,
	pstrUsageRollRightLeft			,
	pstrUsageMoveRightLeft			,
	pstrUsageMoveForwardBackward	,	
	pstrUsageMoveUpDown				,
	pstrUsageLeanRightLeft			,
	pstrUsageLeanForwardBackward	,	
	pstrUsageHeightOfPOV			,	
	pstrUsageFlipper				,	
	pstrUsageSecondaryFlipper		,
	pstrUsageBump					,
	pstrUsageNewGame				,	
	pstrUsageShootBall				,
	pstrUsagePlayer					,
	pstrUsageGunBolt				,	
	pstrUsageGunClip				,	
	pstrUsageGunSelector			,	
	pstrUsageGunSingleShot			,
	pstrUsageGunBurst				,
	pstrUsageGunAutomatic			,
	pstrUsageGunSafety				,
	pstrUsageGamepadFireJump		,
	pstrUsageGamepadTrigger			
};

void ReportDescParser::PrintGameControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x04)
		Notify((char*)pgm_read_word(&gameTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x3a)
		Notify((char*)pgm_read_word(&gameTitles1[usage - 0x20]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *genDevCtrlTitles[] PROGMEM = 
{
	pstrUsageBatteryStrength,
	pstrUsageWirelessChannel,			
	pstrUsageWirelessID,				
	pstrUsageDiscoverWirelessControl,	
	pstrUsageSecurityCodeCharEntered,	
	pstrUsageSecurityCodeCharErased,	
	pstrUsageSecurityCodeCleared		
};

void ReportDescParser::PrintGenericDeviceControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x1f && usage < 0x27)
		Notify((char*)pgm_read_word(&genDevCtrlTitles[usage - 0x20]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *ledTitles[] PROGMEM =
{
	pstrUsageNumLock						,
	pstrUsageCapsLock					,
	pstrUsageScrollLock					,
	pstrUsageCompose					,	
	pstrUsageKana						,
	pstrUsagePower						,
	pstrUsageShift						,
	pstrUsageDoNotDisturb				,
	pstrUsageMute						,
	pstrUsageToneEnable					,
	pstrUsageHighCutFilter				,
	pstrUsageLowCutFilter				,
	pstrUsageEqualizerEnable			,	
	pstrUsageSoundFieldOn				,
	pstrUsageSurroundOn					,
	pstrUsageRepeat						,
	pstrUsageStereo						,
	pstrUsageSamplingRateDetect			,
	pstrUsageSpinning					,
	pstrUsageCAV						,	
	pstrUsageCLV						,	
	pstrUsageRecordingFormatDetect		,
	pstrUsageOffHook					,	
	pstrUsageRing						,
	pstrUsageMessageWaiting				,
	pstrUsageDataMode					,
	pstrUsageBatteryOperation			,
	pstrUsageBatteryOK					,
	pstrUsageBatteryLow					,
	pstrUsageSpeaker					,	
	pstrUsageHeadSet					,	
	pstrUsageHold						,
	pstrUsageMicrophone					,
	pstrUsageCoverage					,
	pstrUsageNightMode					,
	pstrUsageSendCalls					,
	pstrUsageCallPickup					,
	pstrUsageConference					,
	pstrUsageStandBy					,	
	pstrUsageCameraOn					,
	pstrUsageCameraOff					,
	pstrUsageOnLine						,
	pstrUsageOffLine					,	
	pstrUsageBusy						,
	pstrUsageReady						,
	pstrUsagePaperOut					,
	pstrUsagePaperJam					,
	pstrUsageRemote						,
	pstrUsageForward					,	
	pstrUsageReverse					,	
	pstrUsageStop						,
	pstrUsageRewind						,
	pstrUsageFastForward				,	
	pstrUsagePlay						,
	pstrUsagePause						,
	pstrUsageRecord						,
	pstrUsageError						,
	pstrUsageSelectedIndicator			,
	pstrUsageInUseIndicator				,
	pstrUsageMultiModeIndicator			,
	pstrUsageIndicatorOn				,	
	pstrUsageIndicatorFlash				,
	pstrUsageIndicatorSlowBlink			,
	pstrUsageIndicatorFastBlink			,
	pstrUsageIndicatorOff				,
	pstrUsageFlashOnTime				,	
	pstrUsageSlowBlinkOnTime			,	
	pstrUsageSlowBlinkOffTime			,
	pstrUsageFastBlinkOnTime			,	
	pstrUsageFastBlinkOffTime			,
	pstrUsageIndicatorColor				,
	pstrUsageIndicatorRed				,
	pstrUsageIndicatorGreen				,
	pstrUsageIndicatorAmber				,
	pstrUsageGenericIndicator			,
	pstrUsageSystemSuspend				,
	pstrUsageExternalPowerConnected		
};

void ReportDescParser::PrintLEDPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x4e)
		Notify((char*)pgm_read_word(&ledTitles[usage - 1]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *telTitles0			[] PROGMEM = 
{
	pstrUsagePhone				,
	pstrUsageAnsweringMachine	,
	pstrUsageMessageControls	,	
	pstrUsageHandset			,	
	pstrUsageHeadset			,	
	pstrUsageTelephonyKeyPad	,	
	pstrUsageProgrammableButton	
};
const char *telTitles1			[] PROGMEM = 
{
	pstrUsageHookSwitch					,
	pstrUsageFlash						,
	pstrUsageFeature					,	
	pstrUsageHold						,
	pstrUsageRedial						,
	pstrUsageTransfer					,
	pstrUsageDrop						,
	pstrUsagePark						,
	pstrUsageForwardCalls				,
	pstrUsageAlternateFunction			,
	pstrUsageLine						,
	pstrUsageSpeakerPhone				,
	pstrUsageConference				,
	pstrUsageRingEnable				,	
	pstrUsageRingSelect				,	
	pstrUsagePhoneMute				,	
	pstrUsageCallerID				,	
	pstrUsageSend						
};
const char *telTitles2			[] PROGMEM = 
{
	pstrUsageSpeedDial		,
	pstrUsageStoreNumber	,	
	pstrUsageRecallNumber	,
	pstrUsagePhoneDirectory
};
const char *telTitles3			[] PROGMEM = 
{
	pstrUsageVoiceMail		,
	pstrUsageScreenCalls	,	
	pstrUsageDoNotDisturb	,
	pstrUsageMessage		,	
	pstrUsageAnswerOnOff		
};
const char *telTitles4			[] PROGMEM = 
{
	pstrUsageInsideDialTone			,
	pstrUsageOutsideDialTone		,	
	pstrUsageInsideRingTone			,
	pstrUsageOutsideRingTone		,	
	pstrUsagePriorityRingTone		,
	pstrUsageInsideRingback			,
	pstrUsagePriorityRingback		,
	pstrUsageLineBusyTone			,
	pstrUsageReorderTone			,	
	pstrUsageCallWaitingTone		,	
	pstrUsageConfirmationTone1		,
	pstrUsageConfirmationTone2		,
	pstrUsageTonesOff				,
	pstrUsageOutsideRingback		,	
	pstrUsageRinger					
};
const char *telTitles5			[] PROGMEM = 
{
	pstrUsagePhoneKey0		,
	pstrUsagePhoneKey1		,
	pstrUsagePhoneKey2		,
	pstrUsagePhoneKey3		,
	pstrUsagePhoneKey4		,
	pstrUsagePhoneKey5		,
	pstrUsagePhoneKey6		,
	pstrUsagePhoneKey7		,
	pstrUsagePhoneKey8		,
	pstrUsagePhoneKey9		,
	pstrUsagePhoneKeyStar	,
	pstrUsagePhoneKeyPound	,
	pstrUsagePhoneKeyA		,
	pstrUsagePhoneKeyB		,
	pstrUsagePhoneKeyC		,
	pstrUsagePhoneKeyD		
};

void ReportDescParser::PrintTelephonyPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x08)
		Notify((char*)pgm_read_word(&telTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x32)
		Notify((char*)pgm_read_word(&telTitles1[usage - 0x1f]));
	else if (usage > 0x4f && usage < 0x54)
		Notify((char*)pgm_read_word(&telTitles2[usage - 0x4f]));
	else if (usage > 0x6f && usage < 0x75)
		Notify((char*)pgm_read_word(&telTitles3[usage - 0x6f]));
	else if (usage > 0x8f && usage < 0x9f)
		Notify((char*)pgm_read_word(&telTitles4[usage - 0x8f]));
	else if (usage > 0xaf && usage < 0xc0)
		Notify((char*)pgm_read_word(&telTitles5[usage - 0xaf]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *consTitles0[]	PROGMEM	= 
{
	pstrUsageConsumerControl,		
	pstrUsageNumericKeyPad,		
	pstrUsageProgrammableButton,
	pstrUsageMicrophone,
	pstrUsageHeadphone,
	pstrUsageGraphicEqualizer	
};
const char *consTitles1[]	PROGMEM	= 
{
	pstrUsagePlus10	,
	pstrUsagePlus100,	
	pstrUsageAMPM	
};
const char *consTitles2[]	PROGMEM	= 
{
	pstrUsagePower			,
	pstrUsageReset			,
	pstrUsageSleep			,
	pstrUsageSleepAfter		,
	pstrUsageSleepMode		,
	pstrUsageIllumination	,
	pstrUsageFunctionButtons

};
const char *consTitles3[]	PROGMEM	= 
{
	pstrUsageMenu			,
	pstrUsageMenuPick		,
	pstrUsageMenuUp			,
	pstrUsageMenuDown		,
	pstrUsageMenuLeft		,
	pstrUsageMenuRight		,
	pstrUsageMenuEscape		,
	pstrUsageMenuValueIncrease,
	pstrUsageMenuValueDecrease
};
const char *consTitles4[]	PROGMEM	= 
{
	pstrUsageDataOnScreen		,
	pstrUsageClosedCaption		,
	pstrUsageClosedCaptionSelect,	
	pstrUsageVCRTV				,
	pstrUsageBroadcastMode		,
	pstrUsageSnapshot			,
	pstrUsageStill				
};
const char *consTitles5[]	PROGMEM	= 
{
	pstrUsageSelection					,
	pstrUsageAssignSelection			,	
	pstrUsageModeStep					,
	pstrUsageRecallLast					,
	pstrUsageEnterChannel				,
	pstrUsageOrderMovie					,
	pstrUsageChannel					,	
	pstrUsageMediaSelection				,
	pstrUsageMediaSelectComputer		,	
	pstrUsageMediaSelectTV				,
	pstrUsageMediaSelectWWW				,
	pstrUsageMediaSelectDVD				,
	pstrUsageMediaSelectTelephone		,
	pstrUsageMediaSelectProgramGuide	,	
	pstrUsageMediaSelectVideoPhone		,
	pstrUsageMediaSelectGames			,
	pstrUsageMediaSelectMessages		,	
	pstrUsageMediaSelectCD				,
	pstrUsageMediaSelectVCR				,
	pstrUsageMediaSelectTuner			,
	pstrUsageQuit						,
	pstrUsageHelp						,
	pstrUsageMediaSelectTape			,	
	pstrUsageMediaSelectCable			,
	pstrUsageMediaSelectSatellite		,
	pstrUsageMediaSelectSecurity		,	
	pstrUsageMediaSelectHome			,	
	pstrUsageMediaSelectCall			,	
	pstrUsageChannelIncrement			,
	pstrUsageChannelDecrement			,
	pstrUsageMediaSelectSAP				,
	pstrUsagePageReserved				,
	pstrUsageVCRPlus					,	
	pstrUsageOnce						,
	pstrUsageDaily						,
	pstrUsageWeekly						,
	pstrUsageMonthly					
};
const char *consTitles6[]	PROGMEM	= 
{
	pstrUsagePlay					,	
	pstrUsagePause					,	
	pstrUsageRecord					,
	pstrUsageFastForward			,	
	pstrUsageRewind					,
	pstrUsageScanNextTrack			,	
	pstrUsageScanPreviousTrack		,	
	pstrUsageStop					,	
	pstrUsageEject					,	
	pstrUsageRandomPlay				,	
	pstrUsageSelectDisk				,	
	pstrUsageEnterDisk				,	
	pstrUsageRepeat					,
	pstrUsageTracking					,
	pstrUsageTrackNormal				,	
	pstrUsageSlowTracking				,
	pstrUsageFrameForward				,
	pstrUsageFrameBackwards				,
	pstrUsageMark						,
	pstrUsageClearMark					,
	pstrUsageRepeatFromMark				,
	pstrUsageReturnToMark				,
	pstrUsageSearchMarkForward			,
	pstrUsageSearchMarkBackwards		,	
	pstrUsageCounterReset				,
	pstrUsageShowCounter				,	
	pstrUsageTrackingIncrement			,
	pstrUsageTrackingDecrement			,
	pstrUsageStopEject					,
	pstrUsagePlayPause					,
	pstrUsagePlaySkip					
};
const char *consTitles7[]	PROGMEM	= 
{
	pstrUsageVolume						,
	pstrUsageBalance					,	
	pstrUsageMute						,
	pstrUsageBass						,
	pstrUsageTreble						,
	pstrUsageBassBoost					,
	pstrUsageSurroundMode				,
	pstrUsageLoudness					,
	pstrUsageMPX						,	
	pstrUsageVolumeIncrement			,	
	pstrUsageVolumeDecrement				
};
const char *consTitles8[]	PROGMEM	= 
{
	pstrUsageSpeedSelect				,	
	pstrUsagePlaybackSpeed				,
	pstrUsageStandardPlay				,
	pstrUsageLongPlay					,
	pstrUsageExtendedPlay				,
	pstrUsageSlow						
};
const char *consTitles9[]	PROGMEM	= 
{
	pstrUsageFanEnable					,
	pstrUsageFanSpeed					,
	pstrUsageLightEnable				,	
	pstrUsageLightIlluminationLevel		,
	pstrUsageClimateControlEnable		,
	pstrUsageRoomTemperature			,	
	pstrUsageSecurityEnable				,
	pstrUsageFireAlarm					,
	pstrUsagePoliceAlarm				,	
	pstrUsageProximity					,
	pstrUsageMotion						,
	pstrUsageDuresAlarm					,
	pstrUsageHoldupAlarm					,
	pstrUsageMedicalAlarm				
};
const char *consTitlesA[]	PROGMEM	= 
{
	pstrUsageBalanceRight				,
	pstrUsageBalanceLeft				,	
	pstrUsageBassIncrement				,
	pstrUsageBassDecrement				,
	pstrUsageTrebleIncrement			,
	pstrUsageTrebleDecrement				
};
const char *consTitlesB[]	PROGMEM	= 
{
	pstrUsageSpeakerSystem				,
	pstrUsageChannelLeft				,	
	pstrUsageChannelRight				,
	pstrUsageChannelCenter				,
	pstrUsageChannelFront				,
	pstrUsageChannelCenterFront			,
	pstrUsageChannelSide				,	
	pstrUsageChannelSurround			,	
	pstrUsageChannelLowFreqEnhancement	,
	pstrUsageChannelTop					,
	pstrUsageChannelUnknown				
};
const char *consTitlesC[]	PROGMEM	= 
{
	pstrUsageSubChannel					,
	pstrUsageSubChannelIncrement		,	
	pstrUsageSubChannelDecrement		,	
	pstrUsageAlternateAudioIncrement	,	
	pstrUsageAlternateAudioDecrement		
};
const char *consTitlesD[]	PROGMEM	= 
{
	pstrUsageApplicationLaunchButtons	,
	pstrUsageALLaunchButtonConfigTool	,
	pstrUsageALProgrammableButton		,
	pstrUsageALConsumerControlConfig	,	
	pstrUsageALWordProcessor			,	
	pstrUsageALTextEditor				,
	pstrUsageALSpreadsheet				,
	pstrUsageALGraphicsEditor			,
	pstrUsageALPresentationApp			,
	pstrUsageALDatabaseApp				,
	pstrUsageALEmailReader				,
	pstrUsageALNewsreader				,
	pstrUsageALVoicemail				,	
	pstrUsageALContactsAddressBook		,
	pstrUsageALCalendarSchedule			,
	pstrUsageALTaskProjectManager		,
	pstrUsageALLogJournalTimecard		,
	pstrUsageALCheckbookFinance			,
	pstrUsageALCalculator				,
	pstrUsageALAVCapturePlayback		,	
	pstrUsageALLocalMachineBrowser		,
	pstrUsageALLANWANBrow				,
	pstrUsageALInternetBrowser			,
	pstrUsageALRemoteNetISPConnect		,
	pstrUsageALNetworkConference		,	
	pstrUsageALNetworkChat				,
	pstrUsageALTelephonyDialer			,
	pstrUsageALLogon					,	
	pstrUsageALLogoff					,
	pstrUsageALLogonLogoff				,
	pstrUsageALTermLockScrSav			,
	pstrUsageALControlPannel			,	
	pstrUsageALCommandLineProcessorRun	,
	pstrUsageALProcessTaskManager		,
	pstrUsageALSelectTaskApplication	,	
	pstrUsageALNextTaskApplication		,
	pstrUsageALPreviousTaskApplication	,
	pstrUsageALPreemptiveHaltTaskApp	,	
	pstrUsageALIntegratedHelpCenter		,
	pstrUsageALDocuments				,	
	pstrUsageALThesaurus				,	
	pstrUsageALDictionary				,
	pstrUsageALDesktop					,
	pstrUsageALSpellCheck				,
	pstrUsageALGrammarCheck				,
	pstrUsageALWirelessStatus			,
	pstrUsageALKeyboardLayout			,
	pstrUsageALVirusProtection			,
	pstrUsageALEncryption				,
	pstrUsageALScreenSaver				,
	pstrUsageALAlarms					,
	pstrUsageALClock					,	
	pstrUsageALFileBrowser				,
	pstrUsageALPowerStatus				,
	pstrUsageALImageBrowser				,
	pstrUsageALAudioBrowser				,
	pstrUsageALMovieBrowser				,
	pstrUsageALDigitalRightsManager		,
	pstrUsageALDigitalWallet			,
	pstrUsagePageReserved				,
	pstrUsageALInstantMessaging			,
	pstrUsageALOEMFeaturesBrowser		,
	pstrUsageALOEMHelp					,
	pstrUsageALOnlineCommunity			,
	pstrUsageALEntertainmentContentBrow	,
	pstrUsageALOnlineShoppingBrowser	,	
	pstrUsageALSmartCardInfoHelp		,	
	pstrUsageALMarketMonitorFinBrowser	,
	pstrUsageALCustomCorpNewsBrowser		,
	pstrUsageALOnlineActivityBrowser		,
	pstrUsageALResearchSearchBrowser		,
	pstrUsageALAudioPlayer				
};
const char *consTitlesE[]	PROGMEM	= 
{
	pstrUsageGenericGUIAppControls		,
	pstrUsageACNew						,
	pstrUsageACOpen						,
	pstrUsageACClose					,	
	pstrUsageACExit						,
	pstrUsageACMaximize					,
	pstrUsageACMinimize					,
	pstrUsageACSave						,
	pstrUsageACPrint					,	
	pstrUsageACProperties				,
	pstrUsageACUndo						,
	pstrUsageACCopy						,
	pstrUsageACCut						,
	pstrUsageACPaste					,	
	pstrUsageACSelectAll				,	
	pstrUsageACFind						,
	pstrUsageACFindAndReplace			,
	pstrUsageACSearch					,
	pstrUsageACGoto						,
	pstrUsageACHome						,
	pstrUsageACBack						,
	pstrUsageACForward					,
	pstrUsageACStop						,
	pstrUsageACRefresh					,
	pstrUsageACPreviousLink				,
	pstrUsageACNextLink					,
	pstrUsageACBookmarks				,	
	pstrUsageACHistory					,
	pstrUsageACSubscriptions			,	
	pstrUsageACZoomIn					,
	pstrUsageACZoomOut					,
	pstrUsageACZoom						,
	pstrUsageACFullScreenView			,
	pstrUsageACNormalView				,
	pstrUsageACViewToggle				,
	pstrUsageACScrollUp					,
	pstrUsageACScrollDown				,
	pstrUsageACScroll					,
	pstrUsageACPanLeft					,
	pstrUsageACPanRight					,
	pstrUsageACPan						,
	pstrUsageACNewWindow				,	
	pstrUsageACTileHoriz				,	
	pstrUsageACTileVert					,
	pstrUsageACFormat					,
	pstrUsageACEdit						,
	pstrUsageACBold						,
	pstrUsageACItalics					,
	pstrUsageACUnderline				,	
	pstrUsageACStrikethrough			,	
	pstrUsageACSubscript				,	
	pstrUsageACSuperscript				,
	pstrUsageACAllCaps					,
	pstrUsageACRotate					,
	pstrUsageACResize					,
	pstrUsageACFlipHorizontal			,
	pstrUsageACFlipVertical				,
	pstrUsageACMirrorHorizontal			,
	pstrUsageACMirrorVertical			,
	pstrUsageACFontSelect				,
	pstrUsageACFontColor				,	
	pstrUsageACFontSize					,
	pstrUsageACJustifyLeft				,
	pstrUsageACJustifyCenterH			,
	pstrUsageACJustifyRight				,
	pstrUsageACJustifyBlockH			,	
	pstrUsageACJustifyTop				,
	pstrUsageACJustifyCenterV			,
	pstrUsageACJustifyBottom			,	
	pstrUsageACJustifyBlockV			,	
	pstrUsageACIndentDecrease			,
	pstrUsageACIndentIncrease			,
	pstrUsageACNumberedList				,
	pstrUsageACRestartNumbering			,
	pstrUsageACBulletedList				,
	pstrUsageACPromote					,
	pstrUsageACDemote					,
	pstrUsageACYes						,
	pstrUsageACNo						,
	pstrUsageACCancel					,
	pstrUsageACCatalog					,
	pstrUsageACBuyChkout				,	
	pstrUsageACAddToCart				,	
	pstrUsageACExpand					,
	pstrUsageACExpandAll				,	
	pstrUsageACCollapse					,
	pstrUsageACCollapseAll				,
	pstrUsageACPrintPreview				,
	pstrUsageACPasteSpecial				,
	pstrUsageACInsertMode				,
	pstrUsageACDelete					,
	pstrUsageACLock						,
	pstrUsageACUnlock					,
	pstrUsageACProtect					,
	pstrUsageACUnprotect				,	
	pstrUsageACAttachComment			,	
	pstrUsageACDeleteComment			,	
	pstrUsageACViewComment				,
	pstrUsageACSelectWord				,
	pstrUsageACSelectSentence			,
	pstrUsageACSelectParagraph			,
	pstrUsageACSelectColumn				,
	pstrUsageACSelectRow				,	
	pstrUsageACSelectTable				,
	pstrUsageACSelectObject				,
	pstrUsageACRedoRepeat				,
	pstrUsageACSort						,
	pstrUsageACSortAscending			,	
	pstrUsageACSortDescending			,
	pstrUsageACFilter					,
	pstrUsageACSetClock					,
	pstrUsageACViewClock				,	
	pstrUsageACSelectTimeZone			,
	pstrUsageACEditTimeZone				,
	pstrUsageACSetAlarm					,
	pstrUsageACClearAlarm				,
	pstrUsageACSnoozeAlarm				,
	pstrUsageACResetAlarm				,
	pstrUsageACSyncronize				,
	pstrUsageACSendReceive				,
	pstrUsageACSendTo					,
	pstrUsageACReply					,	
	pstrUsageACReplyAll					,
	pstrUsageACForwardMessage			,
	pstrUsageACSend						,
	pstrUsageACAttachFile				,
	pstrUsageACUpload					,
	pstrUsageACDownload					,
	pstrUsageACSetBorders				,
	pstrUsageACInsertRow				,	
	pstrUsageACInsertColumn				,
	pstrUsageACInsertFile				,
	pstrUsageACInsertPicture			,	
	pstrUsageACInsertObject				,
	pstrUsageACInsertSymbol				,
	pstrUsageACSaveAndClose				,
	pstrUsageACRename					,
	pstrUsageACMerge					,
	pstrUsageACSplit					,
	pstrUsageACDistributeHorizontaly	,
	pstrUsageACDistributeVerticaly		
};
	
void ReportDescParser::PrintConsumerPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	
	if (usage > 0x00 && usage < 0x07)
		Notify((char*)pgm_read_word(&consTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x23)
		Notify((char*)pgm_read_word(&consTitles1[usage - 0x1f]));
	else if (usage > 0x2f && usage < 0x37)
		Notify((char*)pgm_read_word(&consTitles2[usage - 0x2f]));
	else if (usage > 0x3f && usage < 0x49)
		Notify((char*)pgm_read_word(&consTitles3[usage - 0x3f]));
	else if (usage > 0x5f && usage < 0x67)
		Notify((char*)pgm_read_word(&consTitles4[usage - 0x5f]));
	else if (usage > 0x7f && usage < 0xa5)
		Notify((char*)pgm_read_word(&consTitles5[usage - 0x7f]));
	else if (usage > 0xaf && usage < 0xcf)
		Notify((char*)pgm_read_word(&consTitles6[usage - 0xaf]));
	else if (usage > 0xdf && usage < 0xeb)
		Notify((char*)pgm_read_word(&consTitles7[usage - 0xdf]));
	else if (usage > 0xef && usage < 0xf6)
		Notify((char*)pgm_read_word(&consTitles8[usage - 0xef]));
	else if (usage > 0xff && usage < 0x10e)
		Notify((char*)pgm_read_word(&consTitles9[usage - 0xff]));
	else if (usage > 0x14f && usage < 0x156)
		Notify((char*)pgm_read_word(&consTitlesA[usage - 0x14f]));
	else if (usage > 0x15f && usage < 0x16b)
		Notify((char*)pgm_read_word(&consTitlesB[usage - 0x15f]));
	else if (usage > 0x16f && usage < 0x175)
		Notify((char*)pgm_read_word(&consTitlesC[usage - 0x16f]));
	else if (usage > 0x17f && usage < 0x1c8)
		Notify((char*)pgm_read_word(&consTitlesD[usage - 0x17f]));
	else if (usage > 0x1ff && usage < 0x29d)
		Notify((char*)pgm_read_word(&consTitlesE[usage - 0x1ff]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *digitTitles0[] PROGMEM = 
{
	pstrUsageDigitizer					,
	pstrUsagePen						,	
	pstrUsageLightPen					,
	pstrUsageTouchScreen				,	
	pstrUsageTouchPad					,
	pstrUsageWhiteBoard					,
	pstrUsageCoordinateMeasuringMachine	,
	pstrUsage3DDigitizer				,	
	pstrUsageStereoPlotter				,
	pstrUsageArticulatedArm				,
	pstrUsageArmature					,
	pstrUsageMultiplePointDigitizer		,
	pstrUsageFreeSpaceWand				
};
const char *digitTitles1[] PROGMEM = 
{
	pstrUsageStylus						,
	pstrUsagePuck						,
	pstrUsageFinger						

};
const char *digitTitles2[] PROGMEM = 
{
	pstrUsageTipPressure			,	
	pstrUsageBarrelPressure			,
	pstrUsageInRange				,	
	pstrUsageTouch					,
	pstrUsageUntouch				,	
	pstrUsageTap					,	
	pstrUsageQuality				,	
	pstrUsageDataValid				,
	pstrUsageTransducerIndex		,	
	pstrUsageTabletFunctionKeys		,
	pstrUsageProgramChangeKeys		,
	pstrUsageBatteryStrength		,	
	pstrUsageInvert					,
	pstrUsageXTilt					,
	pstrUsageYTilt					,
	pstrUsageAzimuth				,	
	pstrUsageAltitude				,
	pstrUsageTwist					,
	pstrUsageTipSwitch				,
	pstrUsageSecondaryTipSwitch		,
	pstrUsageBarrelSwitch			,
	pstrUsageEraser					,
	pstrUsageTabletPick				
};

void ReportDescParser::PrintDigitizerPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	
	if (usage > 0x00 && usage < 0x0e)
		Notify((char*)pgm_read_word(&digitTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x23)
		Notify((char*)pgm_read_word(&digitTitles1[usage - 0x1f]));
	else if (usage > 0x2f && usage < 0x47)
		Notify((char*)pgm_read_word(&digitTitles2[usage - 0x2f]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *aplphanumTitles0[]	PROGMEM =
{
	pstrUsageAlphanumericDisplay,
	pstrUsageBitmappedDisplay	
};
const char *aplphanumTitles1[]	PROGMEM =
{
	pstrUsageDisplayAttributesReport	,	
	pstrUsageASCIICharacterSet			,
	pstrUsageDataReadBack				,
	pstrUsageFontReadBack				,
	pstrUsageDisplayControlReport		,
	pstrUsageClearDisplay				,
	pstrUsageDisplayEnable				,
	pstrUsageScreenSaverDelay			,
	pstrUsageScreenSaverEnable			,
	pstrUsageVerticalScroll				,
	pstrUsageHorizontalScroll			,
	pstrUsageCharacterReport			,	
	pstrUsageDisplayData				,	
	pstrUsageDisplayStatus				,
	pstrUsageStatusNotReady				,
	pstrUsageStatusReady				,	
	pstrUsageErrorNotALoadableCharacter	,
	pstrUsageErrorFotDataCanNotBeRead	,
	pstrUsageCursorPositionReport		,
	pstrUsageRow						,	
	pstrUsageColumn						,
	pstrUsageRows						,
	pstrUsageColumns					,	
	pstrUsageCursorPixelPosition		,	
	pstrUsageCursorMode					,
	pstrUsageCursorEnable				,
	pstrUsageCursorBlink				,	
	pstrUsageFontReport					,
	pstrUsageFontData					,
	pstrUsageCharacterWidth				,
	pstrUsageCharacterHeight			,	
	pstrUsageCharacterSpacingHorizontal	,
	pstrUsageCharacterSpacingVertical	,
	pstrUsageUnicodeCharset				,
	pstrUsageFont7Segment				,
	pstrUsage7SegmentDirectMap			,
	pstrUsageFont14Segment				,
	pstrUsage14SegmentDirectMap			,
	pstrUsageDisplayBrightness			,
	pstrUsageDisplayContrast			,	
	pstrUsageCharacterAttribute			,
	pstrUsageAttributeReadback			,
	pstrUsageAttributeData				,
	pstrUsageCharAttributeEnhance		,
	pstrUsageCharAttributeUnderline		,
	pstrUsageCharAttributeBlink			
};
const char *aplphanumTitles2[]	PROGMEM =
{
	pstrUsageBitmapSizeX				,	
	pstrUsageBitmapSizeY				,
	pstrUsagePageReserved				,
	pstrUsageBitDepthFormat				,
	pstrUsageDisplayOrientation			,
	pstrUsagePaletteReport				,
	pstrUsagePaletteDataSize			,	
	pstrUsagePaletteDataOffset			,
	pstrUsagePaletteData				,	
	pstrUsageBlitReport					,
	pstrUsageBlitRectangleX1			,	
	pstrUsageBlitRectangleY1			,	
	pstrUsageBlitRectangleX2			,	
	pstrUsageBlitRectangleY2			,	
	pstrUsageBlitData					,
	pstrUsageSoftButton					,
	pstrUsageSoftButtonID				,
	pstrUsageSoftButtonSide				,
	pstrUsageSoftButtonOffset1			,
	pstrUsageSoftButtonOffset2			,
	pstrUsageSoftButtonReport			
};

void ReportDescParser::PrintAlphanumDisplayPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	
	if (usage > 0x00 && usage < 0x03)
		Notify((char*)pgm_read_word(&aplphanumTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x4e)
		Notify((char*)pgm_read_word(&aplphanumTitles1[usage - 0x1f]));
	else if (usage > 0x7f && usage < 0x96)
		Notify((char*)pgm_read_word(&digitTitles2[usage - 0x80]));
	else
		Notify(pstrUsagePageUndefined);
}

const char *medInstrTitles0[] PROGMEM =
{
	pstrUsageVCRAcquisition				,
	pstrUsageFreezeThaw					,
	pstrUsageClipStore					,
	pstrUsageUpdate						,
	pstrUsageNext						,
	pstrUsageSave						,
	pstrUsagePrint						,
	pstrUsageMicrophoneEnable			
};
const char *medInstrTitles1[] PROGMEM =
{
	pstrUsageCine						,
	pstrUsageTransmitPower				,
	pstrUsageVolume						,
	pstrUsageFocus						,
	pstrUsageDepth						
};
const char *medInstrTitles2[] PROGMEM =
{
	pstrUsageSoftStepPrimary		,	
	pstrUsageSoftStepSecondary		
};
const char *medInstrTitles3[] PROGMEM =
{
	pstrUsageZoomSelect					,
	pstrUsageZoomAdjust					,
	pstrUsageSpectralDopplerModeSelect	,
	pstrUsageSpectralDopplerModeAdjust	,
	pstrUsageColorDopplerModeSelect		,
	pstrUsageColorDopplerModeAdjust		,
	pstrUsageMotionModeSelect			,
	pstrUsageMotionModeAdjust			,
	pstrUsage2DModeSelect				,
	pstrUsage2DModeAdjust				
};
const char *medInstrTitles4[] PROGMEM =
{
	pstrUsageSoftControlSelect			,
	pstrUsageSoftControlAdjust			
};

void ReportDescParser::PrintMedicalInstrumentPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	
	if (usage == 1)
		Notify(pstrUsageMedicalUltrasound);
	else if (usage > 0x1f && usage < 0x28)
		Notify((char*)pgm_read_word(&medInstrTitles0[usage - 0x1f]));
	else if (usage > 0x3f && usage < 0x45)
		Notify((char*)pgm_read_word(&medInstrTitles1[usage - 0x40]));
	else if (usage > 0x5f && usage < 0x62)
		Notify((char*)pgm_read_word(&medInstrTitles2[usage - 0x60]));
	else if (usage == 0x70)
		Notify(pstrUsageDepthGainCompensation);
	else if (usage > 0x7f && usage < 0x8a)
		Notify((char*)pgm_read_word(&medInstrTitles3[usage - 0x80]));
	else if (usage > 0x9f && usage < 0xa2)
		Notify((char*)pgm_read_word(&medInstrTitles4[usage - 0xa0]));
	else
		Notify(pstrUsagePageUndefined);
}

void HID::PrintEndpointDescriptor( const USB_ENDPOINT_DESCRIPTOR* ep_ptr )
{
	Notify(PSTR("Endpoint descriptor:"));
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

void HID::PrintHidDescriptor(const USB_HID_DESCRIPTOR *pDesc)
{
    Notify(PSTR("\r\n\r\nHID Descriptor:\r\n"));
    Notify(PSTR("bDescLength:\t\t"));
    PrintHex<uint8_t>(pDesc->bLength);
    
    Notify(PSTR("\r\nbDescriptorType:\t"));
    PrintHex<uint8_t>(pDesc->bDescriptorType);
    
    Notify(PSTR("\r\nbcdHID:\t\t\t"));
    PrintHex<uint16_t>(pDesc->bcdHID);
    
    Notify(PSTR("\r\nbCountryCode:\t\t"));
    PrintHex<uint8_t>(pDesc->bCountryCode);
    
    Notify(PSTR("\r\nbNumDescriptors:\t"));
    PrintHex<uint8_t>(pDesc->bNumDescriptors);
    
    Notify(PSTR("\r\nbDescrType:\t\t"));
    PrintHex<uint8_t>(pDesc->bDescrType);
    
    Notify(PSTR("\r\nwDescriptorLength:\t"));
    PrintHex<uint16_t>(pDesc->wDescriptorLength);
}
