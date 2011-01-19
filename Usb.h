/* USB functions */
#ifndef _usb_h_
#define _usb_h_

#include <inttypes.h>
#include "avrpins.h"
#include "max3421e.h"
#include "usbhost.h"
#include "usb_ch9.h"
#include <WProgram.h>

/* Common setup data constant combinations  */
#define bmREQ_GET_DESCR     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE     //get descriptor request type
#define bmREQ_SET           USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE     //set request type for all but 'set feature' and 'set interface'
#define bmREQ_CL_GET_INTF   USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE     //get interface request type
/* HID requests */
#define bmREQ_HIDOUT        USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define bmREQ_HIDIN         USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE 
#define bmREQ_HIDREPORT     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_INTERFACE

#define USB_XFER_TIMEOUT    5000    //USB transfer timeout in milliseconds, per section 9.2.6.1 of USB 2.0 spec
#define USB_NAK_LIMIT       32000   //NAK limit for a transfer. o meand NAKs are not counted
#define USB_RETRY_LIMIT     3       //retry limit for a transfer
#define USB_SETTLE_DELAY    200     //settle delay in milliseconds
#define USB_NAK_NOWAIT      1       //used in Richard's PS2/Wiimote code

#define USB_NUMDEVICES  2           //number of USB devices

/* USB state machine states */

#define USB_STATE_MASK                                      0xf0

#define USB_STATE_DETACHED                                  0x10
#define USB_DETACHED_SUBSTATE_INITIALIZE                    0x11        
#define USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE               0x12
#define USB_DETACHED_SUBSTATE_ILLEGAL                       0x13
#define USB_ATTACHED_SUBSTATE_SETTLE                        0x20
#define USB_ATTACHED_SUBSTATE_RESET_DEVICE                  0x30    
#define USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE           0x40
#define USB_ATTACHED_SUBSTATE_WAIT_SOF                      0x50
#define USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE    0x60
#define USB_STATE_ADDRESSING                                0x70
#define USB_STATE_CONFIGURING                               0x80
#define USB_STATE_RUNNING                                   0x90
#define USB_STATE_ERROR                                     0xa0

// byte usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE

/* USB Setup Packet Structure   */
typedef struct {
    union {                          // offset   description
        uint8_t bmRequestType;         //   0      Bit-map of request type
        struct {
            uint8_t    recipient:  5;  //          Recipient of the request
            uint8_t    type:       2;  //          Type of request
            uint8_t    direction:  1;  //          Direction of data X-fer
        };
    }ReqType_u;
    uint8_t    bRequest;               //   1      Request
    union {
        unsigned int    wValue;             //   2      Depends on bRequest
        struct {
        uint8_t    wValueLo;
        uint8_t    wValueHi;
        };
    }wVal_u;
    unsigned int    wIndex;                 //   4      Depends on bRequest
    unsigned int    wLength;                //   6      Depends on bRequest
} SETUP_PKT, *PSETUP_PKT;

/* Endpoint information structure               */
/* bToggle of endpoint 0 initialized to 0xff    */
/* during enumeration bToggle is set to 00      */
typedef struct {        
    uint8_t epAddr;        //copy from endpoint descriptor. Bit 7 indicates direction ( ignored for control endpoints )
    uint8_t Attr;          // Endpoint transfer type.
    unsigned int MaxPktSize;    // Maximum packet size.
    uint8_t Interval;      // Polling interval in frames.
    uint8_t sndToggle;     //last toggle value, bitmask for HCTL toggle bits
    uint8_t rcvToggle;     //last toggle value, bitmask for HCTL toggle bits
    /* not sure if both are necessary */
} EP_RECORD;
/* device record structure */
typedef struct {
    EP_RECORD* epinfo;      //device endpoint information
    uint8_t devclass;          //device class    
} DEV_RECORD;


typedef MAX3421e<P10, P9>		MAX3421E;

class USB : public MAX3421E
{
//data structures    
/* device table. Filled during enumeration              */
/* index corresponds to device address                  */
/* each entry contains pointer to endpoint structure    */
/* and device class to use in various places            */             
//DEV_RECORD devtable[ USB_NUMDEVICES + 1 ];
//EP_RECORD dev0ep;         //Endpoint data structure used during enumeration for uninitialized device

//byte usb_task_state;

