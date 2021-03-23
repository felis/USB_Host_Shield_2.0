/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */
/* MAX3421E-based USB Host Library header file */


#if !defined(_usb_h_) || defined(_USBHOST_H_)
#error "Never include usbhost.h directly; include Usb.h instead"
#else
#define _USBHOST_H_

#if USING_SPI4TEENSY3
#include <spi4teensy3.h>
#include <sys/types.h>
#endif

/* SPI initialization */
template< typename SPI_CLK, typename SPI_MOSI, typename SPI_MISO, typename SPI_SS > class SPi {
public:
#if USING_SPI4TEENSY3
        static void init() {
                // spi4teensy3 inits everything for us, except /SS
                // CLK, MOSI and MISO are hard coded for now.
                // spi4teensy3::init(0,0,0); // full speed, cpol 0, cpha 0
                spi4teensy3::init(); // full speed, cpol 0, cpha 0
                SPI_SS::SetDirWrite();
                SPI_SS::Set();
        }
#elif defined(SPI_HAS_TRANSACTION)
        static void init() {
                USB_SPI.begin(); // The SPI library with transaction will take care of setting up the pins - settings is set in beginTransaction()
                SPI_SS::SetDirWrite();
                SPI_SS::Set();
        }
#elif defined(STM32F4)
#warning "You need to initialize the SPI interface manually when using the STM32F4 platform"
        static void init() {
                // Should be initialized by the user manually for now
        }
#elif !defined(SPDR)
        static void init() {
                SPI_SS::SetDirWrite();
                SPI_SS::Set();
                USB_SPI.begin();
#if defined(__MIPSEL__)
                USB_SPI.setClockDivider(1);
#elif defined(__ARDUINO_X86__)
                #ifdef SPI_CLOCK_1M // Hack used to check if setClockSpeed is available
                    USB_SPI.setClockSpeed(12000000); // The MAX3421E can handle up to 26MHz, but in practice this was the maximum that I could reliably use
                #else
                    USB_SPI.setClockDivider(SPI_CLOCK_DIV2); // This will set the SPI frequency to 8MHz - it could be higher, but it is not supported in the old API
                #endif
#elif !defined(RBL_NRF51822) && !defined(NRF52_SERIES)
                USB_SPI.setClockDivider(4); // Set speed to 84MHz/4=21MHz - the MAX3421E can handle up to 26MHz
#endif
        }
#else
        static void init() {
                //uint8_t tmp;
                SPI_CLK::SetDirWrite();
                SPI_MOSI::SetDirWrite();
                SPI_MISO::SetDirRead();
                SPI_SS::SetDirWrite();
                /* mode 00 (CPOL=0, CPHA=0) master, fclk/2. Mode 11 (CPOL=11, CPHA=11) is also supported by MAX3421E */
                SPCR = 0x50;
                SPSR = 0x01; // 0x01
                /**/
                //tmp = SPSR;
                //tmp = SPDR;
        }
#endif
};

