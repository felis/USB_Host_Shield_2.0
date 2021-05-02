/* Copyright (C) 2021 Aran Vink. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Aran Vink
  e-mail   :  aranvink@gmail.com
 */

#ifndef _ambxenums_h
#define _ambxenums_h

/** Used to set the colors of the AMBX lights. This is just a limited predefined set, the lights allow ANY value between 0x00 and 0xFF */
enum AmbxColorsEnum {
        Red = 0xFF0000,
        Green = 0x00FF00,
        Blue = 0x0000FF,
        White = 0xFFFFFF,
        Off = 0x000000,
};

/** Used to select light in the AMBX system */
enum AmbxLightsEnum {
        Sidelight_left = 0x0B,
        Sidelight_right = 0x1B,
        Wallwasher_left = 0x2B,
        Wallwasher_center = 0x3B,
        Wallwasher_right = 0x4B,
};

#endif
