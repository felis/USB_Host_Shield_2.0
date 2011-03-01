/* USB functions */

#include "avrpins.h"
#include "max3421e.h"
#include "usbhost.h"
#include "Usb.h"
#include "WProgram.h"
//#include <ptpdebug.h>

static uint8_t usb_error = 0;
static uint8_t usb_task_state;
//DEV_RECORD devtable[ USB_NUMDEVICES + 1 ];
//EP_RECORD dev0ep;         //Endpoint data structure used during enumeration for uninitialized device


/* constructor */

USB::USB ()
{
    usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE;  //set up state machine
    init(); 
}


/* Initialize data structures */
void USB::init()
{
	devConfigIndex = 0;

	//UsbDevice *p = addrPool.GetUsbDevicePtr(0);

	//if (p)
	//{
	//	p->epinfo			= &dev0ep;
	//	dev0ep.sndToggle	= bmSNDTOG0;   //set DATA0/1 toggles to 0
	//	dev0ep.rcvToggle	= bmRCVTOG0;
	//}
}

uint8_t USB::getUsbTaskState( void )
{
    return( usb_task_state );
}
void USB::setUsbTaskState( uint8_t state )
{
    usb_task_state = state;
}     
EP_RECORD* USB::getDevTableEntry( uint8_t addr, uint8_t ep )
{
	UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

	if (p)
		return (p->epinfo + ep);

	return NULL;
}
/* set device table entry */
/* each device is different and has different number of endpoints. This function plugs endpoint record structure, defined in application, to devtable */
uint8_t USB::setDevTableEntry( uint8_t addr, EP_RECORD* eprecord_ptr )
{
	if (!eprecord_ptr)
		return USB_ERROR_INVALID_ARGUMENT;

	UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

	if (!p)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
	
	p->address	= addr;
	p->devclass = 0;
	p->epinfo	= eprecord_ptr;

	return 0;
}
/* Control transfer. Sets address, endpoint, fills control packet with necessary data, dispatches control packet, and initiates bulk IN transfer,   */
/* depending on request. Actual requests are defined as inlines                                                                                      */
/* return codes:                */
/* 00       =   success         */
/* 01-0f    =   non-zero HRSLT  */
uint8_t USB::ctrlReq( uint8_t addr, uint8_t ep, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi, unsigned int wInd, unsigned int nbytes, uint8_t* dataptr, unsigned int nak_limit )
{
	boolean direction = false;     //request direction, IN or OUT
	uint8_t rcode;   
	SETUP_PKT setup_pkt;

	regWr( rPERADDR, addr );                    //set peripheral address
	
	if( bmReqType & 0x80 ) 
		direction = true;                       //determine request direction
	
    /* fill in setup packet */
    setup_pkt.ReqType_u.bmRequestType = bmReqType;
    setup_pkt.bRequest = bRequest;
    setup_pkt.wVal_u.wValueLo = wValLo;
    setup_pkt.wVal_u.wValueHi = wValHi;
    setup_pkt.wIndex = wInd;
    setup_pkt.wLength = nbytes;
    bytesWr( rSUDFIFO, 8, (uint8_t*)&setup_pkt );    //transfer to setup packet FIFO
    rcode = dispatchPkt( tokSETUP, ep, nak_limit );            //dispatch packet
    //Serial.println("Setup packet");   //DEBUG
    if( rcode ) {                                   //return HRSLT if not zero
        Serial.print("Setup packet error: ");
        Serial.print( rcode, HEX );                                          
        return( rcode );
    }
    //Serial.println( direction, HEX ); 
    if( dataptr != NULL ) {                         //data stage, if present
        rcode = ctrlData( addr, ep, nbytes, dataptr, direction );
    }
    if( rcode ) {   //return error
        Serial.print("Data packet error: ");
        Serial.print( rcode, HEX );                                          
        return( rcode );
    }
    rcode = ctrlStatus( ep, direction );                //status stage
    return( rcode );
}
/* Control transfer with status stage and no data stage */
/* Assumed peripheral address is already set */
uint8_t USB::ctrlStatus( uint8_t ep, boolean direction, unsigned int nak_limit )
{
	uint8_t rcode;
    if( direction ) { //GET
        rcode = dispatchPkt( tokOUTHS, ep, nak_limit );
    }
    else {
        rcode = dispatchPkt( tokINHS, ep, nak_limit );
    }
    return( rcode );
}
/* Control transfer with data stage. Stages 2 and 3 of control transfer. Assumes preipheral address is set and setup packet has been sent */
uint8_t USB::ctrlData( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* dataptr, boolean direction, unsigned int nak_limit )
{
	uint8_t		rcode;
	UsbDevice	*p = addrPool.GetUsbDevicePtr(addr);

	//Serial.print("cD");
	//Serial.print("\tA:");
	//Serial.print(addr, HEX);
	//Serial.print("\tE:");
	//Serial.println(ep, HEX);

	if (!p)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

	if (!p->epinfo)
		return USB_ERROR_EPINFO_IS_NULL;

	EP_RECORD *pep = &p->epinfo[ep];

	if( direction )		//IN transfer
	{                      
		pep->rcvToggle = bmRCVTOG1;
		rcode = inTransfer( addr, ep, nbytes, dataptr, nak_limit );
	}
	else				//OUT transfer
	{              
		pep->sndToggle = bmSNDTOG1;
		rcode = outTransfer( addr, ep, nbytes, dataptr, nak_limit );
	}    
	return( rcode );
}
/* IN transfer to arbitrary endpoint. Assumes PERADDR is set. Handles multiple packets if necessary. Transfers 'nbytes' bytes. */
/* Keep sending INs and writes data to memory area pointed by 'data'                                                           */
/* rcode 0 if no errors. rcode 01-0f is relayed from dispatchPkt(). Rcode f0 means RCVDAVIRQ error,
            fe USB xfer timeout */
