#include "masstorage.h"


//bool BulkReadParser::IsValidCSW(uint8_t size, uint8_t *pcsw)
//{
//	if (size != 0x0d)
//	{
//		Notify(PSTR("CSW:Size error"));
//		return false;
//	}
//	if (*((uint32_t*)pcsw) != MASS_CSW_SIGNATURE)
//	{
//		Notify(PSTR("CSW:Sig error"));
//		return false;
//	}
//	//if (size != 0x0d || *((uint32_t*)pcsw) != MASS_CSW_SIGNATURE ||
//	//	((CommandStatusWrapper*)pcsw)->dCSWTag != dCBWTag)
//	//	return false;
//	return true;
//}

//bool BulkReadParser::IsMeaningfulCSW(uint8_t size, uint8_t *pcsw)
//{
//	if (((CommandStatusWrapper*)pcsw)->bCSWStatus		<  2 && 
//		((CommandStatusWrapper*)pcsw)->dCSWDataResidue	<= dCBWDataTransferLength )
//		return true;
//	if ( ((CommandStatusWrapper*)pcsw)->bCSWStatus == 2 )
//		return true;
//	return false;
//}

//void BulkReadParser::Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset)
//{
//	if (offset == 0 && len > sizeof(CommandStatusWrapper))
//		if (IsValidCSW(sizeof(CommandStatusWrapper), pbuf) && IsMeaningfulCSW(sizeof(CommandStatusWrapper), pbuf))
//		{
//			CommandStatusWrapper	*pCSW = (CommandStatusWrapper*)pbuf;
//
//			Serial.println("Sig:");
//			PrintHex<uint32_t>(pCSW->dCSWSignature);
//			Serial.println("Tag:");
//			PrintHex<uint32_t>(pCSW->dCSWTag);
//			Serial.println("Res:");
//			PrintHex<uint32_t>(pCSW->dCSWDataResidue);
//			Serial.println("Ret:");
//			PrintHex<uint8_t>(pCSW->bCSWStatus);
//		}
//}

const uint8_t	BulkOnly::epDataInIndex			= 1;			
const uint8_t	BulkOnly::epDataOutIndex		= 2;		
const uint8_t	BulkOnly::epInterruptInIndex	= 3;	

BulkOnly::BulkOnly(USB *p /*, CDCAsyncOper *pasync*/) :
	pUsb(p),
	//pAsync(pasync),
	bAddress(0),
	qNextPollTime(0),	
	bPollEnable(false),	
	bIface(0),	
	bNumEP(1)			
{
	for(uint8_t i=0; i<MASS_MAX_ENDPOINTS; i++)
	{
		epInfo[i].epAddr		= 0;
		epInfo[i].maxPktSize	= (i) ? 0 : 8;
		epInfo[i].epAttribs		= 0;

		if (!i)
			epInfo[i].bmNakPower	= USB_NAK_MAX_POWER;
	}
	if (pUsb)
		pUsb->RegisterDeviceClass(this);
}

uint8_t BulkOnly::Init(uint8_t parent, uint8_t port, bool lowspeed)
{
	const uint8_t constBufSize = sizeof(USB_DEVICE_DESCRIPTOR);

	uint8_t		buf[constBufSize];
	uint8_t		rcode;
	UsbDevice	*p = NULL;
	EpInfo		*oldep_ptr = NULL;
	uint8_t		num_of_conf;	// number of configurations

	AddressPool	&addrPool = pUsb->GetAddressPool();

	USBTRACE("MS Init\r\n");

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
	rcode = pUsb->getDevDescr( 0, 0, constBufSize, (uint8_t*)buf );

	// Restore p->epinfo
	p->epinfo = oldep_ptr;

	if( rcode ) 
		goto FailGetDevDescr;

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

	num_of_conf = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;

	// Assign epInfo to epinfo pointer
	rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);

	if (rcode)
		goto FailSetDevTblEntry;

	USBTRACE2("NC:", num_of_conf);

	for (uint8_t i=0; i<num_of_conf; i++)
	{
		HexDumper<USBReadParser, uint16_t, uint16_t>		HexDump;
		ConfigDescParser<	USB_CLASS_MASS_STORAGE, 
							MASS_SUBCLASS_SCSI, 
							MASS_PROTO_BBB, 
							CP_MASK_COMPARE_CLASS | 
							CP_MASK_COMPARE_SUBCLASS | 
							CP_MASK_COMPARE_PROTOCOL>		BulkOnlyParser(this);
		
		rcode = pUsb->getConfDescr(bAddress, 0, i, &HexDump);
		rcode = pUsb->getConfDescr(bAddress, 0, i, &BulkOnlyParser);


		if (bNumEP > 1)
			break;
	} // for
	
	if (bNumEP < 3)
		return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

	// Assign epInfo to epinfo pointer
	rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);

	USBTRACE2("Conf:", bConfNum);

	// Set Configuration Value
	rcode = pUsb->setConf(bAddress, 0, bConfNum);

	if (rcode)
		goto FailSetConf;

	delay(5000);

	//rcode = pAsync->OnInit(this);

	//if (rcode)
	//	goto FailOnInit;

	rcode = GetMaxLUN(&bMaxLUN);

	if (rcode)
		goto FailGetMaxLUN;

	delay(10);

    {
        InquiryResponse response;
        rcode = Inquiry(bMaxLUN, sizeof(InquiryResponse), (uint8_t*)&response);

		if (rcode)
			goto FailInquiry;

		//if (response.DeviceType != 0)
		//	goto FailInvalidDevice;
	}

	delay(10);

	USBTRACE("MS configured\r\n");

	bPollEnable = true;

	//USBTRACE("Poll enabled\r\n");
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

