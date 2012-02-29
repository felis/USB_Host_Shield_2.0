/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
*/

/* derived from Konstantin Chizhov's AVR port templates */

#ifndef _avrpins_h_
#define _avrpins_h_

#include <avr/io.h>

#ifdef PORTA
#define USE_PORTA
#endif
#ifdef PORTB
#define USE_PORTB
#endif
#ifdef PORTC
#define USE_PORTC
#endif
#ifdef PORTD
#define USE_PORTD
#endif
#ifdef PORTE
#define USE_PORTE
#endif
#ifdef PORTF
#define USE_PORTF
#endif
#ifdef PORTG
#define USE_PORTG
#endif
#ifdef PORTH
#define USE_PORTH
#endif
#ifdef PORTJ
#define USE_PORTJ
#endif
#ifdef PORTK
#define USE_PORTK
#endif
#ifdef PORTL
#define USE_PORTL
#endif
#ifdef PORTQ
#define USE_PORTQ
#endif
#ifdef PORTR
#define USE_PORTR
#endif

#ifdef TCCR0A
#define USE_TCCR0A
#endif
#ifdef TCCR1A
#define USE_TCCR1A
#endif
#ifdef TCCR2A
#define USE_TCCR2A
#endif

//Port definitions for AtTiny, AtMega families.

#define MAKE_PORT(portName, ddrName, pinName, className, ID) \
    class className{\
    public:\
      typedef uint8_t DataT;\
    public:\
      static void Write(DataT value){portName = value;}\
      static void ClearAndSet(DataT clearMask, DataT value){portName = (portName & ~clearMask) | value;}\
      static DataT Read(){return portName;}\
      static void DirWrite(DataT value){ddrName = value;}\
      static DataT DirRead(){return ddrName;}\
      static void Set(DataT value){portName |= value;}\
      static void Clear(DataT value){portName &= ~value;}\
      static void Toggle(DataT value){portName ^= value;}\
      static void DirSet(DataT value){ddrName |= value;}\
      static void DirClear(DataT value){ddrName &= ~value;}\
      static void DirToggle(DataT value){ddrName ^= value;}\
      static DataT PinRead(){return pinName;}\
      enum{Id = ID};\
      enum{Width=sizeof(DataT)*8};\
    };

// TCCR registers to set/clear Arduino PWM
#define MAKE_TCCR(TccrName, className) \
    class className{\
    public:\
      typedef uint8_t DataT;\
    public:\
      static void Write(DataT value){TccrName = value;}\
      static void ClearAndSet(DataT clearMask, DataT value){TccrName = (TccrName & ~clearMask) | value;}\
      static DataT Read(){return TccrName;}\
      static void Set(DataT value){TccrName |= value;}\
      static void Clear(DataT value){TccrName &= ~value;}\
      static void Toggle(DataT value){TccrName ^= value;}\
      enum{Width=sizeof(DataT)*8};\
    };

#ifdef USE_PORTA
MAKE_PORT(PORTA, DDRA, PINA, Porta, 'A')
#endif
#ifdef USE_PORTB
MAKE_PORT(PORTB, DDRB, PINB, Portb, 'B')
#endif
#ifdef USE_PORTC
MAKE_PORT(PORTC, DDRC, PINC, Portc, 'C')
#endif
#ifdef USE_PORTD
MAKE_PORT(PORTD, DDRD, PIND, Portd, 'D')
#endif
#ifdef USE_PORTE
MAKE_PORT(PORTE, DDRE, PINE, Porte, 'E')
#endif
#ifdef USE_PORTF
MAKE_PORT(PORTF, DDRF, PINF, Portf, 'F')
#endif
#ifdef USE_PORTG
MAKE_PORT(PORTG, DDRG, PING, Portg, 'G')
#endif
#ifdef USE_PORTH
MAKE_PORT(PORTH, DDRH, PINH, Porth, 'H')
#endif
#ifdef USE_PORTJ
MAKE_PORT(PORTJ, DDRJ, PINJ, Portj, 'J')
#endif
#ifdef USE_PORTK
MAKE_PORT(PORTK, DDRK, PINK, Portk, 'K')
#endif
#ifdef USE_PORTL
MAKE_PORT(PORTL, DDRL, PINL, Portl, 'L')
#endif
#ifdef USE_PORTQ
MAKE_PORT(PORTQ, DDRQ, PINQ, Portq, 'Q')
#endif
#ifdef USE_PORTR
MAKE_PORT(PORTR, DDRR, PINR, Portr, 'R')
#endif

