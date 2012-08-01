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

#ifndef _rfcomm_h_
#define _rfcomm_h_

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Usb.h"
#include "confdescparser.h"

/* CSR Bluetooth data taken from descriptors */
#define BULK_MAXPKTSIZE     64 // max size for ACL data

// used in control endpoint header for HCI Commands
#define bmREQ_HCI_OUT USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE

/* Bluetooth HCI states for hci_task() */
#define HCI_INIT_STATE          0
#define HCI_RESET_STATE         1
#define HCI_BDADDR_STATE        2
#define HCI_SET_NAME_STATE      3
#define HCI_SCANNING_STATE      4
#define HCI_CONNECT_IN_STATE    5
#define HCI_REMOTE_NAME_STATE   6
#define HCI_CONNECTED_STATE     7
#define HCI_DISABLE_SCAN        8
#define HCI_DONE_STATE          9
#define HCI_DISCONNECT_STATE    10

/* HCI event flags*/
#define HCI_FLAG_CMD_COMPLETE           0x01
#define HCI_FLAG_CONN_COMPLETE          0x02
#define HCI_FLAG_DISCONN_COMPLETE       0x04
#define HCI_FLAG_REMOTE_NAME_COMPLETE   0x08
#define HCI_FLAG_INCOMING_REQUEST       0x10
#define HCI_FLAG_READ_BDADDR            0x20

/*Macros for HCI event flag tests */
#define hci_cmd_complete (hci_event_flag & HCI_FLAG_CMD_COMPLETE)
#define hci_connect_complete (hci_event_flag & HCI_FLAG_CONN_COMPLETE)
#define hci_disconnect_complete (hci_event_flag & HCI_FLAG_DISCONN_COMPLETE)
#define hci_remote_name_complete (hci_event_flag & HCI_FLAG_REMOTE_NAME_COMPLETE)
#define hci_incoming_connect_request (hci_event_flag & HCI_FLAG_INCOMING_REQUEST)
#define hci_read_bdaddr_complete (hci_event_flag & HCI_FLAG_READ_BDADDR)

/* HCI Events managed */
#define EV_COMMAND_COMPLETE                         0x0E
#define EV_COMMAND_STATUS                           0x0F
#define EV_CONNECT_COMPLETE                         0x03
#define EV_DISCONNECT_COMPLETE                      0x05
#define EV_NUM_COMPLETE_PKT                         0x13
#define EV_INQUIRY_COMPLETE                         0x01
#define EV_INQUIRY_RESULT                           0x02
#define EV_REMOTE_NAME_COMPLETE                     0x07
#define EV_INCOMING_CONNECT                         0x04
#define EV_ROLE_CHANGED                             0x12
#define EV_PAGE_SCAN_REP_MODE                       0x20
#define EV_DATA_BUFFER_OVERFLOW                     0x1A
#define EV_LOOPBACK_COMMAND                         0x19
#define EV_CHANGE_CONNECTION_LINK                   0x09
#define EV_AUTHENTICATION_COMPLETE                  0x06
#define EV_MAX_SLOTS_CHANGE                         0x1B
#define EV_PIN_CODE_REQUEST                         0x16
#define EV_LINK_KEY_REQUEST                         0x17
#define EV_QOS_SETUP_COMPLETE                       0x0D
#define EV_LINK_KEY_NOTIFICATION                    0x18
#define EV_ENCRYPTION_CHANGE                        0x08
#define EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE 0x0C

/* Bluetooth L2CAP states for SDP_task() and RFCOMM_task() */
#define L2CAP_SDP_WAIT                  0
#define L2CAP_SDP_SETUP                 1
#define L2CAP_SDP_REQUEST               2
#define L2CAP_SDP_SUCCESS               3
#define L2CAP_SDP_DONE                  4
#define L2CAP_RFCOMM_WAIT               5
#define L2CAP_RFCOMM_SETUP              6
#define L2CAP_RFCOMM_REQUEST            7
#define L2CAP_RFCOMM_SUCCESS            8
#define L2CAP_RFCOMM_DONE               9
#define L2CAP_DISCONNECT_RESPONSE       10

/* L2CAP event flags */
#define L2CAP_FLAG_CONNECTION_SDP_REQUEST       0x001
#define L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST    0x002
#define L2CAP_FLAG_CONFIG_SDP_REQUEST           0x004
#define L2CAP_FLAG_CONFIG_RFCOMM_REQUEST        0x008
#define L2CAP_FLAG_CONFIG_SDP_SUCCESS           0x010
#define L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS        0x020
#define L2CAP_FLAG_DISCONNECT_SDP_REQUEST       0x040
#define L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST    0x080
#define L2CAP_FLAG_DISCONNECT_RESPONSE          0x100

