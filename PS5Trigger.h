/**
 * @file PS5Trigger.h
 * @author Ludwig Füchsl, adapted for USB_Host_Library SAMD by Joseph Duchesne
 * @brief Based on Ludwig Füchsl's DualSense Windows driver https://github.com/Ohjurot/DualSense-Windows
 * @version 0.1
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

#ifndef _ps5trigger_h_
#define _ps5trigger_h_

#include <inttypes.h>

class PS5Trigger {
private:
    // Type of trigger effect
    typedef enum _EffectType : uint8_t {
        NoResitance = 0x00,    // No resistance is applied
        ContinuousResitance = 0x01,   // Continuous Resitance is applied
        SectionResitance = 0x02, // Seciton resistance is appleyed
        EffectEx = 0x26,  // Extended trigger effect
        Calibrate = 0xFC,  // Calibrate triggers
    } EffectType;

    // Trigger effect
    typedef struct _EffectData {
        // Trigger effect type
        EffectType effectType;

        // Union for effect parameters
        union {
            // Union one raw data
            uint8_t _u1_raw[6];

            // For type == ContinuousResitance
            struct {
                uint8_t startPosition;  // Start position of resistance
                uint8_t force;  // Force of resistance
                uint8_t _pad[4];  // PAD / UNUSED
            } __attribute__((packed)) Continuous;

            // For type == SectionResitance
            struct {
                uint8_t startPosition;  // Start position of resistance
                uint8_t endPosition; // End position of resistance (>= start)
                uint8_t _pad[4];  // PAD / UNUSED
            } __attribute__((packed)) Section;

            // For type == EffectEx
            struct {
                uint8_t startPosition;  // Position at witch the effect starts
                bool keepEffect; // Wher the effect should keep playing when trigger goes beyond 255
                uint8_t beginForce;  // Force applied when trigger >= (255 / 2)
                uint8_t middleForce;  // Force applied when trigger <= (255 / 2)
                uint8_t endForce;   // Force applied when trigger is beyond 255
                uint8_t frequency;  // Vibration frequency of the trigger
            } __attribute__((packed)) EffectEx;
        } __attribute__((packed));
    } __attribute__((packed)) EffectData;

    EffectData data;

public:
    bool reportChanged = false;

    /**
     * @brief Apply the trigger data to a PS5 update buffer
     *
     * @param buffer The buffer at the start offset for this trigger data
     */
    void processTrigger(uint8_t* buffer);

    /**
     * Clear force feedback on trigger without report changed
     */
    void Reset() {
        data.effectType = EffectType::NoResitance;

        reportChanged = false;
    };

    /**
     * Clear force feedback on trigger
     */
    void clearTriggerForce() {
        data.effectType = EffectType::NoResitance;

        reportChanged = true;
    };

    /**
     * Set continuous force feedback on trigger
     * @param start 0-255 trigger pull to start resisting
     * @param force The force amount
     */
    void setTriggerForce(uint8_t start, uint8_t force) {
        if (force == 0)
            data.effectType = EffectType::NoResitance;
        else {
            data.effectType = EffectType::ContinuousResitance;
            data.Continuous.startPosition = start;
            data.Continuous.force = force;
        }

        reportChanged = true;
    };

    /**
     * Set section force feedback on trigger
     * @param start trigger pull to start resisting
     * @param end trigger pull to stop resisting
     */
    void setTriggerForceSection(uint8_t start, uint8_t end) {
        data.effectType = EffectType::SectionResitance;
        data.Section.startPosition = start;
        data.Section.endPosition = end;

        reportChanged = true;
    };

    /**
     * Set effect force feedback on trigger
     * @param start trigger pull to start resisting
     * @param keep Keep effect active after max trigger pull
     * @param begin_force 0-255 force at start position
     * @param mid_force 0-255 force half way between start and max pull
     * @param end_force 0-255 force at max pull
     * @param frequency Vibration frequency of the trigger
     */
    void setTriggerForceEffect(uint8_t start, bool keep, uint8_t begin_force, uint8_t mid_force, uint8_t end_force, uint8_t frequency) {
        data.effectType = EffectType::SectionResitance;
        data.EffectEx.startPosition = start;
        data.EffectEx.keepEffect = keep;
        data.EffectEx.beginForce = begin_force;
        data.EffectEx.middleForce = mid_force;
        data.EffectEx.endForce = end_force;
        data.EffectEx.frequency = frequency;

        reportChanged = true;
    };

};

#endif
