/* Copyright (C) 2021 Kristian Sloth Lauszus and Dennis Frett. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Sloth Lauszus
 Web      :  https://lauszus.com
 e-mail   :  lauszus@gmail.com

 Dennis Frett
 GitHub   :  https://github.com/dennisfrett
 e-mail   :  dennis.frett@live.com
 */

#pragma once

#include "hiduniversal.h"

#define MINIDSP_VID 0x2752 // MiniDSP
#define MINIDSP_PID 0x0011 // MiniDSP 2x4HD

/**
 * Arduino MiniDSP 2x4HD USB Host Driver by Dennis Frett.
 *
 * This class implements support for the MiniDSP 2x4HD via USB.
 * Based on NodeJS implementation by Mathieu Rene:
 * https://github.com/mrene/node-minidsp and the Python implementation by Mark
 * Kubiak: https://github.com/markubiak/python3-minidsp.
 *
 * It uses the HIDUniversal class for all the USB communication.
 */
class MiniDSP : public HIDUniversal {
public:

        /**
         * Constructor for the MiniDSP class.
         * @param  p   Pointer to the USB class instance.
         */
        MiniDSP(USB *p) : HIDUniversal(p) {
        };

        /**
         * Used to check if a MiniDSP 2x4HD is connected.
         * @return Returns true if it is connected.
         */
        bool connected() {
                return HIDUniversal::isReady() && HIDUniversal::VID == MINIDSP_VID && HIDUniversal::PID == MINIDSP_PID;
        };

        /**
         * Used to call your own function when the device is successfully
         * initialized.
         * @param funcOnInit Function to call.
         */
        void attachOnInit(void (*funcOnInit)(void)) {
                pFuncOnInit = funcOnInit;
        };

        /**
         * Used to call your own function when the volume has changed.
         * The volume is passed as an unsigned integer that represents twice the
         * -dB value. Example: 19 represents -9.5dB.
         * @param funcOnVolumeChange Function to call.
         */
        void attachOnVolumeChange(void (*funcOnVolumeChange)(uint8_t)) {
                pFuncOnVolumeChange = funcOnVolumeChange;
        }

        /**
         * Used to call your own function when the muted status has changed.
         * The muted status is passed as a boolean. True means muted, false
         * means unmuted.
         * @param funcOnMutedChange Function to call.
         */
        void attachOnMutedChange(void (*funcOnMutedChange)(bool)) {
                pFuncOnMutedChange = funcOnMutedChange;
        }

        /**
         * Retrieve the current volume of the MiniDSP.
         * The volume is passed as an unsigned integer that represents twice the
         * -dB value. Example: 19 represents -9.5dB.
         * @return Current volume.
         */
        int getVolume() const {
                return volume;
        }

        /**
         * Retrieve the current volume of the MiniDSP in -dB.
         * @return Current volume.
         */
        float getVolumeDB() const {
                return volume / -2.0;
        }

        /**
         * Retrieve the current muted status of the MiniDSP
         * @return `true` if the device is muted, `false` otherwise.
         */
        bool isMuted() const {
                return muted;
        }

protected:
        /** @name HIDUniversal implementation */
        /**
         * Used to parse USB HID data.
         * @param hid       Pointer to the HID class.
         * @param is_rpt_id Only used for Hubs.
         * @param len       The length of the incoming data.
         * @param buf       Pointer to the data buffer.
         */
        void ParseHIDData(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);

        /**
         * Called when a device is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific
         * way.
         */
        uint8_t OnInitSuccessful();
        /**@}*/

        /** @name USBDeviceConfig implementation */

        /**
         * Used by the USB core to check what this driver support.
         * @param  vid The device's VID.
         * @param  pid The device's PID.
         * @return     Returns true if the device's VID and PID matches this
         * driver.
         */
        virtual bool VIDPIDOK(uint16_t vid, uint16_t pid) {
                return vid == MINIDSP_VID && pid == MINIDSP_PID;
        };
        /**@}*/

private:
        /**
         * Calculate checksum for given buffer.
         * Checksum is given by summing up all bytes in `data` and returning the first byte.
         * @param data Buffer to calculate checksum for.
         * @param data_length Length of the buffer.
         */
        uint8_t Checksum(const uint8_t *data, uint8_t data_length) const;

        /**
         * Send the "Request status" command to the MiniDSP. The response
         * includes the current volume and the muted status.
         */
        void
        RequestStatus() const;

        /**
         * Send the given MiniDSP command. This function will create a buffer
         * with the expected header and checksum and send it to the MiniDSP.
         * Responses will come in throug `ParseHIDData`.
         * @param command Buffer of the command to send.
         * @param command_length Length of the buffer.
         */
        void SendCommand(uint8_t *command, uint8_t command_length) const;

        // Callbacks

        // Pointer to function called in onInit().
        void (*pFuncOnInit)(void) = nullptr;

        // Pointer to function called when volume changes.
        void (*pFuncOnVolumeChange)(uint8_t) = nullptr;

        // Pointer to function called when muted status changes.
        void (*pFuncOnMutedChange)(bool) = nullptr;

        // -----------------------------------------------------------------------------

        // MiniDSP state. Currently only volume and muted status are
        // implemented, but others can be added easily if needed.

        // The volume is stored as an unsigned integer that represents twice the
        // -dB value. Example: 19 represents -9.5dB.
        uint8_t volume = 0;
        bool muted = false;
};
