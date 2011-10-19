#if !defined(__HIDUNIVERSAL_H__)
#define __HIDUNIVERSAL_H__

#include "hid.h"
#include "hidescriptorparser.h"

class HIDUniversal : public HID
{
	struct ReportParser
	{
		uint8_t				rptId;
		HIDReportParser		*rptParser;
	}	rptParsers[MAX_REPORT_PARSERS];

	// HID class specific descriptor type and length info obtained from HID descriptor
	HID_CLASS_DESCRIPTOR_LEN_AND_TYPE	descrInfo[HID_MAX_HID_CLASS_DESCRIPTORS];

	// Returns HID class specific descriptor length by its type and order number
	uint16_t GetHidClassDescrLen(uint8_t type, uint8_t num);

	EpInfo		epInfo[totalEndpoints];

	struct HIDInterface
	{
		struct
		{
			uint8_t		bmInterface	: 3;
			uint8_t		bmAltSet	: 3;
			uint8_t		bmProtocol	: 2;
		};
		uint8_t			epIndex[maxEpPerInterface];
	};
	
	HIDInterface	hidInterfaces[maxHidInterfaces];

	uint8_t		bConfNum;				// configuration number
	uint8_t		bNumIface;				// number of interfaces in the configuration
	uint8_t		bNumEP;					// total number of EP in the configuration
	uint32_t	qNextPollTime;			// next poll time
	bool		bPollEnable;			// poll enable flag

	void Initialize();
	HIDInterface* FindInterface(uint8_t iface, uint8_t alt, uint8_t proto);

protected:
	bool		bHasReportId;

	// HID implementation
	virtual HIDReportParser* GetReportParser(uint8_t id);

public:
	HIDUniversal(USB *p);

	// HID implementation
	virtual bool SetReportParser(uint8_t id, HIDReportParser *prs);

	// USBDeviceConfig implementation
	virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
	virtual uint8_t Release();
	virtual uint8_t Poll();
	virtual uint8_t GetAddress() { return bAddress; };

	// UsbConfigXtracter implementation
	virtual void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep);
};

#endif // __HIDUNIVERSAL_H__