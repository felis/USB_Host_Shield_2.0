#include <avrpins.h>
#include <max3421e.h>
#include <usbhost.h>
#include <usb_ch9.h>
#include <Usb.h>
#include <usbhub.h>
#include <avr/pgmspace.h>
#include <address.h>

#include <adk.h>

USB Usb;

ADK adk(&Usb,"Circuits@Home, ltd.",
            "USB Host Shield",
            "Arduino Terminal for Android",
            "1.0",
            "http://www.circuitsathome.com",
            "0000000000000001");

void setup()
{
	Serial.begin(115200);
	Serial.println("\r\nADK demo start");
        
        if (Usb.Init() == -1) {
          Serial.println("OSCOKIRQ failed to assert");
        while(1); //halt
        }//if (Usb.Init() == -1...
}

void loop()
{
  uint8_t rcode;
  uint8_t msg[64] = { 0x00 };
  const char* recv = "Received: "; 
   
   Usb.Task();
   
   if( adk.isReady() == false ) {
     return;
   }
   uint16_t len = 64;
   
   rcode = adk.RcvData(&len, msg);
   if( rcode & ( rcode != hrNAK )) {
     USBTRACE2("Data rcv. :", rcode );
   } 
   if(len > 0) {
     USBTRACE("\r\nData Packet.");

    for( uint8_t i = 0; i < len; i++ ) {
      Serial.print(msg[i]);
    }
    /* sending back what was received */    
    rcode = adk.SndData( strlen( recv ), (uint8_t *)recv );    
    rcode = adk.SndData( strlen(( char * )msg ), msg );

   }//if( len > 0 )...

   delay( 1000 );       
}

