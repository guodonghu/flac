#ifndef FUSE_WRAPPER_HPP
#define FUSE_WRAPPER_HPP
#define FUSE_USE_VERSION 26
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include "cJSON.h"

extern cJSON* request_json;

struct _MemoryStruct {
  char *memory;
  size_t size;
};

typedef struct _MemoryStruct MemoryStruct;

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

MemoryStruct getMetadata(MemoryStruct data);

int flacjacket_getattr(const char *path, struct stat *stbuf);

int flacjacket_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int flacjacket_open(const char *path, struct fuse_file_info *fi);

int flacjacket_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int flacjacket_release(const char *path, struct fuse_file_info *fi);

#endif 
