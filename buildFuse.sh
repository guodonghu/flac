fusermount -u hello_mnt
gcc -Wall -std=gnu99 hello.c -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -pthread -lfuse  cJSON.c -lcurl -lm  -o hello
./hello hello_mnt
