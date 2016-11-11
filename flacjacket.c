#define FUSE_USE_VERSION 26
#include "fuse_wrapper.hpp"
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

char *bitRate = NULL;

struct flacjacket_oper:fuse_operations {
  flacjacket_oper() {
    getattr	= flacjacket_getattr;
    readdir	= flacjacket_readdir;
    open = flacjacket_open;
    read = flacjacket_read;
    release = flacjacket_release;
  }
};

static struct flacjacket_oper flacjacket_oper;


void usage(char * name) {
  printf("Usage: %s <mount point> [option]\n", name);
  printf("Mount the music file system on any empty directory you create, converting FLAC files to MP3 files upon access.\n");
  printf("Available Options:\n");
  printf("-b RATE,\n");
  printf("               encoding bitrate: Recommeneded values for RATE\n");
  printf("               include 96, 112, 128, 160, 192, 224, 256, and\n");
  printf("               320; 128 is the default\n");
                                                                        
}

bool isNumber(char *num) {
  for (int i = 0; num[i] != '\0'; i++) {
    if (isdigit(num[i]) == 0) {
      return false;
    }
  }
  return true;  
}

int main(int argc, char *argv[]) {
  if (argc != 2 && argc != 4) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  
  if (argc == 4) {
    if (strcmp(argv[2], "-b") != 0) {
      usage(argv[0]);
    }
    else {
      if (isNumber(argv[3])) {
        bitRate = argv[3];
        argv[2] = argv[3] = NULL;
        argc -= 2;
      }
      else {
        printf("RATE should be a valid number!\n");
        usage(argv[0]);
      } 
    }
  }
  
  
  int exit_status = fuse_main(argc, argv, &flacjacket_oper, NULL);
  if (request_json != NULL) {
    cJSON_Delete(request_json);
  }
  return exit_status;
}
