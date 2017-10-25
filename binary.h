/*----------------------------------------------------------------------------*\
| PART OF THE COSPLAY.LIGHTING SOURCE CODE LIBRARY COLLECTION.                 |
| SOURCE:  https://github.com/cosplaylighting/binary                           |
| LICENSE: https://github.com/cosplaylighting/binary/blob/master/LICENSE       |
+------------------------------------------------------------------------------+
| SIMPLE STRUCTS/UNIONS FOR SWAPPING BETWEEN BIG AND LITTLE ENDIAN.            |
| ALL SWAPPING IS DONE IN-PLACE, SO NO ADDITIONAL RAM IS NEEDED.               |
| SWAPPING IS DONE VIA 3 INLINED XOR COMMANDS, KEEPING THE CODE FOOTPRINT VERY |
| SMALL AS WELL.                                                               |
+------------------------------------------------------------------------------+
| ADDITIONALLY, BIT FIELD, BYTE LISTS, AND MORE HAVE BEEN ADDED TO THESE.      |
| IF YOU NEED A SIMPLIFIED WAY TO ACCESS ARBITRARY BITS FROM A BYTE OR         |
| MULTI-BYTE DATA, THIS WILL BE ONE OF THE EASIEST WAYS TO DO IT.              |
\*----------------------------------------------------------------------------*/




#ifndef __binary_h__
#define __binary_h__




#ifndef INLINE
#define INLINE		inline __attribute__ ((always_inline))
#endif




#ifndef PACKED
#define PACKED		__attribute__ ((packed))
#endif




////////////////////////////////////////////////////////////////////////////////
// BITFIELD - 8-BIT
////////////////////////////////////////////////////////////////////////////////
union PACKED uint8_b {
	INLINE uint8_b()				{ this->byte_0 = 0; }
	INLINE uint8_b(uint8_t byte)	{ this->byte_0 = byte; }
	INLINE uint8_b(uint8_t *byte)	{ this->byte_0 = byte[0]; }


	uint8_t byte[1];
	uint8_t byte_0;


	struct PACKED {
		union PACKED {
			unsigned int nibble_0		: 4;
			struct PACKED {
				unsigned int bit_0		: 1;
				unsigned int bit_1		: 1;
				unsigned int bit_2		: 1;
				unsigned int bit_3		: 1;
			};
		};
		union PACKED {
			unsigned int nibble_1		: 4;
			struct PACKED {
				unsigned int bit_4		: 1;
				unsigned int bit_5		: 1;
				unsigned int bit_6		: 1;
				unsigned int bit_7		: 1;
			};
		};
	};


	INLINE operator uint8_t() const {
		return this->byte_0;
	}


	INLINE uint8_t operator =(const uint8_t byte) {
		return this->byte_0 = byte;
	}


	INLINE void in(uint8_t bit) {
		this->byte_0 = (this->byte_0 << 1) | !!bit;
	}


	INLINE uint8_t out() {
		uint8_t out		= this->byte_0 & 0x01;
		this->byte_0	= this->byte_0 >> 1;
		return out;
	}


	static INLINE uint8_t fill() { return 0xff; }
};




////////////////////////////////////////////////////////////////////////////////
// BITFIELD - 16-BIT
////////////////////////////////////////////////////////////////////////////////
union PACKED uint16_b {
	INLINE uint16_b()				{ this->word_0 = 0; }
	INLINE uint16_b(uint16_t word)	{ this->word_0 = word; }
	INLINE uint16_b(uint16_t *word)	{ this->word_0 = word[0]; }

	INLINE uint16_b(uint8_t *byte)	{
		this->byte_0 = byte[0];
		this->byte_1 = byte[1];
	}

	INLINE uint16_b(uint8_t byte_0, uint8_t byte_1) {
		this->byte_0 = byte_0;
		this->byte_1 = byte_1;
	}


	uint8_b		byte[2];
	uint16_t	word[1];
	uint16_t	word_0;


	struct PACKED {
		union PACKED {
			uint8_t byte_0;
			struct PACKED {
				unsigned int bit_0		: 1;
				unsigned int bit_1		: 1;
				unsigned int bit_2		: 1;
				unsigned int bit_3		: 1;
				unsigned int bit_4		: 1;
				unsigned int bit_5		: 1;
				unsigned int bit_6		: 1;
				unsigned int bit_7		: 1;
			};
		};

		union PACKED {
			uint8_t byte_1;
			struct PACKED {
				unsigned int bit_8		: 1;
				unsigned int bit_9		: 1;
				unsigned int bit_10		: 1;
				unsigned int bit_11		: 1;
				unsigned int bit_12		: 1;
				unsigned int bit_13		: 1;
				unsigned int bit_14		: 1;
				unsigned int bit_15		: 1;
			};
		};
	};