#ifdef USE_TCCR0A
MAKE_TCCR(TCCR0A, Tccr0a)
#endif
#ifdef USE_TCCR1A
MAKE_TCCR(TCCR1A, Tccr1a)
#endif
#ifdef USE_TCCR2A
MAKE_TCCR(TCCR2A, Tccr2a)
#endif

  // this class represents one pin in a IO port.
  // It is fully static.
  template<typename PORT, uint8_t PIN>
  class TPin
  {
//    BOOST_STATIC_ASSERT(PIN < PORT::Width);
  public:
    typedef PORT Port;
    enum{Number = PIN};

    static void Set() { PORT::Set(1 << PIN); }

    static void Set(uint8_t val){
      if(val)
        Set();
      else Clear();}

    static void SetDir(uint8_t val){
      if(val)
        SetDirWrite();
      else SetDirRead();}

    static void Clear(){PORT::Clear(1 << PIN);}

    static void Toggle(){PORT::Toggle(1 << PIN);}

    static void SetDirRead(){PORT::DirClear(1 << PIN);}

    static void SetDirWrite(){PORT::DirSet(1 << PIN);}

    static uint8_t IsSet(){return PORT::PinRead() & (uint8_t)(1 << PIN);} 
  
    static void WaiteForSet(){ while(IsSet()==0){} }

    static void WaiteForClear(){ while(IsSet()){} }
  }; //class TPin...

  // this class represents one bit in TCCR port.
  // used to set/clear TCCRx bits
  // It is fully static.
  template<typename TCCR, uint8_t COM>
  class TCom
  {
//    BOOST_STATIC_ASSERT(PIN < PORT::Width);
  public:
    typedef TCCR Tccr;
    enum{Com = COM};

    static void Set() { TCCR::Set(1 << COM); }

    static void Clear() { TCCR::Clear(1 << COM); }

    static void Toggle() { TCCR::Toggle(1 << COM); }
  }; //class TCom...

//Short pin definitions 
#ifdef USE_PORTA
typedef TPin<Porta, 0> Pa0;
typedef TPin<Porta, 1> Pa1;
typedef TPin<Porta, 2> Pa2;
typedef TPin<Porta, 3> Pa3;
typedef TPin<Porta, 4> Pa4;
typedef TPin<Porta, 5> Pa5;
typedef TPin<Porta, 6> Pa6;
typedef TPin<Porta, 7> Pa7;
#endif

#ifdef USE_PORTB
typedef TPin<Portb, 0> Pb0;
typedef TPin<Portb, 1> Pb1;
typedef TPin<Portb, 2> Pb2;
typedef TPin<Portb, 3> Pb3;
typedef TPin<Portb, 4> Pb4;
typedef TPin<Portb, 5> Pb5;
typedef TPin<Portb, 6> Pb6;
typedef TPin<Portb, 7> Pb7;
#endif

#ifdef USE_PORTC
typedef TPin<Portc, 0> Pc0;
typedef TPin<Portc, 1> Pc1;
typedef TPin<Portc, 2> Pc2;
typedef TPin<Portc, 3> Pc3;
typedef TPin<Portc, 4> Pc4;
typedef TPin<Portc, 5> Pc5;
typedef TPin<Portc, 6> Pc6;
typedef TPin<Portc, 7> Pc7;
#endif

#ifdef USE_PORTD
typedef TPin<Portd, 0> Pd0;
typedef TPin<Portd, 1> Pd1;
typedef TPin<Portd, 2> Pd2;
typedef TPin<Portd, 3> Pd3;
typedef TPin<Portd, 4> Pd4;
typedef TPin<Portd, 5> Pd5;
typedef TPin<Portd, 6> Pd6;
typedef TPin<Portd, 7> Pd7;
#endif

#ifdef USE_PORTE
typedef TPin<Porte, 0> Pe0;
typedef TPin<Porte, 1> Pe1;
typedef TPin<Porte, 2> Pe2;
typedef TPin<Porte, 3> Pe3;
typedef TPin<Porte, 4> Pe4;
typedef TPin<Porte, 5> Pe5;
typedef TPin<Porte, 6> Pe6;
typedef TPin<Porte, 7> Pe7;
#endif

#ifdef USE_PORTF
typedef TPin<Portf, 0> Pf0;
typedef TPin<Portf, 1> Pf1;
typedef TPin<Portf, 2> Pf2;
typedef TPin<Portf, 3> Pf3;
typedef TPin<Portf, 4> Pf4;
typedef TPin<Portf, 5> Pf5;
typedef TPin<Portf, 6> Pf6;
typedef TPin<Portf, 7> Pf7;
#endif

#ifdef USE_PORTG
typedef TPin<Portg, 0> Pg0;
typedef TPin<Portg, 1> Pg1;
typedef TPin<Portg, 2> Pg2;
typedef TPin<Portg, 3> Pg3;
typedef TPin<Portg, 4> Pg4;
typedef TPin<Portg, 5> Pg5;
typedef TPin<Portg, 6> Pg6;
typedef TPin<Portg, 7> Pg7;
#endif

