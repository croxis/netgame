/*
	Acks Example
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

const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0 / 30.0f;
const float BadSendRate = 0.1f;
const float SendRate = 1.0 / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;

int main( int argc, char * argv[] )
{
	if ( !InitializeSockets() )
	{
		printf( "failed to initialize sockets\n" );
		return 1;
	}

	ReliableConnection connection( ProtocolId, TimeOut );
	
	if ( !connection.Start( ClientPort ) )
	{
		printf( "could not start connection on port %d\n", ClientPort );
		return 1;
	}
	
	connection.Connect( Address(127,0,0,1,ServerPort ) );
	
	bool connected = false;
	
	while ( true )
	{
		// send and receive packets
		
		unsigned char packet[PacketSize];
		
		for ( int i = 0; i < 10; ++i )
			connection.SendPacket( packet, sizeof( packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		// detect connection, connection failed...

		if ( !connected && connection.IsConnected() )
		{
			printf( "client connected to server\n" );
			connected = true;
		}
		
		if ( !connected && connection.ConnectFailed() )
		{
			printf( "connection failed\n" );
			break;
		}
		
		// print acked packets
		
		unsigned int * acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks( &acks, ack_count );
		if ( ack_count > 0 )
		{
			printf( "acks: %d", acks[0] );
			for ( int i = 1; i < ack_count; ++i )
				printf( ",%d", acks[i] );
			printf( "\n" );
		}
		
		connection.Update( DeltaTime );

		wait( DeltaTime );
	}
	
	ShutdownSockets();

	return 0;
}
