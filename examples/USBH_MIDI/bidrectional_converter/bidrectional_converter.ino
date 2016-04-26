/*
 *******************************************************************************
 * Legacy Serial MIDI and USB Host bidirectional converter
 * Copyright (C) 2013-2016 Yuuichi Akagawa
 *
 * for use with Arduino MIDI library
 * https://github.com/FortySevenEffects/arduino_midi_library/
 *
 * Note:
 * - If you want use with Leonardo, you must choose Arduino MIDI library v4.0 or higher.
 * - This is sample program. Do not expect perfect behavior.
 *******************************************************************************
 */

#include <MIDI.h>
#include <usbh_midi.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#endif

//Arduino MIDI library v4.2 compatibility
#ifdef MIDI_CREATE_DEFAULT_INSTANCE
MIDI_CREATE_DEFAULT_INSTANCE();
#endif
#ifdef USBCON
#define _MIDI_SERIAL_PORT Serial1
#else
#define _MIDI_SERIAL_PORT Serial
#endif

//////////////////////////
// MIDI Pin assign
// 2 : GND
// 4 : +5V(Vcc) with 220ohm
// 5 : TX
//////////////////////////

USB Usb;
USBH_MIDI Midi(&Usb);

void MIDI_poll();
void doDelay(unsigned long t1, unsigned long t2, unsigned long delayTime);

//If you want handle System Exclusive message, enable this #define otherwise comment out it.
#define USBH_MIDI_SYSEX_ENABLE

#ifdef USBH_MIDI_SYSEX_ENABLE
MidiSysEx sysExData;
//SysEx:
void handle_sysex( byte* sysexmsg, unsigned sizeofsysex) {
  Midi.SendSysEx(sysexmsg, sizeofsysex);
}
#endif

void setup()
{
  MIDI.begin(MIDI_CHANNEL_OMNI);
#ifdef USBH_MIDI_SYSEX_ENABLE
  MIDI.setHandleSystemExclusive(handle_sysex);
#endif
  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );
}

void loop()
{
  unsigned long t1;
  uint8_t msg[4];

  Usb.Task();
  t1 = micros();
  if ( Usb.getUsbTaskState() == USB_STATE_RUNNING )
  {
    MIDI_poll();
    if (MIDI.read()) {
      msg[0] = MIDI.getType();
      switch (msg[0]) {
        case midi::ActiveSensing :
          break;
        case midi::SystemExclusive :
          //SysEx is handled by event.
          break;
        default :
          msg[1] = MIDI.getData1();
          msg[2] = MIDI.getData2();
          Midi.SendData(msg, 0);
          break;
      }
    }
  }
  //delay(1ms)
  doDelay(t1, micros(), 1000);
}

// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll()
{
  uint8_t size;
#ifdef USBH_MIDI_SYSEX_ENABLE
  uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
  uint8_t rcode = 0;     //return code
  uint16_t  rcvd;
  uint8_t   readPtr = 0;

  rcode = Midi.RecvData( &rcvd, recvBuf);

  //data check
  if (rcode != 0) return;
  if ( recvBuf[0] == 0 && recvBuf[1] == 0 && recvBuf[2] == 0 && recvBuf[3] == 0 ) {
    return ;
  }

  uint8_t *p = recvBuf;
  while (readPtr < MIDI_EVENT_PACKET_SIZE)  {
    if (*p == 0 && *(p + 1) == 0) break; //data end
    MidiSysEx::Status rc = sysExData.set(p);
    switch (rc) {
      case MidiSysEx::nonsysex :  //No SysEx message send data to Serial MIDI
        p++;
        size = Midi.lookupMsgSize(*p);
        _MIDI_SERIAL_PORT.write(p, size);
        p += 3;
        break;
      case MidiSysEx::done :      //SysEx end. send data to Serial MIDI
        _MIDI_SERIAL_PORT.write(sysExData.get(), sysExData.getSize());
        /* FALLTHROUGH */
      case MidiSysEx::overflow :  //SysEx buffer over. ignore and flush buffer.
        sysExData.clear();
        /* FALLTHROUGH */
      default:
        p += 4;
        break;
    }
    readPtr += 4;
  }
#else
  uint8_t outBuf[ 3 ];
  do {
    if ( (size = Midi.RecvData(outBuf)) > 0 ) {
      //MIDI Output
      _MIDI_SERIAL_PORT.write(outBuf, size);
    }
  } while (size > 0);
#endif
}

// Delay time (max 16383 us)
void doDelay(unsigned long t1, unsigned long t2, unsigned long delayTime)
{
  unsigned long t3;

  if ( t1 > t2 ) {
    t3 = (0xFFFFFFFF - t1 + t2);
  } else {
    t3 = t2 - t1;
  }

  if ( t3 < delayTime ) {
    delayMicroseconds(delayTime - t3);
  }
}
