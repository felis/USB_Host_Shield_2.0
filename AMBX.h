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

#ifndef _ambxusb_h_
#define _ambxusb_h_

#include "Usb.h"
#include "AMBXEnums.h"

/* AMBX data taken from descriptors */
#define AMBX_EP_MAXPKTSIZE           40 // max size for data via USB

/* Names we give to the 3 AMBX but note only one is actually used (output) */
#define AMBX_CONTROL_PIPE        0
#define AMBX_OUTPUT_PIPE         1
#define AMBX_INPUT_PIPE          2

/* PID and VID of the different devices */
#define AMBX_VID                 0x0471  // Philips
#define AMBX_PID                 0x083F  // AMBX Controller

/* Endpoint addresses */
#define AMBX_ENDPOINT_IN  0x81
#define AMBX_ENDPOINT_OUT 0x02
#define AMBX_ENDPOINT_PNP 0x83

/* Output payload constants */
#define AMBX_PREFIX_COMMAND     0xA1
#define AMBX_SET_COLOR_COMMAND  0x03

/* LEFT/RIGHT lights. Normally placed adjecent to your screen. */
#define AMBX_LIGHT_LEFT  0x0B
#define AMBX_LIGHT_RIGHT 0x1B

/* Wallwasher lights. Normally placed behind your screen. */
#define AMBX_LIGHT_WW_LEFT   0x2B
#define AMBX_LIGHT_WW_CENTER 0x3B
#define AMBX_LIGHT_WW_RIGHT  0x4B

#define AMBX_LIGHT_COMMAND_BUFFER_SIZE 6


#define AMBX_MAX_ENDPOINTS       3

/**
 * This class implements support for AMBX
 * One can only set the color of the bulbs, no other accesories like rumble pad, fans, etc. are supported
 *
 */
class AMBX : public USBDeviceConfig {
public:
        /**
         * Constructor for the AMBX class.
         * @param  pUsb   Pointer to USB class instance.
         */
        AMBX(USB *pUsb);

        /** @name USBDeviceConfig implementation */
        /**
         * Initialize the AMBX Controller.
         * @param  parent   Hub number.
         * @param  port     Port number on the hub.
         * @param  lowspeed Speed of the device.
         * @return          0 on success.
         */
        uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        /**
         * Release the USB device.
         * @return 0 on success.
         */
        uint8_t Release();
        /**
         * Poll the USB Input endpoins and run the state machines.
         * @return 0 on success.
         */
        uint8_t Poll();

        /**
         * Get the device address.
         * @return The device address.
         */
        virtual uint8_t GetAddress() {
                return bAddress;
        };

        /**
         * Used by the USB core to check what this driver support.
         * @param  vid The device's VID.
         * @param  pid The device's PID.
         * @return     Returns true if the device's VID and PID matches this driver.
         */
        virtual bool VIDPIDOK(uint16_t vid, uint16_t pid) {
                return (vid == AMBX_VID && (pid == AMBX_PID));
        };
        /**@}*/

        /**
         * Use this to set the Color using RGB values.
         * @param r,g,b RGB value.
         */
        void setLight(uint8_t ambx_light, uint8_t r, uint8_t g, uint8_t b);
        /**
         * Use this to set the color using the predefined colors in ::ColorsEnum.
         * @param color The desired color.
         */
        void setLight(AmbxLightsEnum ambx_light, AmbxColorsEnum color);

        /**
         * Use this to set the color using the predefined colors in ::ColorsEnum.
         * @param color The desired color.
         */
        void setAllLights(AmbxColorsEnum color);

        /**
         * Used to call your own function when the controller is successfully initialized.
         * @param funcOnInit Function to call.
         */
        void attachOnInit(void (*funcOnInit)(void)) {
                pFuncOnInit = funcOnInit;
        };
        /**@}*/

        bool AMBXConnected;

protected:
        /** Pointer to USB class instance. */
        USB *pUsb;
        /** Device address. */
        uint8_t bAddress;
        /** Endpoint info structure. */
        EpInfo epInfo[AMBX_MAX_ENDPOINTS];

private:
        /**
         * Called when the AMBX controller is successfully initialized.
         */
        void onInit();
        void (*pFuncOnInit)(void); // Pointer to function called in onInit()

        uint8_t writeBuf[AMBX_EP_MAXPKTSIZE]; // General purpose buffer for output data

        /* Private commands */
        void Light_Command(uint8_t *data, uint16_t nbytes);
};

#endif
