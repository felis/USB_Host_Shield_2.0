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

UHS2_GPIO implements "wiring" style GPIO access. Implemented by Brian Walton brian@riban.co.uk
 */

#if !defined(__USB2_GPIO_H__)
#define __USB2_GPIO_H__

#include "Usb.h"

class UHS2_GPIO {
public:
        UHS2_GPIO(USB *pUsb);

        void digitalWrite(uint8_t pin, uint8_t val);
        int digitalRead(uint8_t pin);
        int digitalReadOutput(uint8_t pin);

private:
        USB* m_pUsb;
};

#endif // __USB2_GPIO_H__
