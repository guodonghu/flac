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

extern char* bitRate;
bool isOpened = false;
cJSON* request_json = NULL;
std::unordered_map<std::string, std::unordered_set<std::string> > genreMap;
std::unordered_map<std::string, std::unordered_set<std::string> > artistMap;
std::unordered_map<std::string, std::string> musicMap;
std::unordered_map<std::string, int> musicSize;
const char outfilename[100] = "/tmp/buffer.mp3";

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
  int sz = (int)cJSON_GetArraySize(musics);
  for (int i = 0 ; i < sz; i++) {
    cJSON *music = cJSON_GetArrayItem(musics, i);
    cJSON *title = cJSON_GetObjectItem(music, "title");
    cJSON *key = cJSON_GetObjectItem(music, "key");
    cJSON *duration = cJSON_GetObjectItem(music, "duration");
    int fileSize;
    if (bitRate == NULL) {
      fileSize = 16000 * duration->valuedouble + 1000;
    }
    else {
      fileSize = (atof(bitRate) / 8) * 1000 * duration->valuedouble + 1000;
    }
    std::string mp3(title->valuestring);
    std::string keystr(key->valuestring);
    mp3 += ".mp3";
    musicMap[mp3] = keystr;
    musicSize[mp3] = fileSize;
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

void  getMusicData(std::string key) {
  std::cout << "getMusic data:" << key << std::endl; 
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
    if (bitRate == NULL) {
      curl_easy_setopt(hnd, CURLOPT_URL, "https://www.exoatmospherics.com/transcoder");
    }
    else {
      std::string url = "https://www.exoatmospherics.com/transcoder";
      url += "?quality=";
      url += bitRate;
      curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
    }
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, key.c_str());
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)key.size());
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
    fclose(fp);
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
  std::size_t lastSlash = path_str.find_last_of("/");
  std::string file = path_str.substr(lastSlash + 1);


  if (strcmp(path, "/") == 0 || strcmp(path, "/genre") == 0 
      || strcmp(path, "/artist") == 0 || strcmp(path, "/musicCollection") == 0) {
	  stbuf->st_mode = S_IFDIR | S_IRWXU;
	  stbuf->st_nlink = 2;
  }
  else if ((path_str.find("/genre/") != std::string::npos && genreMap.find(path_str.substr(strlen("/genre/"))) != genreMap.end())
           || (path_str.find("/artist/") != std::string::npos && artistMap.find(path_str.substr(strlen("/artist/"))) != artistMap.end())) {
    stbuf->st_mode = S_IFDIR | S_IRWXU;
    stbuf->st_nlink = 2;
  }
  else if (musicMap.find(file) != musicMap.end()) {
    stbuf->st_mode = S_IFREG | S_IRWXU;
    stbuf->st_nlink = 1;
    stbuf->st_size = musicSize[file];
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
      filler(buf, i.first.c_str(), NULL, 0);
    }
  }
  else if (path_str.find("/genre/") != std::string::npos) {
    std::string file = path_str.substr(strlen("/genre/"));
    std::size_t pos = file.find_first_of("/");
    std::string genre = file.substr(0, pos);
    if (genreMap.find(genre) != genreMap.end()) {
      for (std::string i : genreMap[genre]) {
        filler(buf, i.c_str(), NULL, 0);
      }
    }
  }
  else if (strcmp(path, "/artist") == 0) {
    for (auto i : artistMap) {
      filler(buf, i.first.c_str(), NULL, 0);
    }
  }
  else if (path_str.find("/artist/") != std::string::npos) {
    std::string file = path_str.substr(strlen("/artist/"));
    std::size_t pos = file.find_first_of("/");
    std::string artist = file.substr(0, pos);
    if (artistMap.find(artist) != artistMap.end()) {
      for (std::string i : artistMap[artist]) {
        filler(buf, i.c_str(), NULL, 0);
      }
    }
  }
  else {
    return -ENOENT;
  }
  
  return 0;
}

int flacjacket_open(const char *path, struct fuse_file_info *fi) {
  std::string path_str(path);
  std::size_t lastSlash = path_str.find_last_of("/");
  std::string file = path_str.substr(lastSlash + 1);
  if (isOpened) {
    return 0;
  }
  if (musicMap.find(file) != musicMap.end()) {
    // thread 1: create buffer file and download
    std::cout << "in open call" << std::endl;
    isOpened = true;
    std::string message1 = musicMap[file];
    std::thread thread1(getMusicData, message1);
    thread1.detach();
    // main thread wait here a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    return 0;
  }
  return -1;
}

int flacjacket_release(const char *path, struct fuse_file_info *fi) {
  isOpened = false;
  return 0;
}

int flacjacket_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {
  std::cout << "im parent in read call " << std::endl;
  std::cout << "read offset is: " << offset << std::endl;
  std::string path_str(path);
  std::size_t lastSlash = path_str.find_last_of("/");
  std::string file = path_str.substr(lastSlash + 1);
  // here the range we left for first seek is 5000 bytes from the end
  if (offset > musicSize[file] - 5000 && offset < musicSize[file]) {
    std::cout << "seek for tag, return herer" << std::endl;
    return size;
  }
  ssize_t read_size = 0;
  int fd = open(outfilename, fi->flags);
  fi->fh = fd;
  
  std::cout << "enter read while loop" << std::endl; 
  read_size = pread(fi->fh, buf, size, offset);
  while (read_size <= 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    read_size = pread(fi->fh, buf, size, offset);
  } 
  close(fi->fh);
  return read_size;
}