#ifdef USE_PORTH
typedef TPin<Porth, 0> Ph0;
typedef TPin<Porth, 1> Ph1;
typedef TPin<Porth, 2> Ph2;
typedef TPin<Porth, 3> Ph3;
typedef TPin<Porth, 4> Ph4;
typedef TPin<Porth, 5> Ph5;
typedef TPin<Porth, 6> Ph6;
typedef TPin<Porth, 7> Ph7;
#endif

#ifdef USE_PORTJ
typedef TPin<Portj, 0> Pj0;
typedef TPin<Portj, 1> Pj1;
typedef TPin<Portj, 2> Pj2;
typedef TPin<Portj, 3> Pj3;
typedef TPin<Portj, 4> Pj4;
typedef TPin<Portj, 5> Pj5;
typedef TPin<Portj, 6> Pj6;
typedef TPin<Portj, 7> Pj7;
#endif

#ifdef USE_PORTK
typedef TPin<Portk, 0> Pk0;
typedef TPin<Portk, 1> Pk1;
typedef TPin<Portk, 2> Pk2;
typedef TPin<Portk, 3> Pk3;
typedef TPin<Portk, 4> Pk4;
typedef TPin<Portk, 5> Pk5;
typedef TPin<Portk, 6> Pk6;
typedef TPin<Portk, 7> Pk7;
#endif

#ifdef USE_PORTL
typedef TPin<Portl, 0> Pl0;
typedef TPin<Portl, 1> Pl1;
typedef TPin<Portl, 2> Pl2;
typedef TPin<Portl, 3> Pl3;
typedef TPin<Portl, 4> Pl4;
typedef TPin<Portl, 5> Pl5;
typedef TPin<Portl, 6> Pl6;
typedef TPin<Portl, 7> Pl7;
#endif

#ifdef USE_PORTQ
typedef TPin<Portq, 0> Pq0;
typedef TPin<Portq, 1> Pq1;
typedef TPin<Portq, 2> Pq2;
typedef TPin<Portq, 3> Pq3;
typedef TPin<Portq, 4> Pq4;
typedef TPin<Portq, 5> Pq5;
typedef TPin<Portq, 6> Pq6;
typedef TPin<Portq, 7> Pq7;
#endif

#ifdef USE_PORTR
typedef TPin<Portr, 0> Pr0;
typedef TPin<Portr, 1> Pr1;
typedef TPin<Portr, 2> Pr2;
typedef TPin<Portr, 3> Pr3;
typedef TPin<Portr, 4> Pr4;
typedef TPin<Portr, 5> Pr5;
typedef TPin<Portr, 6> Pr6;
typedef TPin<Portr, 7> Pr7;
#endif

#ifdef USE_TCCR0A
typedef TCom<Tccr0a, COM0A1> Tc0a;  //P6
typedef TCom<Tccr0a, COM0B1> Tc0b;  //P5
#endif

#ifdef USE_TCCR1A
typedef TCom<Tccr1a, COM1A1> Tc1a;  //P9
typedef TCom<Tccr1a, COM1B1> Tc1b;  //P10
#endif

#ifdef USE_TCCR2A
typedef TCom<Tccr2a, COM2A1> Tc2a;  //P11
typedef TCom<Tccr2a, COM2B1> Tc2b;  //P3
#endif

template<typename Tp_pin, typename Tc_bit>
  class Tp_Tc
  {
    public:
      static void SetDir(uint8_t val){
        if(val)
          SetDirWrite();
        else SetDirRead();
      }
      static void SetDirRead(){
        Tp_pin::SetDirRead(); //set pin direction
        Tc_bit::Clear();      //disconnect pin from PWM  
      }
      static void SetDirWrite(){
        Tp_pin::SetDirWrite();
        Tc_bit::Clear();
      }
  };

/* pin definitions for cases where it's necessary to clear compare output mode bits */

//typedef Tp_Tc<Pd3, Tc2b> P3;  //Arduino pin 3
//typedef Tp_Tc<Pd5, Tc0b> P5;  //Arduino pin 5
//typedef Tp_Tc<Pd6, Tc0a> P6;  //Arduino pin 6
//typedef Tp_Tc<Pb1, Tc1a> P9;  //Arduino pin 9
//typedef Tp_Tc<Pb2, Tc1b> P10;  //Arduino pin 10
//typedef Tp_Tc<Pb3, Tc2a> P11;  //Arduino pin 11

/* Arduino pin definitions  */
#if defined(__AVR_ATmega1280__) || (__AVR_ATmega2560__)

//  "Mega" Arduino pin numbers 

#define P0  Pe0
#define P1  Pe1
#define P2  Pe4
#define P3  Pe5
#define P4  Pg5
#define P5  Pe5
#define P6  Ph3 
#define P7  Ph4

