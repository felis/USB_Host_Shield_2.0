# AX88279 native Ethernet and hardware clock

Experimental support for the ASIX AX88279 used by some TimeStick adapters,
through a MAX3421E USB Host Shield. This driver provides raw Ethernet transfers,
L2 PTP hardware timestamps, clock read/set/frequency adjustment, and PPS gating.
Applications supply their own Ethernet protocols and PTP servo.

## Supported configuration

- ASIX VID:PID `0b95:1790`, verified chip code **7**. This ID is shared with other
  ASIX chips; an ID match alone does not enable register writes.
- USB full speed (12 Mbit/s). Ethernet advertisement is restricted to **1000BASE-T
  full duplex**. This is the wire link rate, not the USB transfer rate.
- Native configuration 1, vendor interface 0, alternate setting 0. The driver
  validates bulk IN `0x82`, bulk OUT `0x03`, timestamp IN `0x84` (64-byte packets),
  and interrupt IN `0x81` (8 or 16 bytes). Extra transmit queues are unused.
- AX88279A (chip code 15), AX88179/B, CDC-NCM/ECM, 100 Mb/s, 2.5 Gb/s, jumbo frames,
  offloads, VLAN transformations and TCP/IP integration are not implemented.

Register operations only affect volatile device state. The driver contains no
EEPROM, eFuse, flash or firmware programming operations. USB configuration is
selected before the chip can be queried; unsupported chip codes are rejected
before the native firmware/PHY/PTP initialization sequence.

## Arduino example

Open **File > Examples > USB Host Shield Library 2.0 > AX88279**, select the board,
and upload. Connect a supported adapter to the shield, and connect its Ethernet
port to an active gigabit peer. Open the serial monitor at **115200 baud**.

The example initializes Ethernet and a free-running hardware clock, enables PPS
with a 500 ms active interval, and reports clock and frame counters once per
second. PPS success means register readback, not an electrical measurement.
Time is displayed as hexadecimal high:low seconds and decimal nanoseconds.
It has no UTC source and does not synchronize a peer.

Serial commands, for use on an isolated test link:

| Command | Diagnostic |
| --- | --- |
| `t` | Broadcast a 60-byte frame with experimental EtherType `0x88b5`. |
| `u` | Broadcast a 120-byte frame, exercising a 128-byte USB OUT transfer. |
| `p` | Send one L2 PTP Delay_Req in domain 42 to exercise hardware timestamps. |

`p` is a timestamp diagnostic, not a complete PTP exchange or follower. Received
frames show their length, EtherType, and optional hardware timestamp. TX reports
show message type and sequence ID; a PTP application must match reports to its
own outstanding messages and reject stale/duplicate reports.

The example retries initialization after failure or detected link loss. USB
reconnection also restarts it. `BeginEthernet()` blocks during autonegotiation,
with a 15-second PHY-link polling window plus control-transfer timeouts and
status polling. It is intended for startup/diagnostics, not time-critical code.

## Buffer and API contract

Instantiate `AX88279 adapter(&Usb)` before calling `Usb.Init()`. `Usb.Task()`
performs enumeration. `isReady()` means USB enumeration succeeded;
`BeginEthernet()` and `BeginPTP()` are explicit initialization steps. Call
`LinkUp()` periodically; after link loss, repeat initialization and reset both
receive buffers. Buffers must also be reset on USB disconnect/reconnect.

The driver owns endpoint state but allocates no packet heap or full-size frame
buffer. Applications supply an `AX88279Buffer` for each receive endpoint.
`Receive()` and `ReceiveTimestamps()` read at most one 64-byte USB packet per
call. A successful call can leave a partial aggregate; inspect `complete` before
parsing. `hrNAK` preserves the partial buffer. The next receive call automatically
resets a completed buffer. Do not reuse or alter storage during a partial read,
and do not call these methods recursively from a parser callback.

Oversized aggregates are drained to their USB short-packet boundary and return
`BUFFER_TOO_SMALL`, without delivering truncated frames. USB errors invalidate
the partial aggregate and drain through the next short packet before reuse.
`ParseRx()` validates the entire aggregate before delivering any callback.
Payload and timestamp pointers are borrowed and must not be retained.

The example's 256-byte RX storage suits short PTP frames and diagnostics; it will
drop larger or heavily aggregated Ethernet traffic. A maximum-size 1518-byte
frame requires at least 1552 bytes for its aligned payload, timestamp metadata
and trailer. Multiple frames need more space. Size buffers for the application
and board: a classic UNO has only 2 KB of SRAM. TX timestamp reports need 131 bytes.

`SendFrame(buffer, length, capacity)` expects the Ethernet frame at `buffer + 8`.
Reserve `8 + ((length + 7) & ~7)` bytes. The method fills the USB header and padding
in place. Length excludes FCS and must be 60..1518 bytes; pad short Ethernet frames
to 60 before calling. No checksum offload or automatic FCS bytes are supplied by
the caller. The Ethernet hardware handles the wire FCS.

`AX88279Time` preserves 48-bit seconds and nanoseconds. `SetClock()` performs an
absolute set with USB latency; it is not a precise phase-step primitive.
`SetFrequency()` accepts +/-100000 ppb. `BeginPTP()` initializes the addend to its
nominal value. `EnablePPS()` controls the board's physical PPS output gate; the
board must expose the appropriate chip output.

Return 0 indicates success. Standard USB errors pass through unchanged. Driver
errors are `BUFFER_TOO_SMALL` (E4), `MALFORMED_DATA` (E5), `NOT_READY` (E6),
`LINK_TIMEOUT` (E7), and `SHORT_READ` (E8). An error aborts initialization and
leaves the relevant readiness flag false. The example does not discipline time.

## Validation

Run `python3 tests/ax88279/run.py` with Python 3 and clang++. It builds the actual
driver against a USB transport mock with AddressSanitizer and
UndefinedBehaviorSanitizer. Tests cover chip rejection, endpoint validation,
address cleanup, short/failed transfers, oversized aggregate draining, packet
boundaries, timestamps, 48-bit clock setting, frequency limits, and 20,000
random parser inputs. Test templates use `.in` suffixes to keep host-only code
out of Arduino/PlatformIO library builds.

The Arduino example is included in the existing build matrix. Hardware test
results, including any limitations, are recorded with the pull request.

## Protocol references and license

Register sequences are adapted from the GPL-licensed ASIX driver sources:

- [ax88179a_772d.c: AX88279 link setup and native packet framing](https://github.com/Time-Appliances-Project/TimeStick/blob/master/TimeStick2/DRV/ax88179a_772d.c)
- [ax_ptp.c: AX88279 clock, timestamp reports and PPS](https://github.com/Time-Appliances-Project/TimeStick/blob/master/TimeStick2/DRV/ax_ptp.c)
- [ax_main.c: chip identification and control requests](https://github.com/Time-Appliances-Project/TimeStick/blob/master/TimeStick2/DRV/ax_main.c)
- [ASIX AX88279 product information](https://www.asix.com.tw/en/product/USBEthernet/Super-Speed_USB_Ethernet/AX88279)

The referenced driver files carry ASIX's 2022 copyright and GPL version 2 or
later notices. This contribution is distributed under GPL-2.0-or-later; see the
library's `gpl2.txt`. It does not include the TimeStick hardware design files.