/* SPI pin definitions. see avrpins.h   */
#if defined(PIN_SPI_SCK) && defined(PIN_SPI_MOSI) && defined(PIN_SPI_MISO) && defined(PIN_SPI_SS)
// Use pin defines: https://github.com/arduino/Arduino/pull/4814
// Based on: https://www.mikeash.com/pyblog/friday-qa-2015-03-20-preprocessor-abuse-and-optional-parentheses.html
#define NOTHING_EXTRACT
#define EXTRACT(...) EXTRACT __VA_ARGS__
#define PASTE(x, ...) x ## __VA_ARGS__
#define EVALUATING_PASTE(x, ...) PASTE(x, __VA_ARGS__)
#define UNPAREN(x) EVALUATING_PASTE(NOTHING_, EXTRACT x)
#define APPEND_PIN(pin) P ## pin // Appends the pin to 'P', e.g. 1 becomes P1
#define MAKE_PIN(x) EVALUATING_PASTE(APPEND_, PIN(UNPAREN(x)))
typedef SPi< MAKE_PIN(PIN_SPI_SCK), MAKE_PIN(PIN_SPI_MOSI), MAKE_PIN(PIN_SPI_MISO), MAKE_PIN(PIN_SPI_SS) > spi;
#undef MAKE_PIN
#elif defined(__AVR_ATmega1280__) || (__AVR_ATmega2560__) || defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
typedef SPi< Pb1, Pb2, Pb3, Pb0 > spi;
#elif  defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
typedef SPi< Pb5, Pb3, Pb4, Pb2 > spi;
#elif defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
typedef SPi< Pb7, Pb5, Pb6, Pb4 > spi;
#elif (defined(CORE_TEENSY) && (defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) || defined(__MKL26Z64__))) || defined(__ARDUINO_ARC__) || defined(__ARDUINO_X86__) || defined(__MIPSEL__) || defined(STM32F4)
typedef SPi< P13, P11, P12, P10 > spi;
#elif defined(ARDUINO_SAM_DUE) && defined(__SAM3X8E__)
typedef SPi< P76, P75, P74, P10 > spi;
#elif defined(RBL_NRF51822)
typedef SPi< P16, P18, P17, P10 > spi;
#elif defined(ESP8266)
typedef SPi< P14, P13, P12, P15 > spi;
#elif defined(ESP32)
typedef SPi< P18, P23, P19, P5 > spi;
#elif defined(ARDUINO_NRF52840_FEATHER)
typedef SPi< P26, P25, P24, P5 > spi;
#else
#error "No SPI entry in usbhost.h"
#endif

typedef enum {
        vbus_on = 0,
        vbus_off = GPX_VBDET
} VBUS_t;

template< typename SPI_SS, typename INTR > class MAX3421e /* : public spi */ {
        static uint8_t vbusState;

public:
        MAX3421e();
        void regWr(uint8_t reg, uint8_t data);
        uint8_t* bytesWr(uint8_t reg, uint8_t nbytes, uint8_t* data_p);
        void gpioWr(uint8_t data);
        uint8_t regRd(uint8_t reg);
        uint8_t* bytesRd(uint8_t reg, uint8_t nbytes, uint8_t* data_p);
        uint8_t gpioRd();
        uint8_t gpioRdOutput();
        uint16_t reset();
        int8_t Init();
        int8_t Init(int mseconds);

        void vbusPower(VBUS_t state) {
                regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL | state));
        }

        uint8_t getVbusState(void) {
                return vbusState;
        };
        void busprobe();
        uint8_t GpxHandler();
        uint8_t IntHandler();
        uint8_t Task();
};

template< typename SPI_SS, typename INTR >
        uint8_t MAX3421e< SPI_SS, INTR >::vbusState = 0;

/* constructor */
template< typename SPI_SS, typename INTR >
MAX3421e< SPI_SS, INTR >::MAX3421e() {
        // Leaving ADK hardware setup in here, for now. This really belongs with the other parts.
#ifdef BOARD_MEGA_ADK
        // For Mega ADK, which has a Max3421e on-board, set MAX_RESET to output mode, and then set it to HIGH
        P55::SetDirWrite();
        P55::Set();
#endif
};

