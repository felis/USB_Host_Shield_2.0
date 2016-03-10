#include <hidcomposite.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#endif

class HIDSelector : public HIDComposite
{
public:
  HIDSelector(USB *p) : HIDComposite(p) {};

protected:
  void ParseHIDData(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf); // Called by the HIDComposite library
  bool SelectInterface(uint8_t iface, uint8_t proto);
};

bool HIDSelector::SelectInterface(uint8_t iface, uint8_t proto)
{
  if(proto != 0)
    return true;

  return false;
}

void HIDSelector::ParseHIDData(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
#if 1
        if (len && buf)  {
                Notify(PSTR("\r\n"), 0x80);
                for (uint8_t i = 0; i < len; i++) {
                        D_PrintHex<uint8_t > (buf[i], 0x80);
                        Notify(PSTR(" "), 0x80);
                }
        }
#endif
}


USB     Usb;
//USBHub     Hub(&Usb);
HIDSelector    hidSelector(&Usb);


void setup()
{
  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
    Serial.println("OSC did not start.");

  // Set this to higher values to enable more debug information
  // minimum 0x00, maximum 0xff, default 0x80
  UsbDEBUGlvl = 0xff;

  delay( 200 );
}

void loop()
{
  Usb.Task();
}

