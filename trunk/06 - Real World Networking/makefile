# makefile for macosx

flags = -Wall -DDEBUG # -O3

% : %.cpp NetPlatform.h NetReliability.h NetFlowControl.h NetTransport.h lan/NetAddress.h lan/NetSockets.h lan/NetBeacon.h lan/NetConnection.h lan/NetNodeMesh.h
	g++ $< -o $@ NetTransport.cpp lan/NetLAN.cpp ${flags}

all : Client Server Test

client : Client
	./Client
	
server : Server
	./Server
	
test : Test
	./Test
	
clean:
	rm -f Client Server Test