uint8_t USB::inTransfer( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* data, unsigned int nak_limit )
{
	uint8_t rcode;
	uint8_t pktsize;

	//Serial.println("iT");

	UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

	if (!p)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

	if (!p->epinfo)
		return USB_ERROR_EPINFO_IS_NULL;

	EP_RECORD *pep = &p->epinfo[ep];

	uint8_t maxpktsize = pep->MaxPktSize; 

	unsigned int xfrlen = 0;
    regWr( rHCTL, pep->rcvToggle );    //set toggle value

	while( 1 )		// use a 'return' to exit this loop
	{ 
        rcode = dispatchPkt( tokIN, ep, nak_limit );           //IN packet to EP-'endpoint'. Function takes care of NAKS.

		if( rcode ) 
            return( rcode );                            //should be 0, indicating ACK. Else return error code.
        
        /* check for RCVDAVIRQ and generate error if not present */ 
        /* the only case when absense of RCVDAVIRQ makes sense is when toggle error occured. Need to add handling for that */
        if(( regRd( rHIRQ ) & bmRCVDAVIRQ ) == 0 ) {
            return ( 0xf0 );                            //receive error
        }
        pktsize = regRd( rRCVBC );                      //number of received bytes
        data = bytesRd( rRCVFIFO, pktsize, data );
        regWr( rHIRQ, bmRCVDAVIRQ );                    // Clear the IRQ & free the buffer
        xfrlen += pktsize;                              // add this packet's byte count to total transfer length
        /* The transfer is complete under two conditions:           */
        /* 1. The device sent a short packet (L.T. maxPacketSize)   */
        /* 2. 'nbytes' have been transferred.                       */
        if (( pktsize < maxpktsize ) || (xfrlen >= nbytes ))		// have we transferred 'nbytes' bytes?
		{      
            if( regRd( rHRSL ) & bmRCVTOGRD )						//save toggle value
                pep->rcvToggle = bmRCVTOG1;
            else 
                pep->rcvToggle = bmRCVTOG0;

			return( 0 );
        } // if
	} //while( 1 )
}
/* OUT transfer to arbitrary endpoint. Assumes PERADDR is set. Handles multiple packets if necessary. Transfers 'nbytes' bytes. */
/* Handles NAK bug per Maxim Application Note 4000 for single buffer transfer   */
/* rcode 0 if no errors. rcode 01-0f is relayed from HRSL                       */
/* major part of this function borrowed from code shared by Richard Ibbotson    */
uint8_t USB::outTransfer( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* data, unsigned int nak_limit )
{
	uint8_t rcode, retry_count;
	uint8_t* data_p = data;   //local copy of the data pointer
	unsigned int bytes_tosend, nak_count;
	unsigned int bytes_left = nbytes;

	UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

	if (!p)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

	EP_RECORD *pep = p->epinfo + ep;

 	if (!p->epinfo)
		return USB_ERROR_EPINFO_IS_NULL;

	uint8_t maxpktsize = pep->MaxPktSize; 
 
	unsigned long timeout = millis() + USB_XFER_TIMEOUT;
 
	if (!maxpktsize) { //todo: move this check close to epinfo init. Make it 1< pktsize <64
		return 0xFE;
	}
 
	regWr( rHCTL, pep->sndToggle );    //set toggle value
	
	while( bytes_left ) 
	{
		retry_count = 0;
		nak_count = 0;
		bytes_tosend = ( bytes_left >= maxpktsize ) ? maxpktsize : bytes_left;
		bytesWr( rSNDFIFO, bytes_tosend, data_p );      //filling output FIFO
		regWr( rSNDBC, bytes_tosend );                  //set number of bytes    
		regWr( rHXFR, ( tokOUT | ep ));                 //dispatch packet
		while(!(regRd( rHIRQ ) & bmHXFRDNIRQ ));        //wait for the completion IRQ
		regWr( rHIRQ, bmHXFRDNIRQ );                    //clear IRQ
		rcode = ( regRd( rHRSL ) & 0x0f );

		while( rcode && ( timeout > millis())) 
		{
			switch( rcode ) 
			{
			case hrNAK:
				nak_count++;
				if( nak_limit && ( nak_count == USB_NAK_LIMIT )) 
				{
					return( rcode);                                   //return NAK
				}
				break;
			case hrTIMEOUT:
				retry_count++;
				if( retry_count == USB_RETRY_LIMIT ) {
					return( rcode );    //return TIMEOUT
				}
				break;
			default:  
				return( rcode );
			} //switch( rcode...
			/* process NAK according to Host out NAK bug */
			regWr( rSNDBC, 0 );
			regWr( rSNDFIFO, *data_p );
			regWr( rSNDBC, bytes_tosend );
			regWr( rHXFR, ( tokOUT | ep ));                 //dispatch packet
			while(!(regRd( rHIRQ ) & bmHXFRDNIRQ ));        //wait for the completion IRQ
			regWr( rHIRQ, bmHXFRDNIRQ );                    //clear IRQ
			rcode = ( regRd( rHRSL ) & 0x0f );
		}//while( rcode && ....
		bytes_left -= bytes_tosend;
		data_p += bytes_tosend;
	}//while( bytes_left...
	pep->sndToggle = ( regRd( rHRSL ) & bmSNDTOGRD ) ? bmSNDTOG1 : bmSNDTOG0;  //update toggle
	return( rcode );    //should be 0 in all cases
}
/* dispatch usb packet. Assumes peripheral address is set and relevant buffer is loaded/empty       */
/* If NAK, tries to re-send up to nak_limit times                                                   */
/* If nak_limit == 0, do not count NAKs, exit after timeout                                         */
/* If bus timeout, re-sends up to USB_RETRY_LIMIT times                                             */
/* return codes 0x00-0x0f are HRSLT( 0x00 being success ), 0xff means timeout                       */
uint8_t USB::dispatchPkt( uint8_t token, uint8_t ep, unsigned int nak_limit )
{
 unsigned long timeout = millis() + USB_XFER_TIMEOUT;
 uint8_t tmpdata;   
 uint8_t rcode;
 unsigned int nak_count = 0;
 char retry_count = 0;

  while( timeout > millis() ) {
    regWr( rHXFR, ( token|ep ));            //launch the transfer
    rcode = 0xff;   
    while( millis() < timeout ) {           //wait for transfer completion
      tmpdata = regRd( rHIRQ );
      if( tmpdata & bmHXFRDNIRQ ) {
        regWr( rHIRQ, bmHXFRDNIRQ );    //clear the interrupt
        rcode = 0x00;
        break;
      }//if( tmpdata & bmHXFRDNIRQ
    }//while ( millis() < timeout
    if( rcode != 0x00 ) {                //exit if timeout
      return( rcode );
    }
    rcode = ( regRd( rHRSL ) & 0x0f );  //analyze transfer result
    switch( rcode ) {
      case hrNAK:
        nak_count ++;
        if( nak_limit && ( nak_count == nak_limit )) {
          return( rcode );
        }
        break;
      case hrTIMEOUT:
        retry_count ++;
        if( retry_count == USB_RETRY_LIMIT ) {
          return( rcode );
        }
        break;
      default:
        return( rcode );
    }//switch( rcode
  }//while( timeout > millis() 
  return( rcode );
}
/* USB main task. Performs enumeration/cleanup */
void USB::Task( void )      //USB state machine
{
  uint8_t i;   
  uint8_t rcode;
  static uint8_t tmpaddr; 
  uint8_t tmpdata;
  static unsigned long delay = 0;
  USB_DEVICE_DESCRIPTOR buf;

  //MAX3421E::Task();

    tmpdata = getVbusState();
    /* modify USB task state if Vbus changed */

    switch( tmpdata ) 
	{
        case SE1:   //illegal state
            usb_task_state = USB_DETACHED_SUBSTATE_ILLEGAL;
            break;
        case SE0:   //disconnected
            if(( usb_task_state & USB_STATE_MASK ) != USB_STATE_DETACHED ) {
                usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE;
            }
            break;
        case FSHOST:    //attached
        case LSHOST:
			//Serial.println("FSHOST");
            if(( usb_task_state & USB_STATE_MASK ) == USB_STATE_DETACHED ) {
                delay = millis() + USB_SETTLE_DELAY;
                usb_task_state = USB_ATTACHED_SUBSTATE_SETTLE;
            }
            break;
    }// switch( tmpdata

	for (uint8_t i=0; i<USB_NUMDEVICES; i++)
	{
		if (devConfig[i])
		{
			rcode = devConfig[i]->Poll();
		}
	} //for

    switch( usb_task_state ) {
        case USB_DETACHED_SUBSTATE_INITIALIZE:
			Serial.println("INIT");
            init();
            usb_task_state = USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE;
            break;
        case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:     //just sit here
			Serial.println("WFD");
            break;
        case USB_DETACHED_SUBSTATE_ILLEGAL:             //just sit here
			Serial.println("ILL");
            break;
        case USB_ATTACHED_SUBSTATE_SETTLE:              //setlle time for just attached device                  
			Serial.println("STL");
            if( delay < millis() ) {
                usb_task_state = USB_ATTACHED_SUBSTATE_RESET_DEVICE;
            }
            break;
        case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
			Serial.println("RES");
            regWr( rHCTL, bmBUSRST );									//issue bus reset
            usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE;
            break;
        case USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE:
			Serial.println("RCOMP");
            if(( regRd( rHCTL ) & bmBUSRST ) == 0 ) 
			{
                tmpdata = regRd( rMODE ) | bmSOFKAENAB;                 //start SOF generation
                regWr( rMODE, tmpdata );
                usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_SOF;
                delay = millis() + 20; //20ms wait after reset per USB spec
            }
            break;
        case USB_ATTACHED_SUBSTATE_WAIT_SOF:  //todo: change check order
			Serial.println("WSOF");
            if( regRd( rHIRQ ) & bmFRAMEIRQ ) {                         //when first SOF received we can continue
              if( delay < millis() ) {                                  //20ms passed
                usb_task_state = USB_STATE_ADDRESSING;
                //usb_task_state = USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE;
              }
            }
            break;
        case USB_STATE_ADDRESSING:
			Serial.println("ADR");
		
			rcode = Addressing(&tmpaddr);

			if (rcode == hrSUCCESS)
		        usb_task_state = USB_STATE_CONFIGURING;
			else
			{
				usb_error = rcode;
		        usb_task_state = USB_STATE_ERROR;
			}
            break;
        case USB_STATE_CONFIGURING:
			Serial.print("CNF");
			
			rcode = Configuring(tmpaddr);

			if (rcode)
			{
				if (rcode != USB_DEV_CONFIG_ERROR_DEVICE_INIT_INCOMPLETE)
				{
					usb_error = rcode;
					usb_task_state = USB_STATE_ERROR;
				}
			}
			else
				usb_task_state = USB_STATE_RUNNING;
            break;
        case USB_STATE_RUNNING:
			Serial.println("RUN");
            break;
        case USB_STATE_ERROR:
            break;
    } // switch( usb_task_state )
}    

