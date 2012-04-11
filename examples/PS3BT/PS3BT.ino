/*
 Example sketch for the PS3 Bluetooth library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <PS3BT.h>
USB Usb;
PS3BT BT(&Usb);

boolean printTemperature;

void setup()
{
  Serial.begin(115200);

  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nPS3 Bluetooth Library Started"));
}
void loop()
{
  Usb.Task();

  if(BT.PS3BTConnected || BT.PS3NavigationBTConnected) {
    if(BT.getAnalogHat(LeftHatX) > 137 || BT.getAnalogHat(LeftHatX) < 117 || BT.getAnalogHat(LeftHatY) > 137 || BT.getAnalogHat(LeftHatY) < 117 || BT.getAnalogHat(RightHatX) > 137 || BT.getAnalogHat(RightHatX) < 117 || BT.getAnalogHat(RightHatY) > 137 || BT.getAnalogHat(RightHatY) < 117) {
      if(BT.getAnalogHat(LeftHatX) > 137 || BT.getAnalogHat(LeftHatX) < 117) {
        Serial.print(F("LeftHatX: ")); 
        Serial.print(BT.getAnalogHat(LeftHatX), DEC);
        Serial.print("\t");
      } if(BT.getAnalogHat(LeftHatY) > 137 || BT.getAnalogHat(LeftHatY) < 117) {    
        Serial.print(F("LeftHatY: ")); 
        Serial.print(BT.getAnalogHat(LeftHatY), DEC);
        Serial.print("\t");
      } if(BT.getAnalogHat(RightHatX) > 137 || BT.getAnalogHat(RightHatX) < 117) {
        Serial.print(F("RightHatX: ")); 
        Serial.print(BT.getAnalogHat(RightHatX), DEC);
        Serial.print("\t");      
      } if(BT.getAnalogHat(RightHatY) > 137 || BT.getAnalogHat(RightHatY) < 117) {
        Serial.print(F("RightHatY: ")); 
        Serial.print(BT.getAnalogHat(RightHatY), DEC);  
      }
      Serial.println("");
    }

    //Analog button values can be read from almost all buttons
    if(BT.getAnalogButton(L2_ANALOG) > 0 || BT.getAnalogButton(R2_ANALOG) > 0) {
      if(BT.getAnalogButton(L2_ANALOG) > 0) {
        Serial.print(F("L2: ")); 
        Serial.print(BT.getAnalogButton(L2_ANALOG), DEC);
        Serial.print("\t"); 
      } if(BT.getAnalogButton(R2_ANALOG) > 0) {
        Serial.print(F("R2: ")); 
        Serial.print(BT.getAnalogButton(R2_ANALOG), DEC); 
      }
      Serial.println("");
    }

    if(BT.buttonPressed)     
    {
      Serial.print(F("PS3 Controller"));

      if(BT.getButton(PS)) {
        Serial.print(F(" - PS"));
        BT.disconnect();
      } else {
        if(BT.getButton(TRIANGLE))
          Serial.print(F(" - Traingle"));
        if(BT.getButton(CIRCLE))
          Serial.print(F(" - Circle"));
        if(BT.getButton(CROSS))
          Serial.print(F(" - Cross"));
        if(BT.getButton(SQUARE))
          Serial.print(F(" - Square"));

        if(BT.getButton(UP)) {
          Serial.print(F(" - Up"));
          BT.setAllOff();
          BT.setLedOn(LED4);
        } if(BT.getButton(RIGHT)) {
          Serial.print(F(" - Right"));
          BT.setAllOff();
          BT.setLedOn(LED1);          
        } if(BT.getButton(DOWN)) {
          Serial.print(F(" - Down"));
          BT.setAllOff();
          BT.setLedOn(LED2);          
        } if(BT.getButton(LEFT)) {          
          Serial.print(F(" - Left"));
          BT.setAllOff();         
          BT.setLedOn(LED3);          
        } 

        if(BT.getButton(L1))
          Serial.print(F(" - L1"));  
        //if(BT.getButton(L2))
        //Serial.print(F(" - L2"));
        if(BT.getButton(L3))
          Serial.print(F(" - L3")); 
        if(BT.getButton(R1))
          Serial.print(F(" - R1"));  
        //if(BT.getButton(R2))
        //Serial.print(F(" - R2"));              
        if(BT.getButton(R3))
          Serial.print(F(" - R3"));

        if(BT.getButton(SELECT)) {
          Serial.print(F(" - Select - ")); 
          Serial.print(BT.getStatusString());        
        } if(BT.getButton(START))
          Serial.print(F(" - Start"));                                

        Serial.println("");          
      }             
    }     
  }
  else if(BT.PS3MoveBTConnected)
  {
    if(BT.getAnalogButton(T_MOVE_ANALOG) > 0) {
      Serial.print(F("T: ")); 
      Serial.println(BT.getAnalogButton(T_MOVE_ANALOG), DEC); 
    } if(BT.buttonPressed) {
      Serial.print(F("PS3 Move Controller"));

      if(BT.getButton(PS_MOVE)) {
        Serial.print(F(" - PS"));
        BT.disconnect();
      } else {
        if(BT.getButton(SELECT_MOVE)) {
          Serial.print(F(" - Select"));
          printTemperature = false;
        } if(BT.getButton(START_MOVE)) {
          Serial.print(F(" - Start"));
          printTemperature = true;
        } if(BT.getButton(TRIANGLE_MOVE)) {            
          Serial.print(F(" - Triangle"));
          BT.moveSetBulb(Red);
        } if(BT.getButton(CIRCLE_MOVE)) {
          Serial.print(F(" - Circle"));
          BT.moveSetBulb(Green);
        } if(BT.getButton(SQUARE_MOVE)) {
          Serial.print(F(" - Square"));
          BT.moveSetBulb(Blue);
        } if(BT.getButton(CROSS_MOVE)) {
          Serial.print(F(" - Cross"));
          BT.moveSetBulb(Yellow);
        } if(BT.getButton(MOVE_MOVE)) {     
          BT.moveSetBulb(Off);                        
          Serial.print(F(" - Move"));
          Serial.print(F(" - ")); 
          Serial.print(BT.getStatusString());
        }
        //if(BT.getButton(T))
        //Serial.print(F(" - T"));

        Serial.println("");
      }
    }
    if(printTemperature) {
      String templow;
      String temphigh;
      String input = String(BT.getSensor(tempMove), DEC);

      if (input.length() > 3) {
        temphigh = input.substring(0, 2);
        templow = input.substring(2);
      } else {
        temphigh = input.substring(0, 1);
        templow = input.substring(1);
      }
      Serial.print(F("Temperature: ")); 
      Serial.print(temphigh);       
      Serial.print(F(".")); 
      Serial.println(templow);
    }
  }
}
