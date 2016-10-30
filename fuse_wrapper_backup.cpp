#define FUSE_USE_VERSION 26
#include "fuse_wrapper.hpp"
#include <string>
#include <curl/curl.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <iostream>

cJSON* request_json = NULL;
std::unordered_map<std::string, std::unordered_set<std::string> > genreMap;
std::unordered_map<std::string, std::unordered_set<std::string> > artistMap;
std::unordered_set<std::string> musicSet;

void buildMap(std::unordered_map<std::string, std::unordered_set<std::string> > &map, cJSON *music, std::string type) {
  cJSON *title = cJSON_GetObjectItem(music, "title");
  cJSON *category = cJSON_GetObjectItem(music, type.c_str());
  std::string category_str(category->valuestring);
  category_str = "/" + category_str;
  std::string mp3(title->valuestring);
  mp3 += ".mp3";
  map[category_str].insert(mp3); 
}

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  MemoryStruct *mem = (MemoryStruct *)userp;
  mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  return realsize;
}

MemoryStruct getMetadata(MemoryStruct data) {
  CURLcode ret;
  CURL *hnd;
  struct curl_slist *slist1;
  slist1 = NULL;
  slist1 = curl_slist_append(slist1, "x-api-key:UJWr37dxHD4VTmEVZYPpa9U3hNJTr7wgas1Uhvsf");
  //curl set up
  curl_global_init(CURL_GLOBAL_ALL);
  hnd = curl_easy_init();
  if (request_json == NULL) {
    curl_easy_setopt(hnd, CURLOPT_URL, "https://x24cx5vto4.execute-api.us-east-1.amazonaws.com/prod");
  }
  else {
    char temp[256] = "https://x24cx5vto4.execute-api.us-east-1.amazonaws.com/prod?version=";
    char *version = cJSON_GetObjectItem(request_json, "version")->valuestring; 
    strcat(temp, version);
    curl_easy_setopt(hnd, CURLOPT_URL, temp);
  }
  curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.35.0");
  curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(hnd, CURLOPT_WRITEDATA,(void*)&data);
  ret = curl_easy_perform(hnd);
  curl_easy_cleanup(hnd);
  
  if ((int)ret != 0) {
    printf("Failed to get metadata\n");
  }
  
  hnd = NULL;
  curl_slist_free_all(slist1);
  slist1 = NULL;
  curl_global_cleanup();
  return data;
}

void updateData(cJSON * musics) {
  cJSON* title = NULL;
  int sz = (int)cJSON_GetArraySize(musics);
  for (int i = 0 ; i < sz; i++) {
    cJSON * music = cJSON_GetArrayItem(musics, i);
    title = cJSON_GetObjectItem(music, "title");
    std::string mp3(title->valuestring);
    mp3 += ".mp3";
    musicSet.insert(mp3);
    buildMap(genreMap, music, "genre");
    buildMap(artistMap, music, "artist");
  }
}

int flacjacket_getattr(const char *path, struct stat *stbuf) {
	stbuf->st_uid = getuid(); 
  stbuf->st_gid = getgid();
  stbuf->st_atime = time(NULL);
  stbuf->st_mtime = time(NULL);
  std::string path_str(path);
  
  if (strcmp(path, "/") == 0) {
	  stbuf->st_mode = S_IFDIR | S_IRWXU;
	  stbuf->st_nlink = 2;
  } 
  else if (genreMap.find(path_str) != genreMap.end() || artistMap.find(path_str) != artistMap.end()) {
    stbuf->st_mode = S_IFDIR | S_IRWXU;
    stbuf->st_nlink = 2;
  }  
  else {
    stbuf->st_mode = S_IFREG | S_IRWXU;
    stbuf->st_nlink = 1;
    //stbuf->st_size = 11748343;
    stbuf->st_size = 4529005;
  } 
  return 0;
}

int flacjacket_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi) {
  (void) offset;
  (void) fi;
  
  std::string path_str(path);
  MemoryStruct json;
  json.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */ 
  json.size = 0;                   /* no data at this point */ 
  json = getMetadata(json);
  
  if (strcmp(json.memory, "\"not modified\"") != 0) {
    cJSON_Delete(request_json);
    request_json = cJSON_Parse(json.memory);
    genreMap.clear();
    artistMap.clear();
    musicSet.clear();
  } 
  
  cJSON* musics = cJSON_GetObjectItem(request_json, "files");
  
  if (strcmp(path, "/") == 0) {
    updateData(musics);
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    for (auto i : musicSet) {
      filler(buf, i.c_str(), NULL, 0);
    }  
  }
  else if (genreMap.find(path_str) != genreMap.end()) {
    for (auto i : genreMap[path_str]) {
      filler(buf, i.c_str(), NULL, 0);
    }
  }
  else if (artistMap.find(path_str) != artistMap.end()) {
    for (auto i : artistMap[path_str]) {
      filler(buf, i.c_str(), NULL, 0);
    }
  }
  else {
    return -ENOENT;
  }
  
  //clean up
  free(json.memory);
  return 0;
}

int flacjacket_open(const char *path, struct fuse_file_info *fi) {
    std::string path_str(path);
    int fd;
    path_str = path_str.substr(1);
    std::cout << "in open call" << std::endl;
    if (musicSet.find(path_str) != musicSet.end()) {
        std::cout << "in open clause " << std::endl;
        fd = open("/tmp/test.mp3", fi->flags);
        std::cout << "fd is: " << fd << std::endl;
        if (fd < 0) {
            printf("failed to open file, reason is %s\n", strerror(errno));
            return -1;
        } else {
            close(fd);
        }
        return 0;
    }
    return -1;
}

int flacjacket_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {
    int fd;
    std::cout << "in read-open call" << std::endl;
    fd = open("/tmp/test.mp3", fi->flags);
    std::cout << "fd is: " << fd << std::endl;
    fi->fh = fd;
    ssize_t read_size = 0;
    std::cout << "in read call " << std::endl;
    read_size = pread(fi->fh, buf, size, offset);
    if (read_size < 0) {
        return -errno;
    }
    close(fi->fh);
    return read_size;
}