uint8_t USB::Addressing(uint8_t *address)
{
	uint8_t rcode = hrSUCCESS, buf[8];

	Serial.println("Adrsng");

	UsbDevice *p = addrPool.GetUsbDevicePtr(0);

	if (!p)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

	if (!p->epinfo)
	{
		Serial.println("epinfo");
		return USB_ERROR_EPINFO_IS_NULL;
	}
	Serial.print("addr:");
	Serial.println(p->address, HEX);
	Serial.print("ep:");
	Serial.println(p->epinfo->epAddr, HEX);

	//p->epinfo->MaxPktSize = 8;

	Serial.print("Max:");
	Serial.println(p->epinfo->MaxPktSize, HEX);

	rcode = getDevDescr( 0, 0, 8, (uint8_t*)buf );

    if( rcode == 0 ) 
	{
		Serial.println("OK!");
        p->epinfo->MaxPktSize = ((USB_DEVICE_DESCRIPTOR*)buf)->bMaxPacketSize0;
		p->devclass = ((USB_DEVICE_DESCRIPTOR*)buf)->bDeviceClass;
    }
    else 
	{
		Serial.println("getDevDesc:");
        return rcode;
    }

	uint8_t addr = addrPool.AllocAddress(0, (p->devclass == 0x09) ? true : false);

	if (!addr)
		return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

	Serial.print("Addr:");
	Serial.println(addr,HEX);

    rcode = setAddr( 0, 0, addr );

	if (!rcode)
	{
		*address = addr;
	}
	else
	{
		Serial.print("setAddr:");
		Serial.println(rcode, HEX);
		return rcode;
	}
	return rcode;
}

