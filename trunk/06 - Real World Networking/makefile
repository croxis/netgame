# makefile for macosx

flags = -Wall -DDEBUG # -O3

% : %.cpp NetPlatform.h NetLAN.h NetReliability.h NetFlowControl.h NetTransport.h
	g++ $< -o $@ NetTransport.cpp ${flags}

all : Client Server Test

client : Client
	./Client
	
server : Server
	./Server
	
test : Test
	./Test
	
clean:
	rm -f Client Server Test
