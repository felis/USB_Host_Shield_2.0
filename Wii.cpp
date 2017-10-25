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






#include "Wii.h"
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data
//#define PRINTREPORT // Uncomment to print the report send by the Wii controllers


const uint32_t WII_BUTTONS[] = {
	0x00008, // UP
	0x00002, // RIGHT
	0x00004, // DOWN
	0x00001, // LEFT

	0, // Skip
	0x00010, // PLUS
	0x00100, // TWO
	0x00200, // ONE

	0x01000, // MINUS
	0x08000, // HOME
	0x10000, // Z
	0x20000, // C

	0x00400, // B
	0x00800, // A
};



#define CONTROL_DCID	0x0060
#define INTERRUPT_DCID	0x0061




WII::WII(BTD *p) : BluetoothService(p) {
	Reset();
}



void WII::Reset() {
	HIDBuffer[0]		= WII_HID_REPORT;
	HIDBuffer[1]		= 0x00;
	HIDBuffer[2]		= 0x00;

	control_scid		= 0x0000;
	interrupt_scid		= 0x0000;

	wiimoteConnected	= false;
	activeConnection	= false;
	motionPlusInside	= false;

	ButtonState			= 0;
	OldButtonState		= 0;
	ButtonClickState	= 0;
	timer				= 0;

	l2cap_state			= L2CAP_WAIT;

	BluetoothService::Reset();
}




void WII::disconnect() { // Use this void to disconnect any of the controllers
	pBtd->l2cap_disconnection_request(hci_handle, ++identifier, interrupt_scid, INTERRUPT_DCID);
	Reset();
	timer = (uint32_t)millis() + 1000; // We have to wait for the message before the rest of the channels can be deactivated
	l2cap_state = L2CAP_INTERRUPT_DISCONNECT;
}




