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

#ifndef _btd_h_
#define _btd_h_

#include "Usb.h"
#include "confdescparser.h"

//PID and VID of the Sony PS3 devices
#define PS3_VID                 0x054C  // Sony Corporation
#define PS3_PID                 0x0268  // PS3 Controller DualShock 3
#define PS3NAVIGATION_PID       0x042F  // Navigation controller
#define PS3MOVE_PID             0x03D5  // Motion controller

#define IOGEAR_GBU521_VID       0x0A5C // The IOGEAR GBU521 dongle does not presents itself correctly, so we have to check for it manually
#define IOGEAR_GBU521_PID       0x21E8

/* Bluetooth dongle data taken from descriptors */
#define BULK_MAXPKTSIZE         64 // max size for ACL data

// Used in control endpoint header for HCI Commands
#define bmREQ_HCI_OUT USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
// Used in control endpoint header for HID Commands
#define bmREQ_HID_OUT USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define HID_REQUEST_SET_REPORT      0x09

/* Bluetooth HCI states for hci_task() */
#define HCI_INIT_STATE          0
#define HCI_RESET_STATE         1
#define HCI_CLASS_STATE         2
#define HCI_BDADDR_STATE        3
#define HCI_LOCAL_VERSION_STATE 4
#define HCI_SET_NAME_STATE      5
#define HCI_CHECK_WII_SERVICE   6

#define HCI_INQUIRY_STATE       7 // These three states are only used if it should pair and connect to a Wii controller
#define HCI_CONNECT_WII_STATE   8
#define HCI_CONNECTED_WII_STATE 9

#define HCI_SCANNING_STATE      10
#define HCI_CONNECT_IN_STATE    11
#define HCI_REMOTE_NAME_STATE   12
#define HCI_CONNECTED_STATE     13
#define HCI_DISABLE_SCAN_STATE  14
#define HCI_DONE_STATE          15
#define HCI_DISCONNECT_STATE    16

/* HCI event flags*/
#define HCI_FLAG_CMD_COMPLETE           0x01
#define HCI_FLAG_CONN_COMPLETE          0x02
#define HCI_FLAG_DISCONN_COMPLETE       0x04
#define HCI_FLAG_REMOTE_NAME_COMPLETE   0x08
#define HCI_FLAG_INCOMING_REQUEST       0x10
#define HCI_FLAG_READ_BDADDR            0x20
#define HCI_FLAG_READ_VERSION           0x40
#define HCI_FLAG_WII_FOUND              0x80
#define HCI_FLAG_CONNECT_EVENT          0x100

/*Macros for HCI event flag tests */
#define hci_cmd_complete (hci_event_flag & HCI_FLAG_CMD_COMPLETE)
#define hci_connect_complete (hci_event_flag & HCI_FLAG_CONN_COMPLETE)
#define hci_disconnect_complete (hci_event_flag & HCI_FLAG_DISCONN_COMPLETE)
#define hci_remote_name_complete (hci_event_flag & HCI_FLAG_REMOTE_NAME_COMPLETE)
#define hci_incoming_connect_request (hci_event_flag & HCI_FLAG_INCOMING_REQUEST)
#define hci_read_bdaddr_complete (hci_event_flag & HCI_FLAG_READ_BDADDR)
#define hci_read_version_complete (hci_event_flag & HCI_FLAG_READ_VERSION)
#define hci_wii_found (hci_event_flag & HCI_FLAG_WII_FOUND)
#define hci_connect_event (hci_event_flag & HCI_FLAG_CONNECT_EVENT)

/* HCI Events managed */
#define EV_INQUIRY_COMPLETE                             0x01
#define EV_INQUIRY_RESULT                               0x02
#define EV_CONNECT_COMPLETE                             0x03
#define EV_INCOMING_CONNECT                             0x04
#define EV_DISCONNECT_COMPLETE                          0x05
#define EV_AUTHENTICATION_COMPLETE                      0x06
#define EV_REMOTE_NAME_COMPLETE                         0x07
#define EV_ENCRYPTION_CHANGE                            0x08
#define EV_CHANGE_CONNECTION_LINK                       0x09
#define EV_ROLE_CHANGED                                 0x12
#define EV_NUM_COMPLETE_PKT                             0x13
#define EV_PIN_CODE_REQUEST                             0x16
#define EV_LINK_KEY_REQUEST                             0x17
#define EV_LINK_KEY_NOTIFICATION                        0x18
#define EV_DATA_BUFFER_OVERFLOW                         0x1A
#define EV_MAX_SLOTS_CHANGE                             0x1B
#define EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE     0x0C
#define EV_QOS_SETUP_COMPLETE                           0x0D
#define EV_COMMAND_COMPLETE                             0x0E
#define EV_COMMAND_STATUS                               0x0F
#define EV_LOOPBACK_COMMAND                             0x19
#define EV_PAGE_SCAN_REP_MODE                           0x20

