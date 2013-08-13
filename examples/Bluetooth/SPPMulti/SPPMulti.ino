/*
 Example sketch for the RFCOMM/SPP Bluetooth library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or 
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <SPP.h>
#include <usbhub.h>

USB Usb;
USBHub Hub1(&Usb); // Some dongles have a hub inside
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so
SPP* SerialBT[2]; // We will use this pointer to store the two instance, you can easily make it larger if you like, but it will use a lot of RAM!
const uint8_t length = sizeof(SerialBT)/sizeof(SerialBT[0]); // Get the lenght of the array
boolean firstMessage[length] = { true }; // Set all to true
uint8_t buffer[50];

void setup() {
  for(uint8_t i=0;i<length;i++)
    SerialBT[i] = new SPP(&Btd); // This will set the name to the default: "Arduino" and the pin to "1234" for all connections
  
  Serial.begin(115200);
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nSPP Bluetooth Library Started"));
}
void loop() {
  Usb.Task(); // The SPP data is actually not send until this is called, one could call SerialBT.send() directly as well
  
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
    while(Serial.available() && i < sizeof(buffer)) // Read the data
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
        SerialBT[id]->write(buffer,i-1); // Send the data
      }
    }
  } 
}
