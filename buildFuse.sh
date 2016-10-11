fusermount -u flacjacket_mnt
gcc -Wall -Werror -std=gnu99 flacjacket.c cJSON.c  `pkg-config fuse --cflags --libs` -lcurl -lm  -o flacjacket
./flacjacket flacjacket_mnt
