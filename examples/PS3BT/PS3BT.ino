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
  
  if (Usb.Init() == -1) 
  {
    Notify(PSTR("\r\nOSC did not start"));
    while(1); //halt
  }
  Notify(PSTR("\r\nPS3 Bluetooth Library Started"));
}
void loop()
{
  Usb.Task();

  if(BT.PS3BTConnected || BT.PS3NavigationBTConnected)
  {
    if(BT.getAnalogHat(LeftHatX) > 137 || BT.getAnalogHat(LeftHatX) < 117)
    {
      Notify(PSTR("\r\nLeftHatX: ")); 
      Serial.print(BT.getAnalogHat(LeftHatX), DEC);
    }
    if(BT.getAnalogHat(LeftHatY) > 137 || BT.getAnalogHat(LeftHatY) < 117)  
    {    
      Notify(PSTR("\r\nLeftHatY: ")); 
      Serial.print(BT.getAnalogHat(LeftHatY), DEC);
    }
    if(BT.getAnalogHat(RightHatX) > 137 || BT.getAnalogHat(RightHatX) < 117)
    {
      Notify(PSTR("\r\nRightHatX: ")); 
      Serial.print(BT.getAnalogHat(RightHatX), DEC);
    }
    if(BT.getAnalogHat(RightHatY) > 137 || BT.getAnalogHat(RightHatY) < 117)
    {
      Notify(PSTR("\r\nRightHatY: ")); 
      Serial.print(BT.getAnalogHat(RightHatY), DEC);
    }

    //Analog button values can be read from almost all buttons
    if(BT.getAnalogButton(L2_ANALOG) > 0)
    {
      Notify(PSTR("\r\nL2: ")); 
      Serial.print(BT.getAnalogButton(L2_ANALOG), DEC); 
    }
    if(BT.getAnalogButton(R2_ANALOG) > 0)
    {
      Notify(PSTR("\r\nR2: ")); 
      Serial.print(BT.getAnalogButton(R2_ANALOG), DEC); 
    }

    if(BT.ButtonPressed)     
    {
      Notify(PSTR("\r\nPS3 Controller"));
      
      if(BT.getButton(PS))
      {
        Notify(PSTR(" - PS"));
        BT.disconnect();
      }
      else
      {
        if(BT.getButton(TRIANGLE))
          Notify(PSTR(" - Traingle"));
        if(BT.getButton(CIRCLE))
          Notify(PSTR(" - Circle"));
        if(BT.getButton(CROSS))
          Notify(PSTR(" - Cross"));
        if(BT.getButton(SQUARE))
          Notify(PSTR(" - Square"));

        if(BT.getButton(UP))
        {
          Notify(PSTR(" - Up"));
          BT.setAllOff();
          BT.setLedOn(LED4);
        }
        if(BT.getButton(RIGHT))
        {
          Notify(PSTR(" - Right"));
          BT.setAllOff();
          BT.setLedOn(LED1);          
        }
        if(BT.getButton(DOWN))
        {
          Notify(PSTR(" - Down"));
          BT.setAllOff();
          BT.setLedOn(LED2);          
        }
        if(BT.getButton(LEFT))
        {          
          Notify(PSTR(" - Left"));
          BT.setAllOff();         
          BT.setLedOn(LED3);          
        } 

        if(BT.getButton(L1))
          Notify(PSTR(" - L1"));  
        //if(BT.getButton(L2))
          //Notify(PSTR(" - L2"));            
        if(BT.getButton(L3))
          Notify(PSTR(" - L3")); 
        if(BT.getButton(R1))
          Notify(PSTR(" - R1"));  
        //if(BT.getButton(R2))
          //Notify(PSTR(" - R2"));              
        if(BT.getButton(R3))
          Notify(PSTR(" - R3"));

        if(BT.getButton(SELECT))
        {
          Notify(PSTR(" - Select - ")); 
          Serial.print(BT.getStatusString());        
        }
        if(BT.getButton(START))
          Notify(PSTR(" - Start"));                                
      }                  
    }     
  }
  else if(BT.PS3MoveBTConnected)
  {
    if(BT.getAnalogButton(T_MOVE_ANALOG) > 0)
    {
      Notify(PSTR("\r\nT: ")); 
      Serial.print(BT.getAnalogButton(T_MOVE_ANALOG), DEC); 
    }
    if(BT.ButtonPressed)     
    {
      Notify(PSTR("\r\nPS3 Move Controller"));
      
      if(BT.getButton(PS_MOVE))
      {
        Notify(PSTR(" - PS"));
        BT.disconnect();
      }
      else
      {
        if(BT.getButton(SELECT_MOVE))
        {
          Notify(PSTR(" - Select"));
          printTemperature = false;
        }
        if(BT.getButton(START_MOVE))
        {
          Notify(PSTR(" - Start"));
          printTemperature = true;
        }
        if(BT.getButton(TRIANGLE_MOVE))
        {            
          Notify(PSTR(" - Triangle"));
          BT.moveSetBulb(Red);
        }
        if(BT.getButton(CIRCLE_MOVE))
        {
          Notify(PSTR(" - Circle"));
          BT.moveSetBulb(Green);
        }
        if(BT.getButton(SQUARE_MOVE))
        {
          Notify(PSTR(" - Square"));
          BT.moveSetBulb(Blue);
        }
        if(BT.getButton(CROSS_MOVE))
        {
          Notify(PSTR(" - Cross"));
          BT.moveSetBulb(Yellow);
        }
        if(BT.getButton(MOVE_MOVE))
        {     
          BT.moveSetBulb(Off);                        
          Notify(PSTR(" - Move"));
          Notify(PSTR(" - ")); 
          Serial.print(BT.getStatusString());
        }
        //if(BT.getButton(T_MOVE))
          //Notify(PSTR(" - T"));
      }
    }
    if(printTemperature)
    {
      String templow;
      String temphigh;
      String input = String(BT.getSensor(tempMove), DEC);

      if (input.length() > 3)
      {
        temphigh = input.substring(0, 2);
        templow = input.substring(2);
      }
      else
      {
        temphigh = input.substring(0, 1);
        templow = input.substring(1);
      }
      Notify(PSTR("\r\nTemperature: ")); 
      Serial.print(temphigh);       
      Notify(PSTR(".")); 
      Serial.print(templow);
    }
  }
}
