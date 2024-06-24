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

#include "MiniDSP.h"

void MiniDSP::ParseHIDData(USBHID *hid __attribute__ ((unused)), bool is_rpt_id __attribute__ ((unused)), uint8_t len, uint8_t *buf) {
        // Only care about valid data for the MiniDSP 2x4HD.
        if(HIDUniversal::VID != MINIDSP_VID || HIDUniversal::PID != MINIDSP_PID || len <= 4 || buf == nullptr)
                return;

        // Check if this is a requested mute change.
        if(buf[1] == 0x17){
                // Response is of format [ length ] [ 0x17 ] [ muted ]
                muted = (bool)buf[2];
        }

        // Check if this is a requested volume change.
        if(buf[1] == 0x42){
                // Response is of format [ length ] [ 0x42 ] [ volume ]
                volume = buf[2];

        }

        constexpr uint8_t InputCommand[] = {0x05, 0xFF};

        // Only deal with status updates from now on.
        if(memcmp(buf + 1, InputCommand, sizeof (InputCommand)) != 0)
                return;

        if(buf[3] == 0xDA){
                // Parse data.
                // Response is of format [ length ] [ 0x05 0xFF 0xDA ] [ volume ] [ muted ].
                const auto newVolume = buf[4];
                const auto newIsMuted = (bool)buf[5];

                const auto volumeChanged = newVolume != volume;
                const auto mutedChanged = newIsMuted != muted;

                // Update status.
                volume = newVolume;
                muted = newIsMuted;

                // Call callbacks.
                if(pFuncOnVolumeChange != nullptr && volumeChanged)
                        pFuncOnVolumeChange(volume);

                if(pFuncOnMutedChange != nullptr && mutedChanged)
                        pFuncOnMutedChange(muted);
        }


        // Check if this is an input source update.
        if(buf[3] == 0xA9 || buf[3] == 0xD9){
                // Parse data.
                // Response is of format [ length ] [ 0x05 0xFF 0xA9/0xD9 ] [ source ].
                const auto newInputSource = buf[4];

                // Ensure we only interpret valid inputs.
                if(newInputSource >= 0x00 && newInputSource <= 0x02){
                        const auto inputSourceChanged = newInputSource != (char) inputSource;

                        // Update values.
                        inputSource = (InputSource) newInputSource;

                        // Call callbacks.
                        if(pFuncOnInputSourceChange != nullptr && inputSourceChanged)
                                pFuncOnInputSourceChange(inputSource);
                }
        }

        // Check if this is an Config update.
        if(buf[3] == 0xD8){
                // Parse data.
                // Response is of format [ length ] [ 0x05 0xFF 0xD8 ] [ config ].
                const auto newConfig = buf[4];

                // Ensure we only interpret valid inputs.
                if(newConfig >= 0x00 && newConfig <= 0x03){
                        const auto configChanged = newConfig != (char) config;

                        // Update values.
                        config = (Config) newConfig;

                        // Call callbacks.
                        if(pFuncOnConfigChange != nullptr && configChanged)
                                pFuncOnConfigChange(config);
                }
        }
};

uint8_t MiniDSP::OnInitSuccessful() {
        // Verify we're actually connected to the MiniDSP 2x4HD.
        if(HIDUniversal::VID != MINIDSP_VID || HIDUniversal::PID != MINIDSP_PID)
                return 0;

        // Request current information so we can initialize the values.
        RequestStatus();
        RequestInputSource();
        RequestConfig();

        if(pFuncOnInit != nullptr)
                pFuncOnInit();

        return 0;
};

uint8_t MiniDSP::Checksum(const uint8_t *data, uint8_t data_length) const {
        uint16_t sum = 0;
        for(uint8_t i = 0; i < data_length; i++)
                sum += data[i];

        return sum & 0xFF;
}

void MiniDSP::SendCommand(uint8_t *command, uint8_t command_length) const {
        // Sanity check on command length.
        if(command_length > 63)
                return;

        // Message is padded to 64 bytes with 0xFF and is of format:
        // [ length (command + checksum byte) ] [ command ] [ checksum ] [ OxFF... ]

        // MiniDSP expects 64 byte messages.
        uint8_t buf[64];

        // Set length, including checksum byte.
        buf[0] = command_length + 1;

        // Copy actual command.
        memcpy(&buf[1], command, command_length);

        const auto checksumOffset = command_length + 1;

        // Set checksum byte.
        buf[checksumOffset] = Checksum(buf, command_length + 1);

        // Pad the rest.
        memset(&buf[checksumOffset + 1], 0xFF, sizeof (buf) - checksumOffset - 1);

        pUsb->outTransfer(bAddress, epInfo[epInterruptOutIndex].epAddr, sizeof (buf), buf);
}

void MiniDSP::RequestStatus() const {
        uint8_t RequestStatusOutputCommand[] = {0x05, 0xFF, 0xDA, 0x02};

        SendCommand(RequestStatusOutputCommand, sizeof (RequestStatusOutputCommand));
}

void MiniDSP::RequestInputSource() const {
        uint8_t RequestInputSourceCommand[] = {0x05, 0xFF, 0xD9, 0x01};

        SendCommand(RequestInputSourceCommand, sizeof(RequestInputSourceCommand));
}

void MiniDSP::RequestConfig() const {
        uint8_t RequestConfigCommand[] = {0x05, 0xFF, 0xD8, 0x01};

        SendCommand(RequestConfigCommand, sizeof(RequestConfigCommand));
}

void MiniDSP::setVolumeDB(float volumeDB) const {
        // Only accept values between 0dB and -127.5dB.
        // Don't do error handling.
        if(volume > 0 || volume < -127.5){
                return;
        }

        uint8_t SetVolumeCommand[] = {0x42, (uint8_t)(-2*volumeDB)};

        SendCommand(SetVolumeCommand, sizeof(SetVolumeCommand));
}

void MiniDSP::setMuted(bool muted) const {
        uint8_t SetMutedommand[] = {0x17, muted ? (uint8_t)0x01 : (uint8_t)0x00};

        SendCommand(SetMutedommand, sizeof(SetMutedommand));
}