# makefile for macosx

flags = -Wall -DDEBUG # -O3

% : %.cpp NetPlatform.h NetLAN.h NetReliability.h NetTransport.h
	g++ $< -o $@ ${flags}

all : Server

server : Server
	./Server
	
clean:
	rm -f Server