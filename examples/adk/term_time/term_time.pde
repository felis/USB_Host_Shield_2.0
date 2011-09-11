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
  uint8_t buf[ 12 ] = { 0 }; //buffer to convert unsigned long to ASCII
  const char* sec_ela = " seconds elapsed\r";
  uint8_t rcode;
  
   Usb.Task();
   if( adk.isReady() == false ) {
     return;
   }
   
    ultoa( millis()/1000, (char *)buf, 10 );
     
    rcode = adk.SndData( strlen((char *)buf), buf );
    rcode = adk.SndData( strlen( sec_ela), (uint8_t *)sec_ela );
    
      delay( 1000 );       
}
