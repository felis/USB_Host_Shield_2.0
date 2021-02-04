/* Copyright (C) 2014 Kristian Lauszus, TKJ Electronics. All rights reserved.
 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").
 Contact information
 -------------------
 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#include "MiniDSP.h"

namespace {
uint8_t RequestStatusOutputCommand[] = {0x05, 0xFF, 0xDA, 0x02};
uint8_t StatusInputCommand[]{0x05, 0xFF, 0xDA};

// Returns first byte of the sum of given bytes.
uint8_t Checksum(const uint8_t *data, uint16_t nbytes) {
  uint64_t sum = 0;
  for (uint16_t i = 0; i < nbytes; i++) {
    sum += data[i];
  }

  return sum & 0xFF;
}
} // namespace

void MiniDSP::ParseHIDData(USBHID *hid __attribute__((unused)),
                           bool is_rpt_id __attribute__((unused)), uint8_t len,
                           uint8_t *buf) {

  // Only care about valid data for the MiniDSP 2x4HD.
  if (HIDUniversal::VID != MINIDSP_VID || HIDUniversal::PID != MINIDSP_PID ||
      len <= 2 || buf == nullptr) {
    return;
  }

  // Check if this is a status update.
  // First byte is the length, we ignore that for now.
  if (memcmp(buf + 1, StatusInputCommand, sizeof(StatusInputCommand)) == 0) {

    // Parse data.
    // Response is of format [ length ] [ 0x05 0xFF 0xDA ] [ volume ] [ muted ].
    const auto newVolume = buf[sizeof(StatusInputCommand) + 1];
    const auto newIsMuted = (bool)buf[sizeof(StatusInputCommand) + 2];

    const auto volumeUpdated = newVolume != volume;
    const auto isMutedUpdated = newIsMuted != isMuted;

    // Update status.
    volume = newVolume;
    isMuted = newIsMuted;

    // Call callbacks.
    if (volumeChangeCallback != nullptr && volumeUpdated) {
      volumeChangeCallback(volume);
    }

    if (mutedChangeCallback != nullptr && isMutedUpdated) {
      mutedChangeCallback(isMuted);
    }
  }
};

uint8_t MiniDSP::OnInitSuccessful() {
  // Verify we're actually connected to the MiniDSP 2x4HD.
  if (HIDUniversal::VID != MINIDSP_VID || HIDUniversal::PID != MINIDSP_PID) {
    return 0;
  }

  // Request current status so we can initialize the values.
  RequestStatus();

  if (onInitCallback != nullptr) {
    onInitCallback();
  }

  return 0;
};

void MiniDSP::SendCommand(uint8_t *command, uint16_t command_length) const {
  // Only send command if we're actually connected to the MiniDSP 2x4HD.
  if (HIDUniversal::VID != MINIDSP_VID || HIDUniversal::PID != MINIDSP_PID) {
    return;
  }

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
  memset(&buf[checksumOffset + 1], 0xFF, sizeof(buf) - checksumOffset - 1);

  pUsb->outTransfer(bAddress, epInfo[epInterruptOutIndex].epAddr, sizeof(buf),
                    buf);
}

void MiniDSP::RequestStatus() const {
  SendCommand(RequestStatusOutputCommand, sizeof(RequestStatusOutputCommand));
}
