/*
	Unit Tests for Networking Library
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "NetLAN.h"

using namespace std;
using namespace net;

#ifdef DEBUG
#define check assert
#else
#define check(n) if ( !n ) { printf( "check failed\n" ); exit(1); }
#endif

void test_connection_join()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test connection join\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	Connection client( ProtocolId, TimeOut );
	Connection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );
}

void test_connection_join_timeout()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test connection join timeout\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	Connection client( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	
	while ( true )
	{
		if ( !client.IsConnecting() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
	}
	
	check( !client.IsConnected() );
	check( client.ConnectFailed() );
}

void test_connection_join_busy()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test connection join busy\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	// connect client to server
	
	Connection client( ProtocolId, TimeOut );
	Connection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );

	// attempt another connection, verify connect fails (busy)
	
	Connection busy( ProtocolId, TimeOut );
	check( busy.Start( ClientPort + 1 ) );
	busy.Connect( Address(127,0,0,1,ServerPort ) );

	while ( true )
	{
		if ( !busy.IsConnecting() || busy.IsConnected() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		unsigned char busy_packet[] = "i'm so busy!";
		busy.SendPacket( busy_packet, sizeof( busy_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = busy.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		client.Update( DeltaTime );
		server.Update( DeltaTime );
		busy.Update( DeltaTime );
	}

	check( client.IsConnected() );
	check( server.IsConnected() );
	check( !busy.IsConnected() );
	check( busy.ConnectFailed() );
}

void test_connection_rejoin()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test connection rejoin\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	Connection client( ProtocolId, TimeOut );
	Connection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );

	// connect client and server
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );

	// let connection timeout

	while ( client.IsConnected() || server.IsConnected() )
	{
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		client.Update( DeltaTime );
		server.Update( DeltaTime );
	}
	
	check( !client.IsConnected() );
	check( !server.IsConnected() );

	// reconnect client

	client.Connect( Address(127,0,0,1,ServerPort ) );
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );
}

void test_connection_payload()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test connection payload\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int ServerPort = 30000;
	const int ClientPort = 30001;
	const int ProtocolId = 0x11112222;
	const float DeltaTime = 0.001f;
	const float TimeOut = 0.1f;
	
	Connection client( ProtocolId, TimeOut );
	Connection server( ProtocolId, TimeOut );
	
	check( client.Start( ClientPort ) );
	check( server.Start( ServerPort ) );
	
	client.Connect( Address(127,0,0,1,ServerPort ) );
	server.Listen();
	
	while ( true )
	{
		if ( client.IsConnected() && server.IsConnected() )
			break;
			
		if ( !client.IsConnecting() && client.ConnectFailed() )
			break;
		
		unsigned char client_packet[] = "client to server";
		client.SendPacket( client_packet, sizeof( client_packet ) );

		unsigned char server_packet[] = "server to client";
		server.SendPacket( server_packet, sizeof( server_packet ) );
		
		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
			check( strcmp( (const char*) packet, "server to client" ) == 0 );
		}

		while ( true )
		{
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
			check( strcmp( (const char*) packet, "client to server" ) == 0 );
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );
	}
	
	check( client.IsConnected() );
	check( server.IsConnected() );
}

// --------------------------------------------------------

void test_node_join()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test node join\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int MaxNodes = 2;
	const int MeshPort = 30000;
	const int NodePort = 30001;
	const int ProtocolId = 0x12345678;
	const float DeltaTime = 0.01f;
	const float SendRate = 0.01f;
	const float TimeOut = 1.0f;
	
	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	check( mesh.Start( MeshPort ) );
	
	Node node( ProtocolId, SendRate, TimeOut );
	check( node.Start( NodePort ) );
	
	node.Join( Address(127,0,0,1,MeshPort) );
	while ( node.IsJoining() )
	{
		node.Update( DeltaTime );
		mesh.Update( DeltaTime );
	}
	
	check( !node.JoinFailed() );

	mesh.Stop();
}

void test_node_join_fail()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test node join fail\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int MeshPort = 30000;
	const int NodePort = 30001;
	const int ProtocolId = 0x12345678;
	const float DeltaTime = 0.01f;
	const float SendRate = 0.001f;
	const float TimeOut = 0.1f;
	
	Node node( ProtocolId, SendRate, TimeOut );
	check( node.Start( NodePort ) );
	
	node.Join( Address(127,0,0,1,MeshPort) );
	while ( node.IsJoining() )
	{
		node.Update( DeltaTime );
	}
	
	check( node.JoinFailed() );
}

void test_node_join_busy()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test node join busy\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int MaxNodes = 1;
	const int MeshPort = 30000;
	const int NodePort = 30001;
	const int ProtocolId = 0x12345678;
	const float DeltaTime = 0.001f;
	const float SendRate = 0.001f;
	const float TimeOut = 0.1f;
	
	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	check( mesh.Start( MeshPort ) );
	
	Node node( ProtocolId, SendRate, TimeOut );
	check( node.Start( NodePort ) );
	
	node.Join( Address(127,0,0,1,MeshPort) );
	while ( node.IsJoining() )
	{
		node.Update( DeltaTime );
		mesh.Update( DeltaTime );
	}
	
	check( !node.JoinFailed() );

	Node busy( ProtocolId, SendRate, TimeOut );
	check( busy.Start( NodePort + 1 ) );
	
	busy.Join( Address(127,0,0,1,MeshPort) );
	while ( busy.IsJoining() )
	{
		node.Update( DeltaTime );
		busy.Update( DeltaTime );
		mesh.Update( DeltaTime );
	}
	
	check( busy.JoinFailed() );
	check( node.IsConnected() );
	check( mesh.IsNodeConnected( 0 ) );

	mesh.Stop();
}

void test_node_join_multi()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test node join multi\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int MaxNodes = 4;
	const int MeshPort = 30000;
	const int NodePort = 30001;
	const int ProtocolId = 0x12345678;
	const float DeltaTime = 0.01f;
	const float SendRate = 0.01f;
	const float TimeOut = 1.0f;
	
	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	check( mesh.Start( MeshPort ) );
	
	Node * node[MaxNodes];
	for ( int i = 0; i < MaxNodes; ++i )
	{
		node[i] = new Node( ProtocolId, SendRate, TimeOut );
		check( node[i]->Start( NodePort + i ) );
	}
	
	for ( int i = 0; i < MaxNodes; ++i )
		node[i]->Join( Address(127,0,0,1,MeshPort) );		
	
	while ( true )
	{
		bool joining = false;
		for ( int i = 0; i < MaxNodes; ++i )
		{
			node[i]->Update( DeltaTime );	
			if ( node[i]->IsJoining() )
				joining = true;
		}		
		if ( !joining )
			break;
		mesh.Update( DeltaTime );
	}

	for ( int i = 0; i < MaxNodes; ++i )
	{
		check( !node[i]->IsJoining() );
		check( !node[i]->JoinFailed() );
		delete node[i];
	}

	mesh.Stop();
}

void test_node_rejoin()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test node rejoin\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int MaxNodes = 2;
	const int MeshPort = 30000;
	const int NodePort = 30001;
	const int ProtocolId = 0x12345678;
	const float DeltaTime = 0.001f;
	const float SendRate = 0.001f;
	const float TimeOut = 0.1f;
	
	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	check( mesh.Start( MeshPort ) );
	
	Node node( ProtocolId, SendRate, TimeOut * 3 );
	check( node.Start( NodePort ) );
	node.Join( Address(127,0,0,1,MeshPort) );
	while ( node.IsJoining() )
	{
		node.Update( DeltaTime );
		mesh.Update( DeltaTime );
	}
	
	node.Stop();
	check( node.Start( NodePort ) );
	node.Join( Address(127,0,0,1,MeshPort) );
	while ( node.IsJoining() )
	{
		node.Update( DeltaTime );
		mesh.Update( DeltaTime );
	}
		
	check( !node.JoinFailed() );

	mesh.Stop();
}

void test_node_timeout()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test node timeout\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int MaxNodes = 2;
	const int MeshPort = 30000;
	const int NodePort = 30001;
	const int ProtocolId = 0x12345678;
	const float DeltaTime = 0.001f;
	const float SendRate = 0.001f;
	const float TimeOut = 0.1f;
	
	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	check( mesh.Start( MeshPort ) );
	
	Node node( ProtocolId, SendRate, TimeOut );
	check( node.Start( NodePort ) );
	
	node.Join( Address(127,0,0,1,MeshPort) );
	while ( node.IsJoining() || !mesh.IsNodeConnected( 0 ) )
	{
		node.Update( DeltaTime );
		mesh.Update( DeltaTime );
	}
	
	check( !node.JoinFailed() );

	int localNodeId = node.GetLocalNodeId();
	int maxNodes = node.GetMaxNodes();

	check( localNodeId == 0 );
	check( maxNodes == MaxNodes );
	
	check( mesh.IsNodeConnected( localNodeId ) );
	
	while ( mesh.IsNodeConnected( localNodeId ) )
	{
		mesh.Update( DeltaTime );
	}
	
	check( !mesh.IsNodeConnected( localNodeId ) );
	
	while ( node.IsConnected() )
	{
		node.Update( DeltaTime );
	}
	
	check( !node.IsConnected() );
	check( node.GetLocalNodeId() == -1 );

	mesh.Stop();
}

void test_node_payload()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test node payload\n" );
	printf( "-----------------------------------------------------\n" );

	const int MaxNodes = 2;
	const int MeshPort = 30000;
	const int ClientPort = 30001;
	const int ServerPort = 30002;
	const int ProtocolId = 0x12345678;
	const float DeltaTime = 0.01f;
	const float SendRate = 0.01f;
	const float TimeOut = 1.0f;

	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	check( mesh.Start( MeshPort ) );

	Node client( ProtocolId, SendRate, TimeOut );
	check( client.Start( ClientPort ) );

	Node server( ProtocolId, SendRate, TimeOut );
	check( server.Start( ServerPort ) );
	
	mesh.Reserve( 0, Address(127,0,0,1,ServerPort) );

	server.Join( Address(127,0,0,1,MeshPort) );
	client.Join( Address(127,0,0,1,MeshPort) );
	
	bool serverReceivedPacketFromClient = false;
	bool clientReceivedPacketFromServer = false;
	
	while ( !serverReceivedPacketFromClient || !clientReceivedPacketFromServer )
	{
		if ( client.IsConnected() )
		{
			unsigned char packet[] = "client to server";
			client.SendPacket( 0, packet, sizeof(packet) );
		}

		if ( server.IsConnected() )
		{
			unsigned char packet[] = "server to client";
			server.SendPacket( 1, packet, sizeof(packet) );
		}
		
		while ( true )
		{
			int nodeId = -1;
			unsigned char packet[256];
			int bytes_read = client.ReceivePacket( nodeId, packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
			if ( nodeId == 0 && strcmp( (const char*) packet, "server to client" ) == 0 )
				clientReceivedPacketFromServer = true;
		}

		while ( true )
		{
			int nodeId = -1;
			unsigned char packet[256];
			int bytes_read = server.ReceivePacket( nodeId, packet, sizeof(packet) );
			if ( bytes_read == 0 )
				break;
			if ( nodeId == 1 && strcmp( (const char*) packet, "client to server" ) == 0 )
				serverReceivedPacketFromClient = true;
		}
		
		client.Update( DeltaTime );
		server.Update( DeltaTime );

		mesh.Update( DeltaTime );
	}

	check( client.IsConnected() );
	check( server.IsConnected() );

	mesh.Stop();
}

void test_mesh_restart()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test mesh restart\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int MaxNodes = 2;
	const int MeshPort = 30000;
	const int NodePort = 30001;
	const int ProtocolId = 0x12345678;
	const float DeltaTime = 0.001f;
	const float SendRate = 0.001f;
	const float TimeOut = 0.1f;
	
	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	check( mesh.Start( MeshPort ) );
	
	Node node( ProtocolId, SendRate, TimeOut );
	check( node.Start( NodePort ) );
	
	node.Join( Address(127,0,0,1,MeshPort) );
	while ( node.IsJoining() )
	{
		node.Update( DeltaTime );
		mesh.Update( DeltaTime );
	}
	
	check( !node.JoinFailed() );
	check( node.GetLocalNodeId() == 0 );

	mesh.Stop();

	while ( node.IsConnected() )
	{
		node.Update( DeltaTime );
	}

	check( mesh.Start( MeshPort ) );
		
	node.Join( Address(127,0,0,1,MeshPort) );
	while ( node.IsJoining() )
	{
		node.Update( DeltaTime );
		mesh.Update( DeltaTime );
	}
	
	check( !node.JoinFailed() );
	check( node.GetLocalNodeId() == 0 );
}

void test_mesh_nodes()
{
	printf( "-----------------------------------------------------\n" );
	printf( "test mesh nodes\n" );
	printf( "-----------------------------------------------------\n" );
	
	const int MaxNodes = 4;
	const int MeshPort = 30000;
	const int NodePort = 30001;
	const int ProtocolId = 0x12345678;
	const float DeltaTime = 0.01f;
	const float SendRate = 0.01f;
	const float TimeOut = 1.0f;
	
	Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
	check( mesh.Start( MeshPort ) );
	
	Node * node[MaxNodes];
	for ( int i = 0; i < MaxNodes; ++i )
	{
		node[i] = new Node( ProtocolId, SendRate, TimeOut );
		check( node[i]->Start( NodePort + i ) );
	}
	
	for ( int i = 0; i < MaxNodes; ++i )
		node[i]->Join( Address(127,0,0,1,MeshPort) );		
	
	while ( true )
	{
		bool joining = false;
		for ( int i = 0; i < MaxNodes; ++i )
		{
			node[i]->Update( DeltaTime );	
			if ( node[i]->IsJoining() )
				joining = true;
		}		
		if ( !joining )
			break;
		mesh.Update( DeltaTime );
	}

	for ( int i = 0; i < MaxNodes; ++i )
	{
		check( !node[i]->IsJoining() );
		check( !node[i]->JoinFailed() );
	}

	// wait for all nodes to get address info for other nodes

	while ( true )
	{
		bool allConnected = true;
		for ( int i = 0; i < MaxNodes; ++i )
		{
			node[i]->Update( DeltaTime );	
			for ( int j = 0; j < MaxNodes; ++j )
			if ( !node[i]->IsNodeConnected( j ) )
				allConnected = false;
		}		
		if ( allConnected )
			break;
		mesh.Update( DeltaTime );
	}

	// verify each node has correct addresses for all nodes
	
	for ( int i = 0; i < MaxNodes; ++i )
	{
		for ( int j = 0; j < MaxNodes; ++j )
		{
			check( mesh.IsNodeConnected(j) );
			check( node[i]->IsNodeConnected(j) );
			Address a = mesh.GetNodeAddress(j);
			Address b = node[i]->GetNodeAddress(j);
			check( mesh.GetNodeAddress(j) == node[i]->GetNodeAddress(j) );
		}
	}
	
	// disconnect first node, verify all other node see node disconnect
	
	node[0]->Stop();
	
	while ( true )
	{
		bool othersSeeFirstNodeDisconnected = true;
		for ( int i = 1; i < MaxNodes; ++i )
		{
			if ( node[i]->IsNodeConnected(0) )
				othersSeeFirstNodeDisconnected = false;
		}
		
		bool allOthersConnected = true;
		for ( int i = 1; i < MaxNodes; ++i )
		{
			node[i]->Update( DeltaTime );	
			for ( int j = 1; j < MaxNodes; ++j )
				if ( !node[i]->IsNodeConnected( j ) )
					allOthersConnected = false;
		}		
		
		if ( othersSeeFirstNodeDisconnected && allOthersConnected )
			break;
			
		mesh.Update( DeltaTime );
	}
	
	for ( int i = 1; i < MaxNodes; ++i )
		check( !node[i]->IsNodeConnected(0) );
	
	for ( int i = 1; i < MaxNodes; ++i )
	{
		for ( int j = 1; j < MaxNodes; ++j )
			check( node[i]->IsNodeConnected( j ) );
	}
	
	// reconnect node, verify all nodes are connected again

	check( node[0]->Start( NodePort ) );
	node[0]->Join( Address(127,0,0,1,MeshPort) );		
	
	while ( true )
	{
		bool joining = false;
		for ( int i = 0; i < MaxNodes; ++i )
		{
			node[i]->Update( DeltaTime );	
			if ( node[i]->IsJoining() )
				joining = true;
		}		
		if ( !joining )
			break;
		mesh.Update( DeltaTime );
	}

	for ( int i = 0; i < MaxNodes; ++i )
	{
		check( !node[i]->IsJoining() );
		check( !node[i]->JoinFailed() );
	}

	while ( true )
	{
		bool allConnected = true;
		for ( int i = 0; i < MaxNodes; ++i )
		{
			node[i]->Update( DeltaTime );	
			for ( int j = 0; j < MaxNodes; ++j )
			if ( !node[i]->IsNodeConnected( j ) )
				allConnected = false;
		}		
		if ( allConnected )
			break;
		mesh.Update( DeltaTime );
	}

	for ( int i = 0; i < MaxNodes; ++i )
	{
		for ( int j = 0; j < MaxNodes; ++j )
		{
			check( mesh.IsNodeConnected(j) );
			check( node[i]->IsNodeConnected(j) );
			Address a = mesh.GetNodeAddress(j);
			Address b = node[i]->GetNodeAddress(j);
			check( mesh.GetNodeAddress(j) == node[i]->GetNodeAddress(j) );
		}
	}

	// cleanup
	
	for ( int i = 0; i < MaxNodes; ++i )
		delete node[i];

	mesh.Stop();
}

void tests()
{
	test_connection_join();
	test_connection_join_timeout();
	test_connection_join_busy();
	test_connection_rejoin();
	test_connection_payload();
	
	test_node_join();
	test_node_join_fail();
	test_node_join_busy();
	test_node_join_multi();
	test_node_rejoin();
	test_node_timeout();
	test_node_payload();
	test_mesh_restart();
	test_mesh_nodes();

	printf( "-----------------------------------------------------\n" );
	printf( "passed!\n" );
}

int main( int argc, char * argv[] )
{
	if ( !InitializeSockets() )
	{
		printf( "failed to initialize sockets\n" );
		return 1;
	}
	
	tests();
	
	ShutdownSockets();

	return 0;
}