/* L2CAP signaling commands */
#define L2CAP_CMD_COMMAND_REJECT        0x01
#define L2CAP_CMD_CONNECTION_REQUEST    0x02
#define L2CAP_CMD_CONNECTION_RESPONSE   0x03
#define L2CAP_CMD_CONFIG_REQUEST        0x04
#define L2CAP_CMD_CONFIG_RESPONSE       0x05
#define L2CAP_CMD_DISCONNECT_REQUEST    0x06
#define L2CAP_CMD_DISCONNECT_RESPONSE   0x07
#define L2CAP_CMD_INFORMATION_REQUEST   0x0A
#define L2CAP_CMD_INFORMATION_RESPONSE  0x0B

// Used For Connection Response - Remember to Include High Byte
#define PENDING     0x01
#define SUCCESSFUL  0x00

/* Bluetooth L2CAP PSM - see http://www.bluetooth.org/Technical/AssignedNumbers/logical_link.htm */
#define SDP_PSM         0x01 // Service Discovery Protocol PSM Value
#define RFCOMM_PSM      0x03 // RFCOMM PSM Value
#define HID_CTRL_PSM    0x11 // HID_Control PSM Value
#define HID_INTR_PSM    0x13 // HID_Interrupt PSM Value

// Used to determine if it is a Bluetooth dongle
#define WI_SUBCLASS_RF      0x01 // RF Controller
#define WI_PROTOCOL_BT      0x01 // Bluetooth Programming Interface

#define BTD_MAX_ENDPOINTS   4
#define BTD_NUMSERVICES     4 // Max number of Bluetooth services - if you need more than four simply increase this number

/** All Bluetooth services should include this class. */
class BluetoothService {
public:
        /**
         * Used to pass acldata to the Bluetooth service.
         * @param ACLData Pointer to the incoming acldata.
         */
        virtual void ACLData(uint8_t* ACLData);
        /** Used to run the different state machines in the Bluetooth service. */
        virtual void Run();
        /** Used to reset the Bluetooth service. */
        virtual void Reset();
        /** Used to disconnect both the L2CAP Channel and the HCI Connection for the Bluetooth service. */
        virtual void disconnect();
};

/**
 * The Bluetooth Dongle class will take care of all the USB communication
 * and then pass the data to the BluetoothService classes.
 */
class BTD : public USBDeviceConfig, public UsbConfigXtracter {
public:
        /**
         * Constructor for the BTD class.
         * @param  p   Pointer to USB class instance.
         */
        BTD(USB *p);

        /** @name USBDeviceConfig implementation */
        /**
         * Initialize the Bluetooth dongle.
         * @param  parent   Hub number.
         * @param  port     Port number on the hub.
         * @param  lowspeed Speed of the device.
         * @return          0 on success.
         */
        virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        /**
         * Release the USB device.
         * @return 0 on success.
         */
        virtual uint8_t Release();
        /**
         * Poll the USB Input endpoins and run the state machines.
         * @return 0 on success.
         */
        virtual uint8_t Poll();

        /**
         * Get the device address.
         * @return The device address.
         */
        virtual uint8_t GetAddress() {
                return bAddress;
        };

        /**
         * Used to check if the dongle has been initialized.
         * @return True if it's ready.
         */
        virtual bool isReady() {
                return bPollEnable;
        };
        /**
         * Used by the USB core to check what this driver support.
         * @param  klass The device's USB class.
         * @return       Returns true if the device's USB class matches this driver.
         */
        virtual boolean DEVCLASSOK(uint8_t klass) { return (klass == USB_CLASS_WIRELESS_CTRL); }

        /**
         * Used by the USB core to check what this driver support.
         * Used to set the Bluetooth address into the PS3 controllers.
         * @param  vid The device's VID.
         * @param  pid The device's PID.
         * @return     Returns true if the device's VID and PID matches this driver.
         */
        virtual boolean VIDPIDOK(uint16_t vid, uint16_t pid) {
                return ((vid == PS3_VID || vid == IOGEAR_GBU521_VID) && (pid == PS3_PID || pid == PS3NAVIGATION_PID || pid == PS3MOVE_PID || pid == IOGEAR_GBU521_PID));
        };
        /**@}*/

