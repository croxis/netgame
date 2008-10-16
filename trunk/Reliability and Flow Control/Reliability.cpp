/*
	Reliability Example
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
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
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
	float accumulator = 0.0f;
	
	while ( true )
	{
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
		
		unsigned char packet[PacketSize];
		
		connection.SendPacket( packet, sizeof( packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		connection.Update( DeltaTime );

		// show stats
		
		accumulator += DeltaTime;

		if ( accumulator > 0.25f && connection.IsConnected() )
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();
			
			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();
			
			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();
			
			printf( "rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n", 
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets, 
				sent_packets > 0.0f ? (float) lost_packets / (float) sent_packets * 100.0f : 0.0f, 
				sent_bandwidth, acked_bandwidth );
			
			accumulator = 0.0f;
		}

		wait( DeltaTime );
	}
	
	ShutdownSockets();

	return 0;
}
