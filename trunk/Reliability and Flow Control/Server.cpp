/*
	Server using Virtual Connection over UDP
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
	
	if ( !connection.Start( ServerPort ) )
	{
		printf( "could not start connection on port %d\n", ServerPort );
		return 1;
	}
	
	connection.Listen();
	
	while ( true )
	{
		if ( connection.IsConnected() )
		{
			unsigned char packet[PacketSize];
			memset( packet, 0, PacketSize );
			connection.SendPacket( packet, PacketSize );
		}
		
		while ( true )
		{
			unsigned char packet[PacketSize];
			int bytes_read = connection.ReceivePacket( packet, PacketSize );
			if ( bytes_read == 0 )
				break;
		}
		
		connection.Update( DeltaTime );

		wait( DeltaTime );
	}
	
	ShutdownSockets();

	return 0;
}