        /** @name UsbConfigXtracter implementation */
        /**
         * UsbConfigXtracter implementation, used to extract endpoint information.
         * @param conf  Configuration value.
         * @param iface Interface number.
         * @param alt   Alternate setting.
         * @param proto Interface Protocol.
         * @param ep    Endpoint Descriptor.
         */
        virtual void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep);
        /**@}*/

        /** Disconnects both the L2CAP Channel and the HCI Connection for all Bluetooth services. */
        void disconnect() {
                for(uint8_t i = 0; i < BTD_NUMSERVICES; i++)
                        if(btService[i])
                                btService[i]->disconnect();
        };

        /**
         * Register bluetooth dongle members/services.
         * @param  pService Pointer to BluetoothService class instance.
         * @return          The serice ID on succes or -1 on fail.
         */
        int8_t registerServiceClass(BluetoothService *pService) {
                for(uint8_t i = 0; i < BTD_NUMSERVICES; i++) {
                        if(!btService[i]) {
                                btService[i] = pService;
                                return i; // Return ID
                        }
                }
                return -1; // ErrorregisterServiceClass
        };

        /** @name HCI Commands */
        /**
         * Used to send a HCI Command.
         * @param data   Data to send.
         * @param nbytes Number of bytes to send.
         */
        void HCI_Command(uint8_t* data, uint16_t nbytes);
        /** Reset the Bluetooth dongle. */
        void hci_reset();
        /** Read the Bluetooth address of the dongle. */
        void hci_read_bdaddr();
        /** Read the HCI Version of the Bluetooth dongle. */
        void hci_read_local_version_information();
        /**
         * Set the local name of the Bluetooth dongle.
         * @param name Desired name.
         */
        void hci_set_local_name(const char* name);
        /** Enable visibility to other Bluetooth devices. */
        void hci_write_scan_enable();
        /** Disable visibility to other Bluetooth devices. */
        void hci_write_scan_disable();
        /** Read the remote devices name. */
        void hci_remote_name();
        /** Accept the connection with the Bluetooth device. */
        void hci_accept_connection();
        /**
         * Disconnect the HCI connection.
         * @param handle The HCI Handle for the connection.
         */
        void hci_disconnect(uint16_t handle);
        /**
         * Respond with the pin for the connection.
         * The pin is automatically set for the Wii library,
         * but can be customized for the SPP library.
         */
        void hci_pin_code_request_reply();
        /** Respons when no pin was set. */
        void hci_pin_code_negative_request_reply();
        /**
         * Command is used to reply to a Link Key Request event from the BR/EDR Controller
         * if the Host does not have a stored Link Key for the connection.
         */
        void hci_link_key_request_negative_reply();
        /** Used to try to authenticate with the remote device. */
        void hci_authentication_request();
        /** Start a HCI inquiry. */
        void hci_inquiry();
        /** Cancel a HCI inquiry. */
        void hci_inquiry_cancel();
        /** Connect to a device. */
        void hci_connect();
        /** Used to a set the class of the device. */
        void hci_write_class_of_device();
        /**@}*/

        /** @name L2CAP Commands */
        /**
         * Used to send L2CAP Commands.
         * @param handle      HCI Handle.
         * @param data        Data to send.
         * @param nbytes      Number of bytes to send.
         * @param channelLow,channelHigh  Low and high byte of channel to send to.
         * If argument is omitted then the Standard L2CAP header: Channel ID (0x01) for ACL-U will be used.
         */
        void L2CAP_Command(uint16_t handle, uint8_t* data, uint8_t nbytes, uint8_t channelLow = 0x01, uint8_t channelHigh = 0x00);
        /**
         * L2CAP Connection Request.
         * @param handle HCI handle.
         * @param rxid   Identifier.
         * @param scid   Source Channel Identifier.
         * @param psm    Protocol/Service Multiplexer - see: https://www.bluetooth.org/Technical/AssignedNumbers/logical_link.htm.
         */
        void l2cap_connection_request(uint16_t handle, uint8_t rxid, uint8_t* scid, uint16_t psm);
        /**
         * L2CAP Connection Response.
         * @param handle HCI handle.
         * @param rxid   Identifier.
         * @param dcid   Destination Channel Identifier.
         * @param scid   Source Channel Identifier.
         * @param result Result - First send ::PENDING and then ::SUCCESSFUL.
         */
        void l2cap_connection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid, uint8_t result);
        /**
         * L2CAP Config Request.
         * @param handle HCI Handle.
         * @param rxid   Identifier.
         * @param dcid   Destination Channel Identifier.
         */
        void l2cap_config_request(uint16_t handle, uint8_t rxid, uint8_t* dcid);
        /**
         * L2CAP Config Response.
         * @param handle HCI Handle.
         * @param rxid   Identifier.
         * @param scid   Source Channel Identifier.
         */
        void l2cap_config_response(uint16_t handle, uint8_t rxid, uint8_t* scid);
        /**
         * L2CAP Disconnection Request.
         * @param handle HCI Handle.
         * @param rxid   Identifier.
         * @param dcid   Device Channel Identifier.
         * @param scid   Source Channel Identifier.
         */
        void l2cap_disconnection_request(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid);
        /**
         * L2CAP Disconnection Response.
         * @param handle HCI Handle.
         * @param rxid   Identifier.
         * @param dcid   Device Channel Identifier.
         * @param scid   Source Channel Identifier.
         */
        void l2cap_disconnection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid);
        /**
         * L2CAP Information Response.
         * @param handle       HCI Handle.
         * @param rxid         Identifier.
         * @param infoTypeLow,infoTypeHigh  Infotype.
         */
        void l2cap_information_response(uint16_t handle, uint8_t rxid, uint8_t infoTypeLow, uint8_t infoTypeHigh);
        /**@}*/

        /** Use this to see if it is waiting for a incoming connection. */
        bool watingForConnection;
        /** This is used by the service to know when to store the device information. */
        bool l2capConnectionClaimed;
        /** This is used by the SPP library to claim the current SDP incoming request. */
        bool sdpConnectionClaimed;
        /** This is used by the SPP library to claim the current RFCOMM incoming request. */
        bool rfcommConnectionClaimed;

        /** The name you wish to make the dongle show up as. It is set automatically by the SPP library. */
        const char* btdName;
        /** The pin you wish to make the dongle use for authentication. It is set automatically by the SPP library. */
        const char* btdPin;

        /** The bluetooth dongles Bluetooth address. */
        uint8_t my_bdaddr[6];
        /** HCI handle for the last connection. */
        uint16_t hci_handle;
        /** Last incoming devices Bluetooth address. */
        uint8_t disc_bdaddr[6];
        /** First 30 chars of last remote name. */
        uint8_t remote_name[30];
        /**
         * The supported HCI Version read from the Bluetooth dongle.
         * Used by the PS3BT library to check the HCI Version of the Bluetooth dongle,
         * it should be at least 3 to work properly with the library.
         */
        uint8_t hci_version;

        /** Call this function to pair with a Wiimote */
        void pairWithWiimote() {
                pairWithWii = true;
                hci_state = HCI_CHECK_WII_SERVICE;
        };
        /** Used to only send the ACL data to the wiimote. */
        bool connectToWii;
        /** True if a Wiimote is connecting. */
        bool incomingWii;
        /** True when it should pair with the incoming Wiimote. */
        bool pairWithWii;
        /** True if it's the new Wiimote with the Motion Plus Inside or a Wii U Pro Controller. */
        bool motionPlusInside;
        /** True if it's a Wii U Pro Controller. */
        bool wiiUProController;

        /**
         * Read the poll interval taken from the endpoint descriptors.
         * @return The poll interval in ms.
         */
        uint8_t readPollInterval() {
                return pollInterval;
        };