void WII::ACLData(const uint8_t* l2capinbuf) {
	const L2CAP_S *l2c = (const L2CAP_S*) l2capinbuf;


	if (l2c->l2c_command == L2CAP_CMD_CONNECTION_REQUEST) {
		if (!pBtd->l2capConnectionClaimed && pBtd->incomingWii && !wiimoteConnected && !activeConnection) {
			if (l2c->hid_control == HID_CTRL_PSM) {
				motionPlusInside				= pBtd->motionPlusInside;
				pBtd->incomingWii				= false;
				pBtd->l2capConnectionClaimed	= true; // Claim that the incoming connection belongs to this service
				activeConnection				= true;
				hci_handle						= pBtd->hci_handle; // Store the HCI Handle for the connection
				l2cap_state						= L2CAP_WAIT;
			}
		}
	}

	if(!checkHciHandle(l2c, hci_handle)) return;


	if (l2c->l2c_channel == 0x0001) { // l2cap_control - Channel ID for ACL-U

#ifdef DEBUG_USB_HOST
	Serial.print("\r\n L2CAP RESPONSE : ");
	char output[4] = {0,0,0,0};
	for (int i=0; i<l2c->hci_length+4; i++) {
		sprintf(output, "%02x:", l2capinbuf[i]);
		Serial.print(output);
	}
#endif

		if (l2c->l2c_command == L2CAP_CMD_COMMAND_REJECT) {
#ifdef DEBUG_USB_HOST
			Notify(PSTR("\r\nL2CAP Command Rejected - Reason: "), 0x80);
			char output[4] = {0,0,0,0};
			for (int i=0; i<16; i++) {
				printf("test");
				sprintf(output, "%02x:", l2capinbuf[i]);
				Serial.print(output);
			}
#endif


		} else if (l2c->l2c_command == L2CAP_CMD_CONNECTION_RESPONSE) {
			Notify(PSTR("\r\nL2CAP_CMD_CONNECTION_RESPONSE"), 0x80);

			if (!l2c->hid_result_full) { // Success

				if (l2c->hid_channel == CONTROL_DCID) {
					Notify(PSTR("\r\nHID Control Connection Complete"), 0x80);
					identifier		= l2c->identifier;
					control_scid	= l2c->hid_control;
					l2cap_set_flag(L2CAP_FLAG_CONTROL_CONNECTED);

				} else if (l2c->hid_channel == INTERRUPT_DCID) {
					Notify(PSTR("\r\nHID Interrupt Connection Complete"), 0x80);
					identifier		= l2c->identifier;
					interrupt_scid	= l2c->hid_control;
					l2cap_set_flag(L2CAP_FLAG_INTERRUPT_CONNECTED);
				}
			}



		} else if (l2c->l2c_command == L2CAP_CMD_CONNECTION_REQUEST) {
			Notify(PSTR("\r\nL2CAP Connection Request - PSM: "), 0x80);
			D_PrintHex<uint8_t > (l2capinbuf[13], 0x80);
			Notify(PSTR(" "), 0x80);
			D_PrintHex<uint8_t > (l2capinbuf[12], 0x80);
			Notify(PSTR(" SCID: "), 0x80);
			D_PrintHex<uint8_t > (l2capinbuf[15], 0x80);
			Notify(PSTR(" "), 0x80);
			D_PrintHex<uint8_t > (l2capinbuf[14], 0x80);
			Notify(PSTR(" Identifier: "), 0x80);
			D_PrintHex<uint8_t > (l2c->identifier, 0x80);

			if (l2c->hid_control == HID_CTRL_PSM) {
				identifier			= l2c->identifier;
				control_scid		= l2c->hid_channel;
				l2cap_set_flag(L2CAP_FLAG_CONNECTION_CONTROL_REQUEST);

			} else if (l2c->hid_control == HID_INTR_PSM) {
				identifier			= l2c->identifier;
				interrupt_scid		= l2c->hid_channel;
				l2cap_set_flag(L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST);
			}



		} else if(l2c->l2c_command == L2CAP_CMD_CONFIG_RESPONSE) {
			Notify(PSTR("\r\nL2CAP_CMD_CONFIG_RESPONSE"), 0x80);

			if(!l2c->hid_result) { // Success
				if (l2c->hid_control == CONTROL_DCID) {
					Notify(PSTR("\r\nHID Control Configuration Complete"), 0x80);
					identifier = l2c->identifier;
					l2cap_set_flag(L2CAP_FLAG_CONFIG_CONTROL_SUCCESS);

				} else if (l2c->hid_control == INTERRUPT_DCID) {
					Notify(PSTR("\r\nHID Interrupt Configuration Complete"), 0x80);
					identifier = l2c->identifier;
					l2cap_set_flag(L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS);
				}
			}



		} else if(l2c->l2c_command == L2CAP_CMD_CONFIG_REQUEST) {
			Notify(PSTR("\r\nL2CAP_CMD_CONFIG_REQUEST"), 0x80);

			if (l2c->hid_control == CONTROL_DCID) {
				Notify(PSTR("\r\nHID Control Configuration Request"), 0x80);
				pBtd->l2cap_config_response(hci_handle, l2c->identifier, control_scid);

			} else if (l2c->hid_control == INTERRUPT_DCID) {
				Notify(PSTR("\r\nHID Interrupt Configuration Request"), 0x80);
				pBtd->l2cap_config_response(hci_handle, l2c->identifier, interrupt_scid);
			}



		} else if (l2c->l2c_command == L2CAP_CMD_DISCONNECT_REQUEST) {
			Notify(PSTR("\r\nL2CAP_CMD_DISCONNECT_REQUEST"), 0x80);

			if (l2c->hid_control == CONTROL_DCID) {
				Notify(PSTR("\r\nDisconnect Request: Control Channel"), 0x80);
				identifier = l2c->identifier;
				pBtd->l2cap_disconnection_response(hci_handle, identifier, CONTROL_DCID, control_scid);
				Reset();

			} else if (l2c->hid_control == INTERRUPT_DCID) {
				Notify(PSTR("\r\nDisconnect Request: Interrupt Channel"), 0x80);
				identifier = l2c->identifier;
				pBtd->l2cap_disconnection_response(hci_handle, identifier, INTERRUPT_DCID, interrupt_scid);
				Reset();
			}



		} else if(l2c->l2c_command == L2CAP_CMD_DISCONNECT_RESPONSE) {
			Notify(PSTR("\r\nL2CAP_CMD_DISCONNECT_RESPONSE"), 0x80);

			if (l2c->hid_control == control_scid) {
				Notify(PSTR("\r\nDisconnect Response: Control Channel"), 0x80);
				identifier = l2c->identifier;
				l2cap_set_flag(L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE);

			} else if (l2c->hid_control == interrupt_scid) {
				Notify(PSTR("\r\nDisconnect Response: Interrupt Channel"), 0x80);
				identifier = l2c->identifier;
				l2cap_set_flag(L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE);
			}

		} else {
			identifier = l2c->identifier;
			Notify(PSTR("\r\nL2CAP Unknown Signaling Command: "), 0x80);
			D_PrintHex<uint8_t > (l2capinbuf[8], 0x80);
		}



	} else if (l2c->l2c_channel == INTERRUPT_DCID) {
		Notify(PSTR("\r\nL2CAP Interrupt"), 0x80);
		if (l2c->l2c_command == 0xA1) { // HID_THDR_DATA_INPUT


			// These reports include the buttons
			if ( (l2c->wii_report >= 0x20 && l2c->wii_report <= 0x22)
			  || (l2c->wii_report >= 0x30 && l2c->wii_report <= 0x37)
			  || (l2c->wii_report >= 0x3e && l2c->wii_report <= 0x3f)) {

				// Update button state and click state variables
				ButtonState				= l2c->wii_buttons;
				if(ButtonState != OldButtonState) {
					ButtonClickState	= ButtonState & ~OldButtonState;
					OldButtonState		= ButtonState;
				}

				Notify(PSTR("\r\nButtonState: "), 0x80);
				D_PrintHex<uint32_t > (ButtonState, 0x80);
			}


			switch(l2c->wii_report) {



				case WII_REPORT::IN_STATUS:
					// Status Information
					// (a1) 20 BB BB LF 00 00 VV
					break;


				// Read Memory Data
				case WII_REPORT::IN_READ:
					break;


				// Acknowledge output report, return function result
				case WII_REPORT::IN_ACKNOWLEDGE:
					if(l2capinbuf[13] != 0x00) { // Check if there is an error
						Notify(PSTR("\r\nCommand failed: "), 0x80);
						D_PrintHex<uint8_t > (l2capinbuf[12], 0x80);
					}
					break;



				case WII_REPORT::BTN_ONLY:
					// Core Buttons
					// (a1) 30 BB BB
					break;



				case WII_REPORT::BTN_ACC:
					// Core Buttons and Accelerometer
					// (a1) 31 BB BB AA AA AA
					break;



				case WII_REPORT::BTN_EXT_8:
					// Core Buttons with 8 Extension bytes
					// (a1) 32 BB BB EE EE EE EE EE EE EE EE
					break;



				case WII_REPORT::BTN_ACC_IR_12:
					// Core Buttons with Accelerometer and 12 IR bytes
					// (a1) 33 BB BB AA AA AA II II II II II II II II II II II II
					break;



				case WII_REPORT::BTN_EXT_19:
					// Core Buttons with 19 Extension bytes
					// (a1) 34 BB BB EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE
					break;



				case WII_REPORT::BTN_ACC_EXT_16:
					// Core Buttons, Accelerometer, 16 Extension Bytes
					// (a1) 35 BB BB AA AA AA EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE
					break;



				case WII_REPORT::BTN_IR_10_EXT_9:
					// Core buttons, 10 IR bytes, and 8 EXT bytes
					// (a1) 36 BB BB II II II II II II II II II II EE EE EE EE EE EE EE EE EE
					break;



				case WII_REPORT::BTN_ACC_IR_10_EXT_6:
					// Core buttons, Accelerometer, 10 IR bytes, 6 Extension bytes
					// (a1) 37 BB BB AA AA AA II II II II II II II II II II EE EE EE EE EE EE
					break;



				case WII_REPORT::EXT_21:
					// 21 extension bytes - only mode to NOT report button data
					// (a1) 3d EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE EE
					break;



				case WII_REPORT::BTN_ACC_IR_36_A:
					// Core Buttons with Accelerometer and 32 IR bytes
					// (a1 31 BB BB AA AA AA II II II II II II II II II II II II II II II II II II II II II II II II II II II II II II II II
					break;



				case WII_REPORT::BTN_ACC_IR_36_B:
					// Core Buttons with Accelerometer and 32 IR bytes
					// (a1 31 BB BB AA AA AA II II II II II II II II II II II II II II II II II II II II II II II II II II II II II II II II
					break;


#ifdef DEBUG_USB_HOST
				default:
					Notify(PSTR("\r\nUnknown Report type: "), 0x80);
					D_PrintHex<uint8_t > (l2c->wii_report, 0x80);
					break;
#endif
			}
		}
	}


	L2CAP_task();
}








