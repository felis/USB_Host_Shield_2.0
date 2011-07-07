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
/* USB functions */

#include "avrpins.h"
#include "max3421e.h"
#include "usbhost.h"
#include "Usb.h"
#include "WProgram.h"

static uint8_t usb_error = 0;
static uint8_t usb_task_state;

/* constructor */
USB::USB () : bmHubPre(0)
{
    usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE;  //set up state machine
    init(); 
}

/* Initialize data structures */
void USB::init()
{
	devConfigIndex	= 0;
	bmHubPre		= 0;
}

uint8_t USB::getUsbTaskState( void )
{
    return( usb_task_state );
}

void USB::setUsbTaskState( uint8_t state )
{
    usb_task_state = state;
}     

EpInfo* USB::getEpInfoEntry( uint8_t addr, uint8_t ep )
{
	UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

	if (!p || !p->epinfo)
		return NULL;

	EpInfo *pep = p->epinfo;

	for (uint8_t i=0; i<p->epcount; i++)
	{
		if ((pep)->epAddr == ep)
			return pep;

		pep ++;
	}
	return NULL;
}

/* set device table entry */
/* each device is different and has different number of endpoints. This function plugs endpoint record structure, defined in application, to devtable */
uint8_t USB::setEpInfoEntry( uint8_t addr, uint8_t epcount, EpInfo* eprecord_ptr )
{
	if (!eprecord_ptr)
		return USB_ERROR_INVALID_ARGUMENT;

	UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

	if (!p)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
	
	p->address	= addr;
	p->epinfo	= eprecord_ptr;
	p->epcount	= epcount;

	return 0;
}

uint8_t USB::SetAddress(uint8_t addr, uint8_t ep, EpInfo **ppep, uint16_t &nak_limit)
{
	UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

	if (!p)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

 	if (!p->epinfo)
		return USB_ERROR_EPINFO_IS_NULL;

	*ppep = getEpInfoEntry(addr, ep);

	if (!*ppep)
		return USB_ERROR_EP_NOT_FOUND_IN_TBL;

	nak_limit = (0x0001UL << ( ( (*ppep)->bmNakPower > USB_NAK_MAX_POWER ) ? USB_NAK_MAX_POWER : (*ppep)->bmNakPower) ); 

	regWr( rPERADDR, addr );                    //set peripheral address

	uint8_t	mode = regRd( rMODE );

	// Set bmLOWSPEED and bmHUBPRE in case of low-speed device, reset them otherwise
	regWr( rMODE, (p->lowspeed) ? mode | bmLOWSPEED | bmHubPre : mode & ~(bmHUBPRE | bmLOWSPEED)); 

	delay(20);

	return 0;
}

/* Control transfer. Sets address, endpoint, fills control packet with necessary data, dispatches control packet, and initiates bulk IN transfer,   */
/* depending on request. Actual requests are defined as inlines                                                                                      */
/* return codes:                */
/* 00       =   success         */
/* 01-0f    =   non-zero HRSLT  */
uint8_t USB::ctrlReq( uint8_t addr, uint8_t ep, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi, 
					  uint16_t wInd, uint16_t total, uint16_t nbytes, uint8_t* dataptr, USBReadParser *p)
{
	boolean direction = false;     //request direction, IN or OUT
	uint8_t rcode;   
	SETUP_PKT setup_pkt;

	EpInfo		*pep = NULL;
	uint16_t	nak_limit;

	rcode = SetAddress(addr, ep, &pep, nak_limit);

	if (rcode)
		return rcode;

	direction = (( bmReqType & 0x80 ) > 0);

    /* fill in setup packet */
    setup_pkt.ReqType_u.bmRequestType	= bmReqType;
    setup_pkt.bRequest					= bRequest;
    setup_pkt.wVal_u.wValueLo			= wValLo;
    setup_pkt.wVal_u.wValueHi			= wValHi;
    setup_pkt.wIndex					= wInd;
    setup_pkt.wLength					= total;
    
	bytesWr( rSUDFIFO, 8, (uint8_t*)&setup_pkt );		//transfer to setup packet FIFO

    rcode = dispatchPkt( tokSETUP, ep, nak_limit );     //dispatch packet

	if( rcode )		//return HRSLT if not zero
        return( rcode );

	if( dataptr != NULL )								//data stage, if present
	{
		if( direction )				//IN transfer
		{                      
			uint16_t left = total;

			pep->bmRcvToggle = 1;	//bmRCVTOG1;

			while (left)
			{
				// Bytes read into buffer
				uint16_t read = nbytes;
				//uint16_t read = (left<nbytes) ? left : nbytes;

				rcode = InTransfer( pep, nak_limit, &read, dataptr );

				if (rcode)
					return rcode;

				// Invoke callback function if inTransfer completed successfuly and callback function pointer is specified
				if (!rcode && p)
					((USBReadParser*)p)->Parse( read, dataptr, total - left );

				left -= read;

				if (read < nbytes)
					break;
			}
		}
		else						//OUT transfer
		{              
			pep->bmSndToggle = 1;	//bmSNDTOG1;
			rcode = OutTransfer( pep, nak_limit, nbytes, dataptr );
		}    
		if( rcode )											//return error
			return( rcode );
	}
	// Status stage
	return dispatchPkt( (direction) ? tokOUTHS : tokINHS, ep, nak_limit ); //GET if direction
}

