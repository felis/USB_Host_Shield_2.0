/*
 *******************************************************************************
 * eVY1 Shield sample - Say 'Konnichiwa'
 * Copyright (C) 2014-2016 Yuuichi Akagawa
 *
 * This is sample program. Do not expect perfect behavior.
 *******************************************************************************
 */
#include <usbh_midi.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB Usb;
//USBHub Hub(&Usb);
USBH_MIDI  Midi(&Usb);

void MIDI_poll();
void noteOn(uint8_t note);
void noteOff(uint8_t note);

uint16_t pid, vid;
uint8_t exdata[] = {
  0xf0, 0x43, 0x79, 0x09, 0x00, 0x50, 0x10,
  'k', ' ', 'o', ',', //Ko
  'N', '\\', ',',     //N
  'J', ' ', 'i', ',', //Ni
  't', 'S', ' ', 'i', ',', //Chi
  'w', ' ', 'a',      //Wa
  0x00, 0xf7
};

void setup()
{
  vid = pid = 0;
  Serial.begin(115200);

  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );
}

void loop()
{
  Usb.Task();
  if( Midi ) {
    MIDI_poll();
    noteOn(0x3f);
    delay(400);
    noteOff(0x3f);
    delay(100);
  }
}

// Poll USB MIDI Controler
void MIDI_poll()
{
  uint8_t inBuf[ 3 ];

  //first call?
  if (Midi.idVendor() != vid || Midi.idProduct() != pid) {
    vid = Midi.idVendor(); pid = Midi.idProduct();
    Midi.SendSysEx(exdata, sizeof(exdata));
    delay(500);
  }
  Midi.RecvData(inBuf);
}

//note On
void noteOn(uint8_t note)
{
  uint8_t buf[3];
  buf[0] = 0x90;
  buf[1] = note;
  buf[2] = 0x7f;
  Midi.SendData(buf);
}

//note Off
void noteOff(uint8_t note)
{
  uint8_t buf[3];
  buf[0] = 0x80;
  buf[1] = note;
  buf[2] = 0x00;
  Midi.SendData(buf);
}
