/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#ifndef NET_LAN_H
#define NET_LAN_H

#include "NetPlatform.h"
#include "NetReliability.h"
#include "NetTransport.h"

#include <assert.h>
#include <vector>
#include <map>
#include <stack>
#include <list>
#include <algorithm>
#include <functional>

namespace net
{
	// virtual connection over UDP
	//  + very simple, just for learning purposes do not use in production code
	
	class Connection
	{
	public:
		
		enum Mode
		{
			None,	
			Client,
			Server
		};
		
		Connection( unsigned int protocolId, float timeout )
		{
			this->protocolId = protocolId;
			this->timeout = timeout;
			mode = None;
			running = false;
			ClearData();
		}
		
		virtual ~Connection()
		{
			if ( IsRunning() )
				Stop();
		}
		
		bool Start( int port )
		{
			assert( !running );
			printf( "start connection on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			OnStart();
			return true;
		}
		
		void Stop()
		{
			assert( running );
			printf( "stop connection\n" );
			bool connected = IsConnected();
			ClearData();
			socket.Close();
			running = false;
			if ( connected )
				OnDisconnect();
			OnStop();
		}
		
		bool IsRunning() const
		{
			return running;
		}
		
		void Listen()
		{
			printf( "server listening for connection\n" );
			bool connected = IsConnected();
			ClearData();
			if ( connected )
				OnDisconnect();
			mode = Server;
			state = Listening;
		}
		
		void Connect( const Address & address )
		{
			printf( "client connecting to %d.%d.%d.%d:%d\n", 
				address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort() );
			bool connected = IsConnected();
			ClearData();
			if ( connected )
				OnDisconnect();
			mode = Client;
			state = Connecting;
			this->address = address;
		}
		
		bool IsConnecting() const
		{
			return state == Connecting;
		}
		
		bool ConnectFailed() const
		{
			return state == ConnectFail;
		}
		
		bool IsConnected() const
		{
			return state == Connected;
		}
		
		bool IsListening() const
		{
			return state == Listening;
		}
		
		Mode GetMode() const
		{
			return mode;
		}
		
		virtual void Update( float deltaTime )
		{
			assert( running );
			timeoutAccumulator += deltaTime;
			if ( timeoutAccumulator > timeout )
			{
				if ( state == Connecting )
				{
					printf( "connect timed out\n" );
					ClearData();
					state = ConnectFail;
					OnDisconnect();
				}
				else if ( state == Connected )
				{
					printf( "connection timed out\n" );
					ClearData();
					if ( state == Connecting )
						state = ConnectFail;
					OnDisconnect();
				}
			}
		}
		
		virtual bool SendPacket( const unsigned char data[], int size )
		{
			assert( running );
			if ( address.GetAddress() == 0 )
				return false;
			unsigned char packet[size+4];
			packet[0] = (unsigned char) ( protocolId >> 24 );
			packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
			packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
			packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
			memcpy( &packet[4], data, size );
			return socket.Send( address, packet, size + 4 );
		}
		
		virtual int ReceivePacket( unsigned char data[], int size )
		{
			assert( running );
			unsigned char packet[size+4];
			Address sender;
			int bytes_read = socket.Receive( sender, packet, size + 4 );
			if ( bytes_read == 0 )
				return 0;
			if ( bytes_read <= 4 )
				return 0;
			if ( packet[0] != (unsigned char) ( protocolId >> 24 ) || 
				 packet[1] != (unsigned char) ( ( protocolId >> 16 ) & 0xFF ) ||
				 packet[2] != (unsigned char) ( ( protocolId >> 8 ) & 0xFF ) ||
				 packet[3] != (unsigned char) ( protocolId & 0xFF ) )
				return 0;
			if ( mode == Server && !IsConnected() )
			{
				printf( "server accepts connection from client %d.%d.%d.%d:%d\n", 
					sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), sender.GetPort() );
				state = Connected;
				address = sender;
				OnConnect();
			}
			if ( sender == address )
			{
				if ( mode == Client && state == Connecting )
				{
					printf( "client completes connection with server\n" );
					state = Connected;
					OnConnect();
				}
				timeoutAccumulator = 0.0f;
				memcpy( data, &packet[4], bytes_read - 4 );
				return bytes_read - 4;
			}
			return 0;
		}
		