/* IN transfer to arbitrary endpoint. Assumes PERADDR is set. Handles multiple packets if necessary. Transfers 'nbytes' bytes. */
/* Keep sending INs and writes data to memory area pointed by 'data'                                                           */
/* rcode 0 if no errors. rcode 01-0f is relayed from dispatchPkt(). Rcode f0 means RCVDAVIRQ error,
            fe USB xfer timeout */
uint8_t USB::inTransfer( uint8_t addr, uint8_t ep, uint16_t *nbytesptr, uint8_t* data)
{
	EpInfo		*pep = NULL;
	uint16_t	nak_limit = 0;

	uint8_t rcode = SetAddress(addr, ep, &pep, nak_limit);

	if (rcode)
		return rcode;

	return InTransfer(pep, nak_limit, nbytesptr, data);
}

uint8_t USB::InTransfer(EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t* data)
{
	uint8_t rcode = 0;
	uint8_t pktsize;

	uint16_t	nbytes		= *nbytesptr;
	uint8_t		maxpktsize	= pep->maxPktSize; 

	*nbytesptr = 0;
	regWr( rHCTL, (pep->bmRcvToggle) ? bmRCVTOG1 : bmRCVTOG0 );    //set toggle value

	while( 1 )		// use a 'return' to exit this loop
	{ 
        rcode = dispatchPkt( tokIN, pep->epAddr, nak_limit );	//IN packet to EP-'endpoint'. Function takes care of NAKS.
        
		if( rcode ) 
            return( rcode );                            //should be 0, indicating ACK. Else return error code.
        
        /* check for RCVDAVIRQ and generate error if not present */ 
        /* the only case when absense of RCVDAVIRQ makes sense is when toggle error occured. Need to add handling for that */
        if(( regRd( rHIRQ ) & bmRCVDAVIRQ ) == 0 ) 
            return ( 0xf0 );                            //receive error
        
        pktsize = regRd( rRCVBC );                      //number of received bytes
        
        assert(pktsize <= nbytes);
   
		int16_t	 mem_left = (int16_t)nbytes - *((int16_t*)nbytesptr);

		if (mem_left < 0)
			mem_left = 0;

		data = bytesRd( rRCVFIFO, ((pktsize > mem_left) ? mem_left : pktsize), data );

        regWr( rHIRQ, bmRCVDAVIRQ );                    // Clear the IRQ & free the buffer
        *nbytesptr += pktsize;							// add this packet's byte count to total transfer length

        /* The transfer is complete under two conditions:           */
        /* 1. The device sent a short packet (L.T. maxPacketSize)   */
        /* 2. 'nbytes' have been transferred.                       */
        if (( pktsize < maxpktsize ) || (*nbytesptr >= nbytes ))		// have we transferred 'nbytes' bytes?
		{     
			// Save toggle value
			pep->bmRcvToggle = (( regRd( rHRSL ) & bmRCVTOGRD )) ? 1 : 0;

			return( 0 );
        } // if
	} //while( 1 )
}

