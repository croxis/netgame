/*
	Unit Tests for Reliable Connection
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Net.h"

using namespace std;
using namespace net;

#ifdef DEBUG
#define check assert
#else
#define check(n) if ( !n ) { printf( "check failed\n" ); exit(1); }
#endif

void test_join()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test join\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	ReliableConnection client( ProtocolId, TimeOut );
	ReliableConnection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
		
		wait( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );
}

void test_join_timeout()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test join timeout\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	ReliableConnection client( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	
	while ( true )
	{
		if ( !client.IsConnecting() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
		
		wait( DeltaTime );
	}
	
	check( !client.IsConnected() );
	check( client.ConnectFailed() );
}

void test_join_busy()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test join busy\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	// connect client to server
	
	ReliableConnection client( ProtocolId, TimeOut );
	ReliableConnection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
		
		wait( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );

	// attempt another connection, verify connect fails (busy)
	
	ReliableConnection busy( ProtocolId, TimeOut );
	check( busy.Start( ClientPort + 1 ) );
	busy.Connect( Address(127,0,0,1,ServerPort ) );

	while ( true )
	{
		if ( !busy.IsConnecting() || busy.IsConnected() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		unsigned char busy_packet[] = "i'm so busy!";
		busy.SendPacket( busy_packet, sizeof( busy_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = busy.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		client.Update( DeltaTime );
		server.Update( DeltaTime );
		busy.Update( DeltaTime );
		
		wait( DeltaTime );
	}

	check( client.IsConnected() );
	check( server.IsConnected() );
	check( !busy.IsConnected() );
	check( busy.ConnectFailed() );
}

void test_rejoin()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test rejoin\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	ReliableConnection client( ProtocolId, TimeOut );
	ReliableConnection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );

	// connect client and server
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
		
		wait( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );

	// let connection timeout

	while ( client.IsConnected() || server.IsConnected() )
	{
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		client.Update( DeltaTime );
		server.Update( DeltaTime );

		wait( DeltaTime );
	}
	
	check( !client.IsConnected() );
	check( !server.IsConnected() );

	// reconnect client

	client.Connect( Address(127,0,0,1,ServerPort ) );
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
		
		wait( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );
}

void test_payload()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test payload\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	ReliableConnection client( ProtocolId, TimeOut );
	ReliableConnection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
			check( strcmp( (const char*) packet, "server to client" ) == 0 );
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
			check( strcmp( (const char*) packet, "client to server" ) == 0 );
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
		
		wait( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );
}

void test_acks()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test acks\n" );
	printf( "-----------------------------------------------------\n" );

	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	const unsigned int PacketCount = 100;
	
	ReliableConnection client( ProtocolId, TimeOut );
	ReliableConnection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
		
	bool clientAckedPackets[PacketCount];
	bool serverAckedPackets[PacketCount];
	for ( unsigned int i = 0; i < PacketCount; ++i )
	{
		clientAckedPackets[i] = false;
		serverAckedPackets[i] = false;
	}
	
	check( client.GetLocalSequence() == 0 );
	check( server.GetLocalSequence() == 0 );

	bool allPacketsAcked = false;

	while ( true )
	{
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
			
		if ( allPacketsAcked )
			break;
		
		unsigned char packet[256];
		for ( unsigned int i = 0; i < sizeof(packet); ++i )
			packet[i] = (unsigned char) i;
		server.SendPacket( packet, sizeof(packet) );
		client.SendPacket( packet, sizeof(packet) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
			check( bytes_read == sizeof(packet) );
			for ( unsigned int i = 0; i < sizeof(packet); ++i )
				check( packet[i] == (unsigned char) i );
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
			check( bytes_read == sizeof(packet) );
			for ( unsigned int i = 0; i < sizeof(packet); ++i )
				check( packet[i] == (unsigned char) i );
		}
		
		int ack_count = 0;
		unsigned int * acks = NULL;
		client.GetAcks( &acks, ack_count );
		check( ack_count == 0 || ack_count != 0 && acks );
		for ( int i = 0; i < ack_count; ++i )
		{
			unsigned int ack = acks[i];
			if ( ack < PacketCount )
			{
				check( !clientAckedPackets[ack] );
				clientAckedPackets[ack] = true;
			}
		}

		server.GetAcks( &acks, ack_count );
		check( ack_count == 0 || ack_count != 0 && acks );
		for ( int i = 0; i < ack_count; ++i )
		{
			unsigned int ack = acks[i];
			if ( ack < PacketCount )
			{
				check( !serverAckedPackets[ack] );
				serverAckedPackets[ack] = true;
			}
		}
		
		unsigned int clientAckCount = 0;
		unsigned int serverAckCount = 0;
		for ( unsigned int i = 0; i < PacketCount; ++i )
		{
			if ( clientAckedPackets[i] )
				clientAckCount++;
			if ( serverAckedPackets[i] )
				serverAckCount++;
		}
		allPacketsAcked = clientAckCount == PacketCount && serverAckCount == PacketCount;
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
		
		wait( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );
}

void test_ack_bits()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test ack bits\n" );
	printf( "-----------------------------------------------------\n" );

	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	const unsigned int PacketCount = 100;
	
	ReliableConnection client( ProtocolId, TimeOut );
	ReliableConnection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
		
	bool clientAckedPackets[PacketCount];
	bool serverAckedPackets[PacketCount];
	for ( unsigned int i = 0; i < PacketCount; ++i )
	{
		clientAckedPackets[i] = false;
		serverAckedPackets[i] = false;
	}
	
	check( client.GetLocalSequence() == 0 );
	check( server.GetLocalSequence() == 0 );

	bool allPacketsAcked = false;

	while ( true )
	{
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
			
		if ( allPacketsAcked )
			break;
		
		unsigned char packet[256];
		for ( unsigned int i = 0; i < sizeof(packet); ++i )
			packet[i] = (unsigned char) i;

		for ( int i = 0; i < 10; ++i )
		{
			client.SendPacket( packet, sizeof(packet) );
			
			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				check( bytes_read == sizeof(packet) );
				for ( unsigned int i = 0; i < sizeof(packet); ++i )
					check( packet[i] == (unsigned char) i );
			}

			int ack_count = 0;
			unsigned int * acks = NULL;
			client.GetAcks( &acks, ack_count );
			check( ack_count == 0 || ack_count != 0 && acks );
			for ( int i = 0; i < ack_count; ++i )
			{
				unsigned int ack = acks[i];
				if ( ack < PacketCount )
				{
					check( !clientAckedPackets[ack] );
					clientAckedPackets[ack] = true;
//					printf( "client ack %d\n", ack );
				}
			}

			client.Update( DeltaTime * 0.1f );

			wait( DeltaTime * 0.1f );
		}
		
		server.SendPacket( packet, sizeof(packet) );

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
			check( bytes_read == sizeof(packet) );
			for ( unsigned int i = 0; i < sizeof(packet); ++i )
				check( packet[i] == (unsigned char) i );
		}

		int ack_count = 0;
		unsigned int * acks = NULL;
		server.GetAcks( &acks, ack_count );
		check( ack_count == 0 || ack_count != 0 && acks );
		for ( int i = 0; i < ack_count; ++i )
		{
			unsigned int ack = acks[i];
			if ( ack < PacketCount )
			{
				check( !serverAckedPackets[ack] );
				serverAckedPackets[ack] = true;
			}
		}
		
		unsigned int clientAckCount = 0;
		unsigned int serverAckCount = 0;
		for ( unsigned int i = 0; i < PacketCount; ++i )
		{
			if ( clientAckedPackets[i] )
				clientAckCount++;
			if ( serverAckedPackets[i] )
				serverAckCount++;
		}
//		printf( "client ack count = %d, server ack count = %d\n", clientAckCount, serverAckCount );
		allPacketsAcked = clientAckCount == PacketCount && serverAckCount == PacketCount;
		
		server.Update( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );
}

void tests()
{
	test_join();
	test_join_timeout();
	test_join_busy();
	test_rejoin();
	test_payload();
	test_acks();
	test_ack_bits();

	printf( "-----------------------------------------------------\n" );
	printf( "passed!\n" );
}

int main( int argc, char * argv[] )
{
	if ( !InitializeSockets() )
	{
		printf( "failed to initialize sockets\n" );
		return 1;
	}
	
	tests();
	
	ShutdownSockets();

	return 0;
}
