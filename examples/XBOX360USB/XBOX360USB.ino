/*
 Example sketch for the Xbox 360 USB library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <XBOXUSB.h>
USB Usb;
XBOXUSB Xbox(&Usb);

void setup() {
  Serial.begin(115200);

  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }  
  Serial.print(F("\r\nXBOX USB Library Started"));
}
void loop() {
  Usb.Task();
  if(Xbox.Xbox360Connected) {
    Xbox.setRumbleOn(Xbox.getButton(L2),Xbox.getButton(R2));
    if(Xbox.getAnalogHat(LeftHatX) > 7500 || Xbox.getAnalogHat(LeftHatX) < -7500 || Xbox.getAnalogHat(LeftHatY) > 7500 || Xbox.getAnalogHat(LeftHatY) < -7500 || Xbox.getAnalogHat(RightHatX) > 7500 || Xbox.getAnalogHat(RightHatX) < -7500 || Xbox.getAnalogHat(RightHatY) > 7500 || Xbox.getAnalogHat(RightHatY) < -7500) {
      if(Xbox.getAnalogHat(LeftHatX) > 7500 || Xbox.getAnalogHat(LeftHatX) < -7500) {
        Serial.print(F("LeftHatX: ")); 
        Serial.print(Xbox.getAnalogHat(LeftHatX));
        Serial.print("\t");
      } 
      if(Xbox.getAnalogHat(LeftHatY) > 7500 || Xbox.getAnalogHat(LeftHatY) < -7500) {
        Serial.print(F("LeftHatY: ")); 
        Serial.print(Xbox.getAnalogHat(LeftHatY));
        Serial.print("\t");
      } 
      if(Xbox.getAnalogHat(RightHatX) > 7500 || Xbox.getAnalogHat(RightHatX) < -7500) {
        Serial.print(F("RightHatX: ")); 
        Serial.print(Xbox.getAnalogHat(RightHatX));
        Serial.print("\t");      
      } 
      if(Xbox.getAnalogHat(RightHatY) > 7500 || Xbox.getAnalogHat(RightHatY) < -7500) {
        Serial.print(F("RightHatY: ")); 
        Serial.print(Xbox.getAnalogHat(RightHatY));  
      }
      Serial.println("");
    }

    if(Xbox.buttonPressed) {
      Serial.print(F("Xbox 360 Controller"));
      if(Xbox.getButton(UP)) {
        Xbox.setLedOn(LED1);
        Serial.print(F(" - UP"));
      }      
      if(Xbox.getButton(DOWN)) {
        Xbox.setLedOn(LED4);
        Serial.print(F(" - DOWN"));
      }
      if(Xbox.getButton(LEFT)) {
        Xbox.setLedOn(LED3);
        Serial.print(F(" - LEFT"));
      }
      if(Xbox.getButton(RIGHT)) {
        Xbox.setLedOn(LED2);
        Serial.print(F(" - RIGHT"));
      }

      if(Xbox.getButton(START)) {
        Xbox.setLedMode(ALTERNATING);
        Serial.print(F(" - START"));
      }
      if(Xbox.getButton(BACK)) {
        Xbox.setLedBlink(ALL);
        Serial.print(F(" - BACK"));
      }
      if(Xbox.getButton(L3))
        Serial.print(F(" - L3"));
      if(Xbox.getButton(R3))
        Serial.print(F(" - R3"));

      if(Xbox.getButton(L1))
        Serial.print(F(" - L1"));
      if(Xbox.getButton(R1))
        Serial.print(F(" - R1"));
      if(Xbox.getButton(XBOX)) {
        Xbox.setLedMode(ROTATING);
        Serial.print(F(" - XBOX"));        
      }

      if(Xbox.getButton(A))
        Serial.print(F(" - A"));
      if(Xbox.getButton(B))
        Serial.print(F(" - B"));
      if(Xbox.getButton(X))
        Serial.print(F(" - X"));
      if(Xbox.getButton(Y))
        Serial.print(F(" - Y"));

      if(Xbox.getButton(L2)) {
        Serial.print(F(" - L2:"));
        Serial.print(Xbox.getButton(L2));
      }
      if(Xbox.getButton(R2)) {
        Serial.print(F(" - R2:"));
        Serial.print(Xbox.getButton(R2));
      }
      Serial.println();        
    } 
  }
  delay(1);
}
