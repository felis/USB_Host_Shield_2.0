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
#include "hidboot.h"

void MouseReportParser::Parse(HID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
        MOUSEINFO *pmi = (MOUSEINFO*)buf;

        if (prevState.mouseInfo.bmLeftButton == 0 && pmi->bmLeftButton == 1)
                OnLeftButtonDown(pmi);

        if (prevState.mouseInfo.bmLeftButton == 1 && pmi->bmLeftButton == 0)
                OnLeftButtonUp(pmi);

        if (prevState.mouseInfo.bmRightButton == 0 && pmi->bmRightButton == 1)
                OnRightButtonDown(pmi);

        if (prevState.mouseInfo.bmRightButton == 1 && pmi->bmRightButton == 0)
                OnRightButtonUp(pmi);

        if (prevState.mouseInfo.bmMiddleButton == 0 && pmi->bmMiddleButton == 1)
                OnMiddleButtonDown(pmi);

        if (prevState.mouseInfo.bmMiddleButton == 1 && pmi->bmMiddleButton == 0)
                OnMiddleButtonUp(pmi);

        if (prevState.mouseInfo.dX != pmi->dX || prevState.mouseInfo.dY != pmi->dY)
                OnMouseMove(pmi);

        if (len > sizeof (MOUSEINFO))
                for (uint8_t i = 0; i<sizeof (MOUSEINFO); i++)
                        prevState.bInfo[i] = buf[i];
};

void KeyboardReportParser::Parse(HID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
        // On error - return
        if (buf[2] == 1)
                return;

        //KBDINFO	*pki = (KBDINFO*)buf;

	    // provide event for changed control key state
		if (prevState.bInfo[0x00] != buf[0x00]) {
			OnControlKeysChanged(prevState.bInfo[0x00], buf[0x00]);
		}
	
        for (uint8_t i = 2; i < 8; i++) {
                bool down = false;
                bool up = false;

                for (uint8_t j = 2; j < 8; j++) {
                        if (buf[i] == prevState.bInfo[j] && buf[i] != 1)
                                down = true;
                        if (buf[j] == prevState.bInfo[i] && prevState.bInfo[i] != 1)
                                up = true;
                }
                if (!down) {
                        HandleLockingKeys(hid, buf[i]);
                        OnKeyDown(*buf, buf[i]);
                }
                if (!up)
                        OnKeyUp(prevState.bInfo[0], prevState.bInfo[i]);
        }
        for (uint8_t i = 0; i < 8; i++)
                prevState.bInfo[i] = buf[i];
};

uint8_t KeyboardReportParser::HandleLockingKeys(HID *hid, uint8_t key) {
        uint8_t old_keys = kbdLockingKeys.bLeds;

        switch (key) {
                case KEY_NUM_LOCK:
                        kbdLockingKeys.kbdLeds.bmNumLock = ~kbdLockingKeys.kbdLeds.bmNumLock;
                        break;
                case KEY_CAPS_LOCK:
                        kbdLockingKeys.kbdLeds.bmCapsLock = ~kbdLockingKeys.kbdLeds.bmCapsLock;
                        break;
                case KEY_SCROLL_LOCK:
                        kbdLockingKeys.kbdLeds.bmScrollLock = ~kbdLockingKeys.kbdLeds.bmScrollLock;
                        break;
        }

        if (old_keys != kbdLockingKeys.bLeds && hid)
                return (hid->SetReport(0, 0/*hid->GetIface()*/, 2, 0, 1, &kbdLockingKeys.bLeds));

        return 0;
}

const uint8_t KeyboardReportParser::numKeys[] PROGMEM = {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};
const uint8_t KeyboardReportParser::symKeysUp[] PROGMEM = {'_', '+', '{', '}', '|', '~', ':', '"', '~', '<', '>', '?'};
const uint8_t KeyboardReportParser::symKeysLo[] PROGMEM = {'-', '=', '[', ']', '\\', ' ', ';', '\'', '`', ',', '.', '/'};
const uint8_t KeyboardReportParser::padKeys[] PROGMEM = {'/', '*', '-', '+', 0x13};

uint8_t KeyboardReportParser::OemToAscii(uint8_t mod, uint8_t key) {
        uint8_t shift = (mod & 0x22);

        // [a-z]
        if (key > 0x03 && key < 0x1e) {
                // Upper case letters
                if ((kbdLockingKeys.kbdLeds.bmCapsLock == 0 && (mod & 2)) ||
                        (kbdLockingKeys.kbdLeds.bmCapsLock == 1 && (mod & 2) == 0))
                        return (key - 4 + 'A');

                        // Lower case letters
                else
                        return (key - 4 + 'a');
        }// Numbers
        else if (key > 0x1d && key < 0x27) {
                if (shift)
                        return ((uint8_t)pgm_read_byte(&numKeys[key - 0x1e]));
                else
                        return (key - 0x1e + '1');
        }// Keypad Numbers
        else if (key > 0x58 && key < 0x62) {
                if (kbdLockingKeys.kbdLeds.bmNumLock == 1)
                        return (key - 0x59 + '1');
        } else if (key > 0x2c && key < 0x39)
                return ((shift) ? (uint8_t)pgm_read_byte(&symKeysUp[key - 0x2d]) : (uint8_t)pgm_read_byte(&symKeysLo[key - 0x2d]));
        else if (key > 0x53 && key < 0x59)
                return (uint8_t)pgm_read_byte(&padKeys[key - 0x54]);
        else {
                switch (key) {
                        case KEY_SPACE: return (0x20);
                        case KEY_ENTER: return (0x13);
                        case KEY_ZERO: return ((shift) ? ')': '0');
                        case KEY_ZERO2: return ((kbdLockingKeys.kbdLeds.bmNumLock == 1) ? '0': 0);
                        case KEY_PERIOD: return ((kbdLockingKeys.kbdLeds.bmNumLock == 1) ? '.': 0);
                }
        }
        return ( 0);
}
