/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include "NetLAN.h"
#include "NetSockets.h"
#include "NetBeacon.h"
#include "NetNodeMesh.h"

// static interface

bool net::TransportLAN::Initialize()
{
	return InitializeSockets();
}

void net::TransportLAN::Shutdown()
{
	return ShutdownSockets();
}

bool net::TransportLAN::GetHostName( char hostname[], int size )
{
	return net::GetHostName( hostname, size );
}

// lan specific interface

net::TransportLAN::TransportLAN()
{
	mesh = NULL;
	node = NULL;
	beacon = NULL;
	listener = NULL;
	beaconAccumulator = 1.0f;
}

net::TransportLAN::~TransportLAN()
{
	Stop();
}

void net::TransportLAN::Configure( Config & config )
{
	// todo: assert not already running
	this->config = config;
}

const net::TransportLAN::Config & net::TransportLAN::GetConfig() const
{
	return config;
}

bool net::TransportLAN::StartServer( const char name[] )
{
	assert( !node );
	assert( !mesh );
	assert( !beacon );
	assert( !listener );
	beacon = new Beacon( name, config.protocolId, config.serverPort, config.listenerPort );
	if ( !beacon->Start( config.beaconPort ) )
	{
		printf( "failed to start beacon on port %d\n", config.beaconPort );
		Stop();
		return false;
	}
	mesh = new Mesh( config.protocolId, config.maxNodes, config.meshSendRate, config.timeout );
	if ( !mesh->Start( config.meshPort ) )
	{
		printf( "failed to start mesh on port %d\n", config.meshPort );
		Stop();
		return 1;
	}
	node = new Node( config.protocolId, config.meshSendRate, config.timeout );
 	if ( !node->Start( config.serverPort ) )
	{
		printf( "failed to start node on port %d\n", config.serverPort );
		Stop();
		return 1;
	}
	mesh->Reserve( 0, Address(127,0,0,1,config.serverPort) );
	node->Join( Address(127,0,0,1,config.meshPort) );
	return true;
}

bool net::TransportLAN::ConnectClient( const Address & address )
{
	node = new Node( config.protocolId, config.meshSendRate, config.timeout );
 	if ( !node->Start( config.clientPort ) )
	{
		printf( "failed to start node on port %d\n", config.serverPort );
		Stop();
		return 1;
	}
	node->Join( address );
	return true;
}

bool net::TransportLAN::ConnectClient( const char name[] )
{
	assert( !node );
	assert( !mesh );
	assert( !beacon );
	assert( !listener );
	listener = new Listener( config.protocolId, config.timeout );
	if ( !listener->Start( config.listenerPort ) )
	{
		printf( "failed to start listener on port %d\n", config.listenerPort );
		Stop();
		return false;
	}
	// ...
	return true;
}

// todo: EnterLANLobby

void net::TransportLAN::Stop()
{
	if ( mesh )
	{
		delete mesh;
		mesh = NULL;
	}
	if ( node )
	{
		delete node;
		node = NULL;
	}
	if ( beacon )
	{
		delete beacon;
		beacon = NULL;
	}
	if ( listener )
	{
		delete listener;
		listener = NULL;
	}
}

// implement transport interface

bool net::TransportLAN::IsNodeConnected( int nodeId )
{
	assert( node );
	return node->IsNodeConnected( nodeId );
}

int net::TransportLAN::GetLocalNodeId() const
{
	assert( node );
	return node->GetLocalNodeId();
}

int net::TransportLAN::GetMaxNodes() const
{
	assert( node );
	return node->GetMaxNodes();
}

bool net::TransportLAN::SendPacket( int nodeId, const unsigned char data[], int size )
{
	assert( node );
	return node->SendPacket( nodeId, data, size );
}

int net::TransportLAN::ReceivePacket( int & nodeId, unsigned char data[], int size )
{
	assert( node );
	return node->ReceivePacket( nodeId, data, size );
}

class net::ReliabilitySystem & net::TransportLAN::GetReliability( int nodeId )
{
	// todo: implement
	static ReliabilitySystem reliabilitySystem;
	return reliabilitySystem;
}

void net::TransportLAN::Update( float deltaTime )
{
	if ( mesh )
		mesh->Update( deltaTime );
	if ( node )
		node->Update( deltaTime );
	if ( beacon )
	{
		beaconAccumulator += deltaTime;
		while ( beaconAccumulator >= 1.0f )
		{
			beacon->Update( 1.0f );
			beaconAccumulator -= 1.0f;
		}
	}
	if ( listener )
		listener->Update( deltaTime );
	if ( mesh )
		mesh->Update( deltaTime );
	if ( node )
		node->Update( deltaTime );
}

net::TransportType net::TransportLAN::GetType() const
{
	return Transport_LAN;
}
