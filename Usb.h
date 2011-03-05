/* USB functions */
#ifndef _usb_h_
#define _usb_h_

#define USB_METHODS_INLINE

#include <inttypes.h>
#include "avrpins.h"
#include "max3421e.h"
#include "usbhost.h"
#include "usb_ch9.h"
#include "address.h"
#include <WProgram.h>

/* Common setup data constant combinations  */
#define bmREQ_GET_DESCR     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE     //get descriptor request type
#define bmREQ_SET           USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE     //set request type for all but 'set feature' and 'set interface'
#define bmREQ_CL_GET_INTF   USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE     //get interface request type

/* HID requests */
#define bmREQ_HIDOUT        USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define bmREQ_HIDIN         USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE 
#define bmREQ_HIDREPORT     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_INTERFACE



// D7		data transfer direction (0 - host-to-device, 1 - device-to-host)
// D6-5		Type (0- standard, 1 - class, 2 - vendor, 3 - reserved)
// D4-0		Recipient (0 - device, 1 - interface, 2 - endpoint, 3 - other, 4..31 - reserved)


// USB Device Classes
#define USB_CLASS_USE_CLASS_INFO	0x00	// Use Class Info in the Interface Descriptors
#define USB_CLASS_AUDIO				0x01	// Audio
#define USB_CLASS_COM_AND_CDC_CTRL	0x02	// Communications and CDC Control
#define USB_CLASS_HID				0x03	// HID
#define USB_CLASS_PHYSICAL			0x05	// Physical
#define USB_CLASS_IMAGE				0x06	// Image
#define USB_CLASS_PRINTER			0x07	// Printer
#define USB_CLASS_MASS_STORAGE		0x08	// Mass Storage
#define USB_CLASS_HUB				0x09	// Hub
#define USB_CLASS_CDC_DATA			0x0a	// CDC-Data
#define USB_CLASS_SMART_CARD		0x0b	// Smart-Card
#define USB_CLASS_CONTENT_SECURITY	0x0d	// Content Security
#define USB_CLASS_VIDEO				0x0e	// Video
#define USB_CLASS_PERSONAL_HEALTH	0x0f	// Personal Healthcare
#define USB_CLASS_DIAGNOSTIC_DEVICE	0xdc	// Diagnostic Device
#define USB_CLASS_WIRELESS_CTRL		0xe0	// Wireless Controller
#define USB_CLASS_MISC				0xef	// Miscellaneous
#define USB_CLASS_APP_SPECIFIC		0xfe	// Application Specific
#define USB_CLASS_VENDOR_SPECIFIC	0xff	// Vendor Specific


// Hub Requests
#define bmREQ_CLEAR_HUB_FEATURE		USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_CLEAR_PORT_FEATURE	USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_CLEAR_TT_BUFFER		USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_GET_HUB_DESCRIPTOR	USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_GET_HUB_STATUS		USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_GET_PORT_STATUS		USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_RESET_TT				USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_SET_HUB_DESCRIPTOR	USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_SET_HUB_FEATURE		USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_SET_PORT_FEATURE		USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_GET_TT_STATE			USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_STOP_TT				USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER

// Hub Class Requests
#define HUB_REQUEST_CLEAR_TT_BUFFER				8
#define HUB_REQUEST_RESET_TT					9
#define HUB_REQUEST_GET_TT_STATE				10
#define HUB_REQUEST_STOP_TT						11

// Hub Features
#define HUB_FEATURE_C_HUB_LOCAL_POWER			0
#define HUB_FEATURE_C_HUB_OVER_CURRENT			1
#define HUB_FEATURE_PORT_CONNECTION				0
#define HUB_FEATURE_PORT_ENABLE					1
#define HUB_FEATURE_PORT_SUSPEND				2
#define HUB_FEATURE_PORT_OVER_CURRENT			3
#define HUB_FEATURE_PORT_RESET					4
#define HUB_FEATURE_PORT_POWER					8
#define HUB_FEATURE_PORT_LOW_SPEED				9
#define HUB_FEATURE_C_PORT_CONNECTION			16
#define HUB_FEATURE_C_PORT_ENABLE				17
#define HUB_FEATURE_C_PORT_SUSPEND				18
#define HUB_FEATURE_C_PORT_OVER_CURRENT			19
#define HUB_FEATURE_C_PORT_RESET				20
#define HUB_FEATURE_PORT_TEST					21
#define HUB_FEATURE_PORT_INDICATOR				22

