Please see http://wiibrew.org/wiki/Wiimote#IR_Camera for the complete capabilities of the Wii camera. The IR camera code 
was written based on the above website and with support from Kristian Lauszus.
Must omit the "." in the name of the USB_Host_Shiled_2.0 library folder when inserting into the Arudino library folder
This library is large, if you run into memory problems when uploading to the arduino, comment out the #define DEBUG in the 
BTD.cpp and Wii.cpp files

If the IR camera is not being used, you can comment out #define WIICAMERA in Wii.cpp


This library impliments the following settings:

Report sensitivity mode: 00 00 00 00 00 00 90 00 41	 40 00	 Suggested by inio (high sensitivity)
Data Format: Extended mode (0x03).  Full mode is not working yet. The output reports 0x3e and 0x3f need tampering with
	In this mode the camera outputs x and y corridinates and a size dimension for the 4 brightest points. 
	CURRENTLY, CODE IS ONLY WRITTEN FOR THE 2 BRIGHTEST POINTS





