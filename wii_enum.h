#ifndef __WII_ENUM_H__
#define __WII_ENUM_H__



/*
The original Wiimotes (RVL-CNT-01) allowed sending commands using SET REPORT
(0x52) over the control pipe, instead of using the data pipe; however, this form
does not work on the newer RVL-CNT-01-TR, and as such is not recommended.
SOURCE: http://wiibrew.org/wiki/Wiimote#HID_Interface
*/
#define WII_HID_REPORT 0xA2
//#define WII_HID_REPORT 0x52




enum WII_LED {
	RUMBLE				= 0x01,
	P1					= 0x10,
	P2					= 0x20,
	P3					= 0x40,
	P4					= 0x80,
};




//Source: http://wiibrew.org/wiki/Wiimote#HID_Interface
enum WII_REPORT {
	OUT_UNKNOWN			= 0x10,
	OUT_LED				= 0x11,
	OUT_DATA			= 0x12,
	OUT_CAMERA_ENABLE	= 0x13,
	OUT_SPEAKER_ENABLE	= 0x14,
	OUT_STATUS			= 0x15,
	OUT_WRITE			= 0x16,
	OUT_READ			= 0x17,
	OUT_SPEAKER_DATA	= 0x18,
	OUT_SPEAKER_MUTE	= 0x19,
	OUT_CAMERA_ENABLE_2	= 0x1A,

	IN_STATUS			= 0x20,
	IN_READ				= 0x21,
	IN_ACKNOWLEDGE		= 0x22,

	BTN_ONLY			= 0x30,
	BTN_ACC				= 0x31,
	BTN_EXT_8			= 0x32,
	BTN_ACC_IR_12		= 0x33,
	BTN_EXT_19			= 0x34,
	BTN_ACC_EXT_16		= 0x35,
	BTN_IR_10_EXT_9		= 0x36,
	BTN_ACC_IR_10_EXT_6	= 0x37,
	EXT_21				= 0x3D,
	BTN_ACC_IR_36_A		= 0x3E,
	BTN_ACC_IR_36_B		= 0x3F,
};




#endif //__WII_ENUM_H__
