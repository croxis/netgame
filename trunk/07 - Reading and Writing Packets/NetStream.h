/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#ifndef NET_STREAM_H
#define NET_STREAM_H

#include <assert.h>
#include <algorithm>

namespace net
{
	// bitpacker class
	//  + read and write non-8 multiples of bits efficiently
	
	class BitPacker
	{
	public:
		
		enum Mode
		{
			Read,
			Write
		};
		
		BitPacker( Mode mode, void * buffer, int bytes )
		{
			assert( buffer );
			assert( bytes >= 4 );
			assert( ( bytes & 3 ) == 0 );
			this->mode = mode;
			this->buffer = (unsigned char*) buffer;
			this->ptr = (unsigned char*) buffer;
			this->bytes = bytes;
			bit_index = 0;
		}
		
		void WriteBits( unsigned int value, int bits = 32 )
		{
			assert( bits > 0 );
			assert( bits <= 32 );
			assert( mode == Write );
			assert( ptr - buffer < bytes );
			if ( bits < 32 )
			{
				const unsigned int mask = ( 1 << bits ) - 1;
				value &= mask;
			}
			do
			{
				*ptr |= (unsigned char) ( value << bit_index );
				assert( bit_index < 8 );
				const int bits_written = std::min( bits, 8 - bit_index );
				assert( bits_written > 0 );
				assert( bits_written <= 8 );
				bit_index += bits_written;
				if ( bit_index >= 8 )
				{
					ptr++;
					bit_index = 0;
					value >>= bits_written;
					assert( ptr - buffer < bytes );
				}
				bits -= bits_written;
				assert( bits >= 0 );
				assert( bits <= 32 );
			}
			while ( bits > 0 );
		}
		
		int GetBitsWritten() const
		{
			assert( mode == Write );
			return ( ptr - buffer ) * 8 + bit_index;
		}
		
 		void ReadBits( unsigned int & value, int bits = 32 )
		{
			assert( bits > 0 );
			assert( bits <= 32 );
			assert( mode == Read );
			assert( ptr - buffer < bytes );
			int original_bits = bits;
			int value_index = 0;
			value = 0;
			do
			{
				assert( bits >= 0 );
				assert( bits <= 32 );
				int bits_to_read = std::min( 8 - bit_index, bits );
				assert( bits_to_read > 0 );
				assert( bits_to_read <= 8 );
				value |= ( *ptr >> bit_index ) << value_index;
				bits -= bits_to_read;
				bit_index += bits_to_read;
				value_index += bits_to_read;
				assert( value_index >= 0 );
				assert( value_index <= 32 );
				if ( bit_index >= 8 )
				{
					ptr++;
					bit_index = 0;
					assert( ptr - buffer < bytes );
				}
			}
			while ( bits > 0 );
			if ( original_bits < 32 )
			{
				const unsigned int mask = ( 1 << original_bits ) - 1;
				value &= mask;
			}
		}
		
		int GetBitsRead() const
		{
			assert( mode == Read );
			return ( ptr - buffer ) * 8 + bit_index;
		}
		
		int BitsRemaining() const
		{
			return bytes * 8 - ( ( ptr - buffer ) * 8 + bit_index );
		}
		
		Mode GetMode() const
		{
			return mode;
		}
		
	private:
		
		int bit_index;
		unsigned char * ptr;
		unsigned char * buffer;
		int bytes;
		Mode mode;
	};
	
	// arithmetic coder
	//  + todo: implement arithmetic coder based on jon blow's game developer article
	
	class ArithmeticCoder
	{
	public:
		
		enum Mode
		{
			Read,
			Write
		};
		
		ArithmeticCoder( Mode mode, void * buffer, unsigned int size )
		{
			assert( buffer );
			assert( size >= 4 );
			assert( size & 3 == 0 );
			this->mode = mode;
			this->buffer = (unsigned char*) buffer;
			this->size = size;
		}

		bool WriteInteger( unsigned int value, unsigned int minimum = 0, unsigned int maximum = 0xFFFFFFFF )
		{
			// ...
			return false;
		}
		
		bool ReadInteger( unsigned int value, unsigned int minimum = 0, unsigned int maximum = 0xFFFFFFFF )
		{
			// ...
			return false;
		}
		
	private:
		
		unsigned char * buffer;
		int size;
		Mode mode;
	};
	
	// stream class
	//  + unifies read and write into a serialize operation
	//  + provides attribution of stream for debugging purposes
	
	class Stream
	{
	public:
		
		enum Mode
		{
			Read,
			Write
		};
		
		Stream( Mode mode, void * buffer, int bytes )
			: bitpacker( mode == Write ? BitPacker::Write : BitPacker::Read, buffer, bytes )
		{
		}
		
		bool SerializeByte( unsigned char & value, unsigned char min = 0, unsigned char max = 0xFF )
		{
			assert( min < max );
			unsigned int tmp = (unsigned int) value;
			bool result = SerializeInteger( tmp, min, max );
			value = (unsigned char) tmp;
			return result;
		}

		bool SerializeShort( unsigned short & value, unsigned short min = 0, unsigned short max = 0xFFFF )
		{
			assert( min < max );
			unsigned int tmp = (unsigned int) value;
			bool result = SerializeInteger( tmp, min, max );
			value = (unsigned short) tmp;
			return result;
		}
		
		bool SerializeInteger( unsigned int & value, unsigned int min = 0, unsigned int max = 0xFFFFFFFF )
		{
			assert( min < max );
			const int bits_required = BitsRequired( min, max );
			if ( bitpacker.BitsRemaining() < bits_required )
				return false;
			if ( IsReading() )
				bitpacker.ReadBits( value, bits_required );
			else
				bitpacker.WriteBits( value, bits_required );
			return true;
		}
		
		bool SerializeFloat( float & value )
		{
			union FloatInt
			{
				unsigned int i;
				float f;
			};
			if ( IsReading() )
			{
				FloatInt floatInt;
				if ( !SerializeInteger( floatInt.i ) )
					return false;
				value = floatInt.f;
				return true;
			}
			else
			{
				FloatInt floatInt;
				floatInt.f = value;
				return SerializeInteger( floatInt.i );
			}
		}
		
		bool IsReading() const
		{
			return bitpacker.GetMode() == BitPacker::Read;
		}
		
		bool IsWriting() const
		{
			return bitpacker.GetMode() == BitPacker::Write;
		}
		
		int GetBitsProcessed() const
		{
			if ( IsWriting() )
				return bitpacker.GetBitsWritten();
			else
				return bitpacker.GetBitsRead();
		}
		
		int GetBitsRemaining() const
		{
			return bitpacker.BitsRemaining();
		}
		
		static int BitsRequired( unsigned int minimum, unsigned int maximum )
		{
			assert( maximum > minimum );
			assert( maximum >= 1 );
			if ( maximum - minimum >= 0x7FFFFFF )
				return 32;
			return BitsRequired( maximum - minimum + 1 );
		}
		
		static int BitsRequired( unsigned int distinctValues )
		{
			assert( distinctValues > 1 );
			unsigned int maximumValue = distinctValues - 1;
			for ( int index = 0; index < 32; ++index )
			{
				if ( ( maximumValue & ~1 ) == 0 )
					return index + 1;
				maximumValue >>= 1;
			}
			return 32;
		}
		
	private:
		
		BitPacker bitpacker;
	};
}

#endif
