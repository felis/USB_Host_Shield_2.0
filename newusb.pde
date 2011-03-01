/* new USB library tests */

//nclude <digitalWriteFast.h>
#include "usbhost.h"

MAX3421e<P10, P9> Max;
uint8_t buf[10] = {0};

void setup() {
//  Max.regWr( rPINCTL,( bmFDUPSPI + bmINTLEVEL ));
//     Max.regWr( rUSBCTL, bmCHIPRES );                        //Chip reset. This stops the oscillator
//    Max.regWr( rUSBCTL, 0x00 );                             //Remove the reset
//    delay( 100 );
  Serial.begin( 115200 );
  Serial.println("Start");

  //  pinModeFast2( 8, OUTPUT)

}

void loop() {
//  uint16_t i;
//  Max.regWr( rUSBCTL, bmCHIPRES );
//  Max.regWr( rUSBCTL, 0x00 );
//  for( i = 0; i < 65535; i++ ) {
//    if( ( Max.regRd( rUSBIRQ ) & bmOSCOKIRQ )) {
//      break;
//    }
//  }
  Serial.println( Max.reset(), DEC );
  //Max.reset();
  //delay( 100 );
  //Max.regRd( rREVISION );
  //Serial.println( Max.regRd( rREVISION ), HEX );
 
  //Serial.println("tick");
  //  uint8_t tmp;
  //  for( uint16_t i = 0; i < 256; i++ ) {
  //    tmp = Max.SPIxfer( i );
  //    if( tmp != i ) {
  //      Serial.println("error");
  //      }
  //    if( SPSR & 0x40 ) {
  //      Serial.println("WCOL");
  //      } 
  //  }
  //  static bool oldintval = 0;
  //  if( Max.intval() != oldintval ) {
  //    oldintval = Max.intval();
  //    Serial.println( oldintval, DEC );
  //  }
  //  Max.sstoggle();

  //digitalWriteFast2( 8, 0 );
  //digitalWriteFast2( 8, 1 );
}



