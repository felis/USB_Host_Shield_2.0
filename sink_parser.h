/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */

#if !defined(_usb_h_) || defined(__SINK_PARSER_H__)
#error "Never include hexdump.h directly; include Usb.h instead"
#else
#define __SINK_PARSER_H__

extern int UsbDEBUGlvl;

// This parser does absolutely nothing with the data, just swallows it.

template <class BASE_CLASS, class LEN_TYPE, class OFFSET_TYPE>
class SinkParser : public BASE_CLASS {
public:

        SinkParser() {
        };

        void Initialize() {
        };

        void Parse(const LEN_TYPE len, const uint8_t *pbuf, const OFFSET_TYPE &offset) {
        };
};


#endif // __HEXDUMP_H__