/* write single byte into MAX3421 register */
template< typename SPI_SS, typename INTR >
void MAX3421e< SPI_SS, INTR >::regWr(uint8_t reg, uint8_t data) {
        XMEM_ACQUIRE_SPI();
#if defined(SPI_HAS_TRANSACTION)
        USB_SPI.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // The MAX3421E can handle up to 26MHz, use MSB First and SPI mode 0
#endif
        SPI_SS::Clear();

#if USING_SPI4TEENSY3
        uint8_t c[2];
        c[0] = reg | 0x02;
        c[1] = data;
        spi4teensy3::send(c, 2);
#elif defined(SPI_HAS_TRANSACTION) && !defined(ESP8266) && !defined(ESP32)
        uint8_t c[2];
        c[0] = reg | 0x02;
        c[1] = data;
        USB_SPI.transfer(c, 2);
#elif defined(STM32F4)
        uint8_t c[2];
        c[0] = reg | 0x02;
        c[1] = data;
        HAL_SPI_Transmit(&SPI_Handle, c, 2, HAL_MAX_DELAY);
#elif !defined(SPDR) // ESP8266, ESP32
        USB_SPI.transfer(reg | 0x02);
        USB_SPI.transfer(data);
#else
        SPDR = (reg | 0x02);
        while(!(SPSR & (1 << SPIF)));
        SPDR = data;
        while(!(SPSR & (1 << SPIF)));
#endif

        SPI_SS::Set();
#if defined(SPI_HAS_TRANSACTION)
        USB_SPI.endTransaction();
#endif
        XMEM_RELEASE_SPI();
        return;
};
/* multiple-byte write                            */

/* returns a pointer to memory position after last written */
template< typename SPI_SS, typename INTR >
uint8_t* MAX3421e< SPI_SS, INTR >::bytesWr(uint8_t reg, uint8_t nbytes, uint8_t* data_p) {
        XMEM_ACQUIRE_SPI();
#if defined(SPI_HAS_TRANSACTION)
        USB_SPI.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // The MAX3421E can handle up to 26MHz, use MSB First and SPI mode 0
#endif
        SPI_SS::Clear();

#if USING_SPI4TEENSY3
        spi4teensy3::send(reg | 0x02);
        spi4teensy3::send(data_p, nbytes);
        data_p += nbytes;
#elif defined(STM32F4)
        uint8_t data = reg | 0x02;
        HAL_SPI_Transmit(&SPI_Handle, &data, 1, HAL_MAX_DELAY);
        HAL_SPI_Transmit(&SPI_Handle, data_p, nbytes, HAL_MAX_DELAY);
        data_p += nbytes;
#elif !defined(__AVR__) || !defined(SPDR)
#if defined(ESP8266) || defined(ESP32)
        yield();
#endif
        USB_SPI.transfer(reg | 0x02);
        while(nbytes) {
                USB_SPI.transfer(*data_p);
                nbytes--;
                data_p++; // advance data pointer
        }
#else
        SPDR = (reg | 0x02); //set WR bit and send register number
        while(nbytes) {
                while(!(SPSR & (1 << SPIF))); //check if previous byte was sent
                SPDR = (*data_p); // send next data byte
                nbytes--;
                data_p++; // advance data pointer
        }
        while(!(SPSR & (1 << SPIF)));
#endif

        SPI_SS::Set();
#if defined(SPI_HAS_TRANSACTION)
        USB_SPI.endTransaction();
#endif
        XMEM_RELEASE_SPI();
        return ( data_p);
}
/* GPIO write                                           */
/*GPIO byte is split between 2 registers, so two writes are needed to write one byte */

/* GPOUT bits are in the low nibble. 0-3 in IOPINS1, 4-7 in IOPINS2 */
template< typename SPI_SS, typename INTR >
void MAX3421e< SPI_SS, INTR >::gpioWr(uint8_t data) {
        regWr(rIOPINS1, data);
        data >>= 4;
        regWr(rIOPINS2, data);
        return;
}

