/*
 *******************************************************************************
 * USB-MIDI dump utility
 * Copyright (C) 2013-2021 Yuuichi Akagawa
 *
 * for use with USB Host Shield 2.0 from Circuitsathome.com
 * https://github.com/felis/USB_Host_Shield_2.0
 *
 * This is sample program. Do not expect perfect behavior.
 *******************************************************************************
 */

#include <usbh_midi.h>
#include <usbhub.h>

USB Usb;
USBHub Hub(&Usb);
USBH_MIDI  Midi(&Usb);

void MIDI_poll();

void onInit()
{
  char buf[20];
  uint16_t vid = Midi.idVendor();
  uint16_t pid = Midi.idProduct();
  sprintf(buf, "VID:%04X, PID:%04X", vid, pid);
  Serial.println(buf); 
}

void setup()
{
  Serial.begin(115200);

  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );

  // Register onInit() function
  Midi.attachOnInit(onInit);
}

void loop()
{
  Usb.Task();
  if ( Midi ) {
    MIDI_poll();
  }
}

// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll()
{
  char buf[16];
  uint8_t bufMidi[MIDI_EVENT_PACKET_SIZE];
  uint16_t  rcvd;

  if (Midi.RecvData( &rcvd,  bufMidi) == 0 ) {
    uint32_t time = (uint32_t)millis();
    sprintf(buf, "%04X%04X:%3d:", (uint16_t)(time >> 16), (uint16_t)(time & 0xFFFF), rcvd); // Split variable to prevent warnings on the ESP8266 platform
    Serial.print(buf);

    for (int i = 0; i < MIDI_EVENT_PACKET_SIZE; i++) {
      sprintf(buf, " %02X", bufMidi[i]);
      Serial.print(buf);
    }
    Serial.println("");
  }
}
