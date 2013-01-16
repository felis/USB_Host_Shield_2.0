/*
Example sketch for the Wii libary showing the IR camera functionality. This example
is for the Bluetooth Wii library developed for the USB shield from Circuits@Home
 
Created by Allan Glover and includes much from what Kristian Lauszus wrote in the existing 
Wii example. Contact Kristian:  http://blog.tkjelectronics.dk/ or send email at kristianl@tkjelectronics.com.
Contact Allan at adglover9.81@gmail.com
 
To test the Wiimote IR camera, you will need access to an IR source. Sunlight will work but is not ideal. 
The simpleist solution is to use the Wii sensor bar, i.e. emitter bar, supplied by the Wii system. Otherwise,
wire up a IR LED yourself.
*/

#include <Wii.h>
#ifndef WIICAMERA // Used to check if WIICAMERA is defined
#error "Uncomment WIICAMERA in Wii.h to use this example"
#endif
USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so
/* You can create the instance of the class in two ways */
WII Wii(&Btd,PAIR); // This will start an inquiry and then pair with your Wiimote - you only have to do this once
//WII Wii(&Btd); // After the wiimote pairs once with the line of code above, you can simply create the instance like so and re upload and then press any button on the Wiimote

bool printIR1;
bool printIR2;

void setup() {
  Serial.begin(115200);
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nWiimote Bluetooth Library Started"));
}

void loop() {
  Usb.Task();
  if(Wii.wiimoteConnected) {
    if(Wii.getButtonClick(HOME)) { // You can use getButtonPress to see if the button is held down
      Serial.print(F("\r\nHOME"));
      Wii.disconnect(); // Disconnect the Wiimote - it will establish the connection again since the Wiimote automatically reconnects
    }    
    else {
      if(Wii.getButtonClick(ONE)) {
        Wii.IRinitialize(); // Run the initialisation sequence
        //Wii.statusRequest(); // This function isn't working right now
      }
      if(Wii.getButtonClick(TWO)) // Check status request. Returns if IR is intialized or not (Serial Monitor only)
        Wii.statusRequest(); // Isn't working proberly. It returns "extension disconnected", will fix soon
      if(Wii.getButtonClick(MINUS)) {
        printIR1 = !printIR1; // Will track 1 bright point
        printIR2 = false;
      }
      if(Wii.getButtonClick(PLUS)) { // Will track 2 brightest points 
        printIR2 = !printIR2;
        printIR1 = false;
      }
    }
    if(printIR1) {
      Serial.print(F("\r\n y1: "));
      Serial.print(Wii.getIRy1());  
      Serial.print(F("\t x1: "));
      Serial.print(Wii.getIRx1()); 
      Serial.print(F("\t s1:"));
      Serial.print(Wii.getIRs1());     
    }
    if(printIR2) {
      Serial.print(F("\r\n y1: "));
      Serial.print(Wii.getIRy1());
      Serial.print(F("\t y2: "));
      Serial.print(Wii.getIRy2());
      Serial.print(F("\t x1: "));
      Serial.print(Wii.getIRx1());
      Serial.print(F("\t x2: "));
      Serial.print(Wii.getIRx2());              
      Serial.print(F("\t s1:"));
      Serial.print(Wii.getIRs1());
      Serial.print(F("\t s2:"));
      Serial.print(Wii.getIRs2());   
    }
  }
}
