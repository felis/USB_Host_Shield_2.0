/*
 Example sketch for the PS3 USB library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <PS3USB.h>
USB Usb;
/* You can create the instance of the class in two ways */
PS3USB PS3(&Usb); // This will just create the instance
//PS3USB PS3(&Usb,0x00,0x15,0x83,0x3D,0x0A,0x57); // This will also store the bluetooth address - this can be obtained from the dongle when running the sketch

boolean printAngle;
uint8_t state = 0;

void setup() {
  Serial.begin(115200);
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }  
  Serial.print(F("\r\nPS3 USB Library Started"));
}
void loop() {
  Usb.Task();

  if(PS3.PS3Connected || PS3.PS3NavigationConnected) {
    if(PS3.getAnalogHat(LeftHatX) > 137 || PS3.getAnalogHat(LeftHatX) < 117 || PS3.getAnalogHat(LeftHatY) > 137 || PS3.getAnalogHat(LeftHatY) < 117 || PS3.getAnalogHat(RightHatX) > 137 || PS3.getAnalogHat(RightHatX) < 117 || PS3.getAnalogHat(RightHatY) > 137 || PS3.getAnalogHat(RightHatY) < 117) {
      Serial.print(F("\r\nLeftHatX: ")); 
      Serial.print(PS3.getAnalogHat(LeftHatX));
      Serial.print(F("\tLeftHatY: ")); 
      Serial.print(PS3.getAnalogHat(LeftHatY));
      if(!PS3.PS3NavigationConnected) {
        Serial.print(F("\tRightHatX: ")); 
        Serial.print(PS3.getAnalogHat(RightHatX));
        Serial.print(F("\tRightHatY: ")); 
        Serial.print(PS3.getAnalogHat(RightHatY));
      } 
    }
    // Analog button values can be read from almost all buttons
    if(PS3.getAnalogButton(L2) || PS3.getAnalogButton(R2)) {
      Serial.print(F("\r\nL2: ")); 
      Serial.print(PS3.getAnalogButton(L2));
      if(!PS3.PS3NavigationConnected) {
        Serial.print(F("\tR2: ")); 
        Serial.print(PS3.getAnalogButton(R2));
      } 
    }
    if(PS3.getButtonClick(PS))
      Serial.print(F("\r\nPS"));

    if(PS3.getButtonClick(TRIANGLE))
      Serial.print(F("\r\nTraingle"));
    if(PS3.getButtonClick(CIRCLE))
      Serial.print(F("\r\nCircle"));
    if(PS3.getButtonClick(CROSS))
      Serial.print(F("\r\nCross"));
    if(PS3.getButtonClick(SQUARE))
      Serial.print(F("\r\nSquare"));

    if(PS3.getButtonClick(UP)) {
      Serial.print(F("\r\nUp"));
      PS3.setAllOff();
      PS3.setLedOn(LED4);
    } 
    if(PS3.getButtonClick(RIGHT)) {
      Serial.print(F("\r\nRight"));
      PS3.setAllOff();
      PS3.setLedOn(LED1);          
    } 
    if(PS3.getButtonClick(DOWN)) {
      Serial.print(F("\r\nDown"));
      PS3.setAllOff();
      PS3.setLedOn(LED2);          
    } 
    if(PS3.getButtonClick(LEFT)) {          
      Serial.print(F("\r\nLeft"));
      PS3.setAllOff();         
      PS3.setLedOn(LED3);          
    } 

    if(PS3.getButtonClick(L1))
      Serial.print(F("\r\nL1"));  
    if(PS3.getButtonClick(L3))
      Serial.print(F("\r\nL3")); 
    if(PS3.getButtonClick(R1))
      Serial.print(F("\r\nR1"));  
    if(PS3.getButtonClick(R3))
      Serial.print(F("\r\nR3"));

    if(PS3.getButtonClick(SELECT)) {
      Serial.print(F("\r\nSelect - ")); 
      Serial.print(PS3.getStatusString());        
    } 
    if(PS3.getButtonClick(START)) {
      Serial.print(F("\r\nStart"));              
      printAngle = !printAngle;
    }                                               
  }
  if(printAngle) {
    Serial.print(F("\r\nPitch: "));               
    Serial.print(PS3.getAngle(Pitch));                  
    Serial.print(F("\tRoll: ")); 
    Serial.print(PS3.getAngle(Roll));      
  }
  else if(PS3.PS3MoveConnected) { // One can only set the color of the bulb, set the rumble, set and get the bluetooth address and calibrate the magnetometer via USB
    switch(state) {
    case 0:
      PS3.moveSetRumble(0);    
      PS3.moveSetBulb(Off);      
      state = 1;
      break;

    case 1:
      PS3.moveSetRumble(75);
      PS3.moveSetBulb(Red);     
      state = 2;
      break;

    case 2:
      PS3.moveSetRumble(125);
      PS3.moveSetBulb(Green);     
      state = 3;
      break;

    case 3:
      PS3.moveSetRumble(150);
      PS3.moveSetBulb(Blue);     
      state = 4;
      break;

    case 4:
      PS3.moveSetRumble(175);
      PS3.moveSetBulb(Yellow);    
      state = 5;
      break;

    case 5:
      PS3.moveSetRumble(200);
      PS3.moveSetBulb(Lightblue);    
      state = 6;
      break;

    case 6:
      PS3.moveSetRumble(225);
      PS3.moveSetBulb(Purble);   
      state = 7;
      break;

    case 7:
      PS3.moveSetRumble(250);
      PS3.moveSetBulb(White);   
      state = 0;
      break;
    }
    delay(1000);
  }
}