void WII::L2CAP_task() {
	switch(l2cap_state) {
			/* These states are used if the Wiimote is the host */




		case L2CAP_CONTROL_SUCCESS:
			if(l2cap_check_flag(L2CAP_FLAG_CONFIG_CONTROL_SUCCESS)) {
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nHID Control Successfully Configured"), 0x80);
#endif
				l2cap_state = L2CAP_INTERRUPT_SETUP;
			}
			break;





		case L2CAP_INTERRUPT_SETUP:
			if(l2cap_check_flag(L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST)) {
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nHID Interrupt Incoming Connection Request"), 0x80);
#endif
				pBtd->l2cap_connection_response(hci_handle, identifier, INTERRUPT_DCID, interrupt_scid, PENDING);
				delay(1);
				pBtd->l2cap_connection_response(hci_handle, identifier, INTERRUPT_DCID, interrupt_scid, SUCCESSFUL);
				identifier++;
				delay(1);
				pBtd->l2cap_config_request(hci_handle, identifier, interrupt_scid);

				l2cap_state = L2CAP_INTERRUPT_CONFIG_REQUEST;
			}
			break;





			/* These states are used if the Arduino is the host */
		case L2CAP_CONTROL_CONNECT_REQUEST:
			if(l2cap_check_flag(L2CAP_FLAG_CONTROL_CONNECTED)) {
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nSend HID Control Config Request"), 0x80);
#endif
				identifier++;
				pBtd->l2cap_config_request(hci_handle, identifier, control_scid);
				l2cap_state = L2CAP_CONTROL_CONFIG_REQUEST;
			}
			break;





		case L2CAP_CONTROL_CONFIG_REQUEST:
			if(l2cap_check_flag(L2CAP_FLAG_CONFIG_CONTROL_SUCCESS)) {
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nSend HID Interrupt Connection Request"), 0x80);
#endif
				identifier++;
				pBtd->l2cap_connection_request(hci_handle, identifier, INTERRUPT_DCID, HID_INTR_PSM);
				l2cap_state = L2CAP_INTERRUPT_CONNECT_REQUEST;
			}
			break;





		case L2CAP_INTERRUPT_CONNECT_REQUEST:
			if(l2cap_check_flag(L2CAP_FLAG_INTERRUPT_CONNECTED)) {
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nSend HID Interrupt Config Request"), 0x80);
#endif
				identifier++;
				pBtd->l2cap_config_request(hci_handle, identifier, interrupt_scid);
				l2cap_state = L2CAP_INTERRUPT_CONFIG_REQUEST;
			}
			break;





		case L2CAP_INTERRUPT_CONFIG_REQUEST:
			if(l2cap_check_flag(L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS)) { // Now the HID channels is established
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nHID Channels Established"), 0x80);
#endif
				pBtd->connectToWii	= false;
				l2cap_state			= TURN_ON_LED;
				statusRequest();
			}
			break;





			/* The next states are in run() */

		case L2CAP_INTERRUPT_DISCONNECT:
			if(l2cap_check_flag(L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE) && ((int32_t)((uint32_t)millis() - timer) >= 0L)) {
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nDisconnected Interrupt Channel"), 0x80);
#endif
				identifier++;
				pBtd->l2cap_disconnection_request(hci_handle, identifier, control_scid, CONTROL_DCID);
				l2cap_state = L2CAP_CONTROL_DISCONNECT;
			}
			break;





		case L2CAP_CONTROL_DISCONNECT:
			if(l2cap_check_flag(L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE)) {
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nDisconnected Control Channel"), 0x80);
#endif
				pBtd->hci_disconnect(hci_handle);
				/*
				hci_handle = -1; // Reset handle
				l2cap_event_flag = 0; // Reset flags
				l2cap_state = L2CAP_WAIT;
				*/
				Reset();
			}
			break;
	}
}











