#if !defined(__USBHUB_H__)
#define __USBHUB_H__

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "avrpins.h"
#include "max3421e.h"
#include "usbhost.h"
#include "usb_ch9.h"
#include "Usb.h"
#include <WProgram.h>

// Hub Port Configuring Substates
#define USB_STATE_HUB_PORT_CONFIGURING						0xb0
#define USB_STATE_HUB_PORT_POWERED_OFF						0xb1
#define USB_STATE_HUB_PORT_WAIT_FOR_POWER_GOOD				0xb2
#define USB_STATE_HUB_PORT_DISCONNECTED						0xb3
#define USB_STATE_HUB_PORT_DISABLED							0xb4
#define USB_STATE_HUB_PORT_RESETTING						0xb5
#define USB_STATE_HUB_PORT_ENABLED							0xb6


// The bit mask to check for all necessary state bits
#define bmHUB_PORT_STATUS_ALL_MAIN				((0UL  | bmHUB_PORT_STATUS_C_PORT_CONNECTION  | bmHUB_PORT_STATUS_C_PORT_ENABLE  | bmHUB_PORT_STATUS_C_PORT_SUSPEND  | bmHUB_PORT_STATUS_C_PORT_RESET) << 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_SUSPEND)

// Hub Port States
//#define bmHUB_PORT_STATE_POWERED				((0UL | bmHUB_PORT_STATUS_PORT_POWER)										<< 16)
#define bmHUB_PORT_STATE_DISCONNECTED			((0UL | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE)		<< 16)

#define bmHUB_PORT_STATE_DISABLED				((0UL | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_CONNECTION))

#define bmHUB_PORT_STATE_RESETTING				((0UL | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_RESET)		<< 16)
#define bmHUB_PORT_STATE_ENABLED				((0UL | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE)		<< 16)
#define bmHUB_PORT_STATE_SUSPENDED				((0UL | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_SUSPEND)		<< 16)

// Hub Port Events
#define bmHUB_PORT_EVENT_CONNECT				(((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION)	<< 16) | bmHUB_PORT_STATE_DISABLED)
#define bmHUB_PORT_EVENT_RESET_COMPLETE			(((0UL | bmHUB_PORT_STATUS_C_PORT_RESET)		<< 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION)

#define bmHUB_PORT_EVENT_PORT_ENABLED			(((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION | bmHUB_PORT_STATUS_C_PORT_ENABLE)		<< 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION)

struct HubEvent
{
	union
	{
		struct
		{
			uint16_t	bmStatus;			// port status bits
			uint16_t	bmChange;			// port status change bits
		};
		uint32_t		bmEvent;
		uint8_t			evtBuff[4];
	};
};


class USBHub : USBDeviceConfig
{
	USB			*pUsb;					// USB class instance pointer

	EP_RECORD	epInfo[2];				// interrupt endpoint info structure

	uint8_t		bAddress;				// address
	uint8_t		bNbrPorts;				// number of ports
	uint8_t		bInitState;				// initialization state variable
//	uint8_t		bPortResetCounter;		// number of ports reset
	uint32_t	qNextPollTime;			// next poll time
	bool		bPollEnable;			// poll enable flag

	uint8_t GetHubStatus(uint8_t addr);
	void HubPortStatusChange(uint8_t addr, uint8_t port, HubEvent &evt);

public:
	USBHub(USB *p);

	virtual uint8_t Init(uint8_t parent, uint8_t port);
	virtual uint8_t Release();
	virtual uint8_t Poll();
};

void PrintHubPortStatus(USB *usbptr, uint8_t addr, uint8_t port, bool print_changes = false);


// ---------------------------------
// | H | H | H | P | P | P | S | S |
// ---------------------------------
//   H - hub ID
//	 P - port number
//	 S - port state

struct UsbHubPortState
{
	union
	{
		struct 
		{
			uint8_t		bmState	: 2;
			uint8_t		bmPort	: 3;
			uint8_t		bmHub	: 3;
		};
		uint8_t			portState;
	};
};

#endif // __USBHUB_H__