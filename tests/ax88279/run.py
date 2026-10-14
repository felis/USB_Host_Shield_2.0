#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Run host tests with Python 3 and clang++, without Arduino hardware."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as directory:
    dest = Path(directory)
    for name in ('ax88279.cpp', 'ax88279.h', 'usb_ch9.h'):
        shutil.copyfile(root / name, dest / name)
    for name in ('Usb.h', 'test.cpp'):
        shutil.copyfile(Path(__file__).parent / (name + '.in'), dest / name)
    subprocess.run(['clang++', '-std=c++11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g',
                    str(dest / 'ax88279.cpp'), str(dest / 'test.cpp'),
                    '-o', str(dest / 'test')], check=True)
    subprocess.run([str(dest / 'test')], check=True, timeout=30)
