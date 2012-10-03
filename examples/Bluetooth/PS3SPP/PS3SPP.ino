/*
 Example sketch for the Bluetooth library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

/* 
 Note: 
 You will need a Arduino Mega 1280/2560 to run this sketch,
 As a normal Arduino (Uno, Duemilanove etc.) doesn't have enough SRAM and FLASH
 */

#include <PS3BT.h>
#include <SPP.h>
USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so

/* You can create the instances of the bluetooth services in two ways */
SPP SerialBT(&Btd); // This will set the name to the defaults: "Arduino" and the pin to "1234"
//SPP SerialBTBT(&Btd,"Lauszus's Arduino","0000"); // You can also set the name and pin like so
PS3BT PS3(&Btd); // This will just create the instance
//PS3BT PS3(&Btd,0x00,0x15,0x83,0x3D,0x0A,0x57); // This will also store the bluetooth address - this can be obtained from the dongle when running the sketch

boolean firstMessage = true;
String output; // We will store the data in these string so we doesn't overflow the dongle

void setup() {
  Serial.begin(115200); // This wil lprint the debugging from the libraries
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nBluetooth Library Started"));
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

  if(PS3.PS3Connected || PS3.PS3NavigationConnected) {
    output = ""; // Reset output string
    if(PS3.getAnalogHat(LeftHatX) > 137 || PS3.getAnalogHat(LeftHatX) < 117 || PS3.getAnalogHat(LeftHatY) > 137 || PS3.getAnalogHat(LeftHatY) < 117 || PS3.getAnalogHat(RightHatX) > 137 || PS3.getAnalogHat(RightHatX) < 117 || PS3.getAnalogHat(RightHatY) > 137 || PS3.getAnalogHat(RightHatY) < 117) {
      output += "LeftHatX: ";
      output += PS3.getAnalogHat(LeftHatX);
      output += "\tLeftHatY: ";
      output += PS3.getAnalogHat(LeftHatY);
      output += "\tRightHatX: ";
      output += PS3.getAnalogHat(RightHatX);
      output += "\tRightHatY: ";
      output += PS3.getAnalogHat(RightHatY);
    }
    //Analog button values can be read from almost all buttons
    if(PS3.getAnalogButton(L2_ANALOG) || PS3.getAnalogButton(R2_ANALOG)) {
      if(output != "")
        output += "\r\n";
      output += "L2: ";
      output += PS3.getAnalogButton(L2_ANALOG);
      output += "\tR2: "; 
      output += PS3.getAnalogButton(R2_ANALOG);
    }
    if(output != "") {      
      Serial.println(output);
      if(SerialBT.connected)
        SerialBT.println(output);
      output = ""; // Reset output string
    }
    if(PS3.getButtonClick(PS)) {
      output += " - PS";
      PS3.disconnect();
    } 
    else {
      if(PS3.getButtonClick(TRIANGLE))
        output += " - Traingle";
      if(PS3.getButtonClick(CIRCLE))
        output += " - Circle";
      if(PS3.getButtonClick(CROSS))
        output += " - Cross";
      if(PS3.getButtonClick(SQUARE))
        output += " - Square";

      if(PS3.getButtonClick(UP)) {
        output += " - Up";
        if(PS3.PS3Connected) {
          PS3.setAllOff();
          PS3.setLedOn(LED4);
        }
      } 
      if(PS3.getButtonClick(RIGHT)) {
        output += " - Right";
        if(PS3.PS3Connected) {
          PS3.setAllOff();
          PS3.setLedOn(LED1); 
        }         
      } 
      if(PS3.getButtonClick(DOWN)) {
        output += " - Down";          
        if(PS3.PS3Connected) {
          PS3.setAllOff();
          PS3.setLedOn(LED2);          
        }
      } 
      if(PS3.getButtonClick(LEFT)) {  
        output += " - Left";         
        if(PS3.PS3Connected) {
          PS3.setAllOff();         
          PS3.setLedOn(LED3);            
        }         
      } 

      if(PS3.getButtonClick(L1))
        output += " - L1";
      if(PS3.getButtonClick(L3))
        output += " - L3";        
      if(PS3.getButtonClick(R1))
        output += " - R1";               
      if(PS3.getButtonClick(R3))
        output += " - R3";               

      if(PS3.getButtonClick(SELECT))
        output += " - Select";
      if(PS3.getButtonClick(START))
        output += " - Start";  

      if(output != "") {
        String string = "PS3 Controller" + output;
        Serial.println(string);
        if(SerialBT.connected)
          SerialBT.println(string);
      }
    }             
  }
}
