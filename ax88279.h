// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef UHS_AX88279_H
#define UHS_AX88279_H

#include "Usb.h"

// Experimental native-mode driver for chip code 7, USB full speed, 1000/full.
// AX88279A (chip code 15) and AX88179 variants are deliberately not claimed.
struct AX88279Time {
        uint64_t seconds; // Low 48 bits are significant.
        uint32_t nanoseconds;
};

// Caller-owned storage, retained between polls. Separate objects are required
// for Ethernet and TX timestamp endpoints. Pointers delivered to callbacks are
// valid only until this storage is reused. Do not re-enter Receive in a callback.
struct AX88279Buffer {
        uint8_t *data;
        uint16_t capacity, size;
        bool complete, discarding;
        AX88279Buffer(uint8_t *p, uint16_t n) : data(p), capacity(n), size(0),
                complete(false), discarding(false) {}
        void Reset() { size = 0; complete = discarding = false; }
};

typedef void (*AX88279FrameCallback)(void *, const uint8_t *, uint16_t, const AX88279Time *);
typedef void (*AX88279TimestampCallback)(void *, uint8_t, uint16_t, const AX88279Time &);

class AX88279 : public USBDeviceConfig {
        USB *pUsb;
        EpInfo epInfo[5];
        uint8_t address, mac[6];
        bool ready, ethernetReady, ptpReady;
        uint8_t ControlRead(uint8_t request, uint16_t value, uint16_t index,
                            uint16_t length, uint8_t *data);
        uint8_t ControlWrite(uint8_t request, uint16_t value, uint16_t index,
                             uint8_t length, uint32_t value32);
        uint8_t MacWrite(uint8_t reg, uint8_t length, uint32_t value);
        uint8_t PbusWrite(uint16_t bank, uint16_t reg, uint32_t value);
        uint8_t PbusRead(uint16_t bank, uint16_t reg, uint32_t &value);
        uint8_t PhyRead(uint8_t reg, uint16_t &value);
        uint8_t ReadTransfer(uint8_t endpoint, AX88279Buffer &buffer);
        void ResetEndpoints();
public:
        enum { BUFFER_TOO_SMALL = 0xE4, MALFORMED_DATA = 0xE5,
               NOT_READY = 0xE6, LINK_TIMEOUT = 0xE7, SHORT_READ = 0xE8 };
        explicit AX88279(USB *usb);
        uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        uint8_t Release();
        uint8_t GetAddress() { return address; }
        bool VIDPIDOK(uint16_t vid, uint16_t pid) { return vid == 0x0B95 && pid == 0x1790; }
        bool isReady() const { return ready; }
        bool isEthernetReady() const { return ethernetReady; }
        bool isPtpReady() const { return ptpReady; }
        const uint8_t *GetMAC() const { return mac; }

        // Blocking bring-up, with up to 15 seconds for autonegotiation. Restricts
        // advertisement to 1000/full. Call again after link loss/reconnection.
        uint8_t BeginEthernet();
        uint8_t LinkUp(bool &up);
        // Frame starts at buffer + 8; capacity includes header and 8-byte padding.
        // Length is 60..1518 bytes, excluding FCS. Caller pads short frames to 60.
        uint8_t SendFrame(uint8_t *buffer, uint16_t length, uint16_t capacity);
        // Read one USB packet per call. hrNAK preserves partial transfers.
        // On success inspect buffer.complete; call ParseRx only when complete.
        uint8_t Receive(AX88279Buffer &buffer);
        static uint8_t ParseRx(const uint8_t *data, uint16_t size,
                               AX88279FrameCallback callback, void *context);
        // Enables L2 PTP timestamping and a free-running hardware clock.
        // Does not implement PTP, select a grandmaster, or set UTC time.
        uint8_t BeginPTP(bool enablePps = false);
        uint8_t ReadClock(AX88279Time &time);
        uint8_t SetClock(const AX88279Time &time);
        uint8_t SetFrequency(int32_t ppb); // Limited to +/-100000 ppb.
        uint8_t EnablePPS(bool enable);
        uint8_t ReceiveTimestamps(AX88279Buffer &buffer); // Needs 131 bytes.
        static uint8_t ParseTimestamps(const uint8_t *data, uint16_t size,
                                       AX88279TimestampCallback callback, void *context);
        static bool DecodeTime(const uint8_t *data, AX88279Time &time);
};
#endif
