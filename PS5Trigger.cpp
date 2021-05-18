/**
 * @file PS5Trigger.cpp
 * @author Ludwig Füchsl, adapted for USB_Host_Library SAMD by Joseph Duchesne
 * @brief Based on Ludwig Füchsl's DualSense Windows driver https://github.com/Ohjurot/DualSense-Windows
 * @date 2020-11-25
 *
 * @copyright Copyright (c) 2020 Ludwig Füchsl
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Minor updates by Kristian Sloth Lauszus.
 */

#include "PS5Trigger.h"

void PS5Trigger::processTrigger(uint8_t* buffer) {
    // Switch on effect
    switch (data.effectType) {
        // Continious
        case EffectType::ContinuousResitance:
            // Mode
            buffer[0x00] = 0x01;
            // Parameters
            buffer[0x01] = data.Continuous.startPosition;
            buffer[0x02] = data.Continuous.force;

            break;

        // Section
        case EffectType::SectionResitance:
              // Mode
              buffer[0x00] = 0x02;
              // Parameters
              buffer[0x01] = data.Section.startPosition;
              buffer[0x02] = data.Section.endPosition;

              break;

        // EffectEx
        case EffectType::EffectEx:
              // Mode
              buffer[0x00] = 0x02 | 0x20 | 0x04;
              // Parameters
              buffer[0x01] = 0xFF - data.EffectEx.startPosition;
              // Keep flag
              if (data.EffectEx.keepEffect)
                buffer[0x02] = 0x02;
              // Forces
              buffer[0x04] = data.EffectEx.beginForce;
              buffer[0x05] = data.EffectEx.middleForce;
              buffer[0x06] = data.EffectEx.endForce;
              // Frequency
              buffer[0x09] = data.EffectEx.frequency / 2;
              if(buffer[0x09] < 1) buffer[0x09] = 1;  // minimum frequency

              break;

        // Calibrate
        case EffectType::Calibrate:
              // Mode
              buffer[0x00] = 0xFC;

              break;

        // No resistance / default
        case EffectType::NoResitance:
        default:
              // All zero
              buffer[0x00] = 0x00;
              buffer[0x01] = 0x00;
              buffer[0x02] = 0x00;

              break;
    }
    reportChanged = false;
}