    public:
        USB( void );
        uint8_t getUsbTaskState( void );
        void setUsbTaskState( uint8_t state );
        EP_RECORD* getDevTableEntry( uint8_t addr, uint8_t ep );
        void setDevTableEntry( uint8_t addr, EP_RECORD* eprecord_ptr );
        uint8_t ctrlReq( uint8_t addr, uint8_t ep, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi, unsigned int wInd, unsigned int nbytes, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
        /* Control requests */
        uint8_t getDevDescr( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t getConfDescr( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t conf, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t getStrDescr( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t index, unsigned int langid, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t setAddr( uint8_t oldaddr, uint8_t ep, uint8_t newaddr, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t setConf( uint8_t addr, uint8_t ep, uint8_t conf_value, unsigned int nak_limit = USB_NAK_LIMIT );
        /**/
        uint8_t setProto( uint8_t addr, uint8_t ep, uint8_t interface, uint8_t protocol, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t getProto( uint8_t addr, uint8_t ep, uint8_t interface, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t getReportDescr( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t setReport( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t interface, uint8_t report_type, uint8_t report_id, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
	      uint8_t getReport( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t interface, uint8_t report_type, uint8_t report_id, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t getIdle( uint8_t addr, uint8_t ep, uint8_t interface, uint8_t reportID, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t setIdle( uint8_t addr, uint8_t ep, uint8_t interface, uint8_t reportID, uint8_t duration, unsigned int nak_limit = USB_NAK_LIMIT );
        /**/
        uint8_t ctrlData( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* dataptr, boolean direction, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t ctrlStatus( uint8_t ep, boolean direction, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t inTransfer( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* data, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t outTransfer( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* data, unsigned int nak_limit = USB_NAK_LIMIT );
        uint8_t dispatchPkt( uint8_t token, uint8_t ep, unsigned int nak_limit = USB_NAK_LIMIT );
        void Task( void );
		//uint8_t nit() {};
    private:
        void init();
};

//get device descriptor
inline uint8_t USB::getDevDescr( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* dataptr, unsigned int nak_limit ) {
    return( ctrlReq( addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0x00, USB_DESCRIPTOR_DEVICE, 0x0000, nbytes, dataptr, nak_limit ));
}
//get configuration descriptor  
inline uint8_t USB::getConfDescr( uint8_t addr, uint8_t ep, unsigned int nuint8_ts, uint8_t conf, uint8_t* dataptr, unsigned int nak_limit ) {
        return( ctrlReq( addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, conf, USB_DESCRIPTOR_CONFIGURATION, 0x0000, nuint8_ts, dataptr, nak_limit ));
}
//get string descriptor
inline uint8_t USB::getStrDescr( uint8_t addr, uint8_t ep, unsigned int nuint8_ts, uint8_t index, unsigned int langid, uint8_t* dataptr, unsigned int nak_limit ) {
    return( ctrlReq( addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, index, USB_DESCRIPTOR_STRING, langid, nuint8_ts, dataptr, nak_limit ));
}
//set address 
inline uint8_t USB::setAddr( uint8_t oldaddr, uint8_t ep, uint8_t newaddr, unsigned int nak_limit ) {
    return( ctrlReq( oldaddr, ep, bmREQ_SET, USB_REQUEST_SET_ADDRESS, newaddr, 0x00, 0x0000, 0x0000, NULL, nak_limit ));
}
//set configuration
inline uint8_t USB::setConf( uint8_t addr, uint8_t ep, uint8_t conf_value, unsigned int nak_limit ) {
    return( ctrlReq( addr, ep, bmREQ_SET, USB_REQUEST_SET_CONFIGURATION, conf_value, 0x00, 0x0000, 0x0000, NULL, nak_limit ));         
}
//class requests
inline uint8_t USB::setProto( uint8_t addr, uint8_t ep, uint8_t interface, uint8_t protocol, unsigned int nak_limit ) {
        return( ctrlReq( addr, ep, bmREQ_HIDOUT, HID_REQUEST_SET_PROTOCOL, protocol, 0x00, interface, 0x0000, NULL, nak_limit ));
}
inline uint8_t USB::getProto( uint8_t addr, uint8_t ep, uint8_t interface, uint8_t* dataptr, unsigned int nak_limit ) {
        return( ctrlReq( addr, ep, bmREQ_HIDIN, HID_REQUEST_GET_PROTOCOL, 0x00, 0x00, interface, 0x0001, dataptr, nak_limit ));        
}
//get HID report descriptor 
inline uint8_t USB::getReportDescr( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* dataptr, unsigned int nak_limit ) {
        return( ctrlReq( addr, ep, bmREQ_HIDREPORT, USB_REQUEST_GET_DESCRIPTOR, 0x00, HID_DESCRIPTOR_REPORT, 0x0000, nbytes, dataptr, nak_limit ));
}
inline uint8_t USB::setReport( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t interface, uint8_t report_type, uint8_t report_id, uint8_t* dataptr, unsigned int nak_limit ) {
    return( ctrlReq( addr, ep, bmREQ_HIDOUT, HID_REQUEST_SET_REPORT, report_id, report_type, interface, nbytes, dataptr, nak_limit ));
}
inline uint8_t USB::getReport( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t interface, uint8_t report_type, uint8_t report_id, uint8_t* dataptr, unsigned int nak_limit ) { // ** RI 04/11/09
    return( ctrlReq( addr, ep, bmREQ_HIDIN, HID_REQUEST_GET_REPORT, report_id, report_type, interface, nbytes, dataptr, nak_limit ));
}
/* returns one byte of data in dataptr */
inline uint8_t USB::getIdle( uint8_t addr, uint8_t ep, uint8_t interface, uint8_t reportID, uint8_t* dataptr, unsigned int nak_limit ) {
        return( ctrlReq( addr, ep, bmREQ_HIDIN, HID_REQUEST_GET_IDLE, reportID, 0, interface, 0x0001, dataptr, nak_limit ));    
}
inline uint8_t USB::setIdle( uint8_t addr, uint8_t ep, uint8_t interface, uint8_t reportID, uint8_t duration, unsigned int nak_limit ) {
           return( ctrlReq( addr, ep, bmREQ_HIDOUT, HID_REQUEST_SET_IDLE, reportID, duration, interface, 0x0000, NULL, nak_limit ));
          }
#endif //_usb_h_