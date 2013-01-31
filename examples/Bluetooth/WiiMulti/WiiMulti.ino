/*
 Example sketch for the Wiimote Bluetooth library - developed by Kristian Lauszus
 This example show how one can use multiple controllers with the library
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <Wii.h>
USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so
//WII Wii(&Btd,PAIR); // You will have to pair each controller with the dongle before you can define the instances like below
WII Wii_1(&Btd);
WII Wii_2(&Btd);
//WII Wii_3(&Btd); // You can create as many instances as you like, but it will take up a lot of RAM!!

WII* Wii[2]; // We will use this pointer to store the two instance, you can easily make it larger if you like
const uint8_t length = sizeof(Wii)/sizeof(Wii[0]); // Get the lenght of the array
bool printAngle[length];

void setup() {
  Wii[0] = &Wii_1;
  Wii[1] = &Wii_2;
  //Wii[2] = &Wii_3; // You only need to uncomment this if you wanted to use another controller

  Serial.begin(115200);
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nWiimote Bluetooth Library Started"));
}
void loop() {
  Usb.Task();

  for(uint8_t i=0;i<length;i++) {
    if(!Wii[i]) continue; // Skip if it hasn't been defined
    if(Wii[i]->wiimoteConnected) {
      if(Wii[i]->getButtonClick(HOME)) { // You can use getButtonPress to see if the button is held down
        Serial.print(F("\r\nHOME"));
        Wii[i]->disconnect();
        delay(1000); // This delay is needed for some Wiimotes, so it doesn't try to reconnect right away
      } 
      else {
        if(Wii[i]->getButtonClick(LEFT)) {
          Wii[i]->setAllOff();
          Wii[i]->setLedOn(LED1);
          Serial.print(F("\r\nLeft"));
        }
        if(Wii[i]->getButtonClick(RIGHT)) {
          Wii[i]->setAllOff();
          Wii[i]->setLedOn(LED3);
          Serial.print(F("\r\nRight"));
        }
        if(Wii[i]->getButtonClick(DOWN)) {
          Wii[i]->setAllOff();
          Wii[i]->setLedOn(LED4);
          Serial.print(F("\r\nDown"));
        }      
        if(Wii[i]->getButtonClick(UP)) {
          Wii[i]->setAllOff();
          Wii[i]->setLedOn(LED2);          
          Serial.print(F("\r\nUp"));
        }

        if(Wii[i]->getButtonClick(PLUS))
          Serial.print(F("\r\nPlus"));
        if(Wii[i]->getButtonClick(MINUS))
          Serial.print(F("\r\nMinus"));

        if(Wii[i]->getButtonClick(ONE))
          Serial.print(F("\r\nOne"));
        if(Wii[i]->getButtonClick(TWO))
          Serial.print(F("\r\nTwo"));

        if(Wii[i]->getButtonClick(A)) {
          printAngle[i] = !printAngle[i];
          Serial.print(F("\r\nA"));
        }      
        if(Wii[i]->getButtonClick(B)) {
          Wii[i]->setRumbleToggle();
          Serial.print(F("\r\nB"));
        }
      }
      if(printAngle[i]) {
        Serial.print(F("\r\nPitch: "));
        Serial.print(Wii[i]->getPitch());
        Serial.print(F("\tRoll: "));
        Serial.print(Wii[i]->getRoll());
        if(Wii[i]->motionPlusConnected) {
          Serial.print(F("\tYaw: "));
          Serial.print(Wii[i]->getYaw());
        }      
        if(Wii[i]->nunchuckConnected) {
          Serial.print(F("\tNunchuck Pitch: "));
          Serial.print(Wii[i]->nunchuckPitch);
          Serial.print(F("\tNunchuck Roll: "));
          Serial.print(Wii[i]->nunchuckRoll);
        }
      }
    }
    if(Wii[i]->nunchuckConnected) {
      if(Wii[i]->getButtonClick(Z))
        Serial.print(F("\r\nZ"));
      if(Wii[i]->getButtonClick(C))
        Serial.print(F("\r\nC"));
      if(Wii[i]->getAnalogHat(HatX) > 137 ||  Wii[i]->getAnalogHat(HatX) < 117 || Wii[i]->getAnalogHat(HatY) > 137 || Wii[i]->getAnalogHat(HatY) < 117) {
        Serial.print(F("\r\nHatX: "));
        Serial.print(Wii[i]->getAnalogHat(HatX));
        Serial.print(F("\tHatY: "));
        Serial.print(Wii[i]->getAnalogHat(HatY));
      }
    }
  }
}
