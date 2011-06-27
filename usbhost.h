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
/* MAX3421E-based USB Host Library header file */
#ifndef _USBHOST_H_
#define _USBHOST_H_

//#include <WProgram.h>
#include "avrpins.h"
#include "max3421e.h"
#include "usb_ch9.h"


/* SPI initialization */
template< typename CLK, typename MOSI, typename MISO, typename SPI_SS > class SPi
{
  public:
    static void init() {
     uint8_t tmp; 
      CLK::SetDirWrite();
      MOSI::SetDirWrite();
      MISO::SetDirRead();
      SPI_SS::SetDirWrite();
      /* mode 00 (CPOL=0, CPHA=0) master, fclk/2. Mode 11 (CPOL=11, CPHA=11) is also supported by MAX3421E */
      SPCR = 0x50;
      SPSR = 0x01;
      /**/
      tmp = SPSR;
      tmp = SPDR;
    }
}; 

/* SPI pin definitions. see avrpins.h   */
#if defined(__AVR_ATmega1280__) || (__AVR_ATmega2560__) || defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
typedef SPi< Pb1, Pb2, Pb3, Pb0 > spi;
#endif
#if  defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
typedef SPi< Pb5, Pb3, Pb4, Pb2 > spi;
#endif
#if defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
typedef SPi< Pb7, Pb5, Pb6, Pb4 > spi;
#endif

template< typename SS, typename INTR > class MAX3421e /* : public spi */
{
	static uint8_t vbusState;

  public:
    MAX3421e();
    void regWr( uint8_t reg, uint8_t data );
    uint8_t* bytesWr( uint8_t reg, uint8_t nbytes, uint8_t* data_p );
    void gpioWr( uint8_t data );
    uint8_t regRd( uint8_t reg );
    uint8_t* bytesRd( uint8_t reg, uint8_t nbytes, uint8_t* data_p );
    uint8_t gpioRd();
    uint16_t reset();
    int8_t Init();
	uint8_t getVbusState( void ) { return vbusState; };
	void busprobe();
	uint8_t GpxHandler();
	uint8_t IntHandler();
	uint8_t Task();
};

template< typename SS, typename INTR >
uint8_t MAX3421e< SS, INTR >::vbusState = 0;

/* constructor */
template< typename SS, typename INTR >
MAX3421e< SS, INTR >::MAX3421e()
{
  /* pin and peripheral setup */
  SS::SetDirWrite();
  SS::Set();
  spi::init();
  INTR::SetDirRead();

  /* MAX3421E - full-duplex SPI, level interrupt */
  regWr( rPINCTL,( bmFDUPSPI + bmINTLEVEL ));
};
/* write single byte into MAX3421 register */
template< typename SS, typename INTR >
void MAX3421e< SS, INTR >::regWr( uint8_t reg, uint8_t data )
{
  SS::Clear();
  SPDR = ( reg | 0x02 );
  while(!( SPSR & ( 1 << SPIF )));
  SPDR = data;
  while(!( SPSR & ( 1 << SPIF )));
  SS::Set();
  return;
};
/* multiple-byte write                            */
/* returns a pointer to memory position after last written */
template< typename SS, typename INTR >
uint8_t* MAX3421e< SS, INTR >::bytesWr( uint8_t reg, uint8_t nbytes, uint8_t* data_p )
{
  SS::Clear();
  SPDR = ( reg | 0x02 );  //set WR bit and send register number
  while( nbytes-- ) {
    while(!( SPSR & ( 1 << SPIF )));  //check if previous byte was sent
    SPDR = ( *data_p );               // send next data byte
    data_p++;                         // advance data pointer
  }
  while(!( SPSR & ( 1 << SPIF )));
  SS::Set();
  return( data_p );
}
/* GPIO write                                           */
/*GPIO byte is split between 2 registers, so two writes are needed to write one byte */
/* GPOUT bits are in the low nibble. 0-3 in IOPINS1, 4-7 in IOPINS2 */
template< typename SS, typename INTR >
void MAX3421e< SS, INTR >::gpioWr( uint8_t data )
{
  regWr( rIOPINS1, data );
  data >>= 4;
  regWr( rIOPINS2, data );
  return;     
}
/* single host register read    */
template< typename SS, typename INTR >
uint8_t MAX3421e< SS, INTR >::regRd( uint8_t reg )
{
  SS::Clear();
  SPDR = reg;
  while(!( SPSR & ( 1 << SPIF )));
  SPDR = 0; //send empty byte
  while(!( SPSR & ( 1 << SPIF )));
  SS::Set();
  return( SPDR );
}
/* multiple-byte register read  */
/* returns a pointer to a memory position after last read   */
template< typename SS, typename INTR >
uint8_t* MAX3421e< SS, INTR >::bytesRd( uint8_t reg, uint8_t nbytes, uint8_t* data_p )
{
  SS::Clear();
  SPDR = reg;      
  while(!( SPSR & ( 1 << SPIF )));    //wait
  while( nbytes ) {
    SPDR = 0; //send empty byte
    nbytes--;
    while(!( SPSR & ( 1 << SPIF )));
    *data_p = SPDR;
    data_p++;
  }
  SS::Set();
  return( data_p );
}
/* GPIO read. See gpioWr for explanation */
/* GPIN pins are in high nibbles of IOPINS1, IOPINS2    */
template< typename SS, typename INTR >
uint8_t MAX3421e< SS, INTR >::gpioRd()
{
  uint8_t gpin = 0;
  gpin = regRd( rIOPINS2 );            //pins 4-7
  gpin &= 0xf0;                        //clean lower nibble
  gpin |= ( regRd( rIOPINS1 ) >>4 ) ;  //shift low bits and OR with upper from previous operation.
  return( gpin );
}
/* reset MAX3421E. Returns number of cycles it took for PLL to stabilize after reset
  or zero if PLL haven't stabilized in 65535 cycles */
