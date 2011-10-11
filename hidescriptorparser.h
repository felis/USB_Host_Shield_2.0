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
#if !defined(__HIDDESCRIPTORPARSER_H__)
#define __HIDDESCRIPTORPARSER_H__

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "avrpins.h"
#include "max3421e.h"
#include "usbhost.h"
#include "usb_ch9.h"
#include "Usb.h"
#include <WProgram.h>

#include "printhex.h"
#include "hexdump.h"
#include "message.h"

#include "confdescparser.h"
//#include "hidusagestr.h"
#include "hid.h"
//#include "..\ptp\simplefifo.h"

class ReportDescParserBase : public USBReadParser
{
public:
	typedef void (*UsagePageFunc)(uint16_t usage);

	static void PrintGenericDesktopPageUsage(uint16_t usage);
	static void PrintSimulationControlsPageUsage(uint16_t usage);
	static void PrintVRControlsPageUsage(uint16_t usage);
	static void PrintSportsControlsPageUsage(uint16_t usage);
	static void PrintGameControlsPageUsage(uint16_t usage);
	static void PrintGenericDeviceControlsPageUsage(uint16_t usage);
	static void PrintLEDPageUsage(uint16_t usage);
	static void PrintButtonPageUsage(uint16_t usage);
	static void PrintOrdinalPageUsage(uint16_t usage);
	static void PrintTelephonyPageUsage(uint16_t usage);
	static void PrintConsumerPageUsage(uint16_t usage);
	static void PrintDigitizerPageUsage(uint16_t usage);
	static void PrintAlphanumDisplayPageUsage(uint16_t usage);
	static void PrintMedicalInstrumentPageUsage(uint16_t usage);

	static void PrintValue(uint8_t *p, uint8_t len);
	static void PrintByteValue(uint8_t data);

	static void PrintItemTitle(uint8_t prefix);

	static const char *usagePageTitles0[];
	static const char *usagePageTitles1[];	 
	static const char *genDesktopTitles0[]; 
	static const char *genDesktopTitles1[]; 
	static const char *genDesktopTitles2[]; 
	static const char *genDesktopTitles3[]; 
	static const char *genDesktopTitles4[]; 
	static const char *simuTitles0[]; 
	static const char *simuTitles1[]; 
	static const char *simuTitles2[]; 
	static const char *vrTitles0[];
	static const char *vrTitles1[];
	static const char *sportsCtrlTitles0[];
	static const char *sportsCtrlTitles1[];
	static const char *sportsCtrlTitles2[];
	static const char *gameTitles0[];
	static const char *gameTitles1[];
	static const char *genDevCtrlTitles[];
	static const char *ledTitles[];
	static const char *telTitles0[]; 
	static const char *telTitles1[]; 
	static const char *telTitles2[]; 
	static const char *telTitles3[]; 
	static const char *telTitles4[]; 
	static const char *telTitles5[];
	static const char *consTitles0[];
	static const char *consTitles1[];
	static const char *consTitles2[];
	static const char *consTitles3[];
	static const char *consTitles4[];
	static const char *consTitles5[];
	static const char *consTitles6[];
	static const char *consTitles7[];
	static const char *consTitles8[];
	static const char *consTitles9[];
	static const char *consTitlesA[];
	static const char *consTitlesB[];
	static const char *consTitlesC[];
	static const char *consTitlesD[];
	static const char *consTitlesE[];
	static const char *digitTitles0[];
	static const char *digitTitles1[];
	static const char *digitTitles2[];
	static const char *aplphanumTitles0[];
	static const char *aplphanumTitles1[];
	static const char *aplphanumTitles2[];
	static const char *medInstrTitles0[];
	static const char *medInstrTitles1[];
	static const char *medInstrTitles2[];
	static const char *medInstrTitles3[];
	static const char *medInstrTitles4[];

protected:
	static UsagePageFunc	usagePageFunctions[];

    MultiValueBuffer		theBuffer;
	MultiByteValueParser	valParser;
	ByteSkipper				theSkipper;
	uint8_t					varBuffer[sizeof(USB_CONFIGURATION_DESCRIPTOR)]; 

	uint8_t					itemParseState;		// Item parser state variable
	uint8_t					itemSize;			// Item size
	uint8_t					itemPrefix;			// Item prefix (first byte)
	uint8_t					rptSize;			// Report Size
	uint8_t					rptCount;			// Report Count

	uint16_t				totalSize;			// Report size in bits

	virtual uint8_t ParseItem(uint8_t **pp, uint16_t *pcntdn);

	UsagePageFunc			pfUsage;

	static void PrintUsagePage(uint16_t page);
	void SetUsagePage(uint16_t page);

public:
	ReportDescParserBase() :
		itemParseState(0),
		itemSize(0),		
		itemPrefix(0),
		rptSize(0),
		rptCount(0),
		pfUsage(NULL)
		{
			theBuffer.pValue = varBuffer; 
			valParser.Initialize(&theBuffer);
			theSkipper.Initialize(&theBuffer);
		};

	virtual void Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset);

	enum 
	{
		enErrorSuccess = 0
		, enErrorIncomplete					// value or record is partialy read in buffer
		, enErrorBufferTooSmall
	};
};

class ReportDescParser : public ReportDescParserBase
{
};

class ReportDescParser2 : public ReportDescParserBase
{
	uint8_t			rptId;					// Report ID
	uint8_t			useMin;					// Usage Minimum
	uint8_t			useMax;					// Usage Maximum
	uint8_t			fieldCount;				// Number of field being currently processed

	void OnInputItem(uint8_t itm);			// Method which is called every time Input item is found

	uint8_t			*pBuf;					// Report buffer pointer
	uint8_t			bLen;					// Report length

protected:
	virtual uint8_t ParseItem(uint8_t **pp, uint16_t *pcntdn);

public:
	ReportDescParser2(uint16_t len, uint8_t *pbuf) : 
			ReportDescParserBase(), bLen(len), pBuf(pbuf), rptId(0), useMin(0), useMax(0), fieldCount(0)
	{};
};

class UniversalReportParser : public HIDReportParser
{
public:
	virtual void Parse(HID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

#endif // __HIDDESCRIPTORPARSER_H__