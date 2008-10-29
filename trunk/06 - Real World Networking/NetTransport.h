/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#ifndef NET_TRANSPORT_H
#define NET_TRANSPORT_H

namespace net
{
	// transport type
	
	enum TransportType
	{
		Transport_None,
		Transport_LAN,
		Transport_RakNet,
		Transport_OpenTNL,
		Transport_eNet
	};
	
	// abstract network transport interface
	//  + implement this interface for different transport layers
	//  + use the reliability system classes to implement seq/ack based reliability
	
	class Transport
	{
	public:

		// static methods
		
		static bool Initialize( TransportType type );
		
		static void Shutdown();
		
		static Transport * Create();
		
		static void Destroy( Transport * transport );
		
		// transport interface
		
		virtual ~Transport() {};
		
		virtual bool IsNodeConnected( int nodeId ) = 0;
		
		virtual int GetLocalNodeId() const = 0;
		
		virtual int GetMaxNodes() const = 0;

		virtual bool SendPacket( int nodeId, const unsigned char data[], int size ) = 0;
		
		virtual int ReceivePacket( int & nodeId, unsigned char data[], int size ) = 0;

		virtual class ReliabilitySystem & GetReliability( int nodeId ) = 0;

		virtual void Update( float deltaTime ) = 0;
		
		virtual TransportType GetType() const = 0;
	};	
}

#endif