void WII::Run() {
	if(l2cap_state == L2CAP_INTERRUPT_DISCONNECT && ((int32_t)((uint32_t)millis() - timer) >= 0L)) {
		L2CAP_task(); // Call the rest of the disconnection routine after we have waited long enough
	}


	switch(l2cap_state) {


		case L2CAP_WAIT:
			if(pBtd->connectToWii && !pBtd->l2capConnectionClaimed && !wiimoteConnected && !activeConnection) {
				pBtd->l2capConnectionClaimed = true;
				activeConnection = true;
				motionPlusInside = pBtd->motionPlusInside;
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nSend HID Control Connection Request"), 0x80);
#endif
				hci_handle = pBtd->hci_handle; // Store the HCI Handle for the connection
				l2cap_event_flag = 0; // Reset flags
				identifier = 0;
				pBtd->l2cap_connection_request(hci_handle, identifier, CONTROL_DCID, HID_CTRL_PSM);
				l2cap_state = L2CAP_CONTROL_CONNECT_REQUEST;
			} else if(l2cap_check_flag(L2CAP_FLAG_CONNECTION_CONTROL_REQUEST)) {
#ifdef DEBUG_USB_HOST
				Notify(PSTR("\r\nHID Control Incoming Connection Request"), 0x80);
#endif
				pBtd->l2cap_connection_response(hci_handle, identifier, CONTROL_DCID, control_scid, PENDING);
				delay(1);
				pBtd->l2cap_connection_response(hci_handle, identifier, CONTROL_DCID, control_scid, SUCCESSFUL);
				identifier++;
				delay(1);
				pBtd->l2cap_config_request(hci_handle, identifier, control_scid);
				l2cap_state = L2CAP_CONTROL_SUCCESS;
			}
			break;





		case TURN_ON_LED:
			wiimoteConnected = true;
			setReportMode(WII_REPORT::BTN_ONLY);
			setLedStatus();
			l2cap_state = L2CAP_DONE;
			break;





		case L2CAP_DONE:
			break;
	}
}