// Hub Port Test Modes
#define HUB_PORT_TEST_MODE_J					1
#define HUB_PORT_TEST_MODE_K					2
#define HUB_PORT_TEST_MODE_SE0_NAK				3
#define HUB_PORT_TEST_MODE_PACKET				4
#define HUB_PORT_TEST_MODE_FORCE_ENABLE			5 

// Hub Port Indicator Color
#define HUB_PORT_INDICATOR_AUTO					0
#define HUB_PORT_INDICATOR_AMBER				1
#define HUB_PORT_INDICATOR_GREEN				2
#define HUB_PORT_INDICATOR_OFF					3

// Hub Port Status Bitmasks 
#define bmHUB_PORT_STATUS_PORT_CONNECTION		0x0001
#define bmHUB_PORT_STATUS_PORT_ENABLE			0x0002
#define bmHUB_PORT_STATUS_PORT_SUSPEND			0x0004
#define bmHUB_PORT_STATUS_PORT_OVER_CURRENT		0x0008
#define bmHUB_PORT_STATUS_PORT_RESET			0x0010
#define bmHUB_PORT_STATUS_PORT_POWER			0x0100
#define bmHUB_PORT_STATUS_PORT_LOW_SPEED		0x0200
#define bmHUB_PORT_STATUS_PORT_HIGH_SPEED		0x0400
#define bmHUB_PORT_STATUS_PORT_TEST				0x0800
#define bmHUB_PORT_STATUS_PORT_INDICATOR		0x1000

// Hub Port Status Change Bitmasks (used one byte instead of two)
#define bmHUB_PORT_STATUS_C_PORT_CONNECTION		0x0001
#define bmHUB_PORT_STATUS_C_PORT_ENABLE			0x0002
#define bmHUB_PORT_STATUS_C_PORT_SUSPEND		0x0004
#define bmHUB_PORT_STATUS_C_PORT_OVER_CURRENT	0x0008
#define bmHUB_PORT_STATUS_C_PORT_RESET			0x0010

// Hub Status Bitmasks (used one byte instead of two)
#define bmHUB_STATUS_LOCAL_POWER_SOURCE			0x01
#define bmHUB_STATUS_OVER_CURRENT				0x12

// Hub Status Change Bitmasks (used one byte instead of two)
#define bmHUB_STATUS_C_LOCAL_POWER_SOURCE		0x01
#define bmHUB_STATUS_C_OVER_CURRENT				0x12

// Additional Error Codes
#define USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED	0xD1
#define USB_DEV_CONFIG_ERROR_DEVICE_INIT_INCOMPLETE	0xD2
#define USB_ERROR_UNABLE_TO_REGISTER_DEVICE_CLASS	0xD3
#define USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL		0xD4
#define USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL			0xD5
#define USB_ERROR_EPINFO_IS_NULL					0xD6
#define USB_ERROR_INVALID_ARGUMENT					0xD7
#define USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE		0xD8

//class USBDeviceConfig
//{
//public:
//	virtual uint8_t Init(uint8_t addr) = 0;
//	virtual uint8_t Release(uint8_t addr) = 0;
//	virtual uint8_t Poll() = 0;
//};

class USBDeviceConfig
{
public:
	virtual uint8_t Init(uint8_t parent, uint8_t port)	= 0;
	virtual uint8_t Release()	= 0;
	virtual uint8_t Poll()		= 0;
};


#define USB_XFER_TIMEOUT		5000    //USB transfer timeout in milliseconds, per section 9.2.6.1 of USB 2.0 spec
#define USB_NAK_LIMIT			32000   //NAK limit for a transfer. o meand NAKs are not counted
#define USB_RETRY_LIMIT			3       //retry limit for a transfer
#define USB_SETTLE_DELAY		200     //settle delay in milliseconds
#define USB_NAK_NOWAIT			1       //used in Richard's PS2/Wiimote code

#define USB_NUMDEVICES			6		//number of USB devices
#define HUB_MAX_HUBS			5		// maximum number of hubs that can be attached to the host controller
#define HUB_PORT_RESET_DELAY	20		// hub port reset delay 10 ms recomended, can be up to 20 ms

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
    uint8_t    bRequest;					//   1      Request
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