FailSetConf:
	USBTRACE("setConf:");
	goto Fail;

FailOnInit:
	USBTRACE("OnInit:");
	goto Fail;

FailGetMaxLUN:
	USBTRACE("GetMaxLUN:");
	goto Fail;

FailInquiry:
	USBTRACE("Inquiry:");
	goto Fail;

Fail:
	Serial.println(rcode, HEX);
	Release();
	return rcode;
}


void BulkOnly::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) 
{
	ErrorMessage<uint8_t>(PSTR("Conf.Val"),	conf);
	ErrorMessage<uint8_t>(PSTR("Iface Num"),iface);
	ErrorMessage<uint8_t>(PSTR("Alt.Set"),	alt);

	bConfNum = conf;

	uint8_t index;

	if ((pep->bmAttributes & 0x03) == 3 && (pep->bEndpointAddress & 0x80) == 0x80)
		index = epInterruptInIndex;
	else 
		if ((pep->bmAttributes & 0x02) == 2)
			index = ((pep->bEndpointAddress & 0x80) == 0x80) ? epDataInIndex : epDataOutIndex; 
		else
			return;

	// Fill in the endpoint info structure
	epInfo[index].epAddr		= (pep->bEndpointAddress & 0x0F);
	epInfo[index].maxPktSize	= (uint8_t)pep->wMaxPacketSize;
	epInfo[index].epAttribs		= 0;

	bNumEP ++;

	PrintEndpointDescriptor(pep);
}

uint8_t BulkOnly::Release()
{
	pUsb->GetAddressPool().FreeAddress(bAddress);

	bIface				= 0;		
	bNumEP				= 1;			

	bAddress			= 0;
	qNextPollTime		= 0;
	bPollEnable			= false;
	return 0;
}

uint8_t BulkOnly::Poll()
{
	uint8_t rcode = 0;

	if (!bPollEnable)
		return 0;

	uint32_t	time_now = millis();

	//if (qNextPollTime <= time_now)
	//{
	//	qNextPollTime = time_now + 100;

	//	uint8_t			rcode;
	//	const uint8_t	constBufSize = 16;
	//	uint8_t			buf[constBufSize];

	//	for (uint8_t i=0; i<constBufSize; i++)
	//		buf[i] = 0;

	//	uint16_t	read = (constBufSize > epInfo[epInterruptInIndex].maxPktSize) 
	//						? epInfo[epInterruptInIndex].maxPktSize : constBufSize;
	//	rcode = pUsb->inTransfer(bAddress, epInfo[epInterruptInIndex].epAddr, &read, buf);

	//	if (rcode)
	//		return rcode;

	//	for (uint8_t i=0; i<read; i++)
	//	{
	//		PrintHex<uint8_t>(buf[i]);
	//		Serial.print(" ");
	//	}
	//	USBTRACE("\r\n");
	//}
	return rcode;
}

bool BulkOnly::IsValidCBW(uint8_t size, uint8_t *pcbw)
{
	if (size != 0x1f || *((uint32_t*)pcbw) != MASS_CBW_SIGNATURE)
		return false;
	return true;
}

bool BulkOnly::IsMeaningfulCBW(uint8_t size, uint8_t *pcbw)
{
	if (((CommandBlockWrapper*)pcbw)->bmReserved1	!= 0		||
		((CommandBlockWrapper*)pcbw)->bmReserved2	!= 0		||
		((CommandBlockWrapper*)pcbw)->bmCBWLUN		>  bMaxLUN	||
		((CommandBlockWrapper*)pcbw)->bmCBWCBLength >  0x10 )
		return false;
	return true;
}

uint8_t BulkOnly::Reset()
{
	return( pUsb->ctrlReq( bAddress, 0, bmREQ_MASSOUT, MASS_REQ_BOMSR, 0, 0, bIface, 0, 0, NULL, NULL ));        
}