	INLINE operator uint16_t() const {
		return this->word_0;
	}


	INLINE uint16_t operator =(uint16_t word) {
		return this->word_0 = word;
	}


	INLINE void in(uint8_t bit) {
		this->word_0 = (this->word_0 << 1) | !!bit;
	}


	INLINE uint8_t out() {
		uint8_t out		= this->word_0 & 0x01;
		this->word_0	= this->word_0 >> 1;
		return out;
	}


	INLINE void swap() {
		byte_0 ^= byte_1;
		byte_1 ^= byte_0;
		byte_0 ^= byte_1;
	}


	static INLINE uint16_t fill() { return 0xffff; }
};




////////////////////////////////////////////////////////////////////////////////
// BITFIELD - 32-BIT
////////////////////////////////////////////////////////////////////////////////
union PACKED uint32_b {
	INLINE uint32_b()					{ this->dword_0 = 0; }
	INLINE uint32_b(uint32_t dword)		{ this->dword_0 = dword; }
	INLINE uint32_b(uint32_t *dword)	{ this->dword_0 = dword[0]; }

	INLINE uint32_b(uint8_t *byte)	{
		this->byte_0 = byte[0];
		this->byte_1 = byte[1];
		this->byte_2 = byte[2];
		this->byte_3 = byte[3];
	}

	INLINE uint32_b(uint16_t *word) {
		this->word_0 = word[0];
		this->word_1 = word[1];
	}

	INLINE uint32_b(uint8_t word_0, uint8_t word_1) {
		this->word_0 = word_0;
		this->word_1 = word_1;
	}

	INLINE uint32_b(uint8_t byte_0, uint8_t byte_1,
					uint8_t byte_2, uint8_t byte_3) {
		this->byte_0 = byte_0;
		this->byte_1 = byte_1;
		this->byte_2 = byte_2;
		this->byte_3 = byte_3;
	}


	uint8_b		byte[4];
	uint16_b	word[2];
	uint32_t	dword[1];
	uint32_t	dword_0;


	struct PACKED {
		union PACKED {
			uint16_t word_0;

			struct PACKED {
				union PACKED {
					uint8_t byte_0;
					struct PACKED {
						unsigned int bit_0		: 1;
						unsigned int bit_1		: 1;
						unsigned int bit_2		: 1;
						unsigned int bit_3		: 1;
						unsigned int bit_4		: 1;
						unsigned int bit_5		: 1;
						unsigned int bit_6		: 1;
						unsigned int bit_7		: 1;
					};
				};

				union PACKED {
					uint8_t byte_1;
					struct PACKED {
						unsigned int bit_8		: 1;
						unsigned int bit_9		: 1;
						unsigned int bit_10		: 1;
						unsigned int bit_11		: 1;
						unsigned int bit_12		: 1;
						unsigned int bit_13		: 1;
						unsigned int bit_14		: 1;
						unsigned int bit_15		: 1;
					};
				};
			};
		};

		union PACKED {
			uint16_t word_1;

			struct PACKED {
				union PACKED {
					uint8_t byte_2;
					struct PACKED {
						unsigned int bit_16		: 1;
						unsigned int bit_17		: 1;
						unsigned int bit_18		: 1;
						unsigned int bit_19		: 1;
						unsigned int bit_20		: 1;
						unsigned int bit_21		: 1;
						unsigned int bit_22		: 1;
						unsigned int bit_23		: 1;
					};
				};

				union PACKED {
					uint8_t byte_3;
					struct PACKED {
						unsigned int bit_24		: 1;
						unsigned int bit_25		: 1;
						unsigned int bit_26		: 1;
						unsigned int bit_27		: 1;
						unsigned int bit_28		: 1;
						unsigned int bit_29		: 1;
						unsigned int bit_30		: 1;
						unsigned int bit_31		: 1;
					};
				};
			};
		};
	};


	INLINE operator uint32_t() const {
		return this->dword_0;
	}


	INLINE uint32_t operator =(uint32_t dword) {
		return this->dword_0 = dword;
	}


	INLINE void in(uint8_t bit) {
		this->dword_0 = (this->dword_0 << 1) | !!bit;
	}


	INLINE uint8_t out() {
		uint8_t out		= this->dword_0 & 0x01;
		this->dword_0	= this->dword_0 >> 1;
		return out;
	}


	INLINE void swap() {
		byte_0 ^= byte_3;
		byte_3 ^= byte_0;
		byte_0 ^= byte_3;

		byte_1 ^= byte_2;
		byte_2 ^= byte_1;
		byte_1 ^= byte_2;
	}


	static INLINE uint32_t fill() { return 0xffffffff; }
};




#endif //__binary_h__
