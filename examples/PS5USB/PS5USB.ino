/*
 Example sketch for the PS5 USB library - developed by Kristian Sloth Lauszus
 For more information visit the Github repository: github.com/felis/USB_Host_Shield_2.0 or
 send me an e-mail: lauszus@gmail.com
 */

#include <PS5USB.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB Usb;
PS5USB PS5(&Usb);

bool printAngle = false, printTouch = false;
uint16_t lastMessageCounter = -1;
uint8_t player_led_mask = 0;
bool microphone_led = false;

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); // Halt
  }
  Serial.print(F("\r\nPS5 USB Library Started"));
}

void loop() {
  Usb.Task();

  if (PS5.connected() && lastMessageCounter != PS5.getMessageCounter()) {
    lastMessageCounter = PS5.getMessageCounter();

    if (PS5.getAnalogHat(LeftHatX) > 137 || PS5.getAnalogHat(LeftHatX) < 117 || PS5.getAnalogHat(LeftHatY) > 137 || PS5.getAnalogHat(LeftHatY) < 117 || PS5.getAnalogHat(RightHatX) > 137 || PS5.getAnalogHat(RightHatX) < 117 || PS5.getAnalogHat(RightHatY) > 137 || PS5.getAnalogHat(RightHatY) < 117) {
      Serial.print(F("\r\nLeftHatX: "));
      Serial.print(PS5.getAnalogHat(LeftHatX));
      Serial.print(F("\tLeftHatY: "));
      Serial.print(PS5.getAnalogHat(LeftHatY));
      Serial.print(F("\tRightHatX: "));
      Serial.print(PS5.getAnalogHat(RightHatX));
      Serial.print(F("\tRightHatY: "));
      Serial.print(PS5.getAnalogHat(RightHatY));
    }

    if (PS5.getAnalogButton(L2) || PS5.getAnalogButton(R2)) { // These are the only analog buttons on the PS5 controller
      Serial.print(F("\r\nL2: "));
      Serial.print(PS5.getAnalogButton(L2));
      Serial.print(F("\tR2: "));
      Serial.print(PS5.getAnalogButton(R2));
    }

    // Set the left trigger to resist at the right trigger's level
    static uint8_t oldR2Value = 0xFF;
    if (PS5.getAnalogButton(R2) != oldR2Value) {
      oldR2Value = PS5.getAnalogButton(R2);
      PS5.leftTrigger.setTriggerForce(oldR2Value, 255);
    }

    if (PS5.getButtonClick(PS))
      Serial.print(F("\r\nPS"));
    if (PS5.getButtonClick(TRIANGLE)) {
      Serial.print(F("\r\nTriangle"));
      PS5.setRumbleOn(RumbleLow);
    }
    if (PS5.getButtonClick(CIRCLE)) {
      Serial.print(F("\r\nCircle"));
      PS5.setRumbleOn(RumbleHigh);
    }
    if (PS5.getButtonClick(CROSS)) {
      Serial.print(F("\r\nCross"));

      // Set the player LEDs
      player_led_mask = (player_led_mask << 1) | 1;
      if (player_led_mask > 0x1F)
        player_led_mask = 0;
      PS5.setPlayerLed(player_led_mask); // The bottom 5 bits set player LEDs
    }
    if (PS5.getButtonClick(SQUARE)) {
      Serial.print(F("\r\nSquare"));
      PS5.setRumbleOff();
    }

    if (PS5.getButtonClick(UP)) {
      Serial.print(F("\r\nUp"));
      PS5.setLed(Red);
    } if (PS5.getButtonClick(RIGHT)) {
      Serial.print(F("\r\nRight"));
      PS5.setLed(Blue);
    } if (PS5.getButtonClick(DOWN)) {
      Serial.print(F("\r\nDown"));
      PS5.setLed(Yellow);
    } if (PS5.getButtonClick(LEFT)) {
      Serial.print(F("\r\nLeft"));
      PS5.setLed(Green);
    }

    if (PS5.getButtonClick(L1))
      Serial.print(F("\r\nL1"));
    if (PS5.getButtonClick(L3))
      Serial.print(F("\r\nL3"));
    if (PS5.getButtonClick(R1))
      Serial.print(F("\r\nR1"));
    if (PS5.getButtonClick(R3))
      Serial.print(F("\r\nR3"));

    if (PS5.getButtonClick(CREATE))
      Serial.print(F("\r\nCreate"));
    if (PS5.getButtonClick(OPTIONS)) {
      Serial.print(F("\r\nOptions"));
      printAngle = !printAngle;
    }
    if (PS5.getButtonClick(TOUCHPAD)) {
      Serial.print(F("\r\nTouchpad"));
      printTouch = !printTouch;
    }
    if (PS5.getButtonClick(MICROPHONE)) {
      Serial.print(F("\r\nMicrophone"));
      microphone_led = !microphone_led;
      PS5.setMicLed(microphone_led);
    }

    if (printAngle) { // Print angle calculated using the accelerometer only
      Serial.print(F("\r\nPitch: "));
      Serial.print(PS5.getAngle(Pitch));
      Serial.print(F("\tRoll: "));
      Serial.print(PS5.getAngle(Roll));
    }

    if (printTouch) { // Print the x, y coordinates of the touchpad
      if (PS5.isTouching(0) || PS5.isTouching(1)) // Print newline and carriage return if any of the fingers are touching the touchpad
        Serial.print(F("\r\n"));
      for (uint8_t i = 0; i < 2; i++) { // The touchpad track two fingers
        if (PS5.isTouching(i)) { // Print the position of the finger if it is touching the touchpad
          Serial.print(F("X")); Serial.print(i + 1); Serial.print(F(": "));
          Serial.print(PS5.getX(i));
          Serial.print(F("\tY")); Serial.print(i + 1); Serial.print(F(": "));
          Serial.print(PS5.getY(i));
          Serial.print(F("\t"));
        }
      }
    }
  }
}
