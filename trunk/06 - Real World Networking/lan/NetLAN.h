/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#ifndef NET_LAN_H
#define NET_LAN_H

#include "../NetTransport.h"

namespace net
{
	// lan transport implementation
	//  + servers are advertized via net beacon
	//  + lan lobby is filled via net listener
	//  + a mesh runs on the server IP and manages node connections
	//  + a node runs on each transport, for the server with the mesh a local node also runs
	
	class TransportLAN : public Transport
	{
	public:

		// static interface
		
		static bool Initialize();

		static void Shutdown();

		static bool GetHostName( char hostname[], int size );

		// lan specific interface
		
		TransportLAN();
		~TransportLAN();
		
		struct Config
		{
			unsigned short meshPort;
			unsigned short serverPort;
			unsigned short clientPort;
			unsigned short beaconPort;
			unsigned short listenerPort;
			unsigned int protocolId;
			float meshSendRate;
			float timeout;
			int maxNodes;
			
			Config()
			{
				meshPort = 30000;
				clientPort = 30001;
				serverPort = 30002;
				beaconPort = 40000;
				listenerPort = 40001;
				protocolId = 0x12345678;
				meshSendRate = 0.25f;
				timeout = 10.0f;
				maxNodes = 4;
			}
		};
		
		void Configure( Config & config );
		
		const Config & GetConfig() const;
		
		bool StartServer( const char name[] );
		
		bool ConnectClient( const class Address & address );
		
		bool ConnectClient( const char name[] );

		// todo: add interface for LAN lobby
		// todo: add option to connect directly by IP address
		
		void Stop();

		// implement transport interface
		
		bool IsNodeConnected( int nodeId );
		
		int GetLocalNodeId() const;
		
		int GetMaxNodes() const;

		bool SendPacket( int nodeId, const unsigned char data[], int size );
		
		int ReceivePacket( int & nodeId, unsigned char data[], int size );

		class ReliabilitySystem & GetReliability( int nodeId );

		void Update( float deltaTime );
		
		TransportType GetType() const;
		
	private:

		Config config;
		class Mesh * mesh;
		class Node * node;
		class Beacon * beacon;
		class Listener * listener;
		float beaconAccumulator;
	};
}

#endif
