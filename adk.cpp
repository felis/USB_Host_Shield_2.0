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
  
  pUsb(p),      //pointer to USB class instance - mandatory
	bAddress(0),  //device address - mandatory		
	bNumEP(1),    //if config descriptor needs to be parsed
	ready(false),
	
  /* ADK ID Strings */
  
  manufacturer(manufacturer),
  model(model),
  description(description),
  version(version),
  uri(uri),
  serial(serial)
  
{
  // initialize endpoint data structures
	for(uint8_t i=0; i<ADK_MAX_ENDPOINTS; i++) {
		epInfo[i].epAddr		= 0;
		epInfo[i].maxPktSize	= (i) ? 0 : 8;
		epInfo[i].epAttribs		= ( 0xfc & ( USB_NAK_MAX_POWER<<2 ));
  }//for(uint8_t i=0; i<ADK_MAX_ENDPOINTS; i++...
  
  //set bulk-IN EP naklimit to 1 
  epInfo[epDataInIndex].epAttribs = ( 0xfc & ( USB_NAK_NOWAIT<<2 ));
  
  // register in USB subsystem
	if (pUsb) {
		pUsb->RegisterDeviceClass(this);  //set devConfig[] entry
	}
}

/* Connection initialization of an Android phone */
uint8_t ADK::Init(uint8_t parent, uint8_t port, bool lowspeed)
{
  
	uint8_t		buf[sizeof(USB_DEVICE_DESCRIPTOR)];
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
	  rcode = pUsb->getDevDescr( 0, 0, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t*)buf );

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
	  if (rcode) {
		  p->lowspeed = false;
		  addrPool.FreeAddress(bAddress);
		  bAddress = 0;
		  //USBTRACE2("setAddr:",rcode);
		  return rcode;
	  }//if (rcode...

	  //USBTRACE2("\r\nAddr:", bAddress);

	  p->lowspeed = false;
	
    //get pointer to assigned address record
	  p = addrPool.GetUsbDevicePtr(bAddress);
	  if (!p) {
		  return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
	  }

	  p->lowspeed = lowspeed;

	  // Assign epInfo to epinfo pointer - only EP0 is known
	  rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
    if (rcode) {
		  goto FailSetDevTblEntry;
		}

		//check if ADK device is already in accessory mode; if yes, configure and exit
		if(((USB_DEVICE_DESCRIPTOR*)buf)->idVendor == ADK_VID &&
            (((USB_DEVICE_DESCRIPTOR*)buf)->idProduct == ADK_PID || ((USB_DEVICE_DESCRIPTOR*)buf)->idProduct == ADB_PID)) {
              USBTRACE("\r\nAcc.mode device detected");
              /* go through configurations, find first bulk-IN, bulk-OUT EP, fill epInfo and quit */
		          num_of_conf = ((USB_DEVICE_DESCRIPTOR*)buf)->bNumConfigurations;
		
		          //USBTRACE2("\r\nNC:",num_of_conf);

		          for (uint8_t i=0; i<num_of_conf; i++) {
			          ConfigDescParser<0, 0, 0, 0> confDescrParser(this);
	    			    rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
                if( rcode ) {
                  goto FailGetConfDescr;
                }
		            if( bNumEP > 2 ) {
			            break;
			          }
		          } // for (uint8_t i=0; i<num_of_conf; i++...
		          
		          if( bNumEP == 3 ) {
        		    // Assign epInfo to epinfo pointer - this time all 3 endpoins
	              rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
                if (rcode) {
		              goto FailSetDevTblEntry;
		            }
		          }
		
		
		
              // Set Configuration Value
	            rcode = pUsb->setConf(bAddress, 0, bConfNum);
	            if( rcode ){ 
		            goto FailSetConf;
		          }
		          /* print endpoint structure */
//		          USBTRACE("\r\nEndpoint Structure:");
//		          USBTRACE("\r\nEP0:");
//		          USBTRACE2("\r\nAddr: ", epInfo[0].epAddr );
//	            USBTRACE2("\r\nMax.pkt.size: ", epInfo[0].maxPktSize );
//	            USBTRACE2("\r\nAttr: ", epInfo[0].epAttribs );
//	            USBTRACE("\r\nEpout:");
//		          USBTRACE2("\r\nAddr: ", epInfo[epDataOutIndex].epAddr );
//	            USBTRACE2("\r\nMax.pkt.size: ", epInfo[epDataOutIndex].maxPktSize );
//	            USBTRACE2("\r\nAttr: ", epInfo[epDataOutIndex].epAttribs );
//	            USBTRACE("\r\nEpin:");
//		          USBTRACE2("\r\nAddr: ", epInfo[epDataInIndex].epAddr );
//	            USBTRACE2("\r\nMax.pkt.size: ", epInfo[epDataInIndex].maxPktSize );
//	            USBTRACE2("\r\nAttr: ", epInfo[epDataInIndex].epAttribs );
		          
              USBTRACE("\r\nConfiguration successful");
              ready = true;
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
		//the Android phone will reset 
		rcode = switchAcc();
		if( rcode ) {
		  goto FailSwAcc; //init fails
		}
		rcode = -1;
		goto SwAttempt;   //switch to accessory mode attempted
		   
    /* diagnostic messages */  
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

FailGetConfDescr:
//	USBTRACE("getConf:");
	goto Fail;
//
FailSetConf:
//	USBTRACE("setConf:");
	goto Fail;
//
//FailOnInit:
//	USBTRACE("OnInit:");
//	goto Fail;
//
Fail:
	//USBTRACE2("\r\nADK Init Failed, error code: ", rcode);
	Release();
	return rcode;
}

/* Extracts bulk-IN and bulk-OUT endpoint information from config descriptor */
void ADK::EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *pep) 
{
	//ErrorMessage<uint8_t>(PSTR("Conf.Val"),	conf);
	//ErrorMessage<uint8_t>(PSTR("Iface Num"),iface);
	//ErrorMessage<uint8_t>(PSTR("Alt.Set"),	alt);
  
  //added by Yuuichi Akagawa
  if( bNumEP == 3 ) {
     return;
  }
  
	bConfNum = conf;

	uint8_t index;
		
		if ((pep->bmAttributes & 0x02) == 2) {
			index = ((pep->bEndpointAddress & 0x80) == 0x80) ? epDataInIndex : epDataOutIndex;
	  }
			  
	// Fill in the endpoint info structure
	epInfo[index].epAddr		= (pep->bEndpointAddress & 0x0F);
	epInfo[index].maxPktSize	= (uint8_t)pep->wMaxPacketSize;

	bNumEP ++;

	//PrintEndpointDescriptor(pep);
}

/* Performs a cleanup after failed Init() attempt */
uint8_t ADK::Release()
{
	pUsb->GetAddressPool().FreeAddress(bAddress);

	bNumEP				= 1;		//must have to be reset to 1	
  
	bAddress			= 0;
  ready = false;
	return 0;
}

uint8_t ADK::RcvData(uint16_t *bytes_rcvd, uint8_t *dataptr)
{
	//USBTRACE2("\r\nAddr: ", bAddress );
	//USBTRACE2("\r\nEP: ",epInfo[epDataInIndex].epAddr);
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
