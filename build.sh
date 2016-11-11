g++ -Wall -Werror  -std=c++11 -pthread flacjacket.c cJSON.c fuse_wrapper.cpp  `pkg-config fuse --cflags --libs` -lcurl -lm  -o flacjacket

