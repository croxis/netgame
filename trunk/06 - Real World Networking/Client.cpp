/*
	Server Example using Node Mesh
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "NetPlatform.h"
#include "NetTransport.h"
#include "lan/NetLAN.h"			// todo: move NetLAN.h interface into NetTransport.h?

using namespace std;
using namespace net;

int main( int argc, char * argv[] )
{
	// initialize and create transport
	
	TransportType type = Transport_LAN;
	
	if ( !Transport::Initialize( Transport_LAN ) )
	{
		printf( "failed to initialize transport layer\n" );
		return 1;
	}
	
	Transport * transport = Transport::Create();
	
	if ( !transport )
	{
		printf( "could not create transport\n" );
		return 1;
	}
	
	// connect to server (transport specific)
	
	switch ( type )
	{
		case Transport_LAN:
		{
			TransportLAN * lan_transport = dynamic_cast<TransportLAN*>( transport );
			char server[64+1] = "127.0.0.1:30000";
//			TransportLAN::GetHostName( server, sizeof(server) );
			if ( argc == 2 )
			{
				strncpy( server, argv[1], sizeof(server) );
				server[64] = '\0';
			}
			lan_transport->ConnectClient( server );
		}
		break;
		
		default:
			break;
	}

	// main loop

	const float DeltaTime = 1.0f / 30.0f;

	while ( true )
	{
		transport->Update( DeltaTime );
		wait_seconds( DeltaTime );
	}
	
	// shutdown
	
	Transport::Destroy( transport );
	
	Transport::Shutdown();

	return 0;
}
