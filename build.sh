fusermount -u flacjacket_mnt
g++ -Wall -Werror -std=c++11 flacjacket.c cJSON.c fuse_wrap.cpp  `pkg-config fuse --cflags --libs` -lcurl -lm  -o flacjacket
./flacjacket flacjacket_mnt