protected:
        /** Pointer to USB class instance. */
        USB *pUsb;
        /** Device address. */
        uint8_t bAddress;
        /** Endpoint info structure. */
        EpInfo epInfo[BTD_MAX_ENDPOINTS];

        /** Configuration number. */
        uint8_t bConfNum;
        /** Total number of endpoints in the configuration. */
        uint8_t bNumEP;
        /** Next poll time based on poll interval taken from the USB descriptor. */
        uint32_t qNextPollTime;

        /** Bluetooth dongle control endpoint. */
        static const uint8_t BTD_CONTROL_PIPE;
        /** HCI event endpoint index. */
        static const uint8_t BTD_EVENT_PIPE;
        /** ACL In endpoint index. */
        static const uint8_t BTD_DATAIN_PIPE;
        /** ACL Out endpoint index. */
        static const uint8_t BTD_DATAOUT_PIPE;

        /**
         * Used to print the USB Endpoint Descriptor.
         * @param ep_ptr Pointer to USB Endpoint Descriptor.
         */
        void PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr);

private:
        BluetoothService* btService[BTD_NUMSERVICES];

        bool bPollEnable;
        uint8_t pollInterval;

        /* Variables used by high level HCI task */
        uint8_t hci_state; //current state of bluetooth hci connection
        uint16_t hci_counter; // counter used for bluetooth hci reset loops
        uint8_t hci_num_reset_loops; // this value indicate how many times it should read before trying to reset
        uint16_t hci_event_flag; // hci flags of received bluetooth events
        uint8_t inquiry_counter;

        uint8_t hcibuf[BULK_MAXPKTSIZE]; //General purpose buffer for hci data
        uint8_t l2capinbuf[BULK_MAXPKTSIZE]; //General purpose buffer for l2cap in data
        uint8_t l2capoutbuf[14]; //General purpose buffer for l2cap out data

        /* State machines */
        void HCI_event_task(); // Poll the HCI event pipe
        void HCI_task(); // HCI state machine
        void ACL_event_task(); // ACL input pipe

        /* Used to set the Bluetooth Address internally to the PS3 Controllers */
        void setBdaddr(uint8_t* BDADDR);
        void setMoveBdaddr(uint8_t* BDADDR);
};
#endif
