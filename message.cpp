/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */

#define DEBUG
#include "message.h"
// 0x80 is the default (i.e. trace) to turn off set this global to something lower.
// this allows for 126 other debugging levels.
// TO-DO: Allow assignment to a different serial port
int UsbDEBUGlvl = 0x80;

void Notifyc(char c, int lvl) {
        if (UsbDEBUGlvl < lvl) return;
#if defined(ARDUINO) && ARDUINO >=100
        Serial.print(c);
#else
        Serial.print(c, BYTE);
#endif
        Serial.flush();
}

void Notify(char const * msg, int lvl) {
        if (UsbDEBUGlvl < lvl) return;
        if (!msg) return;
        char c;

        while ((c = pgm_read_byte(msg++))) Notifyc(c, lvl);
}

void NotifyStr(char const * msg, int lvl) {
        if (UsbDEBUGlvl < lvl) return;
        if (!msg) return;
        char c;

        while (c = *msg++) Notifyc(c, lvl);
}

void NotifyFailGetDevDescr(void) {
        Notify(PSTR("\r\ngetDevDescr"), 0x80);
}

void NotifyFailSetDevTblEntry(void) {
        Notify(PSTR("\r\nsetDevTblEn"), 0x80);
}
void NotifyFailGetConfDescr(void) {
        Notify(PSTR("\r\ngetConf"), 0x80);
}

void NotifyFailSetConfDescr(void) {
        Notify(PSTR("\r\nsetConf"), 0x80);
}

void NotifyFailUnknownDevice(uint16_t VID, uint16_t PID) {
        Notify(PSTR("\r\nUnknown Device Connected - VID: "), 0x80);
        PrintHex<uint16_t > (VID, 0x80);
        Notify(PSTR(" PID: "), 0x80);
        PrintHex<uint16_t > (PID, 0x80);
}

void NotifyFail(uint8_t rcode) {
        PrintHex<uint8_t > (rcode, 0x80);
        Notify(PSTR("\r\n"), 0x80);
}
