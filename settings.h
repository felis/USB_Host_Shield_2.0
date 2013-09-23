/*
 * File:   settings.h
 * Author: AJK
 *
 * Created on September 23, 2013, 12:00 AM
 */

#ifndef SETTINGS_H
#define	SETTINGS_H
#include "macros.h"


////////////////////////////////////////////////////////////////////////////////
// CORE
////////////////////////////////////////////////////////////////////////////////
//#define BOARD_BLACK_WIDDOW
// uncomment to activate
//#define DEBUG_USB_HOST

#ifndef USB_HOST_SERIAL
#define USB_HOST_SERIAL Serial
#endif


////////////////////////////////////////////////////////////////////////////////
// MASS STORAGE
////////////////////////////////////////////////////////////////////////////////
// <<<<<<<<<<<<<<<< IMPORTANT >>>>>>>>>>>>>>>
// Set this to 1 to support single LUN devices, and save RAM. -- I.E. thumb drives.
// Each LUN needs ~13 bytes to be able to track the state of each unit.
#ifndef MASS_MAX_SUPPORTED_LUN
#define MASS_MAX_SUPPORTED_LUN 8
#endif


////////////////////////////////////////////////////////////////////////////////
// AUTOMATIC Settings
////////////////////////////////////////////////////////////////////////////////
#if defined(ARDUINO) && ARDUINO >=100
#include "Arduino.h"
#else
#include <WProgram.h>
#endif

#if defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
#define BOARD_TEENSY_PLUS_PLUS
#endif
#include <avr/pgmspace.h>

#endif	/* SETTINGS_H */