template< typename SS, typename INTR >
uint16_t MAX3421e< SS, INTR >::reset()
{
  uint16_t i = 0;
  regWr( rUSBCTL, bmCHIPRES );
  regWr( rUSBCTL, 0x00 );
  while( ++i ) {
    if(( regRd( rUSBIRQ ) & bmOSCOKIRQ )) {
      break;
    }
  }
  return( i );
}
///* initialize MAX3421E. Set Host mode, pullups, and stuff. Returns 0 if success, -1 if not */
//template< typename SS, typename INTR >
//int8_t MAX3421e< SS, INTR >::Init()
//{
//  if( reset() == 0 ) { //OSCOKIRQ hasn't asserted in time
//    return ( -1 );
//  }
//  regWr( rMODE, bmDPPULLDN|bmDMPULLDN|bmHOST );      // set pull-downs, Host
//  
//  return( 0 );
//}
/* initialize MAX3421E. Set Host mode, pullups, and stuff. Returns 0 if success, -1 if not */
template< typename SS, typename INTR >
int8_t MAX3421e< SS, INTR >::Init()
{
	if( reset() == 0 ) 
	{ //OSCOKIRQ hasn't asserted in time
		return ( -1 );
	}
	regWr( rMODE, bmDPPULLDN|bmDMPULLDN|bmHOST );      // set pull-downs, Host
  
	regWr( rHIEN, bmCONDETIE|bmFRAMEIE );                                             //connection detection

    /* check if device is connected */
    regWr( rHCTL,bmSAMPLEBUS );                                             // sample USB bus
    while(!(regRd( rHCTL ) & bmSAMPLEBUS ));                                //wait for sample operation to finish

	busprobe();                                                             //check if anything is connected

    regWr( rHIRQ, bmCONDETIRQ );                                            //clear connection detect interrupt                 
    regWr( rCPUCTL, 0x01 );                                                 //enable interrupt pin
	return( 0 );
}

/* probe bus to determine device presense and speed and switch host to this speed */
template< typename SS, typename INTR >
void MAX3421e< SS, INTR >::busprobe()
{
	uint8_t bus_sample;
    bus_sample = regRd( rHRSL );							//Get J,K status
    bus_sample &= ( bmJSTATUS|bmKSTATUS );					//zero the rest of the byte
    switch( bus_sample ) {									//start full-speed or low-speed host 
        case( bmJSTATUS ):
            if(( regRd( rMODE ) & bmLOWSPEED ) == 0 ) {
                regWr( rMODE, MODE_FS_HOST );				//start full-speed host
                vbusState = FSHOST;
            }
            else {
                regWr( rMODE, MODE_LS_HOST);				//start low-speed host
                vbusState = LSHOST;
            }
            break;
        case( bmKSTATUS ):
            if(( regRd( rMODE ) & bmLOWSPEED ) == 0 ) {
                regWr( rMODE, MODE_LS_HOST );				//start low-speed host
                vbusState = LSHOST;
            }
            else {
                regWr( rMODE, MODE_FS_HOST );				//start full-speed host
                vbusState = FSHOST;
            }
            break;
        case( bmSE1 ):										//illegal state
            vbusState = SE1;
            break;
        case( bmSE0 ):										//disconnected state
          regWr( rMODE, bmDPPULLDN|bmDMPULLDN|bmHOST|bmSEPIRQ);
          vbusState = SE0;
          break;
        }//end switch( bus_sample )
}
/* MAX3421 state change task and interrupt handler */
template< typename SS, typename INTR >
uint8_t MAX3421e< SS, INTR >::Task( void )
{
	uint8_t rcode = 0;
	uint8_t pinvalue;
    //Serial.print("Vbus state: ");
    //Serial.println( vbusState, HEX );
    pinvalue = INTR::IsSet(); //Read();
    //pinvalue = digitalRead( MAX_INT );    
    if( pinvalue  == 0 ) {
        rcode = IntHandler();
    }
//    pinvalue = digitalRead( MAX_GPX );
//    if( pinvalue == LOW ) {
//        GpxHandler();
//    }
//    usbSM();                                //USB state machine                            
    return( rcode );   
}   
template< typename SS, typename INTR >
uint8_t MAX3421e< SS, INTR >::IntHandler()
{
	uint8_t HIRQ;
	uint8_t HIRQ_sendback = 0x00;
    HIRQ = regRd( rHIRQ );                  //determine interrupt source
    //if( HIRQ & bmFRAMEIRQ ) {               //->1ms SOF interrupt handler
    //    HIRQ_sendback |= bmFRAMEIRQ;
    //}//end FRAMEIRQ handling
    if( HIRQ & bmCONDETIRQ ) {
        busprobe();
        HIRQ_sendback |= bmCONDETIRQ;
    }
    /* End HIRQ interrupts handling, clear serviced IRQs    */
    regWr( rHIRQ, HIRQ_sendback );
    return( HIRQ_sendback );
}
//template< typename SS, typename INTR >
//uint8_t MAX3421e< SS, INTR >::GpxHandler()
//{
//	uint8_t GPINIRQ = regRd( rGPINIRQ );          //read GPIN IRQ register
////    if( GPINIRQ & bmGPINIRQ7 ) {            //vbus overload
////        vbusPwr( OFF );                     //attempt powercycle
////        delay( 1000 );
////        vbusPwr( ON );
////        regWr( rGPINIRQ, bmGPINIRQ7 );
////    }       
//    return( GPINIRQ );
//}

#endif //_USBHOST_H_
