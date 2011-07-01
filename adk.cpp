/* Google ADK interface */

#include "adk.h"

const uint8_t	ADK::epDataInIndex  = 1;			
const uint8_t	ADK::epDataOutIndex = 2;		

ADK::ADK(USB *p,  const char* manufacturer,
                  const char* model,
                  const char* description,
                  const char* version,
                  const char* uri,
                  const char* serial) :
  
  pUsb(p),            //pointer to USB class instance - mandatory for each driver
  manufacturer(manufacturer),
  model(model),
  description(description),
  version(version),
  uri(uri),
  serial(serial),
  
	bAddress(0),        //device address - mandatory for each driver
	bNumEP(1)           //if config descriptor needs to be parsed
{
  /* initialize endpoint data structures */
	for(uint8_t i=0; i<ADK_MAX_ENDPOINTS; i++)
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
/* Android initialization. Performed in 2 steps:
  1. If new device answers to vendor-specific request, another request switched device to accessory mode.
    The device is then reset and comes up on a bus with different VID/PID.
  2. After detecting by VID/PID enpoints are extracted and device is configured 
*/
uint8_t ADK::Init(uint8_t parent, uint8_t port, bool lowspeed)
{
	const uint8_t constBufSize = sizeof(USB_DEVICE_DESCRIPTOR);

	uint8_t		buf[constBufSize];
	uint8_t		rcode;
	UsbDevice	*p = NULL;
	EpInfo		*oldep_ptr = NULL;
	uint8_t		num_of_conf;	// number of configurations
  
  // get memory address of USB device address pool
	AddressPool	&addrPool = pUsb->GetAddressPool();

	USBTRACE("\r\nADK Init");
	
	
    // check if address has already been assigned to an instance
    if (bAddress)
		  return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
		  
    // Get pointer to pseudo device with address 0 assigned
	  p = addrPool.GetUsbDevicePtr(0);

	  if (!p)
		  return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

	  if (!p->epinfo) {
		  USBTRACE("epinfo is null\r\n");
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

	  if( rcode ){ 
		  goto FailGetDevDescr;
		}
		
		// Allocate new address according to device class
	  bAddress = addrPool.AllocAddress(parent, false, port);
	  if (!bAddress) {
		  return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
		}
	  
		// Extract Max Packet Size from the device descriptor
	  epInfo[0].maxPktSize = (uint8_t)((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0; 
		
		/* debug code start */
		num_of_conf = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;
		USBTRACE2("\r\nNum.conf: ", num_of_conf );
		for (uint8_t i=0; i<num_of_conf; i++) {
		//USBTRACE("\r\nHexdumper: ");  
		HexDumper<USBReadParser, uint16_t, uint16_t>		HexDump;
		//USBTRACE("\r\nHexdumper2: ");
		//ConfigDescParser<0, 0, 0, 
		//	0>							confDescrParser(this);
    
		rcode = pUsb->getConfDescr(bAddress, 0, i, &HexDump);
		//rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
		
		} // for (uint8_t i=0; i<num_of_conf; i++...
		
		/* debug code end */
		
		 //USBTRACE("Check!!!");
		//check if ADK device is already in accessory mode
		if(((USB_DEVICE_DESCRIPTOR*)buf)->idVendor == ADK_VID &&
            (((USB_DEVICE_DESCRIPTOR*)buf)->idProduct == ADK_PID || ((USB_DEVICE_DESCRIPTOR*)buf)->idProduct == ADB_PID)){
              USBTRACE("\r\nAcc.mode device detected");
              
              
              // Allocate new address
	            //bAddress = addrPool.AllocAddress(parent, false, port);

	            if (!bAddress) {
		            return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
		          }

	            // Extract Max Packet Size from the device descriptor
	            epInfo[0].maxPktSize = (uint8_t)((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0; 

	            // Assign new address to the device
	            rcode = pUsb->setAddr( 0, 0, bAddress );

	            if (rcode) {
		            p->lowspeed = false;
		            addrPool.FreeAddress(bAddress);
		            bAddress = 0;
		            USBTRACE2("setAddr:",rcode);
		            return rcode;
	            }

	            USBTRACE2("\r\nAddr: ", bAddress);
	            
	            p->lowspeed = false;

	            p = addrPool.GetUsbDevicePtr(bAddress);

	            if (!p) {
		            return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
		          }

	            p->lowspeed = lowspeed;
	            
	            // Assign epInfo to epinfo pointer
	            // rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
/*
	            if (rcode) {
		            goto FailSetDevTblEntry;
		          }
	*/            
              /* initialize endpoint structures */
              
              // Assign epInfo to epinfo pointer
	            //rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);
              
              USBTRACE2("Conf:", 1);

	            // Set Configuration Value
	            rcode = pUsb->setConf(bAddress, 0, bConfNum);

	            if (rcode) {
		            goto FailSetConf;
		          }
              
              return 0; //successful configuration
    }//if( buf->idVendor == ADK_VID...
		
		//probe device - get accessory protocol revision
		{
		  uint16_t adkproto = -1;
		  rcode = getProto((uint8_t*)&adkproto );
		  if( rcode ){
		    goto FailGetProto; //init fails
		  }
		  USBTRACE2("\r\nADK protocol rev. ", adkproto );
		}
		
		//load ID strings and switch to accessory mode
		rcode = switchAcc();
		if( rcode ) {
		  goto FailSwAcc; //init fails
		}
		rcode = -1;
		goto SwAttempt;   //switch to accessory mode attempted
		   
//
//	// Allocate new address according to device class
//	bAddress = addrPool.AllocAddress(parent, false, port);
//
//	if (!bAddress)
//		return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
//
//	// Extract Max Packet Size from the device descriptor
//	epInfo[0].maxPktSize = (uint8_t)((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0; 
//
//	// Assign new address to the device
//	rcode = pUsb->setAddr( 0, 0, bAddress );
//
//	if (rcode)
//	{
//		p->lowspeed = false;
//		addrPool.FreeAddress(bAddress);
//		bAddress = 0;
//		USBTRACE2("setAddr:",rcode);
//		return rcode;
//	}
//
//	USBTRACE2("Addr:", bAddress);
//
//	p->lowspeed = false;
//
//	p = addrPool.GetUsbDevicePtr(bAddress);
//
//	if (!p)
//		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
//
//	p->lowspeed = lowspeed;
//
//	num_of_conf = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;
//
//	// Assign epInfo to epinfo pointer
//	rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
//
//	if (rcode)
//		goto FailSetDevTblEntry;
//
//	USBTRACE2("NC:", num_of_conf);
//
//	for (uint8_t i=0; i<num_of_conf; i++)
//	{
//		ConfigDescParser<	USB_CLASS_COM_AND_CDC_CTRL, 
//							CDC_SUBCLASS_ACM, 
//							CDC_PROTOCOL_ITU_T_V_250, 
//							CP_MASK_COMPARE_CLASS | 
//							CP_MASK_COMPARE_SUBCLASS | 
//							CP_MASK_COMPARE_PROTOCOL>		CdcControlParser(this);
//		
//		ConfigDescParser<USB_CLASS_CDC_DATA, 0, 0, 
//			CP_MASK_COMPARE_CLASS>							CdcDataParser(this);
//
//		rcode = pUsb->getConfDescr(bAddress, 0, i, &CdcControlParser);
//		rcode = pUsb->getConfDescr(bAddress, 0, i, &CdcDataParser);
//		
//		if (bNumEP > 1)
//			break;
//	} // for
//	
//	if (bNumEP < 4)
//		return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
//
//	// Assign epInfo to epinfo pointer
//	rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);
//
//	USBTRACE2("Conf:", bConfNum);
//
//	// Set Configuration Value
//	rcode = pUsb->setConf(bAddress, 0, bConfNum);
//
//	if (rcode)
//		goto FailSetConf;
//
//	rcode = pAsync->OnInit(this);
//
//	if (rcode)
//		goto FailOnInit;
//
//	USBTRACE("ACM configured\r\n");
//
//	//bPollEnable = true;
//
//	//USBTRACE("Poll enabled\r\n");
//	return 0;
//
FailGetDevDescr:
	USBTRACE("\r\ngetDevDescr:");
	goto Fail;

FailGetProto:
  USBTRACE("\r\ngetProto:");
  goto Fail;
  
FailSwAcc:
  USBTRACE("\r\nswAcc:");
  goto Fail;
  
SwAttempt:
  USBTRACE("\r\nAccessory mode switch attempt");
  goto Fail;    

//FailSetDevTblEntry:
//	USBTRACE("setDevTblEn:");
//	goto Fail;
//
//FailGetConfDescr:
//	USBTRACE("getConf:");
//	goto Fail;
//
FailSetConf:
  USBTRACE("\r\nsetConf: ");
  goto Fail;
//
//FailOnInit:
//	USBTRACE("OnInit:");
//	goto Fail;
//
Fail:
	USBTRACE2("\r\nADK Init Failed, error code: ", rcode);
	//Release();
	return rcode;
}


//void ACM::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) 
//{
//	ErrorMessage<uint8_t>(PSTR("Conf.Val"),	conf);
//	ErrorMessage<uint8_t>(PSTR("Iface Num"),iface);
//	ErrorMessage<uint8_t>(PSTR("Alt.Set"),	alt);
//
//	bConfNum = conf;
//
//	uint8_t index;
//
//	if ((pep->bmAttributes & 0x03) == 3 && (pep->bEndpointAddress & 0x80) == 0x80)
//		index = epInterruptInIndex;
//	else 
//		if ((pep->bmAttributes & 0x02) == 2)
//			index = ((pep->bEndpointAddress & 0x80) == 0x80) ? epDataInIndex : epDataOutIndex; 
//		else
//			return;
//
//	// Fill in the endpoint info structure
//	epInfo[index].epAddr		= (pep->bEndpointAddress & 0x0F);
//	epInfo[index].maxPktSize	= (uint8_t)pep->wMaxPacketSize;
//	epInfo[index].epAttribs		= 0;
//
//	bNumEP ++;
//
//	PrintEndpointDescriptor(pep);
//}

uint8_t ADK::Release()
{
//	pUsb->GetAddressPool().FreeAddress(bAddress);
//
//	bControlIface		= 0;	
//	bDataIface			= 0;		
//	bNumEP				= 1;		//must have to be reset to 1	
//
//	bAddress			= 0;
//	qNextPollTime		= 0;
//	bPollEnable			= false;
	return 0;
}

uint8_t ADK::Poll()
{
	uint8_t rcode = 0;

	if (!bPollEnable)
		return 0;

	uint32_t	time_now = millis();

	if (qNextPollTime <= time_now)
	{
		qNextPollTime = time_now + 100;

		uint8_t			rcode;
		const uint8_t	constBufSize = 16;
		uint8_t			buf[constBufSize];

		for (uint8_t i=0; i<constBufSize; i++)
			buf[i] = 0;

//		uint16_t	read = (constBufSize > epInfo[epInterruptInIndex].maxPktSize) 
//							? epInfo[epInterruptInIndex].maxPktSize : constBufSize;
//		rcode = pUsb->inTransfer(bAddress, epInfo[epInterruptInIndex].epAddr, &read, buf);

		if (rcode)
			return rcode;

//		for (uint8_t i=0; i<read; i++)
//		{
//			PrintHex<uint8_t>(buf[i]);
//			Serial.print(" ");
//		}
//		USBTRACE("\r\n");
	}
	return rcode;
}

uint8_t ADK::RcvData(uint16_t *bytes_rcvd, uint8_t *dataptr)
{
	return pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, bytes_rcvd, dataptr);
}

uint8_t ADK::SndData(uint16_t nbytes, uint8_t *dataptr)
{
	return pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, nbytes, dataptr);
}

//uint8_t ACM::SetCommFeature(uint16_t fid, uint8_t nbytes, uint8_t *dataptr)
//{
//	return( pUsb->ctrlReq( bAddress, 0, bmREQ_CDCOUT, CDC_SET_COMM_FEATURE, (fid & 0xff), (fid >> 8), bControlIface, nbytes, nbytes, dataptr, NULL ));        
//}
//
//uint8_t ACM::GetCommFeature(uint16_t fid, uint8_t nbytes, uint8_t *dataptr)
//{
//	return( pUsb->ctrlReq( bAddress, 0, bmREQ_CDCIN, CDC_GET_COMM_FEATURE, (fid & 0xff), (fid >> 8), bControlIface, nbytes, nbytes, dataptr, NULL ));        
//}
//
//uint8_t ACM::ClearCommFeature(uint16_t fid)
//{
//	return( pUsb->ctrlReq( bAddress, 0, bmREQ_CDCOUT, CDC_CLEAR_COMM_FEATURE, (fid & 0xff), (fid >> 8), bControlIface, 0, 0, NULL, NULL ));        
//}
//
//uint8_t ACM::SetLineCoding(const LINE_CODING *dataptr)
//{
//	return( pUsb->ctrlReq( bAddress, 0, bmREQ_CDCOUT, CDC_SET_LINE_CODING, 0x00, 0x00, bControlIface, sizeof(LINE_CODING), sizeof(LINE_CODING), (uint8_t*)dataptr, NULL ));        
//}
//
//uint8_t ACM::GetLineCoding(LINE_CODING *dataptr)
//{
//	return( pUsb->ctrlReq( bAddress, 0, bmREQ_CDCIN, CDC_GET_LINE_CODING, 0x00, 0x00, bControlIface, sizeof(LINE_CODING), sizeof(LINE_CODING), (uint8_t*)dataptr, NULL ));        
//}
//
//uint8_t ACM::SetControlLineState(uint8_t state)
//{
//	return( pUsb->ctrlReq( bAddress, 0, bmREQ_CDCOUT, CDC_SET_CONTROL_LINE_STATE, state, 0, bControlIface, 0, 0, NULL, NULL ));        
//}
//
//uint8_t ACM::SendBreak(uint16_t duration)
//{
//	return( pUsb->ctrlReq( bAddress, 0, bmREQ_CDCOUT, CDC_SEND_BREAK, (duration & 0xff), (duration >> 8), bControlIface, 0, 0, NULL, NULL ));        
//}
//
//
//void ACM::PrintEndpointDescriptor( const USB_ENDPOINT_DESCRIPTOR* ep_ptr )
//{
//	Notify(PSTR("Endpoint descriptor:"));
//	Notify(PSTR("\r\nLength:\t\t"));
//	PrintHex<uint8_t>(ep_ptr->bLength);
//	Notify(PSTR("\r\nType:\t\t"));
//	PrintHex<uint8_t>(ep_ptr->bDescriptorType);
//	Notify(PSTR("\r\nAddress:\t"));
//	PrintHex<uint8_t>(ep_ptr->bEndpointAddress);
//	Notify(PSTR("\r\nAttributes:\t"));
//	PrintHex<uint8_t>(ep_ptr->bmAttributes);
//	Notify(PSTR("\r\nMaxPktSize:\t"));
//	PrintHex<uint16_t>(ep_ptr->wMaxPacketSize);
//	Notify(PSTR("\r\nPoll Intrv:\t"));
//	PrintHex<uint8_t>(ep_ptr->bInterval);
//	Notify(PSTR("\r\n"));
//}
