/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
*/
#if !defined(__CDCACM_H__)
#define __CDCACM_H__

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "avrpins.h"
#include "max3421e.h"
#include "usbhost.h"
#include "usb_ch9.h"
#include "Usb.h"
#include <WProgram.h>

#include "printhex.h"
#include "hexdump.h"
#include "message.h"

#include "confdescparser.h"

#define bmREQ_CDCOUT        USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define bmREQ_CDCIN         USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE 

// CDC Subclass Constants
#define CDC_SUBCLASS_DLCM					0x01	// Direct Line Control Model
#define CDC_SUBCLASS_ACM					0x02	// Abstract Control Model
#define CDC_SUBCLASS_TCM					0x03	// Telephone Control Model
#define CDC_SUBCLASS_MCCM					0x04	// Multi Channel Control Model
#define CDC_SUBCLASS_CAPI					0x05	// CAPI Control Model
#define CDC_SUBCLASS_ETHERNET				0x06	// Ethernet Network Control Model
#define CDC_SUBCLASS_ATM					0x07	// ATM Network Control Model
#define CDC_SUBCLASS_WIRELESS_HANDSET		0x08	// Wireless Handset Control Model
#define CDC_SUBCLASS_DEVICE_MANAGEMENT		0x09	// Device Management
#define CDC_SUBCLASS_MOBILE_DIRECT_LINE		0x0A	// Mobile Direct Line Model
#define CDC_SUBCLASS_OBEX					0x0B	// OBEX
#define CDC_SUBCLASS_ETHERNET_EMU			0x0C	// Ethernet Emulation Model

// Communication Interface Class Control Protocol Codes
#define CDC_PROTOCOL_ITU_T_V_250			0x01	// AT Commands defined by ITU-T V.250
#define CDC_PROTOCOL_PCCA_101				0x02	// AT Commands defined by PCCA-101
#define CDC_PROTOCOL_PCCA_101_O				0x03	// AT Commands defined by PCCA-101 & Annex O
#define CDC_PROTOCOL_GSM_7_07				0x04	// AT Commands defined by GSM 7.07
#define CDC_PROTOCOL_3GPP_27_07				0x05	// AT Commands defined by 3GPP 27.007
#define CDC_PROTOCOL_C_S0017_0				0x06	// AT Commands defined by TIA for CDMA
#define CDC_PROTOCOL_USB_EEM				0x07	// Ethernet Emulation Model

// CDC Commands defined by CDC 1.2
#define CDC_SEND_ENCAPSULATED_COMMAND		0x00
#define CDC_GET_ENCAPSULATED_RESPONSE		0x01

// CDC Commands defined by PSTN 1.2
#define CDC_SET_COMM_FEATURE				0x02
#define CDC_GET_COMM_FEATURE				0x03
#define CDC_CLEAR_COMM_FEATURE				0x04
#define CDC_SET_AUX_LINE_STATE				0x10
#define CDC_SET_HOOK_STATE					0x11
#define CDC_PULSE_SETUP						0x12
#define CDC_SEND_PULSE						0x13
#define CDC_SET_PULSE_TIME					0x14
#define CDC_RING_AUX_JACK					0x15
#define CDC_SET_LINE_CODING					0x20
#define CDC_GET_LINE_CODING					0x21
#define CDC_SET_CONTROL_LINE_STATE			0x22
#define CDC_SEND_BREAK						0x23
#define CDC_SET_RINGER_PARMS				0x30
#define CDC_GET_RINGER_PARMS				0x31
#define CDC_SET_OPERATION_PARMS				0x32
#define CDC_GET_OPERATION_PARMS				0x33
#define CDC_SET_LINE_PARMS					0x34
#define CDC_GET_LINE_PARMS					0x35
#define CDC_DIAL_DIGITS						0x36

// CDC Functional Descriptor Structures
typedef struct
{
	uint8_t		bFunctionLength;
	uint8_t		bDescriptorType;
	uint8_t		bDescriptorSubtype;
	uint8_t		bmCapabilities;
	uint8_t		bDataInterface;
} CALL_MGMNT_FUNC_DESCR;

typedef struct
{
	uint8_t		bFunctionLength;
	uint8_t		bDescriptorType;
	uint8_t		bDescriptorSubtype;
	uint8_t		bmCapabilities;
} ACM_FUNC_DESCR, DLM_FUNC_DESCR, TEL_OPER_MODES_FUNC_DESCR, 
	TEL_CALL_STATE_REP_CPBL_FUNC_DESCR;

typedef struct
{
	uint8_t		bFunctionLength;
	uint8_t		bDescriptorType;
	uint8_t		bDescriptorSubtype;
	uint8_t		bRingerVolSteps;
	uint8_t		bNumRingerPatterns;
} TEL_RINGER_FUNC_DESCR;

typedef struct
{
	uint32_t	dwDTERate;			// Data Terminal Rate in bits per second
	uint8_t		bCharFormat;		// 0 - 1 stop bit, 1 - 1.5 stop bits, 2 - 2 stop bits
	uint8_t		bParityType;		// 0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space
	uint8_t		bDataBits;			// Data bits (5, 6, 7, 8 or 16)
} LINE_CODING;

class ACM;

class CDCAsyncOper
{
public:
	virtual uint8_t OnInit(ACM *pacm) = 0;
	//virtual void OnDataRcvd(ACM *pacm, uint8_t nbytes, uint8_t *dataptr) = 0;
	//virtual void OnDisconnected(ACM *pacm) = 0;
};


#define ACM_MAX_ENDPOINTS			4

class ACM : public USBDeviceConfig, public UsbConfigXtracter
{
protected:
	static const uint8_t	epDataInIndex;			// DataIn endpoint index
	static const uint8_t	epDataOutIndex;			// DataOUT endpoint index
	static const uint8_t	epInterruptInIndex;		// InterruptIN  endpoint index

	USB			*pUsb;
	CDCAsyncOper	*pAsync;
	uint8_t		bAddress;
	uint8_t		bConfNum;				// configuration number
	uint8_t		bControlIface;			// Control interface value
	uint8_t		bDataIface;				// Data interface value
	uint8_t		bNumEP;					// total number of EP in the configuration
	uint32_t	qNextPollTime;			// next poll time
	bool		bPollEnable;			// poll enable flag

	EpInfo		epInfo[ACM_MAX_ENDPOINTS];

	void PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr);

public:
	ACM(USB *pusb, CDCAsyncOper *pasync);

	uint8_t SetCommFeature(uint16_t fid, uint8_t nbytes, uint8_t *dataptr);
	uint8_t GetCommFeature(uint16_t fid, uint8_t nbytes, uint8_t *dataptr);
	uint8_t ClearCommFeature(uint16_t fid);
	uint8_t SetLineCoding(const LINE_CODING *dataptr);
	uint8_t GetLineCoding(LINE_CODING *dataptr);
	uint8_t SetControlLineState(uint8_t state);
	uint8_t SendBreak(uint16_t duration);

	// Methods for recieving and sending data
	uint8_t RcvData(uint16_t *nbytesptr, uint8_t *dataptr);
	uint8_t SndData(uint16_t nbytes, uint8_t *dataptr);

	// USBDeviceConfig implementation
	virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
	virtual uint8_t Release();
	virtual uint8_t Poll();
	virtual uint8_t GetAddress() { return bAddress; };

	// UsbConfigXtracter implementation
	virtual void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep);
};

#endif // __CDCACM_H__