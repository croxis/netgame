# makefile for macosx

flags = -Wall -Iode -DDEBUG # -O3
libs = -Lode -lode -lruby
frameworks = -framework Carbon -framework OpenGL -framework AGL

% : %.cpp Display.h Platform.h Mathematics.h
	g++ $< -o $@ ${flags} ${libs} ${frameworks}

all : Authority

server : Authority
	./Authority

client : Authority
	./Authority 127.0.0.1
	
clean:
	rm -f Authority
	
