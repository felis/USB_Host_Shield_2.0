// SPDX-License-Identifier: GPL-2.0-or-later
#include <ax88279.h>
#include <SPI.h>
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

USB Usb;
AX88279 Ethernet(&Usb);
uint8_t rxStorage[256], timestampStorage[131];
AX88279Buffer rx(rxStorage, sizeof(rxStorage));
AX88279Buffer timestamps(timestampStorage, sizeof(timestampStorage));
uint32_t nextAttempt, lastClock, frames, stampedFrames, txStamps;
bool started;

void PrintTime(const AX88279Time &time) {
  // Display all 48 seconds bits, in hexadecimal, without allocating a String.
  Serial.print(uint16_t(time.seconds >> 32), HEX);
  Serial.print(':'); Serial.print(uint32_t(time.seconds), HEX);
  Serial.print(F(" ns=")); Serial.print(time.nanoseconds);
}
void OnFrame(void *, const uint8_t *frame, uint16_t length, const AX88279Time *time) {
  ++frames;
  if(time) ++stampedFrames;
  Serial.print(F("RX bytes=")); Serial.print(length);
  Serial.print(F(" type=0x")); Serial.print((uint16_t(frame[12]) << 8) | frame[13], HEX);
  if(time) { Serial.print(F(" hw_seconds_hex=")); PrintTime(*time); }
  Serial.println();
}
void OnTimestamp(void *, uint8_t type, uint16_t sequence, const AX88279Time &time) {
  ++txStamps;
  Serial.print(F("TX timestamp type=")); Serial.print(type);
  Serial.print(F(" sequence=")); Serial.print(sequence);
  Serial.print(F(" hw_seconds_hex=")); PrintTime(time); Serial.println();
}
void SendTestFrame(uint16_t length, bool ptp) {
  uint8_t tx[128] = {};
  memset(tx + 8, 0xff, 6);
  memcpy(tx + 14, Ethernet.GetMAC(), 6);
  tx[20] = 0x88; tx[21] = 0xb5; // Local experimental EtherType.
  for(uint8_t i = 14; i < length; ++i) tx[8 + i] = i;
  if(ptp) {
    const uint8_t destination[6] = {1, 0x1b, 0x19, 0, 0, 0};
    memcpy(tx + 8, destination, 6);
    tx[20] = 0x88; tx[21] = 0xf7;
    uint8_t *h = tx + 22; memset(h, 0, 44);
    h[0] = 1; h[1] = 2; h[3] = 44; h[4] = 42; // L2 Delay_Req, domain 42.
    memcpy(h + 20, Ethernet.GetMAC(), 3); h[23] = 0xff; h[24] = 0xfe;
    memcpy(h + 25, Ethernet.GetMAC() + 3, 3); h[29] = 1;
    static uint16_t sequence;
    ++sequence; h[30] = sequence >> 8; h[31] = sequence;
    h[32] = 1; h[33] = 0x7f;
  }
  Serial.print(F("TX test status=0x"));
  Serial.println(Ethernet.SendFrame(tx, length, sizeof(tx)), HEX);
}
void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT); digitalWrite(4, HIGH); // Deselected shield SD slot.
  Serial.println(F("AX88279 native Ethernet/PTP clock diagnostic"));
  if(Usb.Init() == -1) Serial.println(F("USB host init failed"));
}
void loop() {
  Usb.Task();
  if(!Ethernet.isReady()) { started = false; return; }
  if(!started && int32_t(millis() - nextAttempt) >= 0) {
    Serial.println(F("AX88279 enumerated; initializing 1000/full"));
    uint8_t rc = Ethernet.BeginEthernet();
    if(!rc) rc = Ethernet.BeginPTP(true);
    Serial.print(F("Initialization status=0x")); Serial.println(rc, HEX);
    started = rc == 0; nextAttempt = millis() + 5000;
    rx.Reset(); timestamps.Reset();
    if(started) Serial.println(F("PPS enabled (register readback); t/u Ethernet test; p timestamp test"));
  }
  if(!started) return;
  uint8_t rc = Ethernet.Receive(rx);
  if(!rc && rx.complete && rx.size) {
    rc = AX88279::ParseRx(rx.data, rx.size, OnFrame, NULL);
  }
  if(rc && rc != hrNAK) { Serial.print(F("RX status=0x")); Serial.println(rc, HEX); }
  rc = Ethernet.ReceiveTimestamps(timestamps);
  if(!rc && timestamps.complete && timestamps.size)
    rc = AX88279::ParseTimestamps(timestamps.data, timestamps.size, OnTimestamp, NULL);
  if(rc && rc != hrNAK) { Serial.print(F("Timestamp status=0x")); Serial.println(rc, HEX); }
  if(Serial.available()) {
    char command = Serial.read();
    if(command == 't' || command == 'u' || command == 'p')
      SendTestFrame(command == 'u' ? 120 : 60, command == 'p');
  }
  if(uint32_t(millis() - lastClock) >= 1000) {
    lastClock = millis(); bool up;
    rc = Ethernet.LinkUp(up);
    if(rc || !up) { started = false; return; }
    AX88279Time time; rc = Ethernet.ReadClock(time);
    Serial.print(F("Clock status=0x")); Serial.print(rc, HEX);
    if(!rc) { Serial.print(F(" seconds_hex=")); PrintTime(time); }
    Serial.print(F(" rx=")); Serial.print(frames);
    Serial.print(F(" rx_hw=")); Serial.print(stampedFrames);
    Serial.print(F(" tx_hw=")); Serial.println(txStamps);
  }
}
