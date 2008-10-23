/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#ifndef NET_TRANSPORT_H
#define NET_TRANSPORT_H

namespace net
{
	// abstract network transport interface
	//  + implement this interface for different transport layers
	//  + use the reliability system classes to implement seq/ack based reliability
	
	class Transport
	{
	public:

		virtual ~Transport() {};
		
		virtual bool IsNodeConnected( int nodeId ) = 0;
		
		virtual int GetLocalNodeId() const = 0;
		
		virtual int GetMaxAllowedNodes() const = 0;

		virtual bool SendPacket( int nodeId, const unsigned char data[], int size ) = 0;
		
		virtual int ReceivePacket( int & nodeId, unsigned char data[], int size ) = 0;

		virtual class ReliabilitySystem & GetReliability( int nodeId ) = 0;

		virtual void Update( float deltaTime ) = 0;
	};
}

#endif
