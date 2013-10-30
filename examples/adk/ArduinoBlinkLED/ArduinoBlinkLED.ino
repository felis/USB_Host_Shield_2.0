// The source for the Android application can be found at the following link: https://github.com/Lauszus/ArduinoBlinkLED
// The code for the Android application is heavily based on this guide: http://allaboutee.com/2011/12/31/arduino-adk-board-blink-an-led-with-your-phone-code-and-explanation/ by Miguel
#include <adk.h>

USB Usb;
ADK adk(&Usb, "TKJElectronics", // Manufacturer Name
              "ArduinoBlinkLED", // Model Name
              "Example sketch for the USB Host Shield", // Description (user-visible string)
              "1.0", // Version
              "http://www.tkjelectronics.dk/uploads/ArduinoBlinkLED.apk", // URL (web page to visit if no installed apps support the accessory)
              "123456789"); // Serial Number (optional)

#define LED LED_BUILTIN // Pin 13 is occupied by the SCK pin on a normal Arduino (Uno, Duemilanove etc.), so use a different pin

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  if (Usb.Init() == -1) {
    Serial.print("\r\nOSCOKIRQ failed to assert");
    while (1); // halt
  }
  pinMode(LED, OUTPUT);
  Serial.print("\r\nArduino Blink LED Started");
}

void loop() {
  Usb.Task();
  if (adk.isReady()) {
    uint8_t msg[1];
    uint16_t len = sizeof(msg);
    uint8_t rcode = adk.RcvData(&len, msg);
    if (rcode && rcode != hrNAK)
      USBTRACE2("Data rcv. :", rcode);
    else if (len > 0) {
      Serial.print(F("\r\nData Packet: "));
      Serial.print(msg[0]);
      digitalWrite(LED, msg[0] ? HIGH : LOW);
    }
  }
  else
    digitalWrite(LED, LOW);
}
