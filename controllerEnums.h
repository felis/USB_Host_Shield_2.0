/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.
 
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

#ifndef _controllerenums_h
#define _controllerenums_h


enum LED {
    /* Enum used to turn on the LEDs on the different controllers */
    LED1 = 0,
    LED2 = 1,
    LED3 = 2,
    LED4 = 3,
    
    LED5 = 4,
    LED6 = 5,
    LED7 = 6,
    LED8 = 7,
    LED9 = 8,
    LED10 = 9,
};
enum Button {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3,

    /* Wii buttons */
    PLUS  = 4,
    TWO   = 5,
    ONE   = 6,
    B     = 7,
    A     = 8,
    MINUS = 9,
    HOME  = 10,    
    Z     = 11,
    C     = 12,

    /* PS3 controllers buttons */
    SELECT = 4,
    L3 = 5,
    R3 = 6,
    START = 7,    
    
    L2 = 8,
    R2 = 9,
    L1 = 10,
    R1 = 11,
    TRIANGLE = 12,
    CIRCLE = 13,
    CROSS = 14,
    SQUARE = 15,
    
    PS = 16,
    
    MOVE = 17, // covers 12 bits - we only need to read the top 8
    T = 18, // covers 12 bits - we only need to read the top 8
};

#endif