		int GetHeaderSize() const
		{
			return 4;
		}
		
	protected:
		
		virtual void OnStart()		{}
		virtual void OnStop()		{}
		virtual void OnConnect()    {}
		virtual void OnDisconnect() {}
			
	private:
		
		void ClearData()
		{
			state = Disconnected;
			timeoutAccumulator = 0.0f;
			address = Address();
		}
	
		enum State
		{
			Disconnected,
			Listening,
			Connecting,
			ConnectFail,
			Connected
		};

		unsigned int protocolId;
		float timeout;
		
		bool running;
		Mode mode;
		State state;
		Socket socket;
		float timeoutAccumulator;
		Address address;
	};
	
	// connection with reliability (seq/ack)
	//  + just for demonstrating the reliability system

	class ReliableConnection : public Connection
	{
	public:
		
		ReliableConnection( unsigned int protocolId, float timeout, unsigned int max_sequence = 0xFFFFFFFF )
			: Connection( protocolId, timeout ), reliabilitySystem( max_sequence )
		{
			ClearData();
			#ifdef NET_UNIT_TEST
			packet_loss_mask = 0;
			#endif
		}
	
		~ReliableConnection()
		{
			if ( IsRunning() )
				Stop();
		}
		
		// overriden functions from "Connection"
				
		bool SendPacket( const unsigned char data[], int size )
		{
			#ifdef NET_UNIT_TEST
			if ( reliabilitySystem.GetLocalSequence() & packet_loss_mask )
			{
				reliabilitySystem.PacketSent( size );
				return true;
			}
			#endif
			const int header = 12;
			unsigned char packet[header+size];
			unsigned int seq = reliabilitySystem.GetLocalSequence();
			unsigned int ack = reliabilitySystem.GetRemoteSequence();
			unsigned int ack_bits = reliabilitySystem.GenerateAckBits();
			WriteHeader( packet, seq, ack, ack_bits );
			memcpy( packet + header, data, size );
 			if ( !Connection::SendPacket( packet, size + header ) )
				return false;
			reliabilitySystem.PacketSent( size );
			return true;
		}	
		
		int ReceivePacket( unsigned char data[], int size )
		{
			const int header = 12;
			if ( size <= header )
				return false;
			unsigned char packet[header+size];
			int received_bytes = Connection::ReceivePacket( packet, size + header );
			if ( received_bytes == 0 )
				return false;
			if ( received_bytes <= header )
				return false;
			unsigned int packet_sequence = 0;
			unsigned int packet_ack = 0;
			unsigned int packet_ack_bits = 0;
			ReadHeader( packet, packet_sequence, packet_ack, packet_ack_bits );
			reliabilitySystem.PacketReceived( packet_sequence, received_bytes - header );
			reliabilitySystem.ProcessAck( packet_ack, packet_ack_bits );
			memcpy( data, packet + header, received_bytes - header );
			return received_bytes - header;
		}
		
		void Update( float deltaTime )
		{
			Connection::Update( deltaTime );
			reliabilitySystem.Update( deltaTime );
		}
		
		int GetHeaderSize() const
		{
			return Connection::GetHeaderSize() + reliabilitySystem.GetHeaderSize();
		}
		
		ReliabilitySystem & GetReliabilitySystem()
		{
			return reliabilitySystem;
		}

		// unit test controls
		
		#ifdef NET_UNIT_TEST
		void SetPacketLossMask( unsigned int mask )
		{
			packet_loss_mask = mask;
		}
		#endif
		
	protected:		
		
		void WriteHeader( unsigned char * header, unsigned int sequence, unsigned int ack, unsigned int ack_bits )
		{
			WriteInteger( header, sequence );
			WriteInteger( header + 4, ack );
			WriteInteger( header + 8, ack_bits );
		}

