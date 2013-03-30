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
#if !defined(__MESSAGE_H__)
#define __MESSAGE_H__

#include <inttypes.h>
#include <avr/pgmspace.h>

extern int UsbDEBUGlvl;

#include "printhex.h"

void Notify(char const * msg, int lvl);
void NotifyStr(char const * msg, int lvl);
#ifdef DEBUG
void NotifyFailGetDevDescr(void);
void NotifyFailSetDevTblEntry(void);
void NotifyFailGetConfDescr(void);
void NotifyFailSetConfDescr(void);
void NotifyFailUnknownDevice(uint16_t VID, uint16_t PID);
void NotifyFail(uint8_t rcode);
#else
#define NotifyFailGetDevDescr()
#define NotifyFailSetDevTblEntry()
#define NotifyFailGetConfDescr()
#define NotifyFailSetConfDescr()
#define NotifyFailUnknownDevice(VID, PID)
#define NotifyFail(rcode)
#endif

template <class ERROR_TYPE>
void ErrorMessage(char const * msg, ERROR_TYPE rcode = 0) {
        Notify(msg, 0x80);
        Notify(PSTR(": "), 0x80);
        PrintHex<ERROR_TYPE > (rcode, 0x80);
        Notify(PSTR("\r\n"), 0x80);
}

#include "hexdump.h"

#endif // __MESSAGE_H__