/* Macros for L2CAP event flag tests */
#define l2cap_connection_request_sdp_flag (l2cap_event_flag & L2CAP_FLAG_CONNECTION_SDP_REQUEST)
#define l2cap_connection_request_rfcomm_flag (l2cap_event_flag & L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST)
#define l2cap_config_request_sdp_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_SDP_REQUEST)
#define l2cap_config_request_rfcomm_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_RFCOMM_REQUEST)
#define l2cap_config_success_sdp_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_SDP_SUCCESS)
#define l2cap_config_success_rfcomm_flag (l2cap_event_flag & L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS)
#define l2cap_disconnect_request_sdp_flag (l2cap_event_flag & L2CAP_FLAG_DISCONNECT_SDP_REQUEST)
#define l2cap_disconnect_request_rfcomm_flag (l2cap_event_flag & L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST)
#define l2cap_disconnect_response_flag (l2cap_event_flag & L2CAP_FLAG_DISCONNECT_RESPONSE)

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

/* Bluetooth L2CAP PSM */
#define SDP_PSM     0x01 // Service Discovery Protocol PSM Value
#define RFCOMM_PSM  0x03 // RFCOMM PSM Value

// Used For Connection Response - Remember to Include High Byte
#define PENDING     0x01
#define SUCCESSFUL  0x00

// Used to determine if it is a Bluetooth dongle
#define WI_SUBCLASS_RF      0x01 // RF Controller
#define WI_PROTOCOL_BT      0x01 // Bluetooth Programming Interface

#define BTD_MAX_ENDPOINTS   4

/* Used for SDP */
#define SDP_SERVICE_SEARCH_ATTRIBUTE_REQUEST_PDU    0x06 // See the RFCOMM specs
#define SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_PDU   0x07 // See the RFCOMM specs
#define SERIALPORT_UUID     0x1101 // See http://www.bluetooth.org/Technical/AssignedNumbers/service_discovery.htm
#define L2CAP_UUID          0x0100

/* Used for RFCOMM */
#define RFCOMM_SABM     0x2F
#define RFCOMM_UA       0x63
#define RFCOMM_UIH      0xEF
//#define RFCOMM_DM       0x0F
#define RFCOMM_DISC     0x43

#define extendAddress   0x01 // Allways 1

// Multiplexer message types 
#define BT_RFCOMM_PN_CMD     0x83
#define BT_RFCOMM_PN_RSP     0x81
#define BT_RFCOMM_MSC_CMD    0xE3
#define BT_RFCOMM_MSC_RSP    0xE1
#define BT_RFCOMM_RPN_CMD    0x93
#define BT_RFCOMM_RPN_RSP    0x91
/*
#define BT_RFCOMM_TEST_CMD   0x23
#define BT_RFCOMM_TEST_RSP   0x21
#define BT_RFCOMM_FCON_CMD   0xA3
#define BT_RFCOMM_FCON_RSP   0xA1
#define BT_RFCOMM_FCOFF_CMD  0x63
#define BT_RFCOMM_FCOFF_RSP  0x61
#define BT_RFCOMM_RLS_CMD    0x53
#define BT_RFCOMM_RLS_RSP    0x51
#define BT_RFCOMM_NSC_RSP    0x11
*/

class RFCOMM : public USBDeviceConfig, public UsbConfigXtracter {
public:
    RFCOMM(USB *p, const char* name = "Arduino", const char* pin = "1234");
    
    // USBDeviceConfig implementation
    virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
    virtual uint8_t Release();
    virtual uint8_t Poll();
    virtual uint8_t GetAddress() { return bAddress; };
    virtual bool isReady() { return bPollEnable; };    
    
    // UsbConfigXtracter implementation
	virtual void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep); 
    
    bool isWatingForConnection() { return watingForConnection; }; // Use this to indicate when it is ready for a incoming connection
    
    void disconnect(); // Used this void to disconnect the virtual serial port        
    bool connected;// Variable used to indicate if the connection is established
    
    /* Serial commands currently supported */
    void print(const char* data); // Used to send strings
    void print(uint8_t data); // Used to send single bytes
    void print(uint8_t* array, uint8_t length); // Used to send arrays
    void print(const __FlashStringHelper *); // Used to print strings stored in flash    
    
    void println(const char* data); // Include newline and carriage return
    void println(uint8_t data); // Include newline and carriage return
    void println(uint8_t* array, uint8_t length); // Include newline and carriage return
    void println(const __FlashStringHelper *); // Include newline and carriage return
    
    uint8_t available() { return rfcommAvailable; }; // Get the bytes waiting to be read
    uint8_t read(); // Used to read the buffer
    
protected:           
    /* mandatory members */
    USB *pUsb;
    uint8_t	bAddress; // device address
    EpInfo epInfo[BTD_MAX_ENDPOINTS]; //endpoint info structure
    
    uint8_t bConfNum; // configuration number
    uint8_t	bNumEP; // total number of endpoints in the configuration
    uint32_t qNextPollTime;	// next poll time
    
    #define BTD_CONTROL_PIPE 0 // Bluetooth dongles control endpoint
    static const uint8_t BTD_EVENT_PIPE; // HCI event endpoint index
    static const uint8_t BTD_DATAIN_PIPE; // ACL In endpoint index
    static const uint8_t BTD_DATAOUT_PIPE; // ACL Out endpoint index
    
    void PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr);
    
