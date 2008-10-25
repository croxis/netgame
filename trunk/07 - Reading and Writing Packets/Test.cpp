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

	printf( "write bits\n" );
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

	printf( "write bits (odd)\n" );
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

	printf( "read bits\n" );
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

	printf( "read bits (odd)\n" );
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

	printf( "read/write bits\n" );
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
	
	// todo: test bits remaining
}

void test_stream()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test stream\n" );
	printf( "-----------------------------------------------------\n" );

	printf( "bits required\n" );
	{
		check( Stream::BitsRequired( 0, 1 ) == 1 );
		check( Stream::BitsRequired( 0, 3 ) == 2 );
		check( Stream::BitsRequired( 0, 7 ) == 3 );
		check( Stream::BitsRequired( 0, 15 ) == 4 );
		check( Stream::BitsRequired( 0, 31 ) == 5 );
		check( Stream::BitsRequired( 0, 63 ) == 6 );
		check( Stream::BitsRequired( 0, 127 ) == 7 );
		check( Stream::BitsRequired( 0, 255 ) == 8 );
		check( Stream::BitsRequired( 0, 511 ) == 9 );
		check( Stream::BitsRequired( 0, 1023 ) == 10 );
	}

	printf( "serialize byte\n" );
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		
		unsigned char a = 123;
		unsigned char b = 1;
		unsigned char c = 10;
		unsigned char d = 50;
		unsigned char e = 2;
		unsigned char f = 68;
		unsigned char g = 190;
		unsigned char h = 210;
		
		const int total_bits = 256 * 8;
		const int used_bits = 7 + 1 + 4 + 6 + 2 + 7 + 8 + 8;
		
 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		stream.SerializeByte( a, 0, a );
		stream.SerializeByte( b, 0, b );
		stream.SerializeByte( c, 0, c );
		stream.SerializeByte( d, 0, d );
		stream.SerializeByte( e, 0, e );
		stream.SerializeByte( f, 0, f );
		stream.SerializeByte( g, 0, g );
		stream.SerializeByte( h, 0, h );
		check( stream.GetBitsProcessed() == used_bits );
		check( stream.GetBitsRemaining() == total_bits - used_bits );

		unsigned char a_out = 0xFF;
		unsigned char b_out = 0xFF;
		unsigned char c_out = 0xFF;
		unsigned char d_out = 0xFF;
		unsigned char e_out = 0xFF;
		unsigned char f_out = 0xFF;
		unsigned char g_out = 0xFF;
		unsigned char h_out = 0xFF;
		
		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		stream.SerializeByte( a_out, 0, a );
		stream.SerializeByte( b_out, 0, b );
		stream.SerializeByte( c_out, 0, c );
		stream.SerializeByte( d_out, 0, d );
		stream.SerializeByte( e_out, 0, e );
		stream.SerializeByte( f_out, 0, f );
		stream.SerializeByte( g_out, 0, g );
		stream.SerializeByte( h_out, 0, h );
		check( stream.GetBitsProcessed() == used_bits );
		check( stream.GetBitsRemaining() == total_bits - used_bits );
		
		check( a == a_out );
		check( b == b_out );
		check( c == c_out );
		check( d == d_out );
		check( e == e_out );
		check( f == f_out );
		check( g == g_out );
		check( h == h_out );
	}

	printf( "serialize short\n" );
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		
		unsigned short a = 123;
		unsigned short b = 1;
		unsigned short c = 10004;
		unsigned short d = 50234;
		unsigned short e = 2;
		unsigned short f = 55;
		unsigned short g = 40;
		unsigned short h = 100;
		
		const int total_bits = 256 * 8;
		const int used_bits = 7 + 1 + 14 + 16 + 2 + 6 + 6 + 7;
		
 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		stream.SerializeShort( a, 0, a );
		stream.SerializeShort( b, 0, b );
		stream.SerializeShort( c, 0, c );
		stream.SerializeShort( d, 0, d );
		stream.SerializeShort( e, 0, e );
		stream.SerializeShort( f, 0, f );
		stream.SerializeShort( g, 0, g );
		stream.SerializeShort( h, 0, h );
		check( stream.GetBitsProcessed() == used_bits );
		check( stream.GetBitsRemaining() == total_bits - used_bits );

		unsigned short a_out = 0xFFFF;
		unsigned short b_out = 0xFFFF;
		unsigned short c_out = 0xFFFF;
		unsigned short d_out = 0xFFFF;
		unsigned short e_out = 0xFFFF;
		unsigned short f_out = 0xFFFF;
		unsigned short g_out = 0xFFFF;
		unsigned short h_out = 0xFFFF;
		
		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		stream.SerializeShort( a_out, 0, a );
		stream.SerializeShort( b_out, 0, b );
		stream.SerializeShort( c_out, 0, c );
		stream.SerializeShort( d_out, 0, d );
		stream.SerializeShort( e_out, 0, e );
		stream.SerializeShort( f_out, 0, f );
		stream.SerializeShort( g_out, 0, g );
		stream.SerializeShort( h_out, 0, h );
		check( stream.GetBitsProcessed() == used_bits );
		check( stream.GetBitsRemaining() == total_bits - used_bits );
		
		check( a == a_out );
		check( b == b_out );
		check( c == c_out );
		check( d == d_out );
		check( e == e_out );
		check( f == f_out );
		check( g == g_out );
		check( h == h_out );
	}

	printf( "serialize integer\n" );
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
		
		const int total_bits = 256 * 8;
		const int used_bits = 7 + 1 + 14 + 16 + 20 + 6 + 6 + 7;
		
 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		stream.SerializeInteger( a, 0, a );
		stream.SerializeInteger( b, 0, b );
		stream.SerializeInteger( c, 0, c );
		stream.SerializeInteger( d, 0, d );
		stream.SerializeInteger( e, 0, e );
		stream.SerializeInteger( f, 0, f );
		stream.SerializeInteger( g, 0, g );
		stream.SerializeInteger( h, 0, h );
		check( stream.GetBitsProcessed() == used_bits );
		check( stream.GetBitsRemaining() == total_bits - used_bits );

		unsigned int a_out = 0xFFFFFFFF;
		unsigned int b_out = 0xFFFFFFFF;
		unsigned int c_out = 0xFFFFFFFF;
		unsigned int d_out = 0xFFFFFFFF;
		unsigned int e_out = 0xFFFFFFFF;
		unsigned int f_out = 0xFFFFFFFF;
		unsigned int g_out = 0xFFFFFFFF;
		unsigned int h_out = 0xFFFFFFFF;
		
		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		stream.SerializeInteger( a_out, 0, a );
		stream.SerializeInteger( b_out, 0, b );
		stream.SerializeInteger( c_out, 0, c );
		stream.SerializeInteger( d_out, 0, d );
		stream.SerializeInteger( e_out, 0, e );
		stream.SerializeInteger( f_out, 0, f );
		stream.SerializeInteger( g_out, 0, g );
		stream.SerializeInteger( h_out, 0, h );
		check( stream.GetBitsProcessed() == used_bits );
		check( stream.GetBitsRemaining() == total_bits - used_bits );
		
		check( a == a_out );
		check( b == b_out );
		check( c == c_out );
		check( d == d_out );
		check( e == e_out );
		check( f == f_out );
		check( g == g_out );
		check( h == h_out );
	}

	printf( "serialize float\n" );
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		
		float a = 12.3f;
		float b = 1.8753f;
 		float c = 10004.017231f;
		float d = 50234.01231f;
		float e = 1020491.5834f;
		float f = 55.0f;
		float g = 40.9f;
		float h = 100.001f;
		
		const int total_bits = 256 * 8;
		const int used_bits = 8 * 32;
		
 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		stream.SerializeFloat( a );
		stream.SerializeFloat( b );
		stream.SerializeFloat( c );
		stream.SerializeFloat( d );
		stream.SerializeFloat( e );
		stream.SerializeFloat( f );
		stream.SerializeFloat( g );
		stream.SerializeFloat( h );
		check( stream.GetBitsProcessed() == used_bits );
		check( stream.GetBitsRemaining() == total_bits - used_bits );

 		float a_out = 0.0f;
 		float b_out = 0.0f;
 		float c_out = 0.0f;
 		float d_out = 0.0f;
 		float e_out = 0.0f;
 		float f_out = 0.0f;
 		float g_out = 0.0f;
 		float h_out = 0.0f;
		
		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		stream.SerializeFloat( a_out );
		stream.SerializeFloat( b_out );
		stream.SerializeFloat( c_out );
		stream.SerializeFloat( d_out );
		stream.SerializeFloat( e_out );
		stream.SerializeFloat( f_out );
		stream.SerializeFloat( g_out );
		stream.SerializeFloat( h_out );
		check( stream.GetBitsProcessed() == used_bits );
		check( stream.GetBitsRemaining() == total_bits - used_bits );
		
		check( a == a_out );
		check( b == b_out );
		check( c == c_out );
		check( d == d_out );
		check( e == e_out );
		check( f == f_out );
		check( g == g_out );
		check( h == h_out );
	}
	
	printf( "stream attribution\n" );
	{
		
	}
}

int main( int argc, char * argv[] )
{
	test_bit_packer();
	test_stream();

	printf( "-----------------------------------------------------\n" );
	printf( "passed!\n" );

	return 0;
}
