##
# Project Title
#
# @file
# @version 0.1

all:

	g++ -c -fPIC src/*.cpp
	g++ -shared *.o -o libdream.so
	cp src/*.h include/

	sudo cp include/dream.h /usr/include/dream.h
	sudo cp libdream.so /usr/lib/libdream.so
	rm *.o
# end
