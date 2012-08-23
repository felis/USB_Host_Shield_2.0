/*
 Example sketch for the Wiimote Bluetooth library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <Wii.h>
USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so
/* You can create the instance of the class in two ways */
WII Wii(&Btd); // This will start inquiry which will connect to any Wiimote
//WII Wii(&Btd,0x00,0x26,0x59,0x48,0xFF,0xFB); // This will connect to the Wiimote with that specific Bluetooth Address

bool printAngle;

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
      Wii.disconnect(); // If you disconnect you have to reset the Arduino to establish the connection again
    } 
    else {
      if(Wii.getButtonClick(LEFT)) {
        Wii.setAllOff();
        Wii.setLedOn(LED1);
        Serial.print(F("\r\nLeft"));
      }
      if(Wii.getButtonClick(RIGHT)) {
        Wii.setAllOff();
        Wii.setLedOn(LED3);
        Serial.print(F("\r\nRight"));
      }
      if(Wii.getButtonClick(DOWN)) {
        Wii.setAllOff();
        Wii.setLedOn(LED4);
        Serial.print(F("\r\nDown"));
      }      
      if(Wii.getButtonClick(UP)) {
        Wii.setAllOff();
        Wii.setLedOn(LED2);          
        Serial.print(F("\r\nUp"));
      }

      if(Wii.getButtonClick(PLUS))
        Serial.print(F("\r\nPlus"));
      if(Wii.getButtonClick(MINUS))
        Serial.print(F("\r\nMinus"));

      if(Wii.getButtonClick(ONE))
        Serial.print(F("\r\nOne"));
      if(Wii.getButtonClick(TWO))
        Serial.print(F("\r\nTwo"));

      if(Wii.getButtonClick(A)) {
        printAngle = !printAngle;
        Serial.print(F("\r\nA"));
      }      
      if(Wii.getButtonClick(B)) {
        Wii.setRumbleToggle();
        Serial.print(F("\r\nB"));
      }
    }
    if(printAngle) {
      Serial.print(F("\r\nPitch: "));
      Serial.print(Wii.getPitch());
      Serial.print(F("\tRoll: "));
      Serial.print(Wii.getRoll());
      if(Wii.motionPlusConnected) {
        Serial.print(F("\tYaw: "));
        Serial.print(Wii.getYaw());
      }      
      if(Wii.nunchuckConnected) {
        Serial.print(F("\tNunchuck Pitch: "));
        Serial.print(Wii.nunchuckPitch);
        Serial.print(F("\tNunchuck Roll: "));
        Serial.print(Wii.nunchuckRoll);
      }
    }
  }
  if(Wii.nunchuckConnected) {
    if(Wii.getButtonClick(Z))
      Serial.print(F("\r\nZ"));
    if(Wii.getButtonClick(C))
      Serial.print(F("\r\nC"));
    if(Wii.getAnalogHat(HatX) > 137 ||  Wii.getAnalogHat(HatX) < 117 || Wii.getAnalogHat(HatY) > 137 || Wii.getAnalogHat(HatY) < 117) {
      Serial.print(F("\r\nHatX: "));
      Serial.print(Wii.getAnalogHat(HatX));
      Serial.print(F("\tHatY: "));
      Serial.print(Wii.getAnalogHat(HatY));
    }
  }
}
