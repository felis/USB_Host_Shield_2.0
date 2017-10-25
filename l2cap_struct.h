#ifndef __L2CAP_STRUCT_H__
#define __L2CAP_STRUCT_H__




struct PACKED L2CAP_S {

	// 0 - 1
	uint16_t					hci_handle;

	// 2 - 3
	uint16_t					hci_length;

	// 4 - 5
	uint16_t					l2c_length;

	// 6 - 7
	uint16_t					l2c_channel;

	union PACKED {
		uint8_t					l2c_payload[24];

		struct PACKED {

			// 8
			uint8_t				l2c_command;

			// 9
			union PACKED {
				uint8_t			identifier;
				uint8_t			wii_report;
			};

			// 10 - 11
			union PACKED {
				uint16_t		wii_buttons;
				uint16_t		data_length;
			};

			// 12 - 13
			uint16_t			hid_control;

			// 14 - 15
			union PACKED {
				uint16_t		hid_channel;
				uint16_t		hid_flags;
			};

			union PACKED {
				// 16 - 31
				uint8_t			data[16];

				// 16 - 19
				uint32_t		hid_result_full;

				// 16 - 19
				struct PACKED {
					// 16 - 17
					uint16_t	hid_result;

					// 18 - 19
					uint16_t	hid_status;

					// 20 - 21
					uint16_t	hid_config;
				};
			};
		};

	};
};




#endif //__L2CAP_STRUCT_H__
