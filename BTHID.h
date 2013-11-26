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

#ifndef _bthid_h_
#define _bthid_h_

#include "BTD.h"
#include "hidboot.h"

/* Bluetooth L2CAP states for L2CAP_task() */
#define L2CAP_WAIT                      0

// These states are used if the device is the host
#define L2CAP_CONTROL_SUCCESS           1
#define L2CAP_INTERRUPT_SETUP           2

// These states are used if the Arduino is the host
#define L2CAP_CONTROL_CONNECT_REQUEST   3
#define L2CAP_CONTROL_CONFIG_REQUEST    4
#define L2CAP_INTERRUPT_CONNECT_REQUEST 5

#define L2CAP_INTERRUPT_CONFIG_REQUEST  6
#define L2CAP_DONE                      7

#define L2CAP_INTERRUPT_DISCONNECT      8
#define L2CAP_CONTROL_DISCONNECT        9

/* L2CAP event flags */
#define L2CAP_FLAG_CONTROL_CONNECTED                0x01
#define L2CAP_FLAG_INTERRUPT_CONNECTED              0x02
#define L2CAP_FLAG_CONFIG_CONTROL_SUCCESS           0x04
#define L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS         0x08
#define L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE      0x10
#define L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE    0x20
#define L2CAP_FLAG_CONNECTION_CONTROL_REQUEST       0x40
#define L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST     0x80

/* Macros for L2CAP event flag tests */
#define l2cap_connected_control_flag (l2cap_event_flag & L2CAP_FLAG_CONTROL_CONNECTED)
#define l2cap_connected_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_INTERRUPT_CONNECTED)
#define l2cap_config_success_control_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_CONTROL_SUCCESS)
#define l2cap_config_success_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS)
#define l2cap_disconnect_response_control_flag (l2cap_event_flag & L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE)
#define l2cap_disconnect_response_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE)
#define l2cap_connection_request_control_flag (l2cap_event_flag & L2CAP_FLAG_CONNECTION_CONTROL_REQUEST)
#define l2cap_connection_request_interrupt_flag (l2cap_event_flag & L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST)

#define KEYBOARD_PARSER_ID      0
#define MOUSE_PARSER_ID         1
#define epMUL                   2

/** This BluetoothService class implements support for the HID keyboard and mice. */
class BTHID : public BluetoothService {
public:
        /**
         * Constructor for the BTHID class.
         * @param  p   Pointer to the BTD class instance.
         * @param  pair   Set this to true in order to pair with the device. If the argument is omitted then it will not pair with it. One can use ::PAIR to set it to true.
         * @param  pin   Write the pin to BTD#btdPin. If argument is omitted, then "0000" will be used.
         */
        BTHID(BTD *p, bool pair = false, const char *pin = "0000");

        /** @name BluetoothService implementation */
        /**
         * Used to pass acldata to the services.
         * @param ACLData Incoming acldata.
         */
        virtual void ACLData(uint8_t* ACLData);
        /** Used to run part of the state maschine. */
        virtual void Run();
        /** Use this to reset the service. */
        virtual void Reset();
        /** Used this to disconnect any of the controllers. */
        virtual void disconnect();
        /**@}*/

        HIDReportParser *GetReportParser(uint8_t id) {
                return pRptParser[id];
        };

        bool SetReportParser(uint8_t id, HIDReportParser *prs) {
                pRptParser[id] = prs;
                return true;
        };

        void setProtocolMode(uint8_t mode) {
                protocolMode = mode;
        };

        /** Used to set the leds on a keyboard */
        void setLeds(uint8_t data);

        /** True if a device is connected */
        bool connected;

        /** Call this to start the paring sequence with a controller */
        void pair(void) {
                if (pBtd)
                        pBtd->pairWithHID();
        };

        /**
         * Used to call your own function when the controller is successfully initialized.
         * @param funcOnInit Function to call.
         */
        void attachOnInit(void (*funcOnInit)(void)) {
                pFuncOnInit = funcOnInit;
        };

private:
        BTD *pBtd; // Pointer to BTD instance

        HIDReportParser *pRptParser[epMUL];

        /** Set report protocol. */
        void setProtocol();
        uint8_t protocolMode;

        /**
         * Called when the controller is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        void onInit() {
                if (pFuncOnInit)
                        pFuncOnInit(); // Call the user function
        }
        void (*pFuncOnInit)(void); // Pointer to function called in onInit()

        void L2CAP_task(); // L2CAP state machine

        /* Variables filled from HCI event management */
        uint16_t hci_handle;
        bool activeConnection; // Used to indicate if it's already has established a connection

        /* Variables used by high level L2CAP task */
        uint8_t l2cap_state;
        uint8_t l2cap_event_flag; // l2cap flags of received Bluetooth events

        /* L2CAP Channels */
        uint8_t control_scid[2]; // L2CAP source CID for HID_Control
        uint8_t control_dcid[2]; // 0x0070
        uint8_t interrupt_scid[2]; // L2CAP source CID for HID_Interrupt
        uint8_t interrupt_dcid[2]; // 0x0071
        uint8_t identifier; // Identifier for connection
};
#endif