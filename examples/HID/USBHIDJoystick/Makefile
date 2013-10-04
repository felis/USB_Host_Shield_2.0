#
# These are set for a mega 1280 + quadram plus my serial patch.
# If you lack quadram, or want to disable LFN, just change _FS_TINY=1 _USE_LFN=0
#
# If your board is a mega 2560 uncomment the following two lines
# BOARD = mega2560
# PROGRAMMER = wiring
# ...and then comment out the following two lines
BOARD = mega
PROGRAMMER = arduino

# set your Arduino tty port here
PORT = /dev/ttyUSB0


# uncomment the next line to enable debugging
#EXTRA_FLAGS += -D DEBUG_USB_HOST=1

#
# Advanced debug on Serial3
#

# uncomment the next line to enable debug on Serial3
#EXTRA_FLAGS += -D USB_HOST_SERIAL=Serial3

# The following are the libraries used.
LIB_DIRS =
LIB_DIRS += ../libraries/USB_Host_Shield_2_0
# And finally, the part that brings everything together for you.
include ../Arduino_Makefile_master/_Makefile.master
