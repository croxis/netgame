# makefile for macosx

flags = -Wall -DDEBUG # -O3

% : %.cpp Net.h
	g++ $< -o $@ ${flags}

all : FlowControl Acks Client Server Test

test : Test
	./Test
	
server : Server
	./Server

client : Client
	./Client
	
acks : Acks
	./Acks
	
flow : FlowControl
	./FlowControl

clean:
	rm -f FlowControl Acks Client Server Test
