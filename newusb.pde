/* reference USB library test sketch */

#include "usbhost.h"

MAX3421e<P10, P9> Max;

void setup() {
  Serial.begin( 115200 );
  Serial.println("\r\nStart");

  if ( Max.init() == -1 ) {
    Serial.println("OSCOKIRQ failed to assert");
    while(1);
  }
}

void loop() {
  Serial.print("Revision Register: ");
  Serial.println( Max.regRd( rREVISION ), HEX );
  delay( 1000 );
}



