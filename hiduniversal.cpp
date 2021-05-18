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

#include "hiduniversal.h"

uint8_t HIDUniversal::Poll() {
        uint8_t rcode = 0;

        if(!bPollEnable)
                return 0;

        if((int32_t)((uint32_t)millis() - qNextPollTime) >= 0L) {
                qNextPollTime = (uint32_t)millis() + pollInterval;

                uint8_t buf[constBuffLen];

                for(uint8_t i = 0; i < bNumIface; i++) {
                        uint8_t index = hidInterfaces[i].epIndex[epInterruptInIndex];
                        uint16_t read = (uint16_t)epInfo[index].maxPktSize;

                        ZeroMemory(constBuffLen, buf);

                        uint8_t rcode = pUsb->inTransfer(bAddress, epInfo[index].epAddr, &read, buf);

                        if(rcode) {
                                if(rcode != hrNAK)
                                        USBTRACE3("(hiduniversal.h) Poll:", rcode, 0x81);
                                return rcode;
                        }

                        if(read > constBuffLen)
                                read = constBuffLen;

                        // TODO: handle read == 0 ? continue like HIDComposite,
                        //       return early like in the error case above?
                        //       Either way passing an empty buffer to the functions below
                        //       probably makes no sense?

#if 0
                        Notify(PSTR("\r\nBuf: "), 0x80);

                        for(uint8_t i = 0; i < read; i++) {
                                D_PrintHex<uint8_t > (buf[i], 0x80);
                                Notify(PSTR(" "), 0x80);
                        }

                        Notify(PSTR("\r\n"), 0x80);
#endif
                        ParseHIDData(this, bHasReportId, (uint8_t)read, buf);

                        HIDReportParser *prs = GetReportParser(((bHasReportId) ? *buf : 0));

                        if(prs)
                                prs->Parse(this, bHasReportId, (uint8_t)read, buf);
                }
        }
        return rcode;
}