uint8_t BulkOnly::GetMaxLUN(uint8_t *plun)
{
	uint8_t cnt = 3;

	bLastUsbError = pUsb->ctrlReq( bAddress, 0, bmREQ_MASSIN, MASS_REQ_GET_MAX_LUN, 0, 0, bIface, 1, 1, plun, NULL ); 
	
	delay(10);
	//Serial.println(F("bLastUsbError: "));
	//Serial.println(bLastUsbError);

	if (bLastUsbError == hrSTALL)
	{
		*plun = 0;
		bLastUsbError = ClearEpHalt(epDataInIndex);
		return MASS_ERR_SUCCESS;
	}
	if (bLastUsbError == hrJERR)
		return MASS_ERR_DEVICE_DISCONNECTED;
	else if (bLastUsbError)
		return MASS_ERR_GENERAL_USB_ERROR;
	return MASS_ERR_SUCCESS;
}

uint8_t BulkOnly::HandleUsbError(uint8_t index)
{
	uint8_t count = 3;

	while (bLastUsbError && count)
	{
		switch (bLastUsbError)
		{
		case hrSUCCESS:
			return MASS_ERR_SUCCESS;
		case hrJERR: 
			bLastUsbError = hrSUCCESS;
			return MASS_ERR_DEVICE_DISCONNECTED;
		case hrSTALL:
			bLastUsbError = ClearEpHalt(index);
			break;
		default:
			return MASS_ERR_GENERAL_USB_ERROR;
		}
		count --;
	} // while
	
	return MASS_ERR_SUCCESS;
}

uint8_t BulkOnly::ClearEpHalt(uint8_t index)
{
	return (pUsb->ctrlReq( bAddress, 0, USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_ENDPOINT, 
		USB_REQUEST_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, 0, epInfo[index].epAddr, 0, 0, NULL, NULL ));
}

uint8_t BulkOnly::ResetRecovery()
{
	bLastUsbError = Reset();

	if (bLastUsbError) 
		return bLastUsbError;

	delay(6);

	bLastUsbError = ClearEpHalt(epDataInIndex);

	if (bLastUsbError) 
		return bLastUsbError;

	delay(6);

	bLastUsbError = ClearEpHalt(epDataOutIndex);

	delay(6);

	return bLastUsbError;
}

uint8_t BulkOnly::Inquiry(uint8_t lun, uint16_t bsize, uint8_t *buf)
{
	CommandBlockWrapper cbw; 
	
	cbw.dCBWSignature			= MASS_CBW_SIGNATURE;
	cbw.dCBWTag					= 0xdeadbeef;
	cbw.dCBWDataTransferLength	= bsize;
	cbw.bmCBWFlags				= MASS_CMD_DIR_IN,
	cbw.bmCBWLUN				= lun;
	cbw.bmCBWCBLength			= 6;

	for (uint8_t i=0; i<16; i++)
		cbw.CBWCB[i] = 0;

	cbw.CBWCB[0] = SCSI_CMD_INQUIRY;
	cbw.CBWCB[4] = bsize;

	return Transaction(&cbw, bsize, buf, 0);
}

uint8_t BulkOnly::RequestSense(uint8_t lun, uint16_t size, uint8_t *buf)
{
	CommandBlockWrapper cbw; 
	
	cbw.dCBWSignature			= MASS_CBW_SIGNATURE;
	cbw.dCBWTag					= 0xdeadbeef;
	cbw.dCBWDataTransferLength	= size;
	cbw.bmCBWFlags				= MASS_CMD_DIR_IN,
	cbw.bmCBWLUN				= lun;
	cbw.bmCBWCBLength			= 6;

	for (uint8_t i=0; i<16; i++)
		cbw.CBWCB[i] = 0;

	cbw.CBWCB[0] = SCSI_CMD_REQUEST_SENSE;
	cbw.CBWCB[4] = size;

	return Transaction(&cbw, size, buf, 0);
}

uint8_t BulkOnly::ReadCapacity(uint8_t lun, uint16_t bsize, uint8_t *buf)
{
	CommandBlockWrapper cbw; 
	
	cbw.dCBWSignature			= MASS_CBW_SIGNATURE;
	cbw.dCBWTag					= 0xdeadbeef;
	cbw.dCBWDataTransferLength	= bsize;
	cbw.bmCBWFlags				= MASS_CMD_DIR_IN,
	cbw.bmCBWLUN				= lun;
	cbw.bmCBWCBLength			= 10;

	for (uint8_t i=0; i<16; i++)
		cbw.CBWCB[i] = 0;

	cbw.CBWCB[0] = SCSI_CMD_READ_CAPACITY_10;
	cbw.CBWCB[4] = bsize;

	return Transaction(&cbw, bsize, buf, 0);
}

