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
#if !defined(__HEXDUMP_H__)
#define __HEXDUMP_H__

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "printhex.h"

template <class BASE_CLASS, class LEN_TYPE, class OFFSET_TYPE>
class HexDumper : public BASE_CLASS
{
	uint8_t		byteCount;
	OFFSET_TYPE    	byteTotal;

public:
	HexDumper() : byteCount(0), byteTotal(0) {};
	void Initialize() { byteCount = 0; byteTotal = 0; };

	virtual void Parse(const LEN_TYPE len, const uint8_t *pbuf, const OFFSET_TYPE &offset);
};

template <class BASE_CLASS, class LEN_TYPE, class OFFSET_TYPE>
void HexDumper<BASE_CLASS, LEN_TYPE, OFFSET_TYPE>::Parse(const LEN_TYPE len, const uint8_t *pbuf, const OFFSET_TYPE &offset)
{
	for (LEN_TYPE j=0; j<len; j++, byteCount++, byteTotal++)
	{
        	if (!byteCount)
        	{
			PrintHex<OFFSET_TYPE>(byteTotal);
            		Serial.print(": ");
        	}
		PrintHex<uint8_t>(pbuf[j]);
		Serial.print(" ");

		if (byteCount == 15)
		{
			Serial.println("");
			byteCount = 0xFF;
		}
	}
}

#endif // __HEXDUMP_H__