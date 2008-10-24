# makefile for macosx

flags = -Wall -DDEBUG # -O3

% : %.cpp NetStream.h
	g++ $< -o $@ ${flags}

all : Client Server Test

client : Client
	./Client
	
server : Server
	./Server
	
test : Test
	./Test
	
clean:
	rm -f Client Server Test