/* single host register read    */
template< typename SPI_SS, typename INTR >
uint8_t MAX3421e< SPI_SS, INTR >::regRd(uint8_t reg) {
        XMEM_ACQUIRE_SPI();
#if defined(SPI_HAS_TRANSACTION)
        USB_SPI.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // The MAX3421E can handle up to 26MHz, use MSB First and SPI mode 0
#endif
        SPI_SS::Clear();

#if USING_SPI4TEENSY3
        spi4teensy3::send(reg);
        uint8_t rv = spi4teensy3::receive();
        SPI_SS::Set();
#elif defined(STM32F4)
        HAL_SPI_Transmit(&SPI_Handle, &reg, 1, HAL_MAX_DELAY);
        uint8_t rv = 0;
        HAL_SPI_Receive(&SPI_Handle, &rv, 1, HAL_MAX_DELAY);
        SPI_SS::Set();
#elif !defined(SPDR) || defined(SPI_HAS_TRANSACTION)
        USB_SPI.transfer(reg);
        uint8_t rv = USB_SPI.transfer(0); // Send empty byte
        SPI_SS::Set();
#else
        SPDR = reg;
        while(!(SPSR & (1 << SPIF)));
        SPDR = 0; // Send empty byte
        while(!(SPSR & (1 << SPIF)));
        SPI_SS::Set();
        uint8_t rv = SPDR;
#endif

#if defined(SPI_HAS_TRANSACTION)
        USB_SPI.endTransaction();
#endif
        XMEM_RELEASE_SPI();
        return (rv);
}
/* multiple-byte register read  */

/* returns a pointer to a memory position after last read   */
template< typename SPI_SS, typename INTR >
uint8_t* MAX3421e< SPI_SS, INTR >::bytesRd(uint8_t reg, uint8_t nbytes, uint8_t* data_p) {
        XMEM_ACQUIRE_SPI();
#if defined(SPI_HAS_TRANSACTION)
        USB_SPI.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // The MAX3421E can handle up to 26MHz, use MSB First and SPI mode 0
#endif
        SPI_SS::Clear();

#if USING_SPI4TEENSY3
        spi4teensy3::send(reg);
        spi4teensy3::receive(data_p, nbytes);
        data_p += nbytes;
#elif defined(SPI_HAS_TRANSACTION) && !defined(ESP8266) && !defined(ESP32)
        USB_SPI.transfer(reg);
        memset(data_p, 0, nbytes); // Make sure we send out empty bytes
        USB_SPI.transfer(data_p, nbytes);
        data_p += nbytes;
#elif defined(__ARDUINO_X86__)
        USB_SPI.transfer(reg);
        USB_SPI.transferBuffer(NULL, data_p, nbytes);
        data_p += nbytes;
#elif defined(STM32F4)
        HAL_SPI_Transmit(&SPI_Handle, &reg, 1, HAL_MAX_DELAY);
        memset(data_p, 0, nbytes); // Make sure we send out empty bytes
        HAL_SPI_Receive(&SPI_Handle, data_p, nbytes, HAL_MAX_DELAY);
        data_p += nbytes;
#elif !defined(SPDR) // ESP8266, ESP32
        yield();
        USB_SPI.transfer(reg);
        while(nbytes) {
            *data_p++ = USB_SPI.transfer(0);
            nbytes--;
        }
#else
        SPDR = reg;
        while(!(SPSR & (1 << SPIF))); //wait
        while(nbytes) {
                SPDR = 0; // Send empty byte
                nbytes--;
                while(!(SPSR & (1 << SPIF)));
#if 0
                {
                        *data_p = SPDR;
                        printf("%2.2x ", *data_p);
                }
                data_p++;
        }
        printf("\r\n");
#else
                *data_p++ = SPDR;
        }
#endif
#endif

        SPI_SS::Set();
#if defined(SPI_HAS_TRANSACTION)
        USB_SPI.endTransaction();
#endif
        XMEM_RELEASE_SPI();
        return ( data_p);
}
/* GPIO read. See gpioWr for explanation */

/** @brief  Reads the current GPI input values
*   @retval uint8_t Bitwise value of all 8 GPI inputs
*/
/* GPIN pins are in high nibbles of IOPINS1, IOPINS2    */
template< typename SPI_SS, typename INTR >
uint8_t MAX3421e< SPI_SS, INTR >::gpioRd() {
        uint8_t gpin = 0;
        gpin = regRd(rIOPINS2); //pins 4-7
        gpin &= 0xf0; //clean lower nibble
        gpin |= (regRd(rIOPINS1) >> 4); //shift low bits and OR with upper from previous operation.
        return ( gpin);
}

