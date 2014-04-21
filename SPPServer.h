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

#ifndef _sppserver_h_
#define _sppserver_h_

#include "SPPBase.h"

/**
 * This BluetoothService class a Serial Port Protocol (SPP) server.
 * It inherits the Arduino Stream class. This allows it to use all the standard Arduino print functions.
 */
class SPPServer : public SPPBase {
public:
        /**
         * Constructor for the SPPServer class.
         * @param  p     Pointer to BTD class instance.
         * @param  name  Set the name to BTD#btdName. If argument is omitted, then "Arduino" will be used.
         * @param  pin   Write the pin to BTD#btdPin. If argument is omitted, then "0000" will be used.
         */
        SPPServer(BTD *p, const char *name = "Arduino", const char *pin = "0000");

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
        uint32_t l2cap_event_flag; // l2cap flags of received Bluetooth events

        unsigned long timer;
        bool waitForLastCommand;

        bool firstMessage; // Used to see if it's the first SDP request received

        /* State machines */
        void SDP_task(); // SDP state machine
        void RFCOMM_task(); // RFCOMM state machine

        /* SDP Commands */
        virtual void SDP_Command(uint8_t *data, uint8_t nbytes);
        void serviceNotSupported(uint8_t transactionIDHigh, uint8_t transactionIDLow);
        void serialPortResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow);
        void serialPortResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow);
        void l2capResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow);
        void l2capResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow);

        virtual void RFCOMM_Command(uint8_t *data, uint8_t nbytes); // Used for RFCOMM commands
};
#endif
