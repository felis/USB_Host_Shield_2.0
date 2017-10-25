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

 IR camera support added by Allan Glover (adglover9.81@gmail.com) and Kristian Lauszus
 */

#ifndef _wii_h_
#define _wii_h_

#include "BTD.h"
#include "controllerEnums.h"
#include "wii_enum.h"




extern const uint32_t WII_BUTTONS[14];


#define HID_COMMAND(...) {uint8_t buf[]={__VA_ARGS__}; HID_Command(buf, sizeof(buf));}



/**
 * This BluetoothService class implements support for the Wiimote including the Nunchuck and Motion Plus extension.
 *
 * It also support the Wii U Pro Controller.
 */
class WII : public BluetoothService {
public:

	virtual void onInit() {}


	/**
	 * Constructor for the WII class.
	 * @param  p   Pointer to BTD class instance.
	 * @param  pair   Set this to true in order to pair with the Wiimote. If the argument is omitted then it won't pair with it.
	 * One can use ::PAIR to set it to true.
	 */
	WII(BTD *p);

	/** @name BluetoothService implementation */
	/** Used this to disconnect any of the controllers. */
	virtual void disconnect();
	/**@}*/

	/** @name Wii Controller functions */
	/**
	 * getButtonPress(Button b) will return true as long as the button is held down.
	 *
	 * While getButtonClick(Button b) will only return it once.
	 *
	 * So you instance if you need to increase a variable once you would use getButtonClick(Button b),
	 * but if you need to drive a robot forward you would use getButtonPress(Button b).
	 * @param  b          ::ButtonEnum to read.
	 * @return            getButtonPress(ButtonEnum b) will return a true as long as a button is held down, while getButtonClick(ButtonEnum b) will return true once for each button press.
	 */
	bool getButtonPress(ButtonEnum b);
	bool getButtonClick(ButtonEnum b);
	/**@}*/

	/** @name Wii Controller functions */

	/** Call this to start the pairing sequence with a controller */
	INLINE void pair(void) {
		if (pBtd) pBtd->pairWithWiiRemote();
	};




	////////////////////////////////////////////////////////////////////////////
	// GET THE CURRENT RUMBLE STATUS - NEEDED FOR ALL HID COMMANDS
	////////////////////////////////////////////////////////////////////////////
	INLINE uint8_t rumble() {
		return HIDBuffer[2] & WII_LED::RUMBLE;
	}




	////////////////////////////////////////////////////////////////////////////
	// SET THE RAW BINARY VALUE FOR LEDS AND RUMBLE ON THE WII REMOTE
	////////////////////////////////////////////////////////////////////////////
	INLINE void setLedRaw(uint8_t value) {
		HIDBuffer[1] = WII_REPORT::OUT_LED;
		HIDBuffer[2] = value;
		HID_Command(HIDBuffer, sizeof(HIDBuffer));
	}





	////////////////////////////////////////////////////////////////////////////
	// TURN OFF ALL LEDS ON THE WII REMOTE
	////////////////////////////////////////////////////////////////////////////
	INLINE void setAllOff() {
		setLedRaw(0x00);
	}




	////////////////////////////////////////////////////////////////////////////
	// TURN OFF A PARTICULAR LED ON THE WII REMOTE
	////////////////////////////////////////////////////////////////////////////
	INLINE void setLedOff(uint8_t led) {
		setLedRaw(HIDBuffer[2] & ~led);
	}




	////////////////////////////////////////////////////////////////////////////
	// TURN ON A PARTICULAR LED ON THE WII REMOTE
	////////////////////////////////////////////////////////////////////////////
	INLINE void setLedOn(uint8_t led) {
		setLedRaw(HIDBuffer[2] | led);
	}




	////////////////////////////////////////////////////////////////////////////
	// TOGGLE A PARTICULAR LED ON THE WII REMOTE
	////////////////////////////////////////////////////////////////////////////
	INLINE void setLedToggle(uint8_t led) {
		setLedRaw(HIDBuffer[2] ^ led);
	}




	////////////////////////////////////////////////////////////////////////////
	// SET THE LED ACCORDING TO CURRENT PLAYER ID
	////////////////////////////////////////////////////////////////////////////
	INLINE void setLedStatus() {
		setLedOn(WII_LED::P1 << player);
	}



	/**@{*/
	/** Variable used to indicate if a Wiimote is connected. */
	bool wiimoteConnected;


protected:
	/** @name BluetoothService implementation */
	/**
	 * Used to pass acldata to the services.
	 * @param ACLData Incoming acldata.
	 */
	virtual void ACLData(const uint8_t* ACLData);
	/** Used to run part of the state machine. */
	virtual void Run();
	/** Use this to reset the service. */
	virtual void Reset();
	/**
	 * Called when the controller is successfully initialized.
	 * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
	 * This is useful for instance if you want to set the LEDs in a specific way.
	 */
	/**@}*/

private:


	void L2CAP_task(); // L2CAP state machine

	/* Variables filled from HCI event management */
	bool activeConnection; // Used to indicate if it's already has established a connection

	/* Variables used by high level L2CAP task */
	uint8_t l2cap_state;
//	uint8_t wii_event_flag; // Used for Wii flags

	uint16_t ButtonState;
	uint16_t OldButtonState;
	uint16_t ButtonClickState;

	uint8_t HIDBuffer[3]; // Used to store HID commands

	bool motionPlusInside; // True if it's a new Wiimote with the Motion Plus extension build into it

	/* L2CAP Channels */
	uint16_t control_scid;		// L2CAP source CID for HID_Control
	uint16_t interrupt_scid;	// L2CAP source CID for HID_Interrupt

	/* HID Commands */
	void HID_Command(const uint8_t* data, uint8_t nbytes);
	void setReportMode(uint8_t mode, bool continuous=false);

	void statusRequest(); // Used to update the Wiimote state and battery level

	uint32_t timer;

	// Stores the value in l2capinbuf[12]
	// (0x01: Battery is nearly empty)
	// (0x02: An Extension Controller is connected)
	// (0x04: Speaker enabled)
	// (0x08: IR enabled)
	// (0x10: LED1  ---  0x20: LED2  ---  0x40: LED3  ---  0x80: LED4)
	// uint8_t wiiState;
};


#endif