		void ReadHeader( const unsigned char * header, unsigned int & sequence, unsigned int & ack, unsigned int & ack_bits )
		{
			ReadInteger( header, sequence );
			ReadInteger( header + 4, ack );
			ReadInteger( header + 8, ack_bits );
		}

		virtual void OnStop()
		{
			ClearData();
		}
		
		virtual void OnDisconnect()
		{
			ClearData();
		}
		
	private:

		void ClearData()
		{
			reliabilitySystem.Reset();
		}

		#ifdef NET_UNIT_TEST
		unsigned int packet_loss_mask;			// mask sequence number, if non-zero, drop packet - for unit test only
		#endif
		
		ReliabilitySystem reliabilitySystem;	// reliability system: manages sequence numbers and acks, tracks network stats etc.
	};

	// node mesh
	//  + manages node join and leave
	//  + updates each node with set of currently joined nodes
	
	class Mesh
	{
		struct NodeState
		{
			enum Mode { Disconnected, ConnectionAccept, Connected };
			Mode mode;
			float timeoutAccumulator;
			Address address;
			int nodeId;
			NodeState()
			{
				mode = Disconnected;
				address = Address();
				nodeId = -1;
				timeoutAccumulator = 0.0f;
			}
		};
		
		unsigned int protocolId;
		float sendRate;
		float timeout;

		Socket socket;
		std::vector<NodeState> nodes;
		typedef std::map<int,NodeState*> IdToNode;
		typedef std::map<Address,NodeState*> AddrToNode;
		IdToNode id2node;
		AddrToNode addr2node;
		bool running;
		float sendAccumulator;
				
	public:

		Mesh( unsigned int protocolId, int maxNodes = 255, float sendRate = 0.25f, float timeout = 10.0f )
		{
			assert( maxNodes >= 1 );
			assert( maxNodes <= 255 );
			this->protocolId = protocolId;
			this->sendRate = sendRate;
			this->timeout = timeout;
			nodes.resize( maxNodes );
			running = false;
			sendAccumulator = 0.0f;
		}
		
		~Mesh()
		{
			if ( running )
				Stop();
		}
		
		bool Start( int port )
		{
			assert( !running );
			printf( "start mesh on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			return true;
		}
		
		void Stop()
		{
			assert( running );
			printf( "stop mesh\n" );
			socket.Close();
			id2node.clear();
			addr2node.clear();
			for ( unsigned int i = 0; i < nodes.size(); ++i )
				nodes[i] = NodeState();
			running = false;
			sendAccumulator = 0.0f;
		}	
		
		void Update( float deltaTime )
		{
			assert( running );
			ReceivePackets();
			SendPackets( deltaTime );
			CheckForTimeouts( deltaTime );
		}
		
	    bool IsNodeConnected( int nodeId )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			return nodes[nodeId].mode == NodeState::Connected;
		}
		
		Address GetNodeAddress( int nodeId )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			return nodes[nodeId].address;
		}

	    int GetMaxNodes() const
		{
			assert( nodes.size() <= 255 );
			return (int) nodes.size();
		}
		
