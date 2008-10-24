/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#ifndef NET_STREAM_H
#define NET_STREAM_H

#include <assert.h>

namespace net
{
	// bitpacker class
	//  + read and write non-8 multiples of bits efficiently
	
	class BitPacker
	{
		enum Mode
		{
			Read,
			Write
		};
		
		BitPacker( Mode mode, unsigned char * buffer, unsigned int size )
		{
			assert( buffer );
			assert( size >= 4 );
			assert( size & 3 == 0 );
			this->mode = mode;
			this->buffer = buffer;
			this->size = size;
		}
		
		bool WriteBits( unsigned int value, int bits = 32 )
		{
			// ...
			return false;
		}
		
		bool ReadBits( unsigned int & value, int bits = 32 )
		{
			// ...
			return false;
		}
		
		Mode GetMode() const
		{
			return mode;
		}
		
	private:
		
		unsigned char * buffer;
		int size;
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
		
		ArithmeticCoder( Mode mode, unsigned char * buffer, unsigned int size )
		{
			assert( buffer );
			assert( size >= 4 );
			assert( size & 3 == 0 );
			this->mode = mode;
			this->buffer = buffer;
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
		// ...
	};
}

#endif