uint8_t USB::Configuring(uint8_t addr)
{
	static uint8_t dev_index = 0;
	uint8_t rcode = 0, buf[8];

	for (; devConfigIndex<USB_NUMDEVICES; devConfigIndex++)
	{
		rcode = devConfig[devConfigIndex]->Init(addr);

		Serial.println(".");

		if (rcode != USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED)
		{
			// in case of an error dev_index should be reset to 0
			//		in order to start from the very beginning the 
			//		next time the program gets here
			if (rcode != USB_DEV_CONFIG_ERROR_DEVICE_INIT_INCOMPLETE)
				devConfigIndex = 0;

			return rcode;
		}
	}
	// if we get here that means that the device class is not supported by any of registered classes
	devConfigIndex = 0;
	return 0;
}

//class UsbHub
//{
//	uint8_t		bNbrPorts;							// Number of ports
//	uint8_t		portStates[HUB_MAX_PORTS];			// State of each port a hub has
//};
//
//struct HubPort
//{
//	UsbDevAddress	*hubAddr;
//	uint8_t			numPort;
//	uint8_t			portState;
//};

//static uint8_t USB::PortTask(HubPort *phub_port, uint8_t sig)
////static uint8_t USB::PortTask(uint8_t hub_addr, uint8_t port, uint8_t &state, uint8_t sig)
//{
//	switch(state)
//	{
//        case USB_STATE_HUB_PORT_POWERED_OFF:
//			Serial.println("POWERED_OFF");
//
//			for (uint8_t j=1; j<=hubs[0].bNbrPorts; j++)
//				HubPortPowerOn(hubs[0].bAddress, j);
//
//			delay = millis() + USB_SETTLE_DELAY;
//			usb_task_state = USB_STATE_HUB_PORT_WAIT_FOR_POWER_GOOD;
//			break;
//		case USB_STATE_HUB_PORT_WAIT_FOR_POWER_GOOD:
//			Serial.println("WAIT_FOR_POWER_GOOD");
//			if (millis() >= delay)
//				usb_task_state = USB_STATE_HUB_PORT_DISCONNECTED;
////				usb_task_state = USB_STATE_HUB_PORT_DISABLED;
//			break;
//        case USB_STATE_HUB_PORT_DISCONNECTED:
//			Serial.println("PORT_DISCONNECTED");
//
//			for (uint8_t j=1; j<=hubs[0].bNbrPorts; j++)
//				HubClearPortFeatures(hubs[0].bAddress, j, HUB_FEATURE_PORT_ENABLE | HUB_FEATURE_C_PORT_CONNECTION);
//
//			usb_task_state = USB_STATE_HUB_PORT_DISABLED;
//			break;
//        case USB_STATE_HUB_PORT_DISABLED:
//			Serial.println("PORT_DISABLED");
//
//			for (uint8_t j=1; j<=hubs[0].bNbrPorts; j++)
//				HubPortReset(hubs[0].bAddress, j);
//
//            delay = millis() + HUB_PORT_RESET_DELAY;
//			usb_task_state = USB_STATE_HUB_PORT_RESETTING;
//			break;
//        case USB_STATE_HUB_PORT_RESETTING:
//			Serial.println("PORT_RESETTING");
//			if (millis() >= delay)
//				usb_task_state = USB_STATE_HUB_PORT_ENABLED;
//			break;
//        case USB_STATE_HUB_PORT_ENABLED:
//			Serial.println("PORT_ENABLED");
//			usb_task_state = USB_STATE_RUNNING;
//			break;
//	}
//	return 0;
//}


