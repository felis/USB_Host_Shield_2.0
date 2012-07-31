/*
 Example sketch for the RFCOMM Bluetooth library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <RFCOMM.h>
USB Usb;
/* You can create the instance of the class in two ways */
RFCOMM SerialBT(&Usb); // This will set the name to the defaults: "Arduino" and the pin to "1234"
//RFCOMM SerialBT(&Usb, "Lauszus' Arduino","0000"); // You can also set the name and pin like so

boolean firstMessage = true;

void setup() {
  Serial.begin(115200);
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nRFCOMM Bluetooth Library Started"));
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
  delay(5);
}
