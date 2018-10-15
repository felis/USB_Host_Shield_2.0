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

#include "UHS2_gpio.h"

/** @brief  Implement an instance of a UHS2_GPIO object
*   @param  pUSB Pointer to a UHS2 USB object
*/
UHS2_GPIO::UHS2_GPIO(USB *pUsb) : m_pUsb(pUsb)
{
}

/** @brief  Set a GPIO output value
*   @param  pin GPIO output pin on USB Host Shield to set
*   @param  val Value to set the pin to (zero value will clear output, non-zero value will assert output)
*/
void UHS2_GPIO::digitalWrite(uint8_t pin, uint8_t val) {
        if(pin > 7)
                return;
        uint8_t nValue = m_pUsb->gpioRdOutput();
        uint8_t nMask = 1 << pin;
        nValue &= (~nMask);
        if(val)
                nValue |= (nMask);
        m_pUsb->gpioWr(nValue);
}

/** @brief  Read the value from a GPIO input pin
*   @param  pin GPIO input pin on USB Host Shield to read
*   @retval int Value of GPIO input (-1 on fail)
*/
int UHS2_GPIO::digitalRead(uint8_t pin) {
        if(pin > 7)
                return -1;
        uint8_t nMask = 1 << pin;
        uint8_t nValue = m_pUsb->gpioRd();
        return ((nValue & nMask)?1:0);
}

/** @brief  Read the value from a GPIO output pin
*   @param  pin GPIO output pin on USB Host Shield to read
*   @retval int Value of GPIO output (-1 on fail)
*   @note   Value of MAX3421E output register, i.e. what the device has been set to, not the physical value on the pin
*/
int UHS2_GPIO::digitalReadOutput(uint8_t pin) {
        if(pin > 7)
                return -1;
        uint8_t nMask = 1 << pin;
        uint8_t nValue = m_pUsb->gpioRdOutput();
        return ((nValue & nMask)?1:0);
}
