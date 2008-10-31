# makefile for macosx

flags = -Wall -DDEBUG # -O3

net_headers := $(wildcard *.h)
lan_headers := $(wildcard lan/*.h)

NetTransport.o : makefile NetTransport.h NetTransport.cpp NetPlatform.h ${lan_headers}
	g++ NetTransport.cpp -c -o NetTransport.o ${flags}

% : %.cpp NetTransport.o #{net_headers}
	g++ $< -o $@ NetTransport.o ${flags}

all : Client Server Test

transport : NetTransport.o	
	
client : Client
	./Client
	
server : Server
	./Server
	
test : Test
	./Test
	
clean:
	rm -f Client Server Test *.o *.a