/************************************************************/
/*                    HID Commands                          */
/************************************************************/
void WII::HID_Command(const uint8_t* data, uint8_t nbytes) {
	pBtd->L2CAP_Command(
		hci_handle,
		data,
		nbytes,
		motionPlusInside ? interrupt_scid : control_scid
	);
}




void WII::setReportMode(uint8_t mode, bool continuous) {
	Notify(PSTR("\r\nReport mode was changed to: "), 0x80);
	D_PrintHex<uint8_t > (mode, 0x80);

	uint8_t cont = (((uint8_t)continuous)<<2) + rumble();

	HID_COMMAND(
		WII_HID_REPORT,
		WII_REPORT::OUT_DATA,
		cont,
		mode,
	);
}




void WII::statusRequest() {
	HID_COMMAND(
		WII_HID_REPORT,
		WII_REPORT::OUT_STATUS,
		rumble()
	);
}



/************************************************************/
/*                    WII Commands                          */
/************************************************************/

bool WII::getButtonPress(ButtonEnum b) { // Return true when a button is pressed
	return (ButtonState & pgm_read_dword(&WII_BUTTONS[(uint8_t)b]));
}

bool WII::getButtonClick(ButtonEnum b) { // Only return true when a button is clicked
	uint32_t button = pgm_read_dword(&WII_BUTTONS[(uint8_t)b]);
	bool click = (ButtonClickState & button);
	ButtonClickState &= ~button; // clear "click" event
	return click;
}