		void Reserve( int nodeId, const Address & address )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			printf( "mesh reserves node id %d for %d.%d.%d.%d:%d\n", 
				nodeId, address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort() );
			nodes[nodeId].mode = NodeState::ConnectionAccept;
			nodes[nodeId].nodeId = nodeId;
			nodes[nodeId].address = address;
			addr2node.insert( std::make_pair( address, &nodes[nodeId] ) );
		}
		
	protected:
		
		void ReceivePackets()
		{
			while ( true )
			{
				Address sender;
				unsigned char data[256];
				int size = socket.Receive( sender, data, sizeof(data) );
				if ( !size )
					break;
				ProcessPacket( sender, data, size );
			}
		}

		void ProcessPacket( const Address & sender, unsigned char data[], int size )
		{
			assert( sender != Address() );
			assert( size > 0 );
			assert( data );
			// ignore packets that dont have the correct protocol id
			unsigned int firstIntegerInPacket = ( unsigned(data[0]) << 24 ) | ( unsigned(data[1]) << 16 ) |
			                                    ( unsigned(data[2]) << 8 )  | unsigned(data[3]);
			if ( firstIntegerInPacket != protocolId )
				return;
			// determine packet type
			enum PacketType { JoinRequest, KeepAlive };
			PacketType packetType;
			if ( data[4] == 0 )
				packetType = JoinRequest;
			else if ( data[4] == 1 )
				packetType = KeepAlive;
			else
				return;
			// process packet type
			switch ( packetType )
			{
				case JoinRequest:
				{
					// is address already joining or joined?
					AddrToNode::iterator itor = addr2node.find( sender );
					if ( itor == addr2node.end() )
					{
						// no entry for address, start join process...
						int freeSlot = -1;
						for ( unsigned int i = 0; i < nodes.size(); ++i )
						{
							if ( nodes[i].mode == NodeState::Disconnected )
							{
								freeSlot = (int) i;
								break;
							}
						}
						if ( freeSlot >= 0 )
						{
							printf( "mesh accepts %d.%d.%d.%d:%d as node %d\n", 
								sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), sender.GetPort(), freeSlot );
							assert( nodes[freeSlot].mode == NodeState::Disconnected );
							nodes[freeSlot].mode = NodeState::ConnectionAccept;
							nodes[freeSlot].nodeId = freeSlot;
							nodes[freeSlot].address = sender;
							addr2node.insert( std::make_pair( sender, &nodes[freeSlot] ) );
						}
					}
					else if ( itor->second->mode == NodeState::ConnectionAccept )
					{
						// reset timeout accumulator, but only while joining
						itor->second->timeoutAccumulator = 0.0f;
					}
				}
				break;
				case KeepAlive:
				{
					AddrToNode::iterator itor = addr2node.find( sender );
					if ( itor != addr2node.end() )
					{
						// progress from "connection accept" to "connected"
						if ( itor->second->mode == NodeState::ConnectionAccept )
						{
							itor->second->mode = NodeState::Connected;
							printf( "mesh completes join of node %d\n", itor->second->nodeId );
						}
						// reset timeout accumulator for node
						itor->second->timeoutAccumulator = 0.0f;
					}
				}
				break;
			}
		}
		
		void SendPackets( float deltaTime )
		{
			sendAccumulator += deltaTime;
			while ( sendAccumulator > sendRate )
			{
				for ( unsigned int i = 0; i < nodes.size(); ++i )
				{
					if ( nodes[i].mode == NodeState::ConnectionAccept )
					{
						// node is negotiating join: send "connection accepted" packets
						unsigned char packet[7];
						packet[0] = (unsigned char) ( ( protocolId >> 24 ) & 0xFF );
						packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
						packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
						packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
						packet[4] = 0;
						packet[5] = (unsigned char) i;
						packet[6] = (unsigned char) nodes.size();
						socket.Send( nodes[i].address, packet, sizeof(packet) );
					}
					else if ( nodes[i].mode == NodeState::Connected )
					{
						// node is connected: send "update" packets
						unsigned char packet[5+6*nodes.size()];
						packet[0] = (unsigned char) ( ( protocolId >> 24 ) & 0xFF );
						packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
						packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
						packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
						packet[4] = 1;
						unsigned char * ptr = &packet[5];
						for ( unsigned int j = 0; j < nodes.size(); ++j )
						{
							ptr[0] = (unsigned char) nodes[j].address.GetA();
							ptr[1] = (unsigned char) nodes[j].address.GetB();
							ptr[2] = (unsigned char) nodes[j].address.GetC();
							ptr[3] = (unsigned char) nodes[j].address.GetD();
							ptr[4] = (unsigned char) ( ( nodes[j].address.GetPort() >> 8 ) & 0xFF );
							ptr[5] = (unsigned char) ( ( nodes[j].address.GetPort() ) & 0xFF );
							ptr += 6;
						}
						socket.Send( nodes[i].address, packet, sizeof(packet) );
					}
				}
				sendAccumulator -= sendRate;
			}
		}
		
		void CheckForTimeouts( float deltaTime )
		{
			for ( unsigned int i = 0; i < nodes.size(); ++i )
			{
				if ( nodes[i].mode != NodeState::Disconnected )
				{
					nodes[i].timeoutAccumulator += deltaTime;
					if ( nodes[i].timeoutAccumulator > timeout )
					{
						printf( "mesh timed out node %d\n", i );
						AddrToNode::iterator addr_itor = addr2node.find( nodes[i].address );
						assert( addr_itor != addr2node.end() );
						addr2node.erase( addr_itor );
						nodes[i] = NodeState();
					}
				}
			}
		}
	};

	// node
	
	class Node
	{
		struct NodeState
		{
			bool connected;
			Address address;
			NodeState()
			{
				connected = false;
				address = Address();
			}
		};
		
		struct BufferedPacket
		{
			int nodeId;
			std::vector<unsigned char> data;
		};
		
		typedef std::stack<BufferedPacket*> PacketBuffer;
		PacketBuffer receivedPackets;

		unsigned int protocolId;
		float sendRate;
		float timeout;
		int maxPacketSize;

		Socket socket;
		std::vector<NodeState> nodes;
		typedef std::map<int,NodeState*> IdToNode;
		typedef std::map<Address,NodeState*> AddrToNode;
		AddrToNode addr2node;
		bool running;
		float sendAccumulator;
		float timeoutAccumulator;
		enum State
		{
			Disconnected,
			Joining,
			Joined,
			JoinFail
		};
		State state;
		Address meshAddress;
		int localNodeId;

	public:

		Node( unsigned int protocolId, float sendRate = 0.25f, float timeout = 10.0f, int maxPacketSize = 1024 )
		{
			this->protocolId = protocolId;
			this->sendRate = sendRate;
			this->timeout = timeout;
			this->maxPacketSize = maxPacketSize;
			state = Disconnected;
			running = false;
			ClearData();
		}

		~Node()
		{
			if ( running )
				Stop();
		}

		bool Start( int port )
		{
			assert( !running );
			printf( "start node on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			return true;
		}

		void Stop()
		{
			assert( running );
			printf( "stop node\n" );
			ClearData();
			socket.Close();
			running = false;
		}	
		
		void Join( const Address & address )
		{
			printf( "node join %d.%d.%d.%d:%d\n", 
				address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort() );
			ClearData();
			state = Joining;
			meshAddress = address;
		}
		
		bool IsJoining() const
		{
			return state == Joining;
		}
		
		bool JoinFailed() const
		{
			return state == JoinFail;
		}
		
		bool IsConnected() const
		{
			return state == Joined;
		}
		
		int GetLocalNodeId() const
		{
			return localNodeId;
		}

		void Update( float deltaTime )
		{
			assert( running );
			ReceivePackets();
			SendPackets( deltaTime );
			CheckForTimeout( deltaTime );
		}

	    bool IsNodeConnected( int nodeId )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			return nodes[nodeId].connected;
		}

		Address GetNodeAddress( int nodeId )
		{
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			return nodes[nodeId].address;
		}

	    int GetMaxNodes() const
		{
			assert( nodes.size() <= 255 );
			return (int) nodes.size();
		}
		
		bool SendPacket( int nodeId, const unsigned char data[], int size )
		{
			assert( running );
			if ( nodes.size() == 0 )
				return false;	// not connected yet
			assert( nodeId >= 0 );
			assert( nodeId < (int) nodes.size() );
			if ( nodeId < 0 || nodeId >= (int) nodes.size() )
				return false;
			if ( !nodes[nodeId].connected )
				return false;
			assert( size <= maxPacketSize );
			if ( size > maxPacketSize )
				return false;
			return socket.Send( nodes[nodeId].address, data, size );
		}
		
		int ReceivePacket( int & nodeId, unsigned char data[], int size )
		{
			assert( running );
			if ( !receivedPackets.empty() )
			{
				BufferedPacket * packet = receivedPackets.top();
				assert( packet );
				if ( (int) packet->data.size() <= size )
				{
					nodeId = packet->nodeId;
					memcpy( data, &packet->data[0], packet->data.size() );
					delete packet;
					receivedPackets.pop();
					return packet->data.size();
				}
				else
				{
					delete packet;
					receivedPackets.pop();
				}
			}
			return 0;
		}

	protected:

		void ReceivePackets()
		{
			while ( true )
			{
				Address sender;
				unsigned char data[maxPacketSize];
				int size = socket.Receive( sender, data, sizeof(data) );
				if ( !size )
					break;
				ProcessPacket( sender, data, size );
			}
		}

		void ProcessPacket( const Address & sender, unsigned char data[], int size )
		{
			assert( sender != Address() );
			assert( size > 0 );
			assert( data );
			// is packet from the mesh?
			if ( sender == meshAddress )
			{
				// *** packet sent from the mesh ***
				// ignore packets that dont have the correct protocol id
				unsigned int firstIntegerInPacket = ( unsigned(data[0]) << 24 ) | ( unsigned(data[1]) << 16 ) |
				                                    ( unsigned(data[2]) << 8 )  | unsigned(data[3]);
				if ( firstIntegerInPacket != protocolId )
					return;
				// determine packet type
				enum PacketType { ConnectionAccepted, Update };
				PacketType packetType;
				if ( data[4] == 0 )
					packetType = ConnectionAccepted;
				else if ( data[4] == 1 )
					packetType = Update;
				else
					return;
				// handle packet type
				switch ( packetType )
				{
					case ConnectionAccepted:
					{
						if ( size != 7 )
							return;
						if ( state == Joining )
						{
							localNodeId = data[5];
							nodes.resize( data[6] );
							printf( "node accepts join as node %d of %d\n", localNodeId, (int) nodes.size() );
							state = Joined;
						}
						timeoutAccumulator = 0.0f;
					}
					break;
					case Update:
					{
						if ( size != (int) ( 5 + nodes.size() * 6 ) )
							return;
						if ( state == Joined )
						{
							// process update packet
							unsigned char * ptr = &data[5];
							for ( unsigned int i = 0; i < nodes.size(); ++i )
							{
								unsigned char a = ptr[0];
								unsigned char b = ptr[1];
								unsigned char c = ptr[2];
								unsigned char d = ptr[3];
								unsigned short port = (unsigned short)ptr[4] << 8 | (unsigned short)ptr[5];
								Address address( a, b, c, d, port );
								if ( address.GetAddress() != 0 )
								{
									// node is connected
									if ( address != nodes[i].address )
									{
										printf( "node %d: node %d connected\n", localNodeId, i );
										nodes[i].connected = true;
										nodes[i].address = address;
										addr2node[address] = &nodes[i];
									}
								}
								else
								{
									// node is not connected
									if ( nodes[i].connected )
									{
										printf( "node %d: node %d disconnected\n", localNodeId, i );
										AddrToNode::iterator itor = addr2node.find( nodes[i].address );
										assert( itor != addr2node.end() );
										addr2node.erase( itor );
										nodes[i].connected = false;
										nodes[i].address = Address();
									}
								}
								ptr += 6;
							}
						}
						timeoutAccumulator = 0.0f;
					}
					break;
				}
			}
			else
			{
				AddrToNode::iterator itor = addr2node.find( sender );
				if ( itor != addr2node.end() )
				{
					// *** packet sent from another node ***
					NodeState * node = itor->second;
					assert( node );
					int nodeId = (int) ( node - &nodes[0] );
					assert( nodeId >= 0 );
					assert( nodeId < (int) nodes.size() );
					BufferedPacket * packet = new BufferedPacket;
					packet->nodeId = nodeId;
					packet->data.resize( size );
					memcpy( &packet->data[0], data, size );
					receivedPackets.push( packet );
				}
			}
		}

		void SendPackets( float deltaTime )
		{
			sendAccumulator += deltaTime;
			while ( sendAccumulator > sendRate )
			{
				if ( state == Joining )
				{
					// node is joining: send "join request" packets
					unsigned char packet[5];
					packet[0] = (unsigned char) ( ( protocolId >> 24 ) & 0xFF );
					packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
					packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
					packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
					packet[4] = 0;
					socket.Send( meshAddress, packet, sizeof(packet) );
				}
				else if ( state == Joined )
				{
					// node is joined: send "keep alive" packets
					unsigned char packet[5];
					packet[0] = (unsigned char) ( ( protocolId >> 24 ) & 0xFF );
					packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
					packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
					packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
					packet[4] = 1;
					socket.Send( meshAddress, packet, sizeof(packet) );
				}
				sendAccumulator -= sendRate;
			}
		}

		void CheckForTimeout( float deltaTime )
		{
			if ( state == Joining || state == Joined )
			{
				timeoutAccumulator += deltaTime;
				if ( timeoutAccumulator > timeout )
				{
					if ( state == Joining )
					{
						printf( "node join failed\n" );
						state = JoinFail;
					}
					else
					{
						printf( "node timed out\n" );
						state = Disconnected;
					}
					ClearData();
				}
			}
		}
		
		void ClearData()
		{
			nodes.clear();
			addr2node.clear();
			while ( !receivedPackets.empty() )
			{
				BufferedPacket * packet = receivedPackets.top();
				delete packet;
				receivedPackets.pop();
			}
			sendAccumulator = 0.0f;
			timeoutAccumulator = 0.0f;
			localNodeId = -1;
			meshAddress = Address();
		}
	};
	
	// beacon
	//  + sends broadcast UDP packets to the LAN
	//  + use a beacon to advertise the existence of a server on the LAN
	
	class Beacon
	{
	public:
		
		Beacon( const char name[], unsigned int protocolId, unsigned int listenerPort, unsigned int serverPort )
			: socket( Socket::Broadcast | Socket::NonBlocking )
		{
			strncpy( this->name, name, 64 );
			this->name[64] = '\0';
			this->protocolId = protocolId;
			this->listenerPort = listenerPort;
			this->serverPort = serverPort;
			running = false;
		}
		
		~Beacon()
		{
			if ( running )
				Stop();
		}
		
		bool Start( int port )
		{
			assert( !running );
			printf( "start beacon on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			return true;
		}
		
		void Stop()
		{
			assert( running );
			printf( "stop beacon\n" );
			socket.Close();
			running = false;
		}
		
		void Update( float deltaTime )
		{
			assert( running );
			unsigned char packet[12+1+64];
			WriteInteger( packet, 0 );
			WriteInteger( packet + 4, protocolId );
			WriteInteger( packet + 8, serverPort );
			packet[12] = (unsigned char) strlen( name );
			assert( packet[12] < 63);
			memcpy( packet + 13, name, strlen( name ) );
			if ( !socket.Send( Address(255,255,255,255,listenerPort), packet, 12 + 1 + packet[12] ) )
				printf( "failed to send broadcast packet\n" );
			Address sender;
			while ( socket.Receive( sender, packet, 256 ) );
		}
		
	private:
		
		char name[64+1];
		unsigned int protocolId;
		unsigned int listenerPort;
		unsigned int serverPort;
		bool running;
		Socket socket;
	};
	
	// listener entry
	//  + an entry in the list of servers on the LAN
	
	struct ListenerEntry
	{
		char name[64+1];
		Address address;
		float timeoutAccumulator;
	};

	// listener
	//  + listens for broadcast packets sent over the LAN
	//  + use a listener to get a list of all the servers on the LAN
	
	class Listener
	{
	public:
		
		Listener( unsigned int protocolId, float timeout = 10.0f )
		{
			this->protocolId = protocolId;
			this->timeout = timeout;
			running = false;
			ClearData();
		}
		
		~Listener()
		{
			if ( running )
				Stop();
		}
		
		bool Start( int port )
		{
			assert( !running );
			printf( "start listener on port %d\n", port );
			if ( !socket.Open( port ) )
				return false;
			running = true;
			return true;
		}
		
		void Stop()
		{
			assert( running );
			printf( "stop listener\n" );
			socket.Close();
			running = false;
			ClearData();
		}
		
		void Update( float deltaTime )
		{
			assert( running );
			unsigned char packet[256];
			while ( true )
			{
				Address sender;
				int bytes_read = socket.Receive( sender, packet, 256 );
				if ( bytes_read == 0 )
					break;
				if ( bytes_read < 13 )
					continue;
				unsigned int packet_zero;
				unsigned int packet_protocolId;
				unsigned int packet_serverPort;
				unsigned char packet_stringLength;
				ReadInteger( packet, packet_zero );
				ReadInteger( packet + 4, packet_protocolId );
				ReadInteger( packet + 8, packet_serverPort );
				packet_stringLength = packet[12];
				if ( packet_zero != 0 )
					continue;
				if ( packet_protocolId != protocolId )
					continue;
				if ( packet_stringLength > 63 )
					continue;
				if ( packet_stringLength + 12 + 1 > bytes_read )
					continue;
				ListenerEntry entry;
				memcpy( entry.name, packet + 13, packet_stringLength );
				entry.name[packet_stringLength] = '\0';
				entry.address = Address( sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), packet_serverPort );
				entry.timeoutAccumulator = 0.0f;
				ListenerEntry * existingEntry = FindEntry( entry );
				if ( existingEntry )
					existingEntry->timeoutAccumulator = 0.0f;
				else
					entries.push_back( entry );
			}
			std::vector<ListenerEntry>::iterator itor = entries.begin();
			while ( itor != entries.end() )
			{
				itor->timeoutAccumulator += deltaTime;
				if ( itor->timeoutAccumulator > timeout )
					itor = entries.erase( itor );
				else
					++itor;
			}
		}
		
		int GetEntryCount() const
		{
			return entries.size();
		}
		
		const ListenerEntry & GetEntry( int index ) const
		{
			assert( index >= 0 );
			assert( index < (int) entries.size() );
			return entries[index];
		}
		
	protected:
		
		ListenerEntry * FindEntry( const ListenerEntry & entry )
		{
			for ( int i = 0; i < (int) entries.size(); ++i )
			{
				if ( entries[i].address == entry.address && strcmp( entries[i].name, entry.name ) == 0 )
					return &entries[i];
			}
			return NULL;
		}
		
	private:
		
		void ClearData()
		{
			entries.clear();
		}
		
		std::vector<ListenerEntry> entries;
		unsigned int protocolId;
		float timeout;
		bool running;
		Socket socket;
	};
	
	// lan transport implementation
	//  + servers are advertized via net beacon
	//  + lan lobby is filled via net listener
	//  + a mesh runs on the server IP and manages node connections
	//  + a node runs on each transport, for the server with the mesh a local node also runs
	
	class TransportLAN : public Transport
	{
		TransportLAN()
		{
			// ...
		}
		
		~TransportLAN()
		{
			// ...
		}
		
		bool IsNodeConnected( int nodeId )
		{
			return false;
		}
		
		int GetLocalNodeId() const
		{
			return 0;
		}
		
		int GetMaxNodes() const
		{
			return 0;			
		}

		bool SendPacket( int nodeId, const unsigned char data[], int size )
		{
			return false;
		}
		
		int ReceivePacket( int & nodeId, unsigned char data[], int size )
		{
			return 0;
		}

		class ReliabilitySystem & GetReliability( int nodeId )
		{
			static ReliabilitySystem reliabilitySystem;
			return reliabilitySystem;
		}

		void Update( float deltaTime )
		{
			// ...
		}
	};
}

#endif
