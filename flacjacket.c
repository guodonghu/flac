#define FUSE_USE_VERSION 26
#include "fuse_wrap.hpp"
#include <fuse.h>
#include <stdio.h>


struct flacjacket_oper:fuse_operations {
  flacjacket_oper() {
    getattr	= flacjacket_getattr;
    readdir	= flacjacket_readdir;
    open = flacjacket_open;
    read = flacjacket_read;
  }
};

static struct flacjacket_oper flacjacket_oper;


int main(int argc, char *argv[]) {
	int exit_status = fuse_main(argc, argv, &flacjacket_oper, NULL);
  //if (request_json != NULL) {
  //  cJSON_Delete(request_json);
  //}
  return exit_status;
}
