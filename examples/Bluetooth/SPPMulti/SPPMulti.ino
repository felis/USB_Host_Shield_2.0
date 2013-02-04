/*
 Example sketch for the RFCOMM/SPP Bluetooth library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <SPP.h>
USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so

// This will set the name to the defaults: "Arduino" and the pin to "1234" for both connections
SPP SPP_1(&Btd); // This will allow you to communicate with two SPP devices simultaneously
SPP SPP_2(&Btd);
//SPP SPP_3(&Btd); // You can create as many instances as you like, but it will take up a lot of RAM!!

// You can also set the name and pin like so
//SPP SerialBT(&Btd, "Lauszus's Arduino","0000");

SPP* SerialBT[2]; // We will use this pointer to store the two instance, you can easily make it larger if you like
const uint8_t length = sizeof(SerialBT)/sizeof(SerialBT[0]); // Get the lenght of the array
boolean firstMessage[length] = { true }; // Set all to true
uint8_t buffer[50];

void setup() {
  SerialBT[0] = &SPP_1; // This will point to the first instance
  SerialBT[1] = &SPP_2; // This will point to the second instance
  //SerialBT[2] = &SPP_3; // You only need to uncomment this if you wanted to use another instance
  
  Serial.begin(115200);
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nSPP Bluetooth Library Started"));
}
void loop() {
  Usb.Task();
  for(uint8_t i=0;i<length;i++) {
    if(SerialBT[i]->connected) {
      if(firstMessage[i]) {
        firstMessage[i] = false;
        SerialBT[i]->println(F("Hello from Arduino")); // Send welcome message
      }
      if(SerialBT[i]->available())
        Serial.write(SerialBT[i]->read());
    } 
    else 
      firstMessage[i] = true;
  }
  if(Serial.available()) {
    delay(10); // Wait for the rest of the data to arrive
    uint8_t i = 0;
    while(Serial.available()) // Read the data
      buffer[i++] = Serial.read();
    /* 
      Set the connection you want to send to using the first character
      For instace "0Hello World" would send "Hello World" to connection 0 
    */
    uint8_t id = buffer[0]-'0'; // Convert from ASCII
    if(id < length && i > 1) { // And then compare to length and make sure there is any text
      if(SerialBT[id]->connected) { // Check if a device is actually connected
        for(uint8_t i2 = 0; i2 < i-1; i2++) // Don't include the first character
          buffer[i2] = buffer[i2+1];
        SerialBT[id]->println(buffer,i-1); // Send the data
      }
    }
  } 
}
