/*
 Example sketch for the PS4 USB library - developed by Kristian Lauszus
 For more information visit my blog: http://blog.tkjelectronics.dk/ or
 send me an e-mail:  kristianl@tkjelectronics.com
 */

#include <PS4USB.h>

// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

USB Usb;
PS4USB PS4(&Usb);

boolean printAngle, printTouch;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); // Halt
  }
  Serial.print(F("\r\nPS4 USB Library Started"));
}

void loop() {
  Usb.Task();

  if (PS4.connected()) {
    if (PS4.getAnalogHat(LeftHatX) > 137 || PS4.getAnalogHat(LeftHatX) < 117 || PS4.getAnalogHat(LeftHatY) > 137 || PS4.getAnalogHat(LeftHatY) < 117 || PS4.getAnalogHat(RightHatX) > 137 || PS4.getAnalogHat(RightHatX) < 117 || PS4.getAnalogHat(RightHatY) > 137 || PS4.getAnalogHat(RightHatY) < 117) {
      Serial.print(F("\r\nLeftHatX: "));
      Serial.print(PS4.getAnalogHat(LeftHatX));
      Serial.print(F("\tLeftHatY: "));
      Serial.print(PS4.getAnalogHat(LeftHatY));
      Serial.print(F("\tRightHatX: "));
      Serial.print(PS4.getAnalogHat(RightHatX));
      Serial.print(F("\tRightHatY: "));
      Serial.print(PS4.getAnalogHat(RightHatY));
    }

    if (PS4.getAnalogButton(L2) || PS4.getAnalogButton(R2)) { // These are the only analog buttons on the PS4 controller
      Serial.print(F("\r\nL2: "));
      Serial.print(PS4.getAnalogButton(L2));
      Serial.print(F("\tR2: "));
      Serial.print(PS4.getAnalogButton(R2));
    }

    if (PS4.getButtonClick(PS))
      Serial.print(F("\r\nPS"));
    if (PS4.getButtonClick(TRIANGLE))
      Serial.print(F("\r\nTraingle"));
    if (PS4.getButtonClick(CIRCLE))
      Serial.print(F("\r\nCircle"));
    if (PS4.getButtonClick(CROSS))
      Serial.print(F("\r\nCross"));
    if (PS4.getButtonClick(SQUARE))
      Serial.print(F("\r\nSquare"));

    if (PS4.getButtonClick(UP))
      Serial.print(F("\r\nUp"));
    if (PS4.getButtonClick(RIGHT))
      Serial.print(F("\r\nRight"));
    if (PS4.getButtonClick(DOWN))
      Serial.print(F("\r\nDown"));
    if (PS4.getButtonClick(LEFT))
      Serial.print(F("\r\nLeft"));

    if (PS4.getButtonClick(L1))
      Serial.print(F("\r\nL1"));
    if (PS4.getButtonClick(L3))
      Serial.print(F("\r\nL3"));
    if (PS4.getButtonClick(R1))
      Serial.print(F("\r\nR1"));
    if (PS4.getButtonClick(R3))
      Serial.print(F("\r\nR3"));

    if (PS4.getButtonClick(SHARE))
      Serial.print(F("\r\nShare"));
    if (PS4.getButtonClick(OPTIONS)) {
      Serial.print(F("\r\nOptions"));
      printAngle = !printAngle;
    }
    if (PS4.getButtonClick(TOUCHPAD)) {
      Serial.print(F("\r\nTouchpad"));
      printTouch = !printTouch;
    }

    if (printAngle) { // Print angle calculated using the accelerometer only
      Serial.print(F("\r\nPitch: "));
      Serial.print(PS4.getAngle(Pitch));
      Serial.print(F("\tRoll: "));
      Serial.print(PS4.getAngle(Roll));
    }

    if (printTouch) { // Print the x, y coordinates of the touchpad
      if (PS4.isTouching(0) || PS4.isTouching(1)) // Print newline and carriage return if any of the fingers are touching the touchpad
        Serial.print(F("\r\n"));
      for (uint8_t i = 0; i < 2; i++) { // The touchpad track two fingers
        if (PS4.isTouching(i)) { // Print the position of the finger if it is touching the touchpad
          Serial.print(F("X")); Serial.print(i + 1); Serial.print(F(": "));
          Serial.print(PS4.getX(i));
          Serial.print(F("\tY")); Serial.print(i + 1); Serial.print(F(": "));
          Serial.print(PS4.getY(i));
          Serial.print(F("\t"));
        }
      }
    }
  }
}
