#include <avrpins.h>
#include <max3421e.h>
#include <usbhost.h>
#include <usb_ch9.h>
#include <Usb.h>
#include <usbhub.h>
#include <avr/pgmspace.h>
#include <address.h>

#include <printhex.h>
#include <message.h>
#include <hexdump.h>
#include <parsetools.h>

/* variables */
uint8_t rcode;
uint8_t usbstate;
//uint8_t buf[sizeof(USB_DEVICE_DESCRIPTOR)];
USB_DEVICE_DESCRIPTOR buf;

/* objects */
USB Usb;
//USBHub hub(&Usb);

            
void setup()
{
  Serial.begin( 115200 );
  Notify(PSTR("\r\nCircuits At Home 2011"));
  Notify(PSTR("\r\nUSB Host Shield Quality Control Routine"));  
  /* check init */
//  if (Usb.Init() == -1) {
//      Notify(PSTR("\r\nOSCOKIRQ failed to assert"));
//      halt55();
//  }
  /* SPI quick test - check revision register */
  Notify(PSTR("\r\nReading REVISION register... Die revision "));
  {
    uint8_t tmpbyte = Usb.regRd( rREVISION );
    switch( tmpbyte ) {
      case( 0x01):  //rev.01
        Notify(PSTR("01"));
        break;
    case( 0x12):  //rev.02
        Notify(PSTR("02"));
        break;
    case( 0x13):  //rev.03
        Notify(PSTR("03"));
        break;
    default:
      Notify(PSTR("invalid. Value returned: "));
      print_hex( tmpbyte, 8 );
      halt55();
      break;
    }//switch( tmpbyte...
  }//check revision register
  /* SPI long test */
  {
    Notify(PSTR("\r\nSPI long test. Transfers 1MB of data. Each dot is 64K")); 
    uint8_t sample_wr = 0;
    uint8_t sample_rd = 0;
    uint8_t gpinpol_copy = Usb.regRd( rGPINPOL );
    for( uint8_t i = 0; i < 16; i++ ) {
      for( uint16_t j = 0; j < 65535; j++ ) {
        Usb.regWr( rGPINPOL, sample_wr );
        sample_rd = Usb.regRd( rGPINPOL );
        if( sample_rd != sample_wr ) {
          Notify(PSTR("\r\nTest failed.  "));
          Notify(PSTR("Value written: "));
          print_hex( sample_wr, 8 );
          Notify(PSTR(" read: "));
          print_hex( sample_rd, 8 ); 
          halt55();
        }//if( sample_rd != sample_wr..
         sample_wr++;
      }//for( uint16_t j...
      Notify(PSTR("."));
    }//for( uint8_t i...
    Usb.regWr( rGPINPOL, gpinpol_copy );
    Notify(PSTR(" SPI long test passed"));    
  }//SPI long test
  /* GPIO test */
  /* in order to simplify board layout, GPIN pins on text fixture are connected to GPOUT */
  /* in reverse order, i.e, GPIN0 is connected to GPOUT7, GPIN1 to GPOUT6, etc. */
  {
    uint8_t tmpbyte;
    Notify(PSTR("\r\nGPIO test. Connect GPIN0 to GPOUT7, GPIN1 to GPOUT6, and so on"));
    for( uint8_t sample_gpio = 0; sample_gpio < 255; sample_gpio++ ) {
      Usb.gpioWr( sample_gpio );
      tmpbyte = Usb.gpioRd();
      /* bit reversing code copied vetbatim from http://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious */
      tmpbyte  = ((tmpbyte * 0x0802LU & 0x22110LU) | (tmpbyte * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
      if( sample_gpio != tmpbyte ) {
        Notify(PSTR("\r\nTest failed. Value written: "));
        print_hex( sample_gpio, 8 );
        Notify(PSTR(" Value read: "));
        print_hex( tmpbyte , 8 );
        Notify(PSTR(" "));
        press_any_key();
        break;
      }//if( sample_gpio != tmpbyte...
    }//for( uint8_t sample_gpio...
    Notify(PSTR("\r\nGPIO test passed."));            
  }//GPIO test
  /* PLL test. Stops/starts MAX3421E oscillator several times */
  {
  Notify(PSTR("\r\nPLL test. 100 chip resets will be performed"));
  /* check current state of the oscillator */  
  if(!( Usb.regRd( rUSBIRQ ) & bmOSCOKIRQ )) {  //wrong state - should be on
    Notify(PSTR("\r\nCurrent oscillator state unexpected."));  
    press_any_key();
  }  
  /* Restart oscillator */
  Notify(PSTR("\r\nResetting oscillator"));
  for( uint16_t i = 0; i < 101; i++ ) {
    Notify(PSTR("\rReset number "));
    Serial.print( i, DEC );
    Usb.regWr( rUSBCTL, bmCHIPRES );  //reset    
    if( Usb.regRd( rUSBIRQ ) & bmOSCOKIRQ ) { //wrong state - should be off
      Notify(PSTR("\r\nCurrent oscillator state unexpected."));
      halt55();
    }
    Usb.regWr( rUSBCTL, 0x00 ); //release from reset
    uint16_t j = 0;
    for( j = 0; j < 65535; j++ ) { //tracking off to on time
      if( Usb.regRd( rUSBIRQ ) & bmOSCOKIRQ ) {
        Notify(PSTR(" Time to stabilize - "));
        Serial.print( j, DEC );
        Notify(PSTR(" cycles"));
        break;
      }
    }//for( uint16_t j = 0; j < 65535; j++    
    if( j == 0 ) {
      Notify(PSTR("PLL failed to stabilize"));
      press_any_key();
    }
  }//for( uint8_t i = 0; i < 255; i++
    
  }//PLL test
  /* initializing USB stack */
    if (Usb.Init() == -1) {
      Notify(PSTR("\r\nOSCOKIRQ failed to assert"));
      halt55();
  }
  Notify(PSTR("\r\nChecking USB device communication.\r\n"));
}

void loop()
{
  delay( 200 );
  Usb.Task();
  usbstate = Usb.getUsbTaskState();
  //usbstate = Usb.getVbusState();
  //Notify(PSTR("\r\nUSB Task State: "));
  //print_hex( usbstate, 8 );
  /**/
  
  switch( usbstate ) {
    case( USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE ):
      Notify(PSTR("\rWaiting for device ..."));
      break;  
    case( USB_ATTACHED_SUBSTATE_RESET_DEVICE ):
      Notify(PSTR("\r\nDevice connected. Resetting"));
      break;
    case( USB_ATTACHED_SUBSTATE_WAIT_SOF ):
      Notify(PSTR("\rReset complete. Waiting for the first SOF..."));
      break;
    case( USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE ):
      Notify(PSTR("\r\nSOF generation started. Enumerating device."));
      break;
    case( USB_STATE_ADDRESSING ):
      Notify(PSTR("\r\nSetting device address"));      
      break;
    case( USB_STATE_RUNNING ):
      Notify(PSTR("\r\nGetting device descriptor"));
      rcode = Usb.getDevDescr( 1, 0, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t*)&buf );
      
        if( rcode ) {
          Notify(PSTR("\rError reading device descriptor. Error code "));
          print_hex( rcode, 8 );
        }
        else {
          /**/
          Notify(PSTR("\r\nDescriptor Length:\t"));
          print_hex( buf.bLength, 8 );
          Notify(PSTR("\r\nDescriptor type:\t"));
          print_hex( buf.bDescriptorType, 8 );
          Notify(PSTR("\r\nUSB version:\t\t"));
          print_hex( buf.bcdUSB, 16 );
          Notify(PSTR("\r\nDevice class:\t\t")); 
          print_hex( buf.bDeviceClass, 8 );
          Notify(PSTR("\r\nDevice Subclass:\t"));
          print_hex( buf.bDeviceSubClass, 8 );
          Notify(PSTR("\r\nDevice Protocol:\t"));
          print_hex( buf.bDeviceProtocol, 8 );
          Notify(PSTR("\r\nMax.packet size:\t"));
          print_hex( buf.bMaxPacketSize0, 8 );
          Notify(PSTR("\r\nVendor  ID:\t\t"));
          print_hex( buf.idVendor, 16 );
          Notify(PSTR("\r\nProduct ID:\t\t"));
          print_hex( buf.idProduct, 16 );
          Notify(PSTR("\r\nRevision ID:\t\t"));
          print_hex( buf.bcdDevice, 16 );
          Notify(PSTR("\r\nMfg.string index:\t"));
          print_hex( buf.iManufacturer, 8 );
          Notify(PSTR("\r\nProd.string index:\t"));
          print_hex( buf.iProduct, 8 );
          Notify(PSTR("\r\nSerial number index:\t"));
          print_hex( buf.iSerialNumber, 8 );
          Notify(PSTR("\r\nNumber of conf.:\t"));
          print_hex( buf.bNumConfigurations, 8 );
          /**/
          Notify(PSTR("\r\n\nAll tests passed. Press RESET to restart test"));
          while(1);
        }
        break;
    case( USB_STATE_ERROR ):
      Notify(PSTR("\rUSB state machine reached error state"));
      break;
      
    default:
      //Notify(PSTR("\rUndefined State"));
      break;
    }//switch( usbstate...
  
}//loop()...

/* constantly transmits 0x55 via SPI to aid probing */
void halt55()
{

    Notify(PSTR("\r\nUnrecoverable error - test halted!!"));
    Notify(PSTR("\r\n0x55 pattern is transmitted via SPI"));
    Notify(PSTR("\r\nPress RESET to restart test"));
    
    while( 1 ) {
      Usb.regWr( 0x55, 0x55 );
    }
}
/* prints hex numbers with leading zeroes */
void print_hex(int v, int num_places)
{
  int mask=0, n, num_nibbles, digit;

  for (n=1; n<=num_places; n++) {
    mask = (mask << 1) | 0x0001;
  }
  v = v & mask; // truncate v to specified number of places

  num_nibbles = num_places / 4;
  if ((num_places % 4) != 0) {
    ++num_nibbles;
  }
  do {
    digit = ((v >> (num_nibbles-1) * 4)) & 0x0f;
    Serial.print(digit, HEX);
  } while(--num_nibbles);
}
/* prints "Press any key" and returns when key is pressed */
void press_any_key()
{
  Notify(PSTR("\r\nPress any key to continue..."));
  while( Serial.available() == 0 ); //wait for input
  Serial.read();                    //empty input buffer    
  return;
}