private:        
    const char* btdName;
    const char* btdPin;
    
    bool bPollEnable;
    uint8_t pollInterval;
    bool watingForConnection;
    
    /* Set true when a channel is created */
    bool SDPConnected;
    bool RFCOMMConnected;
    
    /*variables filled from HCI event management */
    uint16_t hci_handle;
    uint8_t my_bdaddr[6]; // The bluetooth dongles Bluetooth address
    uint8_t disc_bdaddr[6]; // the bluetooth address is always 6 bytes
    uint8_t remote_name[30]; // first 30 chars of remote name
    
    /* variables used by high level HCI task */    
    uint8_t hci_state;  //current state of bluetooth hci connection
    uint16_t hci_counter; // counter used for bluetooth hci reset loops
    uint8_t hci_num_reset_loops; // this value indicate how many times it should read before trying to reset
    uint16_t hci_event_flag;// hci flags of received bluetooth events        
    
    /* variables used by high level L2CAP task */    
    uint8_t l2cap_sdp_state;
    uint8_t l2cap_rfcomm_state;
    uint16_t l2cap_event_flag;// l2cap flags of received bluetooth events
           
    uint8_t hcibuf[BULK_MAXPKTSIZE];//General purpose buffer for hci data
    uint8_t l2capinbuf[BULK_MAXPKTSIZE];//General purpose buffer for l2cap in data
    uint8_t l2capoutbuf[BULK_MAXPKTSIZE];//General purpose buffer for l2cap out data
    uint8_t rfcommbuf[BULK_MAXPKTSIZE]; // Buffer for RFCOMM Data
    
    /* L2CAP Channels */
    uint8_t sdp_scid[2]; // L2CAP source CID for SDP           
    uint8_t sdp_dcid[2]; // 0x0050
    uint8_t rfcomm_scid[2]; // L2CAP source CID for RFCOMM        
    uint8_t rfcomm_dcid[2]; // 0x0051
    uint8_t identifier; // Identifier for command

    /* RFCOMM Variables */
    uint8_t rfcommChannel;
    uint8_t rfcommChannelPermanent;
    uint8_t rfcommDirection;
    uint8_t rfcommCommandResponse;    
    uint8_t rfcommChannelType;
    uint8_t rfcommPfBit;
    
    unsigned long timer;
    bool waitForLastCommand;
    bool creditSent;    
    
    uint8_t rfcommDataBuffer[256]; // Create a 256 sized buffer for incoming data
    uint8_t rfcommAvailable;
    
    bool firstMessage; // Used to see if it's the first SDP request received    
    uint8_t bytesReceived; // Counter to see when it's time to send more credit
    
    /* State machines */
    void HCI_event_task(); //poll the HCI event pipe
    void HCI_task(); // HCI state machine
    void ACL_event_task(); // start polling the ACL input pipe too, though discard data until connected
    void SDP_task(); // SDP state machine
    void RFCOMM_task(); // RFCOMM state machine
        
    void readReport(); // read incoming data
    void printReport(); // print incoming date - Uncomment "#define PRINTREPORT" to print incoming data debugging    
    
    /* HCI Commands */
    void HCI_Command(uint8_t* data, uint16_t nbytes);
    void hci_reset(); 
    void hci_write_scan_enable();
    void hci_write_scan_disable();
    void hci_read_bdaddr();
    void hci_accept_connection();
    void hci_remote_name();
    void hci_set_local_name(const char* name);
    void hci_pin_code_request_reply(const char* key);
    void hci_link_key_request_negative_reply();
    void hci_disconnect();
    
    /* L2CAP Commands */
    void L2CAP_Command(uint8_t* data, uint8_t nbytes, uint8_t channelLow = 0x01, uint8_t channelHigh = 0x00); // Standard L2CAP header: Channel ID (0x01) for ACL-U
    void l2cap_connection_response(uint8_t rxid, uint8_t* dcid, uint8_t* scid, uint8_t result);
    void l2cap_config_request(uint8_t rxid, uint8_t* dcid);
    void l2cap_config_response(uint8_t rxid, uint8_t* scid);
    void l2cap_disconnection_request(uint8_t rxid, uint8_t* dcid, uint8_t* scid);
    void l2cap_disconnection_response(uint8_t rxid, uint8_t* dcid, uint8_t* scid);
    void l2cap_information_response(uint8_t rxid, uint8_t infoTypeLow, uint8_t infoTypeHigh);
    
    /* SDP Commands */
    void SDP_Command(uint8_t* data, uint8_t nbytes);
    void serviceNotSupported(uint8_t transactionIDHigh, uint8_t transactionIDLow);
    void serialPortResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow);
    void serialPortResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow);
    void l2capResponse1(uint8_t transactionIDHigh, uint8_t transactionIDLow);
    void l2capResponse2(uint8_t transactionIDHigh, uint8_t transactionIDLow);
    
    /* RFCOMM Commands */
    void RFCOMM_Command(uint8_t* data, uint8_t nbytes);
    void sendRfcomm(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t* data, uint8_t length);
    void sendRfcommCredit(uint8_t channel, uint8_t direction, uint8_t CR, uint8_t channelType, uint8_t pfBit, uint8_t credit);
    uint8_t calcFcs(uint8_t *data);
    uint8_t __crc(uint8_t* data);
};
#endif