/* OUT transfer to arbitrary endpoint. Handles multiple packets if necessary. Transfers 'nbytes' bytes. */
/* Handles NAK bug per Maxim Application Note 4000 for single buffer transfer   */
/* rcode 0 if no errors. rcode 01-0f is relayed from HRSL                       */
uint8_t USB::outTransfer( uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* data )
{
	EpInfo		*pep = NULL;
	uint16_t	nak_limit;

	uint8_t rcode = SetAddress(addr, ep, &pep, nak_limit);

	if (rcode)
		return rcode;

	return OutTransfer(pep, nak_limit, nbytes, data);
}

uint8_t USB::OutTransfer(EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t *data)
{
	uint8_t		rcode, retry_count;
	uint8_t		*data_p = data;   //local copy of the data pointer
	uint16_t	bytes_tosend, nak_count;
	uint16_t	bytes_left = nbytes;

	uint8_t maxpktsize = pep->maxPktSize; 
 
	if (maxpktsize < 1 || maxpktsize > 64)
		return USB_ERROR_INVALID_MAX_PKT_SIZE;
 
	unsigned long timeout = millis() + USB_XFER_TIMEOUT;
 
	regWr( rHCTL, (pep->bmSndToggle) ? bmSNDTOG1 : bmSNDTOG0 );    //set toggle value
	
	while( bytes_left ) 
	{
		retry_count = 0;
		nak_count = 0;
		bytes_tosend = ( bytes_left >= maxpktsize ) ? maxpktsize : bytes_left;
		bytesWr( rSNDFIFO, bytes_tosend, data_p );      //filling output FIFO
		regWr( rSNDBC, bytes_tosend );                  //set number of bytes    
		regWr( rHXFR, ( tokOUT | pep->epAddr ));        //dispatch packet
		while(!(regRd( rHIRQ ) & bmHXFRDNIRQ ));        //wait for the completion IRQ
		regWr( rHIRQ, bmHXFRDNIRQ );                    //clear IRQ
		rcode = ( regRd( rHRSL ) & 0x0f );

		while( rcode && ( timeout > millis())) 
		{
			switch( rcode ) 
			{
			case hrNAK:
				nak_count ++;
				if( nak_limit && ( nak_count == nak_limit )) 
					return( rcode );
				break;
			case hrTIMEOUT:
				retry_count ++;
				if( retry_count == USB_RETRY_LIMIT ) 
					return( rcode );
				break;
			default:
				return( rcode );
			}//switch( rcode

			/* process NAK according to Host out NAK bug */
			regWr( rSNDBC, 0 );
			regWr( rSNDFIFO, *data_p );
			regWr( rSNDBC, bytes_tosend );
			regWr( rHXFR, ( tokOUT | pep->epAddr ));        //dispatch packet
			while(!(regRd( rHIRQ ) & bmHXFRDNIRQ ));        //wait for the completion IRQ
			regWr( rHIRQ, bmHXFRDNIRQ );                    //clear IRQ
			rcode = ( regRd( rHRSL ) & 0x0f );
		}//while( rcode && ....
		bytes_left -= bytes_tosend;
		data_p += bytes_tosend;
	}//while( bytes_left...
	pep->bmSndToggle = ( regRd( rHRSL ) & bmSNDTOGRD ) ? 1 : 0; //bmSNDTOG1 : bmSNDTOG0;  //update toggle
	return( rcode );    //should be 0 in all cases
}
/* dispatch usb packet. Assumes peripheral address is set and relevant buffer is loaded/empty       */
/* If NAK, tries to re-send up to nak_limit times                                                   */
/* If nak_limit == 0, do not count NAKs, exit after timeout                                         */
/* If bus timeout, re-sends up to USB_RETRY_LIMIT times                                             */
/* return codes 0x00-0x0f are HRSLT( 0x00 being success ), 0xff means timeout                       */
uint8_t USB::dispatchPkt( uint8_t token, uint8_t ep, uint16_t nak_limit )
{
	unsigned long timeout = millis() + USB_XFER_TIMEOUT;
	uint8_t		tmpdata;   
	uint8_t		rcode;
	uint8_t		retry_count = 0;
	uint16_t	nak_count = 0;

	while( timeout > millis() ) 
	{
		regWr( rHXFR, ( token|ep ));            //launch the transfer
		rcode = 0xff;   

		while( millis() < timeout )				//wait for transfer completion
		{           
			tmpdata = regRd( rHIRQ );

			if( tmpdata & bmHXFRDNIRQ ) 
			{
				regWr( rHIRQ, bmHXFRDNIRQ );    //clear the interrupt
				rcode = 0x00;
				break;
			}//if( tmpdata & bmHXFRDNIRQ

		}//while ( millis() < timeout

		if( rcode != 0x00 )                 //exit if timeout
			return( rcode );

		rcode = ( regRd( rHRSL ) & 0x0f );  //analyze transfer result

		switch( rcode ) 
		{
		case hrNAK:
			nak_count ++;
			if( nak_limit && ( nak_count == nak_limit )) 
				return( rcode );
			break;
		case hrTIMEOUT:
			retry_count ++;
			if( retry_count == USB_RETRY_LIMIT )
				return( rcode );
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
	uint8_t rcode;
	uint8_t tmpdata;
	static unsigned long delay = 0;
	USB_DEVICE_DESCRIPTOR buf;
	bool lowspeed = false;

	MAX3421E::Task();

    tmpdata = getVbusState();

	/* modify USB task state if Vbus changed */
    switch( tmpdata ) 
	{
        case SE1:   //illegal state
            usb_task_state = USB_DETACHED_SUBSTATE_ILLEGAL;
			lowspeed = false;
            break;
        case SE0:   //disconnected
            if(( usb_task_state & USB_STATE_MASK ) != USB_STATE_DETACHED ) 
                usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE;
			lowspeed = false;
            break;
        case LSHOST:
			lowspeed = true;
        case FSHOST:    //attached
            if(( usb_task_state & USB_STATE_MASK ) == USB_STATE_DETACHED ) 
			{
                delay = millis() + USB_SETTLE_DELAY;
                usb_task_state = USB_ATTACHED_SUBSTATE_SETTLE;
            }
            break;
    }// switch( tmpdata

	for (uint8_t i=0; i<USB_NUMDEVICES; i++)
		if (devConfig[i])
			rcode = devConfig[i]->Poll();

    switch( usb_task_state ) {
        case USB_DETACHED_SUBSTATE_INITIALIZE:
            init();

			for (uint8_t i=0; i<USB_NUMDEVICES; i++)
				if (devConfig[i])
					rcode = devConfig[i]->Release();

			usb_task_state = USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE;
            break;
        case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:     //just sit here
            break;
        case USB_DETACHED_SUBSTATE_ILLEGAL:             //just sit here
            break;
        case USB_ATTACHED_SUBSTATE_SETTLE:              //setlle time for just attached device                  
            if( delay < millis() ) 
                usb_task_state = USB_ATTACHED_SUBSTATE_RESET_DEVICE;
            break;
        case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
            regWr( rHCTL, bmBUSRST );									//issue bus reset
            usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE;
            break;
        case USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE:
            if(( regRd( rHCTL ) & bmBUSRST ) == 0 ) 
			{
                tmpdata = regRd( rMODE ) | bmSOFKAENAB;                 //start SOF generation
                regWr( rMODE, tmpdata );
                usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_SOF;
                delay = millis() + 20; //20ms wait after reset per USB spec
            }
            break;
        case USB_ATTACHED_SUBSTATE_WAIT_SOF:  //todo: change check order
            if( regRd( rHIRQ ) & bmFRAMEIRQ )							//when first SOF received we can continue
			{															
				if( delay < millis() )									//20ms passed
					usb_task_state = USB_STATE_CONFIGURING;
            }
            break;
        case USB_STATE_CONFIGURING:
			rcode = Configuring(0, 0, lowspeed);

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
            break;
        case USB_STATE_ERROR:
            break;
    } // switch( usb_task_state )
}    

uint8_t USB::DefaultAddressing(uint8_t parent, uint8_t port, bool lowspeed)
{
	uint8_t		buf[12];
	uint8_t		rcode;
	UsbDevice	*p0 = NULL, *p = NULL;

	// Get pointer to pseudo device with address 0 assigned
	p0 = addrPool.GetUsbDevicePtr(0);

	if (!p0)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

	if (!p0->epinfo)
		return USB_ERROR_EPINFO_IS_NULL;

	p0->lowspeed = (lowspeed) ? true : false;

	// Allocate new address according to device class
	uint8_t bAddress = addrPool.AllocAddress(parent, false, port);

	if (!bAddress)
		return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

	p = addrPool.GetUsbDevicePtr(bAddress);

	if (!p)
		return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

	p->lowspeed = lowspeed;
	
	// Assign new address to the device
	rcode = setAddr( 0, 0, bAddress );

	if (rcode)
	{
		addrPool.FreeAddress(bAddress);
		bAddress = 0;
		return rcode;
	}
	return 0;
};

uint8_t USB::Configuring(uint8_t parent, uint8_t port, bool lowspeed)
{
	static uint8_t dev_index = 0;
	uint8_t rcode = 0;

	for (; devConfigIndex<USB_NUMDEVICES; devConfigIndex++)
	{
		if (!devConfig[devConfigIndex])
			continue;

		rcode = devConfig[devConfigIndex]->Init(parent, port, lowspeed);

		if (!rcode)
		{
			devConfigIndex = 0;
			return 0;
		}
		if (!(rcode == USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED || rcode == USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE))
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

	rcode = DefaultAddressing(parent, port, lowspeed);

	return rcode;
}

uint8_t USB::ReleaseDevice(uint8_t addr)
{
	if (!addr)
		return 0;

	for (uint8_t i=0; i<USB_NUMDEVICES; i++)
		if (devConfig[i]->GetAddress() == addr)
			return devConfig[i]->Release();
}

#if 1 //!defined(USB_METHODS_INLINE)
//get device descriptor
uint8_t USB::getDevDescr( uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* dataptr ) 
{
    return( ctrlReq( addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0x00, USB_DESCRIPTOR_DEVICE, 0x0000, nbytes, nbytes, dataptr, NULL ));
}
//get configuration descriptor  
uint8_t USB::getConfDescr( uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t conf, uint8_t* dataptr ) 
{
	return( ctrlReq( addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, conf, USB_DESCRIPTOR_CONFIGURATION, 0x0000, nbytes, nbytes, dataptr, NULL ));
}

uint8_t USB::getConfDescr( uint8_t addr, uint8_t ep, uint8_t conf, USBReadParser *p )
{
	const uint8_t	bufSize = 64;
	uint8_t			buf[bufSize];

	uint8_t			ret = getConfDescr( addr, ep, 8, conf, buf );

	if (ret)
		return ret;

	uint16_t		total = ((USB_CONFIGURATION_DESCRIPTOR*)buf)->wTotalLength;

	//USBTRACE2("\r\ntotal conf.size:", total);

    return( ctrlReq( addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, conf, USB_DESCRIPTOR_CONFIGURATION, 0x0000, total, bufSize, buf, p ));
}

//get string descriptor
uint8_t USB::getStrDescr( uint8_t addr, uint8_t ep, uint16_t ns, uint8_t index, uint16_t langid, uint8_t* dataptr ) 
{
    return( ctrlReq( addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, index, USB_DESCRIPTOR_STRING, langid, ns, ns, dataptr, NULL ));
}
//set address 
uint8_t USB::setAddr( uint8_t oldaddr, uint8_t ep, uint8_t newaddr ) 
{
    return( ctrlReq( oldaddr, ep, bmREQ_SET, USB_REQUEST_SET_ADDRESS, newaddr, 0x00, 0x0000, 0x0000, 0x0000, NULL, NULL ));
}
//set configuration
uint8_t USB::setConf( uint8_t addr, uint8_t ep, uint8_t conf_value ) 
{
    return( ctrlReq( addr, ep, bmREQ_SET, USB_REQUEST_SET_CONFIGURATION, conf_value, 0x00, 0x0000, 0x0000, 0x0000, NULL, NULL ));         
}

#endif // defined(USB_METHODS_INLINE)