uint8_t USB::HubPortPowerOn(uint8_t addr, uint8_t port)
{
	uint8_t rcode = SetPortFeature(addr, 0, HUB_FEATURE_PORT_POWER, port, 0);
	if (rcode)
	{
		Serial.print("PORT #");
		Serial.print(port, DEC);
		Serial.print(" pwr err:");
		Serial.println(rcode,HEX);
	}
	return rcode;
}

uint8_t USB::HubPortReset(uint8_t addr, uint8_t port)
{
    uint8_t rcode = SetPortFeature(addr, 0, HUB_FEATURE_PORT_RESET, port, 0);
	if (rcode)
	{
		Serial.print("PORT #");
		Serial.print(port, DEC);
		Serial.print(" rst err:");
		Serial.println(rcode,HEX);
	}
	return rcode;
}

uint8_t USB::HubClearPortFeatures(uint8_t addr, uint8_t port, uint8_t bm_features)
{
    uint8_t rcode = ClearPortFeature(addr, 0, bm_features, port, 0);
	if (rcode)
	{
		Serial.print("PORT #");
		Serial.print(port, DEC);
		Serial.print(" f.clr err:");
		Serial.println(rcode,HEX);
	}
	return rcode;
}

#if 0
// Hub Polling
uint8_t USB::PollHub()
{
	uint8_t		rcode;
	uint8_t		buf[8];

//  uint8_t inTransfer( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* data, unsigned int nak_limit = USB_NAK_LIMIT );
	//Serial.println(devtable[1].epinfo->epAddr, HEX);

	rcode = inTransfer(1, 0x1 /*devtable[1].epinfo->epAddr*/, 1, buf, 1);

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
		PrintHubStatus(1);
		//rcode = GetHubStatus(1, 0, 1, 4, buf);
		//if (rcode)
		//{
		//	Serial.print("GetHubStatus Error");
		//	Serial.println(rcode, HEX);
		//	return rcode;
		//}
	}
	if (buf[0] & 0x02)		// Port 1
	{
		rcode = GetPortStatus(1, 0, 1, 4, buf);
		if (rcode)
		{
			Serial.print("GetPortStatus Error");
			Serial.println(rcode, HEX);
			return rcode;
		}
		if (buf[2] & bmHUB_PORT_STATUS_C_PORT_RESET)
		{
			Serial.println("PORT_RESET");

			rcode = ClearPortFeature(1, 0, 1, HUB_FEATURE_C_PORT_RESET);
			if (rcode)
			{
				Serial.print("ClearPortFeature Error");
				Serial.println(rcode, HEX);
				return rcode;
			}
			//usb_task_state = USB_STATE_HUB_PORT_ENABLED; //USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE;
			return 0;
		}
		if (buf[2] & bmHUB_PORT_STATUS_C_PORT_CONNECTION)
		{
			Serial.println("PORT_CONNECTION");

			rcode = ClearPortFeature(1, 0, 1, HUB_FEATURE_C_PORT_CONNECTION);
			if (rcode)
			{
				Serial.print("ClearPortFeature Error");
				Serial.println(rcode, HEX);
				return rcode;
			}
		//	//if (buf & bmHUB_PORT_STATUS_PORT_CONNECTION)
		//	//	usb_task_state = USB_STATE_HUB_PORT_DISABLED;
		//	//else
		//	//	usb_task_state = USB_STATE_RUNNING;

		}

	} // if (buf[0] & 0x02)		// Port 1

}
#endif

void USB::PrintHubStatus(/*USB *usbptr,*/ uint8_t addr)
{
	uint8_t		rcode = 0;
	uint8_t		buf[4];

	rcode = /*usbptr->*/GetHubStatus(addr, 0, 4, (uint8_t*)&buf);

	if (rcode)
	{
		Serial.print("ERROR:");
		Serial.println(rcode, HEX);
		return;
    }
	Serial.println("\r\nHub");
	Serial.println("\r\nStatus");

	Serial.print("Local Pwr Src:\t");
	Serial.println((buf[0] & bmHUB_STATUS_LOCAL_POWER_SOURCE) > 0, DEC);
	Serial.print("Over-current:\t");
	Serial.println((buf[0] & bmHUB_STATUS_OVER_CURRENT) > 0, DEC);

	Serial.println("\r\nChanges");
	Serial.print("Local Pwr Src:\t");
	Serial.println((buf[2] & bmHUB_STATUS_C_LOCAL_POWER_SOURCE) > 0, DEC);
	Serial.print("Over-current:\t");
	Serial.println((buf[2] & bmHUB_STATUS_C_OVER_CURRENT) > 0, DEC);
	Serial.println("");
}
