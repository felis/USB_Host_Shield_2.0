/* MAX3421E-based USB Host Library header file */
#ifndef _USBHOST_H_
#define _USBHOST_H_

//#include <WProgram.h>
#include "avrpins.h"
#include "max3421e.h"
#include "usb_ch9.h"


/* SPI initialization */
template< typename CLK, typename MOSI, typename MISO > class SPi
{
  public:
    static void init() {
     uint8_t tmp; 
      CLK::SetDirWrite();
      MOSI::SetDirWrite();
      MISO::SetDirRead();
      /* mode 00 (CPOL=0, CPHA=0) master, fclk/2. Mode 11 (CPOL=11, CPHA=11) is also supported by MAX3421E */
      SPCR = 0x50;
      SPSR = 0x01;
      /**/
      tmp = SPSR;
      tmp = SPDR;
    }
}; 

/* SPI pin definitions. see avrpins.h   */
#if defined(__AVR_ATmega1280__) || (__AVR_ATmega2560__)
typedef SPi< Pb1, Pb2, Pb3 > spi;
#endif
#if  defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
typedef SPi< Pb5, Pb3, Pb4 > spi;
#endif

template< typename SS, typename INTR > class MAX3421e /* : public spi */
{
  public:
    MAX3421e();
    void regWr( uint8_t reg, uint8_t data );
    uint8_t* bytesWr( uint8_t reg, uint8_t nbytes, uint8_t* data_p );
    void gpioWr( uint8_t data );
    uint8_t regRd( uint8_t reg );
    uint8_t* bytesRd( uint8_t reg, uint8_t nbytes, uint8_t* data_p );
    uint8_t gpioRd();
    uint16_t reset();
    uint8_t init();
};
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
/* initialize MAX3421E. Set Host mode, pullups, and stuff. Returns 0 if success, -1 if not */
template< typename SS, typename INTR >
uint8_t MAX3421e< SS, INTR >::init()
{
  if( reset() == 0 ) { //OSCOKIRQ hasn't asserted in time
    return ( -1 );
  }
  regWr( rMODE, bmDPPULLDN|bmDMPULLDN|bmHOST );      // set pull-downs, Host
  
  return( 0 );
}

#endif //_USBHOST_H_