uint8_t BulkOnly::TestUnitReady(uint8_t lun)
{
	CommandBlockWrapper cbw; 
	
	cbw.dCBWSignature			= MASS_CBW_SIGNATURE;
	cbw.dCBWTag					= 0xdeadbeef;
	cbw.dCBWDataTransferLength	= 0;
	cbw.bmCBWFlags				= MASS_CMD_DIR_OUT,
	cbw.bmCBWLUN				= lun;
	cbw.bmCBWCBLength			= 6;

	for (uint8_t i=0; i<16; i++)
		cbw.CBWCB[i] = 0;

	cbw.CBWCB[0] = SCSI_CMD_TEST_UNIT_READY;

	return Transaction(&cbw, 0, NULL, 0);
}

uint8_t BulkOnly::Read(uint8_t lun, uint32_t addr, uint16_t bsize, USBReadParser *prs)
{
	CommandBlockWrapper cbw; 
	
	cbw.dCBWSignature			= MASS_CBW_SIGNATURE;
	cbw.dCBWTag					= 0xdeadbeef;
	cbw.dCBWDataTransferLength	= bsize;
	cbw.bmCBWFlags				= MASS_CMD_DIR_IN,
	cbw.bmCBWLUN				= lun;
	cbw.bmCBWCBLength			= 10;

	for (uint8_t i=0; i<16; i++)
		cbw.CBWCB[i] = 0;

	cbw.CBWCB[0] = SCSI_CMD_READ_10;
	cbw.CBWCB[8] = 1;
	cbw.CBWCB[5] = (addr & 0xff);
	cbw.CBWCB[4] = ((addr >> 8) & 0xff);
	cbw.CBWCB[3] = ((addr >> 16) & 0xff);
	cbw.CBWCB[2] = ((addr >> 24) & 0xff);

	return Transaction(&cbw, bsize, prs, 1);
}

uint8_t BulkOnly::Transaction(CommandBlockWrapper *cbw, uint16_t size, void *buf, uint8_t flags)
{
	uint16_t read;
	{
		bLastUsbError = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, sizeof(CommandBlockWrapper), (uint8_t*)cbw);

		uint8_t ret = HandleUsbError(epDataOutIndex);

		if (ret)
		{
			ErrorMessage<uint8_t>(PSTR("CBW"), ret);
			return ret;
		}
	}

	if (size && buf)
	{
		read = size;

		if (cbw->bmCBWFlags & MASS_CMD_DIR_IN)
		{
			if ((flags & MASS_TRANS_FLG_CALLBACK) == MASS_TRANS_FLG_CALLBACK)
			{
				const uint8_t	bufSize = 64;
				uint16_t		total	= size;
				uint16_t		count	= 0;
				uint8_t			rbuf[bufSize];

				read = bufSize;

				while(count < total && 
					((bLastUsbError = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, &read, (uint8_t*)rbuf)) == hrSUCCESS)
					)
				{
					((USBReadParser*)buf)->Parse(read, rbuf, count);

					count += read;
					read = bufSize;
				}

				if (bLastUsbError == hrSTALL)
					bLastUsbError = ClearEpHalt(epDataInIndex);

				if (bLastUsbError)
				{
					ErrorMessage<uint8_t>(PSTR("RDR"), bLastUsbError);
					return MASS_ERR_GENERAL_USB_ERROR;
				}
			} // if ((flags & 1) == 1)
			else
				bLastUsbError = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, &read, (uint8_t*)buf);
		} // if (cbw->bmCBWFlags & MASS_CMD_DIR_IN)

		else if (cbw->bmCBWFlags & MASS_CMD_DIR_OUT)
			bLastUsbError = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, read, (uint8_t*)buf);
	}

	uint8_t ret = HandleUsbError((cbw->bmCBWFlags & MASS_CMD_DIR_IN) ? epDataInIndex : epDataOutIndex);

	if (ret)
	{
		ErrorMessage<uint8_t>(PSTR("RSP"), ret);
		return MASS_ERR_GENERAL_USB_ERROR;
	}
	{
		CommandStatusWrapper	csw;
		read = sizeof(CommandStatusWrapper);

		bLastUsbError = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, &read, (uint8_t*)&csw);

		uint8_t ret = HandleUsbError(epDataInIndex);

		if (ret)
		{
			ErrorMessage<uint8_t>(PSTR("CSW"), ret);
			return ret;
		}
		//if (csw.bCSWStatus == MASS_ERR_PHASE_ERROR)
		//	bLastUsbError = ResetRecovery();

		return csw.bCSWStatus;
	}
	//return MASS_ERR_SUCCESS;
}

void BulkOnly::PrintEndpointDescriptor( const USB_ENDPOINT_DESCRIPTOR* ep_ptr )
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
	Notify(PSTR("\r\n"));
}
