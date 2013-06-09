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

EXTRA_FLAGS = -D _FS_TINY=0
EXTRA_FLAGS += -D _USE_LFN=1

# Don't worry if you don't have external RAM, xmem2 detects this situation.
# You *WILL* be wanting to get some kind of external ram on your mega in order to
# do anything that is intense.
EXTRA_FLAGS += -D HAVEXMEM=1
EXTRA_FLAGS += -D EXT_RAM_STACK=1
EXTRA_FLAGS += -D EXT_RAM_HEAP=1



EXTRA_FLAGS += -D DISABLE_SERIAL1
EXTRA_FLAGS += -D DISABLE_SERIAL2
EXTRA_FLAGS += -D DISABLE_SERIAL3

# You should not need to change this, but I place it here if you want to play.
# These are the defaults for the optimization of the flash and ram
#OPT_FLAGS = -Os -fno-exceptions -ffunction-sections -fdata-sections -MMD

# The following are the libraries used.
LIB_DIRS =
LIB_DIRS += ../libraries/xmem
LIB_DIRS += ../libraries/USB_Host_Shield_2.0
LIB_DIRS += ../libraries/generic_storage

# And finally, the part that brings everything together for you.
include ../Arduino_Makefile_master/_Makefile.master
