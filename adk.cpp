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
  
  pUsb(p),            //pointer to USB class instance - mandatory
  /* ADK ID Strings */
  manufacturer(manufacturer),
  model(model),
  description(description),
  version(version),
  uri(uri),
  serial(serial),
  
	bAddress(0),        //device address - mandatory		
	bNumEP(1) //if config descriptor needs to be parsed
{
  /* initialize endpoint data structures */
	for(uint8_t i=0; i<ADK_MAX_ENDPOINTS; i++)
	{
		epInfo[i].epAddr		= 0;
		epInfo[i].maxPktSize	= (i) ? 0 : 8;
		epInfo[i].epAttribs		= 0;

		if (!i) {
			epInfo[i].bmNakPower	= USB_NAK_MAX_POWER;
		}
	}
	if (pUsb) {
		pUsb->RegisterDeviceClass(this);  //set devConfig[] entry
	}
}

/* Connect/disconnect initialization of a phone */
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
    if (bAddress) {
      USBTRACE("\r\nAddress in use");
		  return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
		}
		  
		//USBTRACE("\r\nHere");
		  
		  
    // Get pointer to pseudo device with address 0 assigned
	  p = addrPool.GetUsbDevicePtr(0);

	  if (!p) {
	    USBTRACE("\r\nAddress not found");
		  return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }
    
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

	// Extract Max Packet Size from device descriptor
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

	USBTRACE2("\r\nAddr:", bAddress);

	p->lowspeed = false;
	
  //get pointer to assigned address record
	p = addrPool.GetUsbDevicePtr(bAddress);
	if (!p) {
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
	}

	p->lowspeed = lowspeed;

	// Assign epInfo to epinfo pointer
	rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);

	if (rcode)
		goto FailSetDevTblEntry;


		/* debug code start */
		num_of_conf = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;
		
		USBTRACE2("\r\nNC:",num_of_conf);
		USBTRACE2("\r\nNP:",epInfo[0].bmNakPower);

		for (uint8_t i=0; i<num_of_conf; i++) 
		{
			USBTRACE("\r\nHexdumper: ");  
			HexDumper<USBReadParser, uint16_t, uint16_t>		HexDump;
			//ConfigDescParser<0, 0, 0, 0>							confDescrParser(this);
	    
			rcode = pUsb->getConfDescr(bAddress, 0, i, &HexDump);
			//rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
		
		} // for (uint8_t i=0; i<num_of_conf; i++...
		
		/* debug code end */
		
		 //USBTRACE("Check!!!");
		//check if ADK device is already in accessory mode
		if(((USB_DEVICE_DESCRIPTOR*)buf)->idVendor == ADK_VID &&
            (((USB_DEVICE_DESCRIPTOR*)buf)->idProduct == ADK_PID || ((USB_DEVICE_DESCRIPTOR*)buf)->idProduct == ADB_PID)){
              USBTRACE("\r\nAcc.mode device detected");
              
              /* set endpoint info, config */
              
              // Allocate new address
	            //bAddress = addrPool.AllocAddress(parent, false, port);

	            //if (!bAddress) {
		          //  return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
		          //}

	            // Extract Max Packet Size from the device descriptor
	            //epInfo[0].maxPktSize = (uint8_t)((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0; 
              // Set Configuration Value
	            rcode = pUsb->setConf(bAddress, 0, bConfNum);

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
		
		//sending ID strings
		sendStr( ACCESSORY_STRING_MANUFACTURER, manufacturer);
    sendStr( ACCESSORY_STRING_MODEL, model);
    sendStr( ACCESSORY_STRING_DESCRIPTION, description);
    sendStr( ACCESSORY_STRING_VERSION, version);
    sendStr( ACCESSORY_STRING_URI, uri);
    sendStr( ACCESSORY_STRING_SERIAL, serial);
		
		//switch to accessory mode
		//the phone will reset 
		rcode = switchAcc();
		if( rcode ) {
		  goto FailSwAcc; //init fails
		}
		rcode = -1;
		goto SwAttempt;   //switch to accessory mode attempted
		   

FailGetDevDescr:
	USBTRACE("\r\ngetDevDescr:");
	goto Fail;

FailSetDevTblEntry:
	USBTRACE("\r\nsetDevTblEn:");
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
//FailSetConf:
//	USBTRACE("setConf:");
//	goto Fail;
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

/* Extracts bulk-IN and bulk-OUT endpoint information from config descriptor */
void ADK::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) 
{
	ErrorMessage<uint8_t>(PSTR("Conf.Val"),	conf);
	ErrorMessage<uint8_t>(PSTR("Iface Num"),iface);
	ErrorMessage<uint8_t>(PSTR("Alt.Set"),	alt);

	bConfNum = conf;

	uint8_t index;
	
	

		if ((pep->bmAttributes & 0x02) == 2) {
			index = ((pep->bEndpointAddress & 0x80) == 0x80) ? epDataInIndex : epDataOutIndex;
	  }
			  

	// Fill in the endpoint info structure
	epInfo[index].epAddr		= (pep->bEndpointAddress & 0x0F);
	epInfo[index].maxPktSize	= (uint8_t)pep->wMaxPacketSize;
	epInfo[index].epAttribs		= 0;

	bNumEP ++;

	PrintEndpointDescriptor(pep);
}

/* Performs a cleanup after failed Init() attempt */
uint8_t ADK::Release()
{
	pUsb->GetAddressPool().FreeAddress(bAddress);
//
//	bControlIface		= 0;	
//	bDataIface			= 0;		
	bNumEP				= 1;		//must have to be reset to 1	
//
	bAddress			= 0;
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

void ADK::PrintEndpointDescriptor( const USB_ENDPOINT_DESCRIPTOR* ep_ptr )
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
