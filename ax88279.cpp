// SPDX-License-Identifier: GPL-2.0-or-later
// Register sequences adapted from the ASIX Linux driver, Copyright (c) 2022
// ASIX Electronic Corporation. See examples/AX88279/README.md for source links.
#include "ax88279.h"

namespace {
uint32_t ReadLE32(const uint8_t *p) {
        return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
                (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
void WriteLE32(uint8_t *p, uint32_t value) {
        for(uint8_t i = 0; i < 4; ++i) p[i] = value >> (8 * i);
}
class ReadCount : public USBReadParser {
public:
        uint16_t received;
        ReadCount() : received(0) {}
        void Parse(const uint16_t len, const uint8_t *, const uint16_t &) { received += len; }
};
// Validate the observed vendor configuration rather than assuming endpoint
// numbers from VID/PID. Descriptors can span USB packets; keep only the header.
class NativeConfig : public USBReadParser {
        uint8_t head[9], used, length, endpoints, expected, seen;
        uint16_t received, total;
        bool valid, inInterface;
public:
        uint8_t interruptSize;
        NativeConfig() : used(0), length(0), endpoints(0), expected(0), seen(0), received(0), total(0),
                valid(true), inInterface(false), interruptSize(0) {}
        void Parse(const uint16_t len, const uint8_t *data, const uint16_t &offset) {
                if(!valid || offset != received) { valid = false; return; }
                received += len;
                for(uint16_t i = 0; i < len; ++i) {
                        if(!used) { length = data[i]; if(length < 2) { valid = false; return; } }
                        if(used < sizeof(head)) head[used] = data[i];
                        if(++used != length) continue;
                        used = 0;
                        if(!total && head[1] != USB_DESCRIPTOR_CONFIGURATION) { valid = false; return; }
                        if(head[1] == USB_DESCRIPTOR_CONFIGURATION) {
                                if(length != 9 || total || head[4] != 1 || head[5] != 1) { valid = false; return; }
                                total = uint16_t(head[2]) | (uint16_t(head[3]) << 8);
                        } else if(head[1] == USB_DESCRIPTOR_INTERFACE) {
                                if(length < 9 || inInterface || head[2] || head[3] || head[4] < 4 || head[5] != 0xff) { valid = false; return; }
                                inInterface = true; expected = head[4];
                        } else if(head[1] == USB_DESCRIPTOR_ENDPOINT) {
                                if(length < 7 || !inInterface) { valid = false; return; }
                                ++seen;
                                uint8_t ep = head[2] & 15;
                                if(ep > 4) continue; // Additional queues are unused.
                                if(ep < 1 || head[2] != (ep == 3 ? 3 : (0x80 | ep)) ||
                                   head[3] != (ep == 1 ? 3 : 2) || (ep == 1 ? (head[4] != 8 && head[4] != 16) : head[4] != 64) ||
                                   head[5] || (endpoints & (1 << ep))) { valid = false; return; }
                                endpoints |= 1 << ep;
                                if(ep == 1) interruptSize = head[4];
                        }
                }
        }
        bool Complete() const { return valid && !used && total == received && endpoints == 0x1e && seen == expected; }
};
}

AX88279::AX88279(USB *usb) : pUsb(usb), address(0), ready(false),
        ethernetReady(false), ptpReady(false) {
        ResetEndpoints(); memset(mac, 0, sizeof(mac));
        if(pUsb) pUsb->RegisterDeviceClass(this);
}
void AX88279::ResetEndpoints() {
        memset(epInfo, 0, sizeof(epInfo));
        epInfo[0].maxPktSize = 8;
        for(uint8_t i = 0; i < 5; ++i) {
                epInfo[i].epAddr = i;
                if(i) epInfo[i].maxPktSize = i == 1 ? 16 : 64;
                epInfo[i].bmNakPower = (i == 0 || i == 3) ? USB_NAK_MAX_POWER : USB_NAK_NOWAIT;
        }
}
uint8_t AX88279::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        if(address) return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
        if(!pUsb || lowspeed) return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
        ResetEndpoints();
        AddressPool &pool = pUsb->GetAddressPool();
        UsbDevice *zero = pool.GetUsbDevicePtr(0);
        if(!zero || !zero->epinfo) return USB_ERROR_EPINFO_IS_NULL;
        EpInfo *old = zero->epinfo;
        bool oldSpeed = zero->lowspeed;
        zero->epinfo = epInfo; zero->lowspeed = false;
        USB_DEVICE_DESCRIPTOR descriptor = {};
        uint8_t rc = pUsb->getDevDescr(0, 0, 8, reinterpret_cast<uint8_t *>(&descriptor));
        if(!rc) {
                uint8_t packet = descriptor.bMaxPacketSize0;
                if(packet != 8 && packet != 16 && packet != 32 && packet != 64)
                        rc = USB_ERROR_INVALID_MAX_PKT_SIZE;
                else {
                        epInfo[0].maxPktSize = packet;
                        descriptor = USB_DEVICE_DESCRIPTOR();
                        rc = pUsb->getDevDescr(0, 0, sizeof(descriptor), reinterpret_cast<uint8_t *>(&descriptor));
                }
        }
        if(!rc && (descriptor.bLength != sizeof(descriptor) || descriptor.bDescriptorType != USB_DESCRIPTOR_DEVICE ||
                   descriptor.bMaxPacketSize0 != epInfo[0].maxPktSize ||
                   !VIDPIDOK(descriptor.idVendor, descriptor.idProduct) || !descriptor.bNumConfigurations))
                rc = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
        if(!rc) {
                address = pool.AllocAddress(parent, false, port);
                rc = address ? pUsb->setAddr(0, 0, address) : USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
        }
        zero->epinfo = old; zero->lowspeed = oldSpeed;
        if(rc) { Release(); return rc; }
        UsbDevice *device = pool.GetUsbDevicePtr(address);
        if(!device) { Release(); return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL; }
        device->lowspeed = false;
        rc = pUsb->setEpInfoEntry(address, 1, epInfo);
        // Read configuration 0 with an explicit header and byte-count check.
        uint8_t config[9] = {};
        if(!rc) {
                ReadCount count;
                rc = pUsb->ctrlReq(address, 0, 0x80, USB_REQUEST_GET_DESCRIPTOR,
                        0, USB_DESCRIPTOR_CONFIGURATION, 0, 9, 9, config, &count);
                if(!rc && count.received != 9) rc = SHORT_READ;
        }
        if(!rc && (config[0] != 9 || config[1] != 2 || config[5] != 1 || config[3] || config[2] < 46))
                rc = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
        if(!rc) {
                NativeConfig parser;
                uint8_t scratch[64];
                rc = pUsb->ctrlReq(address, 0, 0x80, USB_REQUEST_GET_DESCRIPTOR,
                        0, USB_DESCRIPTOR_CONFIGURATION, 0, config[2], 64, scratch, &parser);
                if(!rc && !parser.Complete()) rc = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
                if(!rc) epInfo[1].maxPktSize = parser.interruptSize;
        }
        if(!rc) rc = pUsb->setConf(address, 0, 1);
        uint8_t chip = 0;
        if(!rc) { delay(100); rc = ControlRead(1, 5, 1, 1, &chip); }
        if(!rc && (chip >> 4) != 7) rc = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
        if(!rc) rc = pUsb->setEpInfoEntry(address, 5, epInfo);
        if(!rc) rc = ControlRead(1, 0x10, 6, 6, mac);
        if(rc) { Release(); return rc; }
        ready = true;
        return 0;
}
uint8_t AX88279::Release() {
        if(pUsb && address) pUsb->GetAddressPool().FreeAddress(address);
        address = 0; ready = ethernetReady = ptpReady = false;
        memset(mac, 0, sizeof(mac)); ResetEndpoints();
        return 0;
}
uint8_t AX88279::ControlRead(uint8_t request, uint16_t value, uint16_t index,
                           uint16_t length, uint8_t *data) {
        if(!length) return 0;
        memset(data, 0, length);
        ReadCount count;
        uint8_t rc = pUsb->ctrlReq(address, 0, 0xc0, request, uint8_t(value), uint8_t(value >> 8),
                index, length, length, data, &count);
        return rc ? rc : (count.received == length ? 0 : SHORT_READ);
}
uint8_t AX88279::ControlWrite(uint8_t request, uint16_t value, uint16_t index,
                            uint8_t length, uint32_t word) {
        uint8_t data[4]; WriteLE32(data, word);
        if(length > sizeof(data)) return USB_ERROR_INVALID_ARGUMENT;
        return pUsb->ctrlReq(address, 0, 0x40, request, uint8_t(value), uint8_t(value >> 8),
                index, length, length, length ? data : NULL, NULL);
}
uint8_t AX88279::MacWrite(uint8_t reg, uint8_t length, uint32_t value) {
        return ControlWrite(1, reg, length, length, value);
}
uint8_t AX88279::PbusWrite(uint16_t bank, uint16_t reg, uint32_t value) {
        return ControlWrite(0x11, reg, bank, 4, value);
}
uint8_t AX88279::PbusRead(uint16_t bank, uint16_t reg, uint32_t &value) {
        uint8_t data[4]; uint8_t rc = ControlRead(0x11, reg, bank, 4, data);
        if(rc) return rc;
        value = ReadLE32(data);
        return value == 0xdeadbeefUL ? NOT_READY : 0;
}
uint8_t AX88279::PhyRead(uint8_t reg, uint16_t &value) {
        uint8_t data[2]; uint8_t rc = ControlRead(2, 3, reg, 2, data);
        if(!rc) value = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
        return rc;
}
// Stop immediately on the first failed transfer; never continue a partial setup.
#define AX_TRY(call) do { uint8_t rc = (call); if(rc) return rc; } while(0)
uint8_t AX88279::LinkUp(bool &up) {
        up = false;
        if(!ready) return NOT_READY;
        uint16_t status;
        AX_TRY(PhyRead(1, status)); AX_TRY(PhyRead(1, status));
        up = status & 4;
        if(!up) ethernetReady = ptpReady = false;
        return 0;
}
uint8_t AX88279::BeginEthernet() {
        ethernetReady = ptpReady = false;
        if(!ready) return NOT_READY;
        AX_TRY(ControlWrite(8, 1, 2, 0, 0)); // Native firmware, timestamp endpoint.
        AX_TRY(ControlWrite(6, 0, 0, 0, 0)); // Reload volatile configuration.
        AX_TRY(ControlWrite(0x31, 0, 0, 1, 2)); // Power up PHY.
        delay(500);
        uint16_t adv, gigabit;
        uint8_t multi[2];
        AX_TRY(PhyRead(4, adv)); AX_TRY(PhyRead(9, gigabit));
        AX_TRY(ControlRead(0x27, 7, 32, 2, multi));
        uint16_t multigig = uint16_t(multi[0]) | (uint16_t(multi[1]) << 8);
        AX_TRY(ControlWrite(0x27, 7, 32, 2, multigig & ~uint16_t(0x80)));
        AX_TRY(ControlRead(0x27, 7, 32, 2, multi));
        if(multi[0] & 0x80) return NOT_READY;
        AX_TRY(ControlWrite(2, 3, 4, 2, (adv & ~uint16_t(0x01e0)) | 0x1000));
        AX_TRY(ControlWrite(2, 3, 9, 2, (gigabit & ~uint16_t(0x0100)) | 0x0a00));
        AX_TRY(ControlWrite(2, 3, 0, 2, 0x1200));
        bool up = false;
        for(uint8_t attempt = 0; attempt < 15; ++attempt) {
                AX_TRY(LinkUp(up)); if(up) break; delay(1000);
        }
        if(!up) return LINK_TIMEOUT;
        delay(500);
        bool gigabitFull = false;
        for(uint8_t attempt = 0; attempt < 10; ++attempt) {
                uint8_t status[16]; uint16_t count = epInfo[1].maxPktSize;
                uint8_t rc = pUsb->inTransfer(address, 1, &count, status);
                if(rc && rc != hrNAK) return rc;
                if(!rc && count >= 3 && (status[1] & 7) == 3 && (status[2] & 1)) { gigabitFull = true; break; }
                delay(160);
        }
        uint16_t partner;
        AX_TRY(PhyRead(9, gigabit)); AX_TRY(PhyRead(10, partner));
        if(!gigabitFull || !(gigabit & 0x0200) || !(partner & 0x0800)) return NOT_READY;
        AX_TRY(MacWrite(0x0b, 2, 0));
        AX_TRY(MacWrite(0xb7, 1, 0));
        AX_TRY(MacWrite(0x34, 1, 0));
        AX_TRY(MacWrite(0x35, 1, 0));
        AX_TRY(MacWrite(0xb9, 1, 2));
        AX_TRY(MacWrite(0x2b, 1, 0));
        AX_TRY(MacWrite(0x41, 1, 0xff));
        AX_TRY(MacWrite(0x43, 1, 0));
        AX_TRY(MacWrite(0x46, 1, 0));
        AX_TRY(MacWrite(0x9e, 1, 0));
        AX_TRY(MacWrite(0xb1, 1, 0));
        AX_TRY(MacWrite(0xcc, 1, 1));
        AX_TRY(MacWrite(0xb0, 1, 0));
        AX_TRY(MacWrite(0xb5, 1, 0xa5));
        AX_TRY(MacWrite(0x54, 2, 0x0410));
        AX_TRY(MacWrite(0x0d, 1, 0));
        AX_TRY(MacWrite(0xf9, 1, 7));
        AX_TRY(MacWrite(0xb8, 1, 0x29));
        AX_TRY(MacWrite(0xb2, 3, 0x3ef148UL));
        AX_TRY(MacWrite(0x6d, 3, 0x006078UL));
        AX_TRY(MacWrite(0xc0, 2, 0x3440));
        AX_TRY(MacWrite(0x0e, 1, 0));
        AX_TRY(MacWrite(0xc3, 1, 0));
        uint8_t bulk[5] = {7, 0, 0, 3, 0x3f};
        AX_TRY(pUsb->ctrlReq(address, 0, 0x40, 1, 0x2e, 0, 5, 5, 5, bulk, NULL));
        AX_TRY(MacWrite(0x22, 2, 0x0133));
        AX_TRY(MacWrite(0x0b, 2, 0x01aa));
        AX_TRY(MacWrite(0xb7, 1, 3));
        ethernetReady = true;
        return 0;
}
uint8_t AX88279::SendFrame(uint8_t *buffer, uint16_t length, uint16_t capacity) {
        if(!ethernetReady) return NOT_READY;
        if(!buffer || length < 60 || length > 1518) return USB_ERROR_INVALID_ARGUMENT;
        uint16_t bytes = 8 + ((length + 7) & ~uint16_t(7));
        if(bytes > capacity) return BUFFER_TOO_SMALL;
        memset(buffer, 0, 8);
        WriteLE32(buffer, uint32_t(length) | (uint32_t(length & 0x7ff) << 21));
        memset(buffer + 8 + length, 0, bytes - 8 - length);
        return pUsb->outTransfer(address, 3, bytes, buffer);
}
uint8_t AX88279::ReadTransfer(uint8_t endpoint, AX88279Buffer &buffer) {
        if(!buffer.data || !buffer.capacity) return USB_ERROR_INVALID_ARGUMENT;
        if(buffer.complete) buffer.Reset();
        uint8_t packet[64]; uint16_t count = sizeof(packet);
        uint8_t rc = pUsb->inTransfer(address, endpoint, &count, packet);
        if(rc == hrNAK) return rc;
        if(rc) { buffer.discarding = true; buffer.size = 0; return rc; }
        if(buffer.size > buffer.capacity || count > buffer.capacity - buffer.size) buffer.discarding = true;
        if(!buffer.discarding) { memcpy(buffer.data + buffer.size, packet, count); buffer.size += count; }
        if(count == sizeof(packet)) return 0;
        buffer.complete = true;
        if(buffer.discarding) { buffer.size = 0; return BUFFER_TOO_SMALL; }
        return 0;
}
uint8_t AX88279::Receive(AX88279Buffer &buffer) {
        return ethernetReady ? ReadTransfer(2, buffer) : NOT_READY;
}
bool AX88279::DecodeTime(const uint8_t *data, AX88279Time &time) {
        time.nanoseconds = ReadLE32(data);
        time.seconds = uint64_t(ReadLE32(data + 4)) |
                (uint64_t(data[8]) << 32) | (uint64_t(data[9]) << 40);
        return time.nanoseconds < 1000000000UL;
}
uint8_t AX88279::ParseRx(const uint8_t *data, uint16_t size,
                        AX88279FrameCallback callback, void *context) {
        if(!data || size < 16) return MALFORMED_DATA;
        uint32_t trailer = ReadLE32(data + size - 8);
        uint16_t count = trailer & 0x1fff;
        uint32_t offset = trailer >> 13;
        if(!count || offset >= uint16_t(size - 8) || count > (size - 8 - offset) / 8) return MALFORMED_DATA;
        // Validate the entire aggregate before any callback can change state.
        for(uint8_t pass = 0; pass < 2; ++pass) {
                uint16_t payload = 0, meta = uint16_t(offset);
                for(uint16_t i = 0; i < count; ++i) {
                        if(meta > size - 16) return MALFORMED_DATA;
                        const uint8_t *m = data + meta;
                        uint16_t length = uint16_t(m[2]) | (uint16_t(m[3] & 0x7f) << 8);
                        bool stamped = m[7] & 1;
                        uint8_t metaSize = stamped ? 24 : 8;
                        if(payload > offset || length < 14 || length > offset - payload ||
                           metaSize > size - 8 - meta) return MALFORMED_DATA;
                        uint16_t padded = (length + 7) & ~uint16_t(7);
                        if(padded > offset - payload) return MALFORMED_DATA;
                        AX88279Time time;
                        if(stamped && !DecodeTime(m + 8, time)) return MALFORMED_DATA;
                        if(pass && callback && (m[1] & 8) && !(m[3] & 0x80))
                                callback(context, data + payload, length, stamped ? &time : NULL);
                        payload += padded; meta += metaSize;
                }
                if(payload != offset || meta != size - 8) return MALFORMED_DATA;
        }
        return 0;
}
uint8_t AX88279::BeginPTP(bool enablePps) {
        ptpReady = false;
        if(!ethernetReady) return NOT_READY;
        AX_TRY(PbusWrite(0x12, 0x0000, 0x41080001UL));
        AX_TRY(PbusWrite(0x12, 0x1040, 500000000UL));
        AX_TRY(PbusWrite(0x12, 0x1000, 0x5f));
        AX_TRY(PbusWrite(0x12, 0x1030, 0xccccccccUL));
        AX_TRY(PbusWrite(0x12, 0x1034, 10));
        uint8_t bfm;
        AX_TRY(ControlRead(1, 0xc1, 1, 1, &bfm));
        AX_TRY(MacWrite(0xc1, 1, bfm | 0xc0));
        AX_TRY(PbusWrite(0x13, 0x1008, 0x070622UL));
        AX_TRY(PbusWrite(0x12, 0x2000, 0x00017ff1UL));
        AX_TRY(PbusWrite(0x12, 0x3000, 0x08017ff1UL));
        AX_TRY(PbusWrite(0x12, 0x1038, 0));
        AX_TRY(PbusWrite(0x12, 0x103c, 0));
        AX_TRY(ControlWrite(0x15, 6, 0, 1, 0x50));
        AX_TRY(PbusWrite(0x13, 0x3004, 0x003f0100UL));
        uint32_t value;
        AX_TRY(PbusRead(0x12, 0x1000, value));
        if(value != 0x5f) return NOT_READY;
        uint8_t clock[10];
        AX_TRY(ControlRead(0x13, 2, 0x1014, sizeof(clock), clock));
        AX88279Time time;
        if(!DecodeTime(clock, time)) return MALFORMED_DATA;
        ptpReady = true;
        uint8_t rc = EnablePPS(enablePps);
        if(rc) ptpReady = false;
        return rc;
}
uint8_t AX88279::ReadClock(AX88279Time &time) {
        if(!ptpReady) return NOT_READY;
        uint8_t data[10]; AX_TRY(ControlRead(0x13, 2, 0x1014, sizeof(data), data));
        return DecodeTime(data, time) ? 0 : MALFORMED_DATA;
}
uint8_t AX88279::SetClock(const AX88279Time &time) {
        if(!ptpReady) return NOT_READY;
        if(time.nanoseconds >= 1000000000UL || (time.seconds >> 48)) return USB_ERROR_INVALID_ARGUMENT;
        uint8_t data[12] = {};
        WriteLE32(data, time.nanoseconds);
        for(uint8_t i = 0; i < 6; ++i) data[4 + i] = time.seconds >> (8 * i);
        return pUsb->ctrlReq(address, 0, 0x40, 0x11, 8, 0x10, 0x12, 12, 12, data, NULL);
}
uint8_t AX88279::SetFrequency(int32_t ppb) {
        if(!ptpReady) return NOT_READY;
        if(ppb < -100000L || ppb > 100000L) return USB_ERROR_INVALID_ARGUMENT;
        uint32_t addend = 0xccccccccUL + int64_t(0xccccccccUL) * ppb / 1000000000LL;
        return PbusWrite(0x12, 0x1030, addend);
}
uint8_t AX88279::EnablePPS(bool enable) {
        if(!ptpReady) return NOT_READY;
        uint32_t gate;
        AX_TRY(PbusRead(0x0c, 0xf8c8, gate));
        AX_TRY(PbusWrite(0x0c, 0xf8c8, enable ? gate | 0x4000UL : gate & ~0x4000UL));
        AX_TRY(PbusRead(0x0c, 0xf8c8, gate));
        return bool(gate & 0x4000UL) == enable ? 0 : NOT_READY;
}
uint8_t AX88279::ReceiveTimestamps(AX88279Buffer &buffer) {
        return ptpReady ? ReadTransfer(4, buffer) : NOT_READY;
}
uint8_t AX88279::ParseTimestamps(const uint8_t *data, uint16_t size,
                                AX88279TimestampCallback callback, void *context) {
        if(!data || size != 131) return MALFORMED_DATA;
        uint8_t base = (data[130] & 1) ? 0 : 65;
        for(uint8_t pass = 0; pass < 2; ++pass) {
                for(uint8_t i = 0; i < 5; ++i) {
                        const uint8_t *record = data + base + i * 13;
                        if(!(record[0] & 8)) continue;
                        AX88279Time time;
                        if(!DecodeTime(record + 3, time)) return MALFORMED_DATA;
                        if(pass && callback) callback(context, record[0] >> 4,
                                uint16_t(record[1]) | (uint16_t(record[2]) << 8), time);
                }
        }
        return 0;
}
#undef AX_TRY
