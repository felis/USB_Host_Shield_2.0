/*
 Example sketch for the Xbox Wireless Reciver library - developed by Kristian Lauszus
 It supports up to four controllers wirelessly
 For more information see the blog post: http://blog.tkjelectronics.dk/2012/12/xbox-360-receiver-added-to-the-usb-host-library/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <XBOXRECV.h>
USB Usb;
XBOXRECV Xbox(&Usb);

void setup() {
  Serial.begin(115200);
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nXbox Wireless Receiver Library Started"));
}
void loop() {
  Usb.Task();
  if(Xbox.XboxReceiverConnected) {
    for(uint8_t i=0;i<4;i++) {
      if(Xbox.Xbox360Connected[i]) {
        if(Xbox.getButtonPress(i,L2) || Xbox.getButtonPress(i,R2)) {
          Serial.print("L2: ");
          Serial.print(Xbox.getButtonPress(i,L2));
          Serial.print("\tR2: ");
          Serial.println(Xbox.getButtonPress(i,R2));
          Xbox.setRumbleOn(i,Xbox.getButtonPress(i,L2),Xbox.getButtonPress(i,R2));
        }
        if(Xbox.getAnalogHat(i,LeftHatX) > 7500 || Xbox.getAnalogHat(i,LeftHatX) < -7500 || Xbox.getAnalogHat(i,LeftHatY) > 7500 || Xbox.getAnalogHat(i,LeftHatY) < -7500 || Xbox.getAnalogHat(i,RightHatX) > 7500 || Xbox.getAnalogHat(i,RightHatX) < -7500 || Xbox.getAnalogHat(i,RightHatY) > 7500 || Xbox.getAnalogHat(i,RightHatY) < -7500) {
          if(Xbox.getAnalogHat(i,LeftHatX) > 7500 || Xbox.getAnalogHat(i,LeftHatX) < -7500) {
            Serial.print(F("LeftHatX: ")); 
            Serial.print(Xbox.getAnalogHat(i,LeftHatX));
            Serial.print("\t");
          } 
          if(Xbox.getAnalogHat(i,LeftHatY) > 7500 || Xbox.getAnalogHat(i,LeftHatY) < -7500) {
            Serial.print(F("LeftHatY: ")); 
            Serial.print(Xbox.getAnalogHat(i,LeftHatY));
            Serial.print("\t");
          } 
          if(Xbox.getAnalogHat(i,RightHatX) > 7500 || Xbox.getAnalogHat(i,RightHatX) < -7500) {
            Serial.print(F("RightHatX: ")); 
            Serial.print(Xbox.getAnalogHat(i,RightHatX));
            Serial.print("\t");      
          } 
          if(Xbox.getAnalogHat(i,RightHatY) > 7500 || Xbox.getAnalogHat(i,RightHatY) < -7500) {
            Serial.print(F("RightHatY: ")); 
            Serial.print(Xbox.getAnalogHat(i,RightHatY));  
          }
          Serial.println();
        }

        if(Xbox.getButtonClick(i,UP)) {
          Xbox.setLedOn(i,LED1);
          Serial.println(F("Up"));
        }      
        if(Xbox.getButtonClick(i,DOWN)) {
          Xbox.setLedOn(i,LED4);
          Serial.println(F("Down"));
        }
        if(Xbox.getButtonClick(i,LEFT)) {
          Xbox.setLedOn(i,LED3);
          Serial.println(F("Left"));
        }
        if(Xbox.getButtonClick(i,RIGHT)) {
          Xbox.setLedOn(i,LED2);
          Serial.println(F("Right"));
        }        

        if(Xbox.getButtonClick(i,START)) {
          Xbox.setLedMode(i,ALTERNATING);
          Serial.println(F("Start"));
        }
        if(Xbox.getButtonClick(i,BACK)) {
          Xbox.setLedBlink(i,ALL);
          Serial.println(F("Back"));
        }
        if(Xbox.getButtonClick(i,L3))
          Serial.println(F("L3"));
        if(Xbox.getButtonClick(i,R3))
          Serial.println(F("R3"));

        if(Xbox.getButtonClick(i,L1))
          Serial.println(F("L1"));
        if(Xbox.getButtonClick(i,R1))
          Serial.println(F("R1"));
        if(Xbox.getButtonClick(i,XBOX)) {
          Xbox.setLedMode(i,ROTATING);
          Serial.print(F("Xbox (Battery: "));
          Serial.print(Xbox.getBatteryLevel(i));
          Serial.println(F("%)"));
        }
        if(Xbox.getButtonClick(i,SYNC))
          Serial.println(F("Sync"));

        if(Xbox.getButtonClick(i,A))
          Serial.println(F("A"));
        if(Xbox.getButtonClick(i,B))
          Serial.println(F("B"));
        if(Xbox.getButtonClick(i,X))
          Serial.println(F("X"));
        if(Xbox.getButtonClick(i,Y))
          Serial.println(F("Y"));
      }
    }
  }  
  delay(1);
}
