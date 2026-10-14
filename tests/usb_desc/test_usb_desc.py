#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test the example's parser and unknown-descriptor printer without USB hardware.

Run with Python 3 and clang++ (AddressSanitizer/UndefinedBehaviorSanitizer):
    python3 tests/usb_desc/test_usb_desc.py [repository-root]

USB transfers and the known-descriptor printers are mocked. The two functions
under test are extracted unchanged from USB_desc.ino.
"""
import pathlib
import subprocess
import sys
import tempfile

repo = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parents[2]
source = (repo / 'examples/USB_desc/USB_desc.ino').read_text()

def function(signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

preamble = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#define _usb_h_
#include "usb_ch9.h"
#define PROGMEM
#include "examples/USB_desc/pgmstrings.h"
#define USB_ERROR_FailGetConfDescr 0xE3
struct HubDescriptor { uint8_t data[7]; };
std::string output;
int printed;
void printProgStr(const char *s) { output += s; }
void print_hex(int value, int) {
  const char *digits = "0123456789ABCDEF";
  output += digits[(value >> 4) & 15];
  output += digits[value & 15];
}
void printconfdescr(uint8_t *) { ++printed; }
void printintfdescr(uint8_t *) { ++printed; }
void printepdescr(uint8_t *) { ++printed; }
void printhubdescr(uint8_t *, uint8_t) { ++printed; }
struct MockUsb {
  std::vector<uint8_t> header, body;
  uint8_t errors[2] = {0, 0};
  int calls = 0;
  uint8_t getConfDescr(uint8_t, uint8_t, uint16_t n, uint8_t, uint8_t *out) {
    assert(calls < 2);
    auto &v = calls == 0 ? header : body;
    uint8_t error = errors[calls++];
    if (!error) std::copy_n(v.begin(), std::min<size_t>(n, v.size()), out);
    return error;
  }
} Usb;
'''
checks = r'''
int tests = 0;
void prepare(std::vector<uint8_t> data, unsigned total = 0) {
  if (!total) total = data.size();
  data[2] = total & 255; data[3] = total >> 8;
  Usb = MockUsb();
  Usb.header.assign(data.begin(), data.begin() + 4);
  Usb.body = data;
  printed = 0; output.clear();
}
std::vector<uint8_t> config() { return {9, 2, 9, 0, 1, 1, 0, 0x80, 50}; }
void expect(uint8_t rc, int count) {
  assert(getconfdescr(1, 0) == rc);
  assert(printed == count);
  ++tests;
}
int main() {
  // Exact allocations make the former unknown-descriptor overread visible to ASan.
  {
    uint8_t unknown[] = {4, 0x30, 0xAB, 0xCD};
    printunkdescr(unknown);
    assert(output.substr(output.find("Contents:\t") + 10) == "ABCD");
    output.clear(); ++tests;
    uint8_t empty[] = {2, 0x30};
    printunkdescr(empty);
    assert(output.substr(output.find("Contents:\t") + 10).empty());
    ++tests;
  }
  prepare(config()); expect(0, 1);
  auto valid = config();
  valid.insert(valid.end(), {9, 4, 0, 0, 1, 0xFF, 0, 0, 0,
                            7, 5, 0x81, 2, 64, 0, 1,
                            7, 0x29, 1, 0, 0, 0, 0, 4, 0x30, 0xAB, 0xCD});
  prepare(valid); expect(0, 4);
  prepare(config()); Usb.errors[0] = 0x0D; expect(0x0D, 0); assert(Usb.calls == 1);
  prepare(config()); Usb.errors[1] = 0x0E; expect(0x0E, 0);
  prepare(config()); Usb.header.resize(1); expect(0xE3, 0);
  prepare(config()); Usb.body.clear(); expect(0xE3, 0);
  prepare(config()); Usb.header[0] = 8; expect(0xE3, 0);
  prepare(config()); Usb.header[1] = 3; expect(0xE3, 0);
  prepare(config(), 8); expect(0xE3, 0);
  for (unsigned len : {0, 1}) {
    auto data = config(); data.insert(data.end(), {(uint8_t)len, 0x30});
    prepare(data); expect(0xE3, 1);
  }
  for (unsigned type : {2, 4, 5, 0x29}) {
    auto data = config(); data.insert(data.end(), {2, (uint8_t)type});
    prepare(data); expect(0xE3, 1);
  }
  auto data = config(); data.push_back(7);
  prepare(data); expect(0xE3, 1); // Missing type byte.
  data.push_back(5);
  prepare(data); expect(0xE3, 1); // Descriptor extends beyond total length.
  for (unsigned end : {253, 255, 256}) {
    data = config(); data.resize(end, 0xAB);
    data[9] = end - 9; data[10] = 0x30;
    data.insert(data.end(), {7, 5, 0x81, 2, 64, 0, 1});
    prepare(data); expect(0, 1); // Capped buffer: split body, split header, exact boundary.
    assert(output.find(Conf_Trunc_str) != std::string::npos);
  }
  data = config(); data.resize(256, 0xAB); data[9] = 247; data[10] = 0x30;
  prepare(data); expect(0, 1); // Full buffer with no truncation.
  std::cout << tests << " descriptor regression cases passed\n";
}
'''
text = preamble + '\n' + function('void printunkdescr( uint8_t* descr_ptr )') + '\n' + function('uint8_t getconfdescr( uint8_t addr, uint8_t conf )') + checks
with tempfile.TemporaryDirectory() as tmp:
    cpp = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    cpp.write_text(text)
    subprocess.run(['clang++', '-std=c++11', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-I', str(repo), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=10)