/** @brief  Reads the current GPI output values
*   @retval uint8_t Bitwise value of all 8 GPI outputs
*/
/* GPOUT pins are in low nibbles of IOPINS1, IOPINS2    */
template< typename SPI_SS, typename INTR >
uint8_t MAX3421e< SPI_SS, INTR >::gpioRdOutput() {
        uint8_t gpout = 0;
        gpout = regRd(rIOPINS1); //pins 0-3
        gpout &= 0x0f; //clean upper nibble
        gpout |= (regRd(rIOPINS2) << 4); //shift high bits and OR with lower from previous operation.
        return ( gpout);
}

/* reset MAX3421E. Returns number of cycles it took for PLL to stabilize after reset
  or zero if PLL haven't stabilized in 65535 cycles */
template< typename SPI_SS, typename INTR >
uint16_t MAX3421e< SPI_SS, INTR >::reset() {
        uint16_t i = 0;
        regWr(rUSBCTL, bmCHIPRES);
        regWr(rUSBCTL, 0x00);
        while(++i) {
                if((regRd(rUSBIRQ) & bmOSCOKIRQ)) {
                        break;
                }
        }
        return ( i);
}

/* initialize MAX3421E. Set Host mode, pullups, and stuff. Returns 0 if success, -1 if not */
template< typename SPI_SS, typename INTR >
int8_t MAX3421e< SPI_SS, INTR >::Init() {
        XMEM_ACQUIRE_SPI();
        // Moved here.
        // you really should not init hardware in the constructor when it involves locks.
        // Also avoids the vbus flicker issue confusing some devices.
        /* pin and peripheral setup */
        SPI_SS::SetDirWrite();
        SPI_SS::Set();
        spi::init();
        INTR::SetDirRead();
        XMEM_RELEASE_SPI();
        /* MAX3421E - full-duplex SPI, level interrupt */
        // GPX pin on. Moved here, otherwise we flicker the vbus.
        regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL));

        if(reset() == 0) { //OSCOKIRQ hasn't asserted in time
                return ( -1);
        }

        regWr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST); // set pull-downs, Host

        regWr(rHIEN, bmCONDETIE | bmFRAMEIE); //connection detection

        /* check if device is connected */
        regWr(rHCTL, bmSAMPLEBUS); // sample USB bus
        while(!(regRd(rHCTL) & bmSAMPLEBUS)); //wait for sample operation to finish

        busprobe(); //check if anything is connected

        regWr(rHIRQ, bmCONDETIRQ); //clear connection detect interrupt
        regWr(rCPUCTL, 0x01); //enable interrupt pin

        return ( 0);
}

/* initialize MAX3421E. Set Host mode, pullups, and stuff. Returns 0 if success, -1 if not */
template< typename SPI_SS, typename INTR >
int8_t MAX3421e< SPI_SS, INTR >::Init(int mseconds) {
        XMEM_ACQUIRE_SPI();
        // Moved here.
        // you really should not init hardware in the constructor when it involves locks.
        // Also avoids the vbus flicker issue confusing some devices.
        /* pin and peripheral setup */
        SPI_SS::SetDirWrite();
        SPI_SS::Set();
        spi::init();
        INTR::SetDirRead();
        XMEM_RELEASE_SPI();
        /* MAX3421E - full-duplex SPI, level interrupt, vbus off */
        regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL | GPX_VBDET));

        if(reset() == 0) { //OSCOKIRQ hasn't asserted in time
                return ( -1);
        }

        // Delay a minimum of 1 second to ensure any capacitors are drained.
        // 1 second is required to make sure we do not smoke a Microdrive!
        if(mseconds < 1000) mseconds = 1000;
        delay(mseconds);

        regWr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST); // set pull-downs, Host

        regWr(rHIEN, bmCONDETIE | bmFRAMEIE); //connection detection

        /* check if device is connected */
        regWr(rHCTL, bmSAMPLEBUS); // sample USB bus
        while(!(regRd(rHCTL) & bmSAMPLEBUS)); //wait for sample operation to finish

        busprobe(); //check if anything is connected

        regWr(rHIRQ, bmCONDETIRQ); //clear connection detect interrupt
        regWr(rCPUCTL, 0x01); //enable interrupt pin

        // GPX pin on. This is done here so that busprobe will fail if we have a switch connected.
        regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL));

        return ( 0);
}