#define P8  Ph5
#define P9  Ph6
#define P10  Pb4
#define P11  Pb5
#define P12  Pb6
#define P13  Pb7

#define P14  Pj1
#define P15  Pj0
#define P16  Ph1
#define P17  Ph0
#define P18  Pd3
#define P19  Pd2
#define P20  Pd1
#define P21  Pd0 

#define P22 Pa0
#define P23 Pa1
#define P24 Pa2
#define P25 Pa3
#define P26 Pa4
#define P27 Pa5
#define P28 Pa6
#define P29 Pa7
#define P30 Pc7
#define P31 Pc6
#define P32 Pc5
#define P33 Pc4
#define P34 Pc3
#define P35 Pc2
#define P36 Pc1
#define P37 Pc0

#define P38 Pd7
#define P39 Pg2
#define P40 Pg1
#define P41 Pg0
#define P42 Pl7
#define P43 Pl6
#define P44 Pl5
#define P45 Pl4
#define P46 Pl3
#define P47 Pl2
#define P48 Pl1
#define P49 Pl0
#define P50 Pb3
#define P51 Pb2
#define P52 Pb1
#define P53 Pb0

#endif  //"Mega" pin numbers

#if  defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
//"Classic" Arduino pin numbers

#define P0  Pd0
#define P1  Pd1
#define P2  Pd2
#define P3  Pd3
#define P4  Pd4
#define P5  Pd5
#define P6  Pd6 
#define P7  Pd7

#define P8  Pb0
#define P9  Pb1
#define P10  Pb2
#define P11  Pb3
#define P12  Pb4
#define P13  Pb5

#define P14  Pc0
#define P15  Pc1
#define P16  Pc2
#define P17  Pc3
#define P18  Pc4
#define P19  Pc5

#endif // "Classic" Arduino pin numbers

#if defined(__AVR_ATmega32U4__)
// Teensy 2.0 pin numbers
// http://www.pjrc.com/teensy/pinout.html
#define P0  Pb0
#define P1  Pb1
#define P2  Pb2
#define P3  Pb3
#define P4  Pb7
#define P5  Pd0
#define P6  Pd1
#define P7  Pd2
#define P8  Pd3
#define P9  Pc6
#define P10 Pc7
#define P11 Pd6
#define P12 Pd7
#define P13 Pb4
#define P14 Pb5
#define P15 Pb6
#define P16 Pf7
#define P17 Pf6
#define P18 Pf5
#define P19 Pf4
#define P20 Pf1
#define P21 Pf0
#define P22 Pd4
#define P23 Pd5
#define P24 Pe6
#endif // Teensy 2.0

#if defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
// Teensy++ 2.0 pin numbers
// http://www.pjrc.com/teensy/pinout.html
#define P0  Pd0
#define P1  Pd1
#define P2  Pd2
#define P3  Pd3
#define P4  Pd4
#define P5  Pd5
#define P6  Pd6
#define P7  Pd7
#define P8  Pe0
#define P9  Pe1
#define P10 Pc0
#define P11 Pc1
#define P12 Pc2
#define P13 Pc3
#define P14 Pc4
#define P15 Pc5
#define P16 Pc6
#define P17 Pc7
#define P18 Pe6
#define P19 Pe7
#define P20 Pb0
#define P21 Pb1
#define P22 Pb2
#define P23 Pb3
#define P24 Pb4
#define P25 Pb5
#define P26 Pb6
#define P27 Pb7
#define P28 Pa0
#define P29 Pa1
#define P30 Pa2
#define P31 Pa3
#define P32 Pa4
#define P33 Pa5
#define P34 Pa6
#define P35 Pa7
#define P36 Pe4
#define P37 Pe5
#define P38 Pf0
#define P39 Pf1
#define P40 Pf2
#define P41 Pf3
#define P42 Pf4
#define P43 Pf5
#define P44 Pf6
#define P45 Pf7
#endif // Teensy++ 2.0

#if defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
// Sanguino pin numbers
// http://sanguino.cc/hardware
#define P0  Pb0
#define P1  Pb1
#define P2  Pb2
#define P3  Pb3
#define P4  Pb4
#define P5  Pb5
#define P6  Pb6
#define P7  Pb7
#define P8  Pd0
#define P9  Pd1
#define P10 Pd2
#define P11 Pd3
#define P12 Pd4
#define P13 Pd5
#define P14 Pd6
#define P15 Pd7
#define P16 Pc0
#define P17 Pc1
#define P18 Pc2
#define P19 Pc3
#define P20 Pc4
#define P21 Pc5
#define P22 Pc6
#define P23 Pc7
#define P24 Pa0
#define P25 Pa1
#define P26 Pa2
#define P27 Pa3
#define P28 Pa4
#define P29 Pa5
#define P30 Pa6
#define P31 Pa7
#endif // Sanguino

#endif //_avrpins_h_
