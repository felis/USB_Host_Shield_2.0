/*
 * File:   settings.h
 * Author: xxxajk
 *
 * Created on September 23, 2013, 12:00 AM
 */

#ifndef USB_HOST_SHIELD_SETTINGS_H
#define	USB_HOST_SHIELD_SETTINGS_H
#include "macros.h"

////////////////////////////////////////////////////////////////////////////////
// DEBUGGING
////////////////////////////////////////////////////////////////////////////////

/* Set this to 1 to activate serial debugging */
#define ENABLE_UHS_DEBUGGING 0

/* This can be used to select which serial port to use for debugging if
 * multiple serial ports are available.
 * For example Serial3.
 */
#ifndef USB_HOST_SERIAL
#define USB_HOST_SERIAL Serial
#endif

////////////////////////////////////////////////////////////////////////////////
// Manual board activation
////////////////////////////////////////////////////////////////////////////////

/* Set this to 1 if you are using an Arduino Mega ADK board with MAX3421e built-in */
#define USE_UHS_MEGA_ADK 0 // If you are using Arduino 1.5.5 or newer there is no need to do this manually

/* Set this to 1 if you are using a Black Widdow */
#define USE_UHS_BLACK_WIDDOW 0

/* Set this to a one to use the xmem2 lock. This is needed for multitasking and threading */
#define USE_XMEM_SPI_LOCK 0

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

// No user serviceable parts below this line.
// DO NOT change anything below here unless you are a developer!

#if !defined(DEBUG_USB_HOST) && ENABLE_UHS_DEBUGGING
#define DEBUG_USB_HOST
#endif

// When will we drop support for the older bug-ridden stuff?
#if defined(ARDUINO) && ARDUINO >=100
#include <Arduino.h>
#else
#include <WProgram.h>
// I am not sure what WProgram.h does not include, so these are here. --xxxajk
#include <pins_arduino.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#endif

#if USE_XMEM_SPI_LOCK | defined(USE_MULTIPLE_APP_API)
#include <xmem.h>
#else
#define XMEM_ACQUIRE_SPI() (void(0))
#define XMEM_RELEASE_SPI() (void(0))
#endif

#endif	/* SETTINGS_H */
