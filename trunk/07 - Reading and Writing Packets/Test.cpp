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

	printf( "test write bits\n" );
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

	printf( "test read bits\n" );
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		buffer[0] = 0xFF;
		buffer[1] = 0xFF;
		buffer[2] = 0xFF;
		buffer[3] = 0xFF;
		buffer[4] = 0xFF;
		buffer[5] = 0xFF;
		buffer[6] = 0xFF;
		BitPacker bitpacker( BitPacker::Read, buffer, sizeof(buffer) );
		unsigned int value;
		bitpacker.ReadBits( value, 32 );
		assert( value == 0xFFFFFFFF );
		check( bitpacker.GetBitsRead() == 32 );
		bitpacker.ReadBits( value, 16 );
		assert( value == 0x0000FFFF );
		check( bitpacker.GetBitsRead() == 32 + 16 );
		bitpacker.ReadBits( value, 8 );
		assert( value == 0x000000FF );
		check( bitpacker.GetBitsRead() == 32 + 16 + 8 );
	}

	printf( "test read bits (odd)\n" );
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		buffer[0] = 0xFF;
		buffer[1] = 0xFF;
		buffer[2] = 0xFF;
		buffer[3] = 0xFF;
		buffer[4] = 0x00;

		BitPacker bitpacker( BitPacker::Read, buffer, sizeof(buffer) );

		unsigned int value;
		bitpacker.ReadBits( value, 9 );
		check( bitpacker.GetBitsRead() == 9 );
		check( value == ( 1 << 9 ) - 1 );

		bitpacker.ReadBits( value, 1 );
		check( bitpacker.GetBitsRead() == 9 + 1 );
		check( value == 1 );

		bitpacker.ReadBits( value, 11 );
		check( bitpacker.GetBitsRead() == 9 + 1 + 11 );
		check( value == ( 1 << 11 ) - 1 );

		bitpacker.ReadBits( value, 6 );
		check( bitpacker.GetBitsRead() == 9 + 1 + 11 + 6 );
		check( value == ( 1 << 6 ) - 1 );

		bitpacker.ReadBits( value, 5 );
		check( bitpacker.GetBitsRead() == 32 );
		check( value == ( 1 << 5 ) - 1 );
	}

	printf( "test read/write bits\n" );
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		
		unsigned int a = 123;
		unsigned int b = 1;
		unsigned int c = 10004;
		unsigned int d = 50234;
		unsigned int e = 1020491;
		unsigned int f = 55;
		unsigned int g = 40;
		unsigned int h = 100;
		
		BitPacker bitpacker( BitPacker::Write, buffer, sizeof(buffer) );
		bitpacker.WriteBits( a, 7 );
		bitpacker.WriteBits( b, 1 );
		bitpacker.WriteBits( c, 14 );
		bitpacker.WriteBits( d, 16 );
		bitpacker.WriteBits( e, 20 );
		bitpacker.WriteBits( f, 6 );
		bitpacker.WriteBits( g, 6 );
		bitpacker.WriteBits( h, 7 );
		check( bitpacker.GetBitsWritten() == 7 + 1 + 14 + 16 + 20 + 6 + 6 + 7 );

		unsigned int a_out = 0xFFFFFFFF;
		unsigned int b_out = 0xFFFFFFFF;
		unsigned int c_out = 0xFFFFFFFF;
		unsigned int d_out = 0xFFFFFFFF;
		unsigned int e_out = 0xFFFFFFFF;
		unsigned int f_out = 0xFFFFFFFF;
		unsigned int g_out = 0xFFFFFFFF;
		unsigned int h_out = 0xFFFFFFFF;
		
		bitpacker = BitPacker( BitPacker::Read, buffer, sizeof(buffer ) );
		bitpacker.ReadBits( a_out, 7 );
		bitpacker.ReadBits( b_out, 1 );
		bitpacker.ReadBits( c_out, 14 );
		bitpacker.ReadBits( d_out, 16 );
		bitpacker.ReadBits( e_out, 20 );
		bitpacker.ReadBits( f_out, 6 );
		bitpacker.ReadBits( g_out, 6 );
		bitpacker.ReadBits( h_out, 7 );
		check( bitpacker.GetBitsRead() == 7 + 1 + 14 + 16 + 20 + 6 + 6 + 7 );
		
		check( a == a_out );
		check( b == b_out );
		check( c == c_out );
		check( d == d_out );
		check( e == e_out );
		check( f == f_out );
		check( g == g_out );
		check( h == h_out );
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
