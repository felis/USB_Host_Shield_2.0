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

// Additional Error Codes
#define HUB_ERROR_PORT_HAS_BEEN_RESET						0xb1


// The bit mask to check for all necessary state bits
#define bmHUB_PORT_STATUS_ALL_MAIN				((0UL  | bmHUB_PORT_STATUS_C_PORT_CONNECTION  | bmHUB_PORT_STATUS_C_PORT_ENABLE  | bmHUB_PORT_STATUS_C_PORT_SUSPEND  | bmHUB_PORT_STATUS_C_PORT_RESET) << 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_SUSPEND)

// Bit mask to check for DISABLED state in HubEvent::bmStatus field 
#define bmHUB_PORT_STATE_CHECK_DISABLED			(0x0000 | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_SUSPEND)

// Hub Port States
#define bmHUB_PORT_STATE_DISABLED				(0x0000 | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_CONNECTION)

// Hub Port Events
#define bmHUB_PORT_EVENT_CONNECT				(((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION)	<< 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_CONNECTION)
#define bmHUB_PORT_EVENT_DISCONNECT				(((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION)	<< 16) | bmHUB_PORT_STATUS_PORT_POWER)
#define bmHUB_PORT_EVENT_RESET_COMPLETE			(((0UL | bmHUB_PORT_STATUS_C_PORT_RESET)		<< 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION)

#define bmHUB_PORT_EVENT_LS_CONNECT				(((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION)	<< 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_LOW_SPEED)
#define bmHUB_PORT_EVENT_LS_RESET_COMPLETE		(((0UL | bmHUB_PORT_STATUS_C_PORT_RESET)		<< 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_LOW_SPEED)
#define bmHUB_PORT_EVENT_LS_PORT_ENABLED		(((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION | bmHUB_PORT_STATUS_C_PORT_ENABLE)		<< 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_LOW_SPEED)

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
	static bool bResetInitiated;		// True when reset is triggered

	USB			*pUsb;					// USB class instance pointer

	EpInfo		epInfo[2];				// interrupt endpoint info structure

	uint8_t		bAddress;				// address
	uint8_t		bNbrPorts;				// number of ports
	uint8_t		bInitState;				// initialization state variable
	uint32_t	qNextPollTime;			// next poll time
	bool		bPollEnable;			// poll enable flag

	uint8_t GetHubStatus(uint8_t addr);
	uint8_t HubPortStatusChange(uint8_t addr, uint8_t port, HubEvent &evt);

public:
	USBHub(USB *p);

	virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
	virtual uint8_t Release();
	virtual uint8_t Poll();
	virtual uint8_t GetAddress() { return bAddress; };
};

void PrintHubPortStatus(USB *usbptr, uint8_t addr, uint8_t port, bool print_changes = false);

#endif // __USBHUB_H__