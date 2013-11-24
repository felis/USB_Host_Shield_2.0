/* Copyright (C) 2013 Kristian Lauszus, TKJ Electronics. All rights reserved.

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

/*
 This header file is used to store different enums for the controllers,
 This is necessary so all the different libraries can be used at once
 */

/** Enum used to turn on the LEDs on the different controllers. */
enum LED {
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
        /** Used to blink all LEDs on the Xbox controller */
        ALL = 4,
};

/** This enum is used to read all the different buttons on the different controllers */
enum Button {
        /**@{*/
        /** These buttons are available on all the the controllers */
        UP = 0,
        RIGHT = 1,
        DOWN = 2,
        LEFT = 3,
        /**@}*/

        /**@{*/
        /** Wii buttons */
        PLUS = 5,
        TWO = 6,
        ONE = 7,
        MINUS = 8,
        HOME = 9,
        Z = 10,
        C = 11,
        B = 12,
        A = 13,
        /**@}*/

        /**@{*/
        /** These are only available on the Wii U Pro Controller */
        L = 16,
        R = 17,
        ZL = 18,
        ZR = 19,
        /**@}*/

        /**@{*/
        /** PS3 controllers buttons */
        SELECT = 4,
        START = 5,
        L3 = 6,
        R3 = 7,

        L2 = 8,
        R2 = 9,
        L1 = 10,
        R1 = 11,
        TRIANGLE = 12,
        CIRCLE = 13,
        CROSS = 14,
        SQUARE = 15,

        PS = 16,

        MOVE = 17, // Covers 12 bits - we only need to read the top 8
        T = 18, // Covers 12 bits - we only need to read the top 8
        /**@}*/

        /**@{*/
        /** Xbox buttons */
        BACK = 4,
        X = 14,
        Y = 15,
        XBOX = 16,
        SYNC = 17,
        BLACK = 8, // Available on the original Xbox controller
        WHITE = 9, // Available on the original Xbox controller
        /**@}*/
};

/** Joysticks on the PS3 and Xbox controllers. */
enum AnalogHat {
        /** Left joystick x-axis */
        LeftHatX = 0,
        /** Left joystick y-axis */
        LeftHatY = 1,
        /** Right joystick x-axis */
        RightHatX = 2,
        /** Right joystick y-axis */
        RightHatY = 3,
};

#endif