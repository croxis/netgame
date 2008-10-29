/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include "NetTransport.h"
#include "NetLAN.h"

static net::TransportType transportType = net::Transport_None;
static int transportCount = 0;

bool net::Transport::Initialize( TransportType type )
{
	assert( type != Transport_None );
	bool result = false;
	switch ( type )
	{
		case Transport_LAN: result = InitializeSockets(); break;
		default: break;
	}
	transportType = type;
	return result;
}

void net::Transport::Shutdown()
{
	switch ( transportType )
	{
		case Transport_LAN: ShutdownSockets();
		default: break;
	}
}

net::Transport * net::Transport::Create()
{
	Transport * transport = NULL;
	
	assert( transportCount >= 0 );
	
	switch ( transportType )
	{
		case Transport_LAN: 	transport = new TransportLAN(); 		break;
//		case Transport_RakNet:	transport = new TransportRakNet(); 		break;
//		case Transport_OpenTNL:	transport = new TransportOpenTNL();		break;
//		case Transport_eNet:	transport = new TransportENet();		break;
		default: break;
	}

	assert( transport );
	assert( transport->GetType() == transportType );
	
	transportCount++;
	
	return transport;
}

void net::Transport::Destroy( Transport * transport )
{
	assert( transport );
	assert( transport->GetType() == transportType );
	assert( transportCount > 0 );
	delete transport;
	transportCount--;
}