/* probe bus to determine device presence and speed and switch host to this speed */
template< typename SPI_SS, typename INTR >
void MAX3421e< SPI_SS, INTR >::busprobe() {
        uint8_t bus_sample;
        bus_sample = regRd(rHRSL); //Get J,K status
        bus_sample &= (bmJSTATUS | bmKSTATUS); //zero the rest of the byte
        switch(bus_sample) { //start full-speed or low-speed host
                case( bmJSTATUS):
                        if((regRd(rMODE) & bmLOWSPEED) == 0) {
                                regWr(rMODE, MODE_FS_HOST); //start full-speed host
                                vbusState = FSHOST;
                        } else {
                                regWr(rMODE, MODE_LS_HOST); //start low-speed host
                                vbusState = LSHOST;
                        }
                        break;
                case( bmKSTATUS):
                        if((regRd(rMODE) & bmLOWSPEED) == 0) {
                                regWr(rMODE, MODE_LS_HOST); //start low-speed host
                                vbusState = LSHOST;
                        } else {
                                regWr(rMODE, MODE_FS_HOST); //start full-speed host
                                vbusState = FSHOST;
                        }
                        break;
                case( bmSE1): //illegal state
                        vbusState = SE1;
                        break;
                case( bmSE0): //disconnected state
                        regWr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST | bmSEPIRQ);
                        vbusState = SE0;
                        break;
        }//end switch( bus_sample )
}

/* MAX3421 state change task and interrupt handler */
template< typename SPI_SS, typename INTR >
uint8_t MAX3421e< SPI_SS, INTR >::Task(void) {
        uint8_t rcode = 0;
        uint8_t pinvalue;
        //USB_HOST_SERIAL.print("Vbus state: ");
        //USB_HOST_SERIAL.println( vbusState, HEX );
        pinvalue = INTR::IsSet(); //Read();
        //pinvalue = digitalRead( MAX_INT );
        if(pinvalue == 0) {
                rcode = IntHandler();
        }
        //    pinvalue = digitalRead( MAX_GPX );
        //    if( pinvalue == LOW ) {
        //        GpxHandler();
        //    }
        //    usbSM();                                //USB state machine
        return ( rcode);
}

template< typename SPI_SS, typename INTR >
uint8_t MAX3421e< SPI_SS, INTR >::IntHandler() {
        uint8_t HIRQ;
        uint8_t HIRQ_sendback = 0x00;
        HIRQ = regRd(rHIRQ); //determine interrupt source
        //if( HIRQ & bmFRAMEIRQ ) {               //->1ms SOF interrupt handler
        //    HIRQ_sendback |= bmFRAMEIRQ;
        //}//end FRAMEIRQ handling
        if(HIRQ & bmCONDETIRQ) {
                busprobe();
                HIRQ_sendback |= bmCONDETIRQ;
        }
        /* End HIRQ interrupts handling, clear serviced IRQs    */
        regWr(rHIRQ, HIRQ_sendback);
        return ( HIRQ_sendback);
}
//template< typename SPI_SS, typename INTR >
//uint8_t MAX3421e< SPI_SS, INTR >::GpxHandler()
//{
//    uint8_t GPINIRQ = regRd( rGPINIRQ );          //read GPIN IRQ register
////    if( GPINIRQ & bmGPINIRQ7 ) {            //vbus overload
////        vbusPwr( OFF );                     //attempt powercycle
////        delay( 1000 );
////        vbusPwr( ON );
////        regWr( rGPINIRQ, bmGPINIRQ7 );
////    }
//    return( GPINIRQ );
//}

#endif // _USBHOST_H_
