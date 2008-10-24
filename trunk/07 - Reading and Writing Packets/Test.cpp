/*
	Unit Tests for Networking Library
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define NET_UNIT_TEST

#include "NetStream.h"

using namespace std;
using namespace net;

#ifdef DEBUG
#define check assert
#else
#define check(n) if ( !n ) { printf( "check failed\n" ); exit(1); }
#endif

void test_bit_packer()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test bit packer\n" );
	printf( "-----------------------------------------------------\n" );

	printf( "test write bits (even)\n" );
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		BitPacker bitpacker( BitPacker::Write, buffer, sizeof(buffer) );
		bitpacker.WriteBits( 0xFFFFFFFF, 32 );
		check( bitpacker.GetBitsWritten() == 32 );
		check( buffer[0] == 0xFF );
		check( buffer[1] == 0xFF );
		check( buffer[2] == 0xFF );
		check( buffer[3] == 0xFF );
		check( buffer[4] == 0x00 );
		bitpacker.WriteBits( 0x1111FFFF, 16 );
		check( bitpacker.GetBitsWritten() == 32 + 16 );
		check( buffer[0] == 0xFF );
		check( buffer[1] == 0xFF );
		check( buffer[2] == 0xFF );
		check( buffer[3] == 0xFF );
		check( buffer[4] == 0xFF );
		check( buffer[5] == 0xFF );
		check( buffer[6] == 0x00 );
		bitpacker.WriteBits( 0x111111FF, 8 );
		check( bitpacker.GetBitsWritten() == 32 + 16 + 8 );
		check( buffer[0] == 0xFF );
		check( buffer[1] == 0xFF );
		check( buffer[2] == 0xFF );
		check( buffer[3] == 0xFF );
		check( buffer[4] == 0xFF );
		check( buffer[5] == 0xFF );
		check( buffer[6] == 0xFF );
		check( buffer[7] == 0x00 );
	}

	printf( "test write bits (odd)\n" );
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		BitPacker bitpacker( BitPacker::Write, buffer, sizeof(buffer) );
		bitpacker.WriteBits( 0xFFFFFFFF, 9 );
		check( bitpacker.GetBitsWritten() == 9 );
		bitpacker.WriteBits( 0xFFFFFFFF, 1 );
		check( bitpacker.GetBitsWritten() == 9 + 1 );
		bitpacker.WriteBits( 0xFFFFFFFF, 11 );
		check( bitpacker.GetBitsWritten() == 9 + 1 + 11 );
		bitpacker.WriteBits( 0xFFFFFFFF, 6 );
		check( bitpacker.GetBitsWritten() == 9 + 1 + 11 + 6 );
		bitpacker.WriteBits( 0xFFFFFFFF, 5 );
		check( bitpacker.GetBitsWritten() == 32 );
		check( buffer[0] == 0xFF );
		check( buffer[1] == 0xFF );
		check( buffer[2] == 0xFF );
		check( buffer[3] == 0xFF );
		check( buffer[4] == 0x00 );		
	}
}

void test_arithmetic_coder()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test connection join timeout\n" );
	printf( "-----------------------------------------------------\n" );

	// ...
}

void test_stream()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test stream\n" );
	printf( "-----------------------------------------------------\n" );

	// ...
}

int main( int argc, char * argv[] )
{
	test_bit_packer();
	test_arithmetic_coder();
	test_stream();

	printf( "-----------------------------------------------------\n" );
	printf( "passed!\n" );

	return 0;
}
