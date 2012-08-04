/*
 Example sketch for the PS3 Bluetooth library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include "PS3BT.h"
#include "SPP.h"
USB Usb;
BTD Btd(&Usb);
/* You can create the instances of the class in two ways */
SPP SerialBT(&Btd); // This will set the name to the defaults: "Arduino" and the pin to "1234"
//SPP SerialBTBT(&Btd, "Lauszus' Arduino","0000"); // You can also set the name and pin like so
PS3BT PS3(&Btd); // This will just create the instance
//PS3BT PS3(&Btd,0x00,0x15,0x83,0x3D,0x0A,0x57); // This will also store the bluetooth address - this can be obtained from the dongle when running the sketch

boolean printTemperature;
boolean printAngle;
boolean firstMessage = true;

void setup() {
  Serial.begin(115200); // This wil lprint the debugging from the libraries
  if (Usb.Init() == -1) {
    SerialBT.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nPS3 Bluetooth Library Started"));
}
void loop() {
  Usb.Task();
 if(SerialBT.connected) {
    if(firstMessage) {
      firstMessage = false;
      SerialBT.println(F("Hello from Arduino")); // Send welcome message
    }
    if(Serial.available())
      SerialBT.print(Serial.read());
    if(SerialBT.available())
      Serial.write(SerialBT.read());
  } 
  else 
    firstMessage = true;
    
  if((PS3.PS3Connected || PS3.PS3NavigationConnected) && SerialBT.connected) {
    if(PS3.getAnalogHat(LeftHatX) > 137 || PS3.getAnalogHat(LeftHatX) < 117 || PS3.getAnalogHat(LeftHatY) > 137 || PS3.getAnalogHat(LeftHatY) < 117 || PS3.getAnalogHat(RightHatX) > 137 || PS3.getAnalogHat(RightHatX) < 117 || PS3.getAnalogHat(RightHatY) > 137 || PS3.getAnalogHat(RightHatY) < 117) {
      if(PS3.getAnalogHat(LeftHatX) > 137 || PS3.getAnalogHat(LeftHatX) < 117) {
        SerialBT.print(F("LeftHatX: ")); 
        SerialBT.print(PS3.getAnalogHat(LeftHatX));
        SerialBT.print("\t");
      } if(PS3.getAnalogHat(LeftHatY) > 137 || PS3.getAnalogHat(LeftHatY) < 117) {    
        SerialBT.print(F("LeftHatY: ")); 
        SerialBT.print(PS3.getAnalogHat(LeftHatY));
        SerialBT.print("\t");
      } if(PS3.getAnalogHat(RightHatX) > 137 || PS3.getAnalogHat(RightHatX) < 117) {
        SerialBT.print(F("RightHatX: ")); 
        SerialBT.print(PS3.getAnalogHat(RightHatX));
        SerialBT.print("\t");      
      } if(PS3.getAnalogHat(RightHatY) > 137 || PS3.getAnalogHat(RightHatY) < 117) {
        SerialBT.print(F("RightHatY: ")); 
        SerialBT.print(PS3.getAnalogHat(RightHatY));  
      }
      SerialBT.println("");
    }

    //Analog button values can be read from almost all buttons
    if(PS3.getAnalogButton(L2_ANALOG) > 0 || PS3.getAnalogButton(R2_ANALOG) > 0) {
      if(PS3.getAnalogButton(L2_ANALOG) > 0) {
        SerialBT.print(F("L2: ")); 
        SerialBT.print(PS3.getAnalogButton(L2_ANALOG));
        SerialBT.print("\t"); 
      } if(PS3.getAnalogButton(R2_ANALOG) > 0) {
        SerialBT.print(F("R2: ")); 
        SerialBT.print(PS3.getAnalogButton(R2_ANALOG)); 
      }
      SerialBT.println("");
    }

    if(PS3.buttonPressed)     
    {
      SerialBT.print(F("PS3 Controller"));

      if(PS3.getButton(PS)) {
        SerialBT.print(F(" - PS"));
        PS3.disconnect();
      } else {
        if(PS3.getButton(TRIANGLE))
          SerialBT.print(F(" - Traingle"));
        if(PS3.getButton(CIRCLE))
          SerialBT.print(F(" - Circle"));
        if(PS3.getButton(CROSS))
          SerialBT.print(F(" - Cross"));
        if(PS3.getButton(SQUARE))
          SerialBT.print(F(" - Square"));

        if(PS3.getButton(UP)) {
          SerialBT.print(F(" - Up"));          
          if(PS3.PS3Connected) {
            PS3.setAllOff();
            PS3.setLedOn(LED4);
          }
        } if(PS3.getButton(RIGHT)) {
          SerialBT.print(F(" - Right"));
          if(PS3.PS3Connected) {
            PS3.setAllOff();
            PS3.setLedOn(LED1); 
          }         
        } if(PS3.getButton(DOWN)) {
          SerialBT.print(F(" - Down"));
          if(PS3.PS3Connected) {
            PS3.setAllOff();
            PS3.setLedOn(LED2);          
          }
        } if(PS3.getButton(LEFT)) {          
          SerialBT.print(F(" - Left"));          
          if(PS3.PS3Connected) {
            PS3.setAllOff();         
            PS3.setLedOn(LED3);            
          }         
        } 

        if(PS3.getButton(L1))
          SerialBT.print(F(" - L1"));  
        //if(PS3.getButton(L2))
        //SerialBT.print(F(" - L2"));
        if(PS3.getButton(L3))
          SerialBT.print(F(" - L3")); 
        if(PS3.getButton(R1))
          SerialBT.print(F(" - R1"));  
        //if(PS3.getButton(R2))
        //SerialBT.print(F(" - R2"));              
        if(PS3.getButton(R3))
          SerialBT.print(F(" - R3"));

        if(PS3.getButton(SELECT)) {
          SerialBT.print(F(" - Select - ")); 
//          SerialBT.print(PS3.getStatusString());        
        } if(PS3.getButton(START)) {
          SerialBT.print(F(" - Start"));              
          printAngle = !printAngle;
          while(PS3.getButton(START))
            Usb.Task();
        }                    
        SerialBT.println("");          
      }             
    }
    if(printAngle) {
      SerialBT.print(F("Pitch: "));               
      SerialBT.print(PS3.getAngle(Pitch));                  
      SerialBT.print(F("\tRoll: ")); 
      SerialBT.println(PS3.getAngle(Roll));
    }
  }
  else if(PS3.PS3MoveConnected && SerialBT.connected)
  {
    if(PS3.getAnalogButton(T_ANALOG) > 0) {
      SerialBT.print(F("T: ")); 
      SerialBT.println(PS3.getAnalogButton(T_ANALOG)); 
    } if(PS3.buttonPressed) {
      SerialBT.print(F("PS3 Move Controller"));

      if(PS3.getButton(PS)) {
        SerialBT.print(F(" - PS"));
        PS3.disconnect();
      } else {
        if(PS3.getButton(SELECT)) {
          SerialBT.print(F(" - Select"));
          printTemperature = !printTemperature;
          while(PS3.getButton(SELECT))
            Usb.Task();          
        } if(PS3.getButton(START)) {
          SerialBT.print(F(" - Start"));
          printAngle = !printAngle;
          while(PS3.getButton(START))
            Usb.Task();                              
        } if(PS3.getButton(TRIANGLE)) {            
          SerialBT.print(F(" - Triangle"));
          PS3.moveSetBulb(Red);
        } if(PS3.getButton(CIRCLE)) {
          SerialBT.print(F(" - Circle"));
          PS3.moveSetBulb(Green);
        } if(PS3.getButton(SQUARE)) {
          SerialBT.print(F(" - Square"));
          PS3.moveSetBulb(Blue);
        } if(PS3.getButton(CROSS)) {
          SerialBT.print(F(" - Cross"));
          PS3.moveSetBulb(Yellow);
        } if(PS3.getButton(MOVE)) {     
          PS3.moveSetBulb(Off);                        
          SerialBT.print(F(" - Move"));
          SerialBT.print(F(" - ")); 
//          SerialBT.print(PS3.getStatusString());
        }
        //if(PS3.getButton(T))
        //SerialBT.print(F(" - T"));

        SerialBT.println("");
      }
    }
    if(printAngle) {
      SerialBT.print(F("Pitch: "));               
      SerialBT.print(PS3.getAngle(Pitch));                  
      SerialBT.print(F("\tRoll: ")); 
      SerialBT.println(PS3.getAngle(Roll));
    }
    else if(printTemperature) {
      SerialBT.print(F("Temperature: "));
//      SerialBT.println(PS3.getTemperature());
    }
  }
  delay(5);
}
