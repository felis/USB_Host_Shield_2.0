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

 Enhanced by Dmitry Pakhomenko to initiate connection with remote SPP-aware device
 04.04.2014, Magictale Electronics
 */

#ifndef _sppclient_h_
#define _sppclient_h_

#include "SPPBase.h"

/* Bluetooth L2CAP states for SDP_task() */
#define L2CAP_SDP_WAIT                  0
#define L2CAP_SDP_REQUEST               1
#define L2CAP_SDP_DONE                  2
#define L2CAP_DISCONNECT_RESPONSE       3
#define L2CAP_SDP_CONN_RESPONSE         4
#define L2CAP_SDP_CONFIG_REQUEST        5
#define L2CAP_SDP_SERVICE_SEARCH_ATTR1  6
#define L2CAP_SDP_SERVICE_SEARCH_ATTR2  7

/* Bluetooth L2CAP states for RFCOMM_task() */
#define L2CAP_RFCOMM_WAIT               0
#define L2CAP_RFCOMM_REQUEST            1
#define L2CAP_RFCOMM_DONE               3
#define L2CAP_RFCOMM_CONN_RESPONSE      4
#define L2CAP_RFCOMM_CONFIG_REQUEST     6
#define L2CAP_RFCOMM_CONFIG_RESPONSE    7

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

/**
 * This BluetoothService class a Serial Port Protocol (SPP) client.
 * It inherits the Arduino Stream class. This allows it to use all the standard Arduino print functions.
 */
class SPPClient : public SPPBase {
public:
        /**
         * Constructor for the SPPClient class.
         * @param  p     Pointer to BTD class instance.
         * @param  name  Set the name to BTD#btdName. If argument is omitted, then "Arduino" will be used.
         * @param  pin   Write the pin to BTD#btdPin. If argument is omitted, then "0000" will be used.
         * @param  pair  Set this to true if you want to pair with a device.
         * @param  addr  Set this to the address you want to connect to.
         */
        SPPClient(BTD *p, const char *name = "Arduino", const char *pin = "0000", bool pair = false, uint8_t *addr = NULL);

#if GCC_VERSION > 40700 // Test for GCC > 4.7.0 - then C++11 should be supported
        SPPClient(BTD *p, bool pair = false, uint8_t *addr = NULL) : SPPClient(p, "Arduino", "0000", pair, addr) {}; // Use a delegating constructor
#endif

        /** @name SPPBase implementation */
        /**
         * Used to pass acldata to the services.
         * @param ACLData Incoming acldata.
         */
        virtual void ACLData(uint8_t *ACLData);
        /** Used to establish the connection automatically. */
        virtual void Run();
        /** Use this to reset the service. */
        virtual void Reset();
        /**@}*/

private:
        uint8_t remainingBytes;
        uint8_t rfcomm_uuid_sign_idx; // A progressing index while searching for "RFCOMM UUID" signature, starts from 0 (nothing found) and ends with 5 (found full sequence)
        uint8_t rfcomm_found;

        /* SDP Commands */
        virtual void SDP_Command(uint8_t *data, uint8_t nbytes);
        void SDP_Service_Search_Attr(uint8_t transactionIDHigh, uint8_t transactionIDLow, uint8_t remainingLen = 0);

        /* RFCOMM Commands */
        virtual void RFCOMM_Command(uint8_t *data, uint8_t nbytes); // Used for RFCOMM commands
        void parseAttrReply(uint8_t *l2capinbuf);
};
#endif