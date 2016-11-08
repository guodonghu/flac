#define FUSE_USE_VERSION 26
#include <unistd.h>
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
#include <fstream>
#include <sys/wait.h>
#include <chrono>
#include <thread>
cJSON* request_json = NULL;

std::unordered_map<std::string, std::unordered_set<std::string> > genreMap;
std::unordered_map<std::string, std::unordered_set<std::string> > artistMap;
std::unordered_map<std::string, std::string> musicMap;

char outfilename[100] = "/tmp/buffer.mp3";

void buildMap(std::unordered_map<std::string, std::unordered_set<std::string> > &map, cJSON *music, std::string type) {
  cJSON *title = cJSON_GetObjectItem(music, "title");
  cJSON *category = cJSON_GetObjectItem(music, type.c_str());
  std::string category_str(category->valuestring);
  std::string mp3(title->valuestring);
  mp3 += ".mp3";
  map[category_str].insert(mp3); 
}

size_t WriteMetadataCallback(void *contents, size_t size, size_t nmemb, void *userp) {
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


size_t WriteMusicCallback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  std::cout << "im child im downloading music" << std::endl;
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
}

void updateData(cJSON * musics) {
  cJSON* title = NULL;
  cJSON* key = NULL;
  int sz = (int)cJSON_GetArraySize(musics);
  for (int i = 0 ; i < sz; i++) {
    cJSON * music = cJSON_GetArrayItem(musics, i);
    title = cJSON_GetObjectItem(music, "title");
    key = cJSON_GetObjectItem(music, "key");
    std::string mp3(title->valuestring);
    std::string keystr(key->valuestring);
    mp3 += ".mp3";
    musicMap[mp3] = keystr;
    buildMap(genreMap, music, "genre");
    buildMap(artistMap, music, "artist");
  }
}

void getMetadata() {
  MemoryStruct data;
  data.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */ 
  data.size = 0;                   /* no data at this point */ 
  
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
  curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteMetadataCallback);
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

  if (strcmp(data.memory, "\"not modified\"") != 0) {
    cJSON_Delete(request_json);
    request_json = cJSON_Parse(data.memory);
    genreMap.clear();
    artistMap.clear();
    musicMap.clear();
  } 
  cJSON* musics = cJSON_GetObjectItem(request_json, "files");
  updateData(musics); // was in "/" clause, which means updates only when "ls" is used, when "ls /genre" before "ls" -> bug
  // put it here solves this problem
  //clean up
  free(data.memory);
  return;
}

void  getMusicData(std::string name) {
  CURLcode ret;
  CURL *hnd;
  FILE* fp = NULL;
  struct curl_slist *slist1;
  slist1 = NULL;
  slist1 = curl_slist_append(slist1, "api-key: s00per_seekrit");
  curl_global_init(CURL_GLOBAL_ALL);
  hnd = curl_easy_init();
  if (hnd) {
    fp = fopen(outfilename, "wb");
    if (fp) {
      std::cout << "open file success" << std::endl;
    }
    curl_easy_setopt(hnd, CURLOPT_URL, "https://www.exoatmospherics.com/transcoder");
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, "Four Tet - Randoms - 01 Moma.flac");
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)33);
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.35.0");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
    curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteMusicCallback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, fp);
    ret = curl_easy_perform(hnd);
    if (ret != 0) {
      perror("wrong ret value\n");
    }
    /***************clean up********************/
    curl_easy_cleanup(hnd);
    hnd = NULL;
    curl_slist_free_all(slist1);
    slist1 = NULL;
    //curl_global_cleanup();
    /*******************************************/
    return;
  }
  perror("curl handler is null\n");
  return;
}

int flacjacket_getattr(const char *path, struct stat *stbuf) {
  int res = 0;
	stbuf->st_uid = getuid(); 
  stbuf->st_gid = getgid();
  stbuf->st_atime = time(NULL);
  stbuf->st_mtime = time(NULL);
  std::string path_str(path);

  if (strcmp(path, "/") == 0 || strcmp(path, "/genre") == 0 
      || strcmp(path, "/artist") == 0 || strcmp(path, "/musicCollection") == 0) {
	  stbuf->st_mode = S_IFDIR | S_IRWXU;
	  stbuf->st_nlink = 2;
  }
  else if (genreMap.find(path_str.substr(strlen("/genre/"))) != genreMap.end() || artistMap.find(path_str.substr(strlen("/artist/"))) != artistMap.end()) {
    stbuf->st_mode = S_IFDIR | S_IRWXU;
    stbuf->st_nlink = 2;
  }
  else if (musicMap.find(path_str.substr(strlen("/musicCollection/"))) != musicMap.end()) {
    stbuf->st_mode = S_IFREG | S_IRWXU;
    stbuf->st_nlink = 1;
    stbuf->st_size = 4528805;
  }
  else {
    res = -ENOENT;
  }
  
  return res;
}

int flacjacket_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
  (void) offset;
  (void) fi;
 
  std::string path_str(path);
  std::thread (getMetadata).detach();
  //try with GUI, could live witout this
  if (musicMap.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
  }

  if (strcmp(path, "/") == 0) {
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, "musicCollection", NULL, 0);
    filler(buf, "genre", NULL, 0);
    filler(buf, "artist", NULL, 0);
  }
  else if (strcmp(path, "/musicCollection") == 0) {
    for (auto i : musicMap) {
      filler(buf, i.first.c_str(), NULL, 0);
    }  
  }
  else if (strcmp(path, "/genre") == 0) {
    for (auto i : genreMap) {
      std::cout << i.first << std::endl;
      filler(buf, i.first.c_str(), NULL, 0);
    }
  }
  else if (strcmp(path, "/artist") == 0) {
    for (auto i : artistMap) {
      filler(buf, i.first.c_str(), NULL, 0);
    }
  }
  else {
    return -ENOENT;
  }
  
  return 0;
}

int flacjacket_open(const char *path, struct fuse_file_info *fi) {
  std::string path_str(path);
  path_str = path_str.substr(1);
  
  if (musicMap.find(path_str) != musicMap.end()) {
    // thread 1: create buffer file and download
    std::cout << "in open call" << std::endl;
    std::string message1 = musicMap[path_str];
    std::thread thread1(getMusicData, message1);
    thread1.detach();
    // main thread wait here a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    return 0;
  }
  return -1;
}

int flacjacket_release(const char *path, struct fuse_file_info *fi) {
  return 0;
}

int flacjacket_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {
  std::cout << "im parent in read call " << std::endl;
  std::cout << "read offset is: " << offset << std::endl;
  // here the range we left for first seek is 5000 bytes from the end
  if (offset > 4528805-5000 && offset < 4528805) {
    std::cout << "seek for tag, return herer" << std::endl;
    return size;
  }
  ssize_t read_size = 0;
  int fd = open(outfilename, fi->flags);
  fi->fh = fd;
  while (1) {
    read_size = pread(fi->fh, buf, size, offset);
    if (read_size <= 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      read_size = pread(fi->fh, buf, size, offset);
    } else {
      break;
    }
  }
  close(fi->fh);
  return read_size;
}
