#if !defined(__CONFDESCPARSER_H__)
#define __CONFDESCPARSER_H__

#include <inttypes.h>
#include "..\ParseTools\parsetools.h"

class UsbConfigXtracter
{
public:
	//virtual void ConfigXtract(const USB_CONFIGURATION_DESCRIPTOR *conf) = 0;
	//virtual void InterfaceXtract(uint8_t conf, const USB_INTERFACE_DESCRIPTOR *iface) = 0;
	virtual void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep) = 0;
};

#define CP_MASK_COMPARE_CLASS			1
#define CP_MASK_COMPARE_SUBCLASS		2
#define CP_MASK_COMPARE_PROTOCOL		4
#define CP_MASK_COMPARE_ALL				7

// Configuration Descriptor Parser Class Template
template <const uint8_t CLASS_ID, const uint8_t SUBCLASS_ID, const uint8_t PROTOCOL_ID, const uint8_t MASK>
class ConfigDescParser : public USBReadParser
{
	UsbConfigXtracter		*theXtractor;
    MultiValueBuffer		theBuffer;
	MultiByteValueParser	valParser;
	ByteSkipper				theSkipper;
	uint8_t					varBuffer[sizeof(USB_CONFIGURATION_DESCRIPTOR)]; 

	uint8_t					stateParseDescr;	// ParseDescriptor state 

	uint8_t					dscrLen;			// Descriptor length
	uint8_t					dscrType;			// Descriptor type

	bool					isGoodInterface;	// Apropriate interface flag	
	uint8_t					confValue;			// Configuration value
	uint8_t					protoValue;			// Protocol value
	uint8_t					ifaceNumber;		// Interface number
	uint8_t					ifaceAltSet;		// Interface alternate settings

	bool ParseDescriptor(uint8_t **pp, uint16_t *pcntdn);

public:
	ConfigDescParser(UsbConfigXtracter *xtractor);
	virtual void Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset);
};

template <const uint8_t CLASS_ID, const uint8_t SUBCLASS_ID, const uint8_t PROTOCOL_ID, const uint8_t MASK>
ConfigDescParser<CLASS_ID, SUBCLASS_ID, PROTOCOL_ID, MASK>::ConfigDescParser(UsbConfigXtracter *xtractor) : 
	stateParseDescr(0), 
	dscrLen(0), 
	dscrType(0),
	theXtractor(xtractor)
{
	theBuffer.pValue = varBuffer; 
	valParser.Initialize(&theBuffer);
	theSkipper.Initialize(&theBuffer);
};

template <const uint8_t CLASS_ID, const uint8_t SUBCLASS_ID, const uint8_t PROTOCOL_ID, const uint8_t MASK>
void ConfigDescParser<CLASS_ID, SUBCLASS_ID, PROTOCOL_ID, MASK>::Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset)
{
	uint16_t	cntdn	= (uint16_t)len;
	uint8_t		*p		= (uint8_t*)pbuf; 

	while(cntdn)
		if (!ParseDescriptor(&p, &cntdn))
			return;
}

template <const uint8_t CLASS_ID, const uint8_t SUBCLASS_ID, const uint8_t PROTOCOL_ID, const uint8_t MASK>
bool ConfigDescParser<CLASS_ID, SUBCLASS_ID, PROTOCOL_ID, MASK>::ParseDescriptor(uint8_t **pp, uint16_t *pcntdn)
{
	switch (stateParseDescr)
	{
	case 0:
        theBuffer.valueSize = 2;
		valParser.Initialize(&theBuffer);
		stateParseDescr		= 1;
	case 1:
		if (!valParser.Parse(pp, pcntdn))
            return false;
        dscrLen			= *((uint8_t*)theBuffer.pValue); 
        dscrType		= *((uint8_t*)theBuffer.pValue + 1); 
		stateParseDescr	= 2;
	case 2:
		// This is a sort of hack. Assuming that two bytes are allready in the buffer 
		//	the pointer is positioned two bytes ahead in order for the rest of descriptor 
		//	to be read right after the size and the type fields.
		// This should be used carefuly. varBuffer should be used directly to handle data
		//	in the buffer.
		theBuffer.pValue	= varBuffer + 2;	
		stateParseDescr		= 3;
	case 3:
		switch (dscrType)
		{
		case USB_DESCRIPTOR_INTERFACE:
			isGoodInterface = false;
		case USB_DESCRIPTOR_CONFIGURATION:
			theBuffer.valueSize = sizeof(USB_CONFIGURATION_DESCRIPTOR)-2;
			break;
		case USB_DESCRIPTOR_ENDPOINT:
			theBuffer.valueSize = sizeof(USB_ENDPOINT_DESCRIPTOR)-2;
			break;
		}
		valParser.Initialize(&theBuffer);
		stateParseDescr		= 4;
	case 4:
		switch (dscrType)
		{
		case USB_DESCRIPTOR_CONFIGURATION:
			if (!valParser.Parse(pp, pcntdn))
				return false;
			confValue = ((USB_CONFIGURATION_DESCRIPTOR*)varBuffer)->bConfigurationValue;
			break;
		case USB_DESCRIPTOR_INTERFACE:
			if (!valParser.Parse(pp, pcntdn))
				return false;
			if ((MASK & CP_MASK_COMPARE_CLASS) && ((USB_INTERFACE_DESCRIPTOR*)varBuffer)->bInterfaceClass != CLASS_ID)
				break;
			if ((MASK & CP_MASK_COMPARE_SUBCLASS) && ((USB_INTERFACE_DESCRIPTOR*)varBuffer)->bInterfaceSubClass != SUBCLASS_ID)
				break;
			if ((MASK & CP_MASK_COMPARE_PROTOCOL) && ((USB_INTERFACE_DESCRIPTOR*)varBuffer)->bInterfaceProtocol != PROTOCOL_ID)
				break;
			
			isGoodInterface = true;
			ifaceNumber = ((USB_INTERFACE_DESCRIPTOR*)varBuffer)->bInterfaceNumber;
			ifaceAltSet = ((USB_INTERFACE_DESCRIPTOR*)varBuffer)->bAlternateSetting;
			protoValue  = ((USB_INTERFACE_DESCRIPTOR*)varBuffer)->bInterfaceProtocol;
			break;
		case USB_DESCRIPTOR_ENDPOINT:
			if (!valParser.Parse(pp, pcntdn))
				return false;
			if (isGoodInterface)
				if (theXtractor)
					theXtractor->EndpointXtract(confValue, ifaceNumber, ifaceAltSet, protoValue, (USB_ENDPOINT_DESCRIPTOR*)varBuffer);
			break;
		default:
			if (!theSkipper.Skip(pp, pcntdn, dscrLen-2))
				return false;
		}
		theBuffer.pValue = varBuffer; 
		stateParseDescr = 0;
	}
	return true;
}

#endif // __CONFDESCPARSER_H__