/* device record structure */
typedef struct 
{
    EP_RECORD		*epinfo;		//device endpoint information
    uint8_t			devclass;       //device class    
} DEV_RECORD;


struct HubDescriptor
{
	uint8_t		bDescLength;						// descriptor length
	uint8_t		bDescriptorType;					// descriptor type
	uint8_t		bNbrPorts;							// number of ports a hub equiped with
	
	struct 
	{
		uint16_t	LogPwrSwitchMode		: 2;
		uint16_t	CompoundDevice			: 1;
		uint16_t	OverCurrentProtectMode	: 2;
		uint16_t	TTThinkTime				: 2;
		uint16_t	PortIndicatorsSupported : 1;
		uint16_t	Reserved				: 8;
	};

	uint8_t		bPwrOn2PwrGood;
	uint8_t		bHubContrCurrent;
};



//typedef MAX3421e<P6, P3>		MAX3421E;		// Black Widdow
typedef MAX3421e<P10, P9>		MAX3421E;		// Duemielanove

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


		AddressPoolImpl<USB_NUMDEVICES>		addrPool;
		USBDeviceConfig*					devConfig[USB_NUMDEVICES];
		uint8_t								devConfigIndex;

    public:
        USB( void );

		AddressPool& GetAddressPool()
		{
			return (AddressPool&)addrPool;
		};
		uint8_t RegisterDeviceClass(USBDeviceConfig *pdev)
		{
			for (uint8_t i=0; i<USB_NUMDEVICES; i++)
			{
				if (!devConfig[i])
				{
					devConfig[i] = pdev;
					return 0;
				}
			}
			return USB_ERROR_UNABLE_TO_REGISTER_DEVICE_CLASS;
		};
		void ForEachUsbDevice(UsbDeviceHandleFunc pfunc)
		{
			addrPool.ForEachUsbDevice(pfunc);
		};
		uint8_t getUsbTaskState( void );
        void setUsbTaskState( uint8_t state );

        EP_RECORD* getDevTableEntry( uint8_t addr, uint8_t ep );
        uint8_t setDevTableEntry( uint8_t addr, EP_RECORD* eprecord_ptr );

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

		// Hub Methods
		uint8_t ClearHubFeature( uint8_t addr, uint8_t ep, uint8_t fid, unsigned int nak_limit = USB_NAK_LIMIT );
		uint8_t ClearPortFeature( uint8_t addr, uint8_t ep, uint8_t fid, uint8_t port, uint8_t sel = 0, unsigned int nak_limit = USB_NAK_LIMIT );
		uint8_t GetHubDescriptor( uint8_t addr, uint8_t ep, uint8_t index, uint16_t nbytes, uint8_t *dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
		uint8_t GetHubStatus( uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
		uint8_t GetPortStatus( uint8_t addr, uint8_t ep, uint8_t port, uint16_t nbytes, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
		uint8_t SetHubDescriptor( uint8_t addr, uint8_t ep, uint8_t port, uint16_t nbytes, uint8_t* dataptr, unsigned int nak_limit = USB_NAK_LIMIT );
		uint8_t SetHubFeature( uint8_t addr, uint8_t ep, uint8_t fid, unsigned int nak_limit = USB_NAK_LIMIT );
		uint8_t SetPortFeature( uint8_t addr, uint8_t ep, uint8_t fid, uint8_t port, uint8_t sel = 0, unsigned int nak_limit = USB_NAK_LIMIT );

		uint8_t HubPortPowerOn(uint8_t addr, uint8_t port);
		uint8_t HubPortReset(uint8_t addr, uint8_t port);
		uint8_t HubClearPortFeatures(uint8_t addr, uint8_t port, uint8_t bm_features);

		uint8_t GetNumDevices();
		uint8_t	GetNumHubs();

		uint8_t PollHubs();
		uint8_t PollHub();

		void PrintHubStatus(/*USB *usbptr,*/ uint8_t addr);

        void Task( void );

		uint8_t DefaultAddressing();

		uint8_t Configuring(uint8_t parent, uint8_t port);
		
    private:
        void init();
};

#if defined(USB_METHODS_INLINE)
//get device descriptor
inline uint8_t USB::getDevDescr( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t* dataptr, unsigned int nak_limit ) {
    return( ctrlReq( addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0x00, USB_DESCRIPTOR_DEVICE, 0x0000, nbytes, dataptr, nak_limit ));
}
//get configuration descriptor  
inline uint8_t USB::getConfDescr( uint8_t addr, uint8_t ep, unsigned int nbytes, uint8_t conf, uint8_t* dataptr, unsigned int nak_limit ) {
        return( ctrlReq( addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, conf, USB_DESCRIPTOR_CONFIGURATION, 0x0000, nbytes, dataptr, nak_limit ));
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

// uint8_t ctrlReq( 
		//uint8_t			addr, 
		//uint8_t			ep, 
		//uint8_t			bmReqType,	
		//uint8_t			bRequest, 
		//uint8_t			wValLo, 
		//uint8_t			wValHi, 
		//unsigned int		wInd, 
		//unsigned int		nbytes, 
		//uint8_t*			dataptr, 
		//unsigned int		nak_limit = USB_NAK_LIMIT );

// Clear Hub Feature
inline uint8_t USB::ClearHubFeature( uint8_t addr, uint8_t ep, uint8_t fid, unsigned int nak_limit ) 
{
	return( ctrlReq( addr, ep, bmREQ_CLEAR_HUB_FEATURE, USB_REQUEST_CLEAR_FEATURE, fid, 0, 0, 0, NULL, nak_limit ));
}
// Clear Port Feature
inline uint8_t USB::ClearPortFeature( uint8_t addr, uint8_t ep, uint8_t fid, uint8_t port, uint8_t sel, unsigned int nak_limit ) 
{
	return( ctrlReq( addr, ep, bmREQ_CLEAR_PORT_FEATURE, USB_REQUEST_CLEAR_FEATURE, fid, 0, ((0x0000|port)|(sel<<8)), 0, NULL, nak_limit ));
}
// Get Hub Descriptor
inline uint8_t USB::GetHubDescriptor( uint8_t addr, uint8_t ep, uint8_t index, uint16_t nbytes, uint8_t *dataptr, unsigned int nak_limit ) 
{
	return( ctrlReq( addr, ep, bmREQ_GET_HUB_DESCRIPTOR, USB_REQUEST_GET_DESCRIPTOR, index, 0x29, 0, nbytes, dataptr, nak_limit ));
}
// Get Hub Status
inline uint8_t USB::GetHubStatus( uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* dataptr, unsigned int nak_limit ) 
{
    return( ctrlReq( addr, ep, bmREQ_GET_HUB_STATUS, USB_REQUEST_GET_STATUS, 0, 0, 0x0000, nbytes, dataptr, nak_limit ));
}
// Get Port Status
inline uint8_t USB::GetPortStatus( uint8_t addr, uint8_t ep, uint8_t port, uint16_t nbytes, uint8_t* dataptr, unsigned int nak_limit ) 
{
	//Serial.println(bmREQ_GET_PORT_STATUS, BIN);
    return( ctrlReq( addr, ep, bmREQ_GET_PORT_STATUS, USB_REQUEST_GET_STATUS, 0, 0, port, nbytes, dataptr, nak_limit ));
}
// Set Hub Descriptor
inline uint8_t USB::SetHubDescriptor( uint8_t addr, uint8_t ep, uint8_t port, uint16_t nbytes, uint8_t* dataptr, unsigned int nak_limit ) 
{
    return( ctrlReq( addr, ep, bmREQ_SET_HUB_DESCRIPTOR, USB_REQUEST_SET_DESCRIPTOR, 0, 0, port, nbytes, dataptr, nak_limit ));
}
// Set Hub Feature
inline uint8_t USB::SetHubFeature( uint8_t addr, uint8_t ep, uint8_t fid, unsigned int nak_limit ) 
{
    return( ctrlReq( addr, ep, bmREQ_SET_HUB_FEATURE, USB_REQUEST_SET_FEATURE, fid, 0, 0, 0, NULL, nak_limit ));
}
// Set Port Feature
inline uint8_t USB::SetPortFeature( uint8_t addr, uint8_t ep, uint8_t fid, uint8_t port, uint8_t sel, unsigned int nak_limit ) 
{
    return( ctrlReq( addr, ep, bmREQ_SET_PORT_FEATURE, USB_REQUEST_SET_FEATURE, fid, 0, (((0x0000|sel)<<8)|port), 0, NULL, nak_limit ));
}
#endif // defined(USB_METHODS_INLINE)

#endif //_usb_h_