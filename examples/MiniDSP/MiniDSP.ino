/*
 Example sketch for the MiniDSP 2x4HD library - developed by Dennis Frett
 */

#include <MiniDSP.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB Usb;
MiniDSP MiniDSP(&Usb);

void OnMiniDSPConnected() {
  Serial.println("MiniDSP connected");
}

void OnVolumeChange(uint8_t volume) {
  Serial.println("Volume is: " + String(volume));
}

void OnMutedChange(bool isMuted) {
  Serial.println("Muted status: " + String(isMuted ? "muted" : "unmuted"));
}

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while(!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  if(Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while(1); // Halt
  }
  Serial.println(F("\r\nMiniDSP 2x4HD Library Started"));

  // Register callbacks.
  MiniDSP.attachOnInit(&OnMiniDSPConnected);
  MiniDSP.attachOnVolumeChange(&OnVolumeChange);
  MiniDSP.attachOnMutedChange(&OnMutedChange);
}

void loop() {
  Usb.Task();
}
