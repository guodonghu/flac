all: flacjacket

clean:
	rm  *~ *# *.o 

flacjacket: fuse_wrap.o flacjacket.o cJSON.o
	g++  -Wall -Werror -std=c++11  -o flacjacket fuse_wrap.o flacjacket.o cJSON.o `pkg-config fuse --cflags --libs` -lcurl -lm


flacjacket.o: fuse_wrap.hpp cJSON.h
	gcc  -Wall -Werror -std=gnu99  -c  flacjacket.c   -o flacjacket.o `pkg-config fuse --cflags --libs` -lcurl -lm

fuse_wrap.o: fuse_wrap.hpp cJSON.h
	g++  -Wall -Werror -std=c++11  -c  fuse_wrap.cpp  -o fuse_wrap.o `pkg-config fuse --cflags --libs` -lcurl -lm

cJSON.o: cJSON.h
	gcc -Wall -Werror -std=gnu99 -c cJSON.c -o cJSON.o
