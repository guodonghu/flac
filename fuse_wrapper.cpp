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
using namespace std;
cJSON* request_json = NULL;
std::unordered_map<std::string, std::unordered_set<std::string> > genreMap;
std::unordered_map<std::string, std::unordered_set<std::string> > artistMap;
std::unordered_map<std::string, std::string> musicMap;

pid_t pid;
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


size_t WriteMusicCallback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    cout << "write music data to old file" << endl;
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
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

void  getMusicData(string name) {
    cout << "im child, get music data" << endl;
    CURLcode ret;
    CURL *hnd;
    FILE* fp = NULL;
    struct curl_slist *slist1;
    slist1 = NULL;
    slist1 = curl_slist_append(slist1, "api-key: s00per_seekrit");
    curl_global_init(CURL_GLOBAL_ALL);
    char outfilename[100] = "/tmp/buffer.mp3";
    hnd = curl_easy_init();
    if (hnd) {
        fp = fopen(outfilename, "wb");
        if (fp) {
            cout << "open file success" << endl;
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

int flacjacket_getattr(const char *path, struct stat *stbuf) {
	stbuf->st_uid = getuid(); 
    stbuf->st_gid = getgid();
  stbuf->st_atime = time(NULL);
  stbuf->st_mtime = time(NULL);
  std::string path_str(path);
  
  if (strcmp(path, "/") == 0) {
	  stbuf->st_mode = S_IFDIR | S_IRWXU;
	  stbuf->st_nlink = 2;
  } else if (genreMap.find(path_str) != genreMap.end() || artistMap.find(path_str) != artistMap.end()) {
    stbuf->st_mode = S_IFDIR | S_IRWXU;
    stbuf->st_nlink = 2;
  } else {
    stbuf->st_mode = S_IFREG | S_IRWXU;
    stbuf->st_nlink = 1;
    //stbuf->st_size = 11748343;
    stbuf->st_size = 4528586;
    //stbuf->st_size = 1000;
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
        musicMap.clear();
    } 
  
    cJSON* musics = cJSON_GetObjectItem(request_json, "files");
    updateData(musics); // was in "/" clause, which means updates only when "ls" is used, when "ls /genre" before "ls" -> bug
    if (strcmp(path, "/") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        for (auto i : musicMap) {
            filler(buf, i.first.c_str(), NULL, 0);
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
  /*cout << "in open call" << endl;
  std::string path_str(path);
  path_str = path_str.substr(1);
  if (musicMap.find(path_str) != musicMap.end()) {
    cout << "int open statment"<< endl;
    pid = fork();
    if (pid == 0) {
      cout << "im child" << endl; ;
      // download tmp file
      cout << "key is: " << musicMap[path_str] << endl;
      getMusicData(musicMap[path_str]);
    } else if (pid < 0) { 
      perror("fork failed\n");
    } else {
      cout << "im parent" << endl;
      int fd = open("/tmp/buffer.mp3", O_RDWR);
      if (fd == -1){
	perror("failed to open file\n");
      }
      fi->fh = fd;
      return 0;
    } 
  }
  return -1;*/
    cout << "in open call" << endl;
    std::string path_str(path);
    path_str = path_str.substr(1);
    FILE* fp;
    string str(4528805, 'x');
    fp = fopen("/tmp/buffer.mp3", "w");
    int tmp = (int)fwrite(str.c_str(), 1, str.size(), fp);
    cout << "open and write a file" << endl;
    cout << "file size is: "  << tmp << endl;
    fclose(fp);
    if (musicMap.find(path_str) != musicMap.end()) {
        cout << "int open statment"<< endl;
        pid = fork();
        if (pid == 0) {
            cout << "im child" << endl; ;
            // download tmp file
            cout << "key is: " << musicMap[path_str] << endl;
            getMusicData(musicMap[path_str]);
        } else if (pid < 0) { 
            perror("fork failed\n");
        } else {
            cout << "im parent" << endl;
            int fd = open("/tmp/buffer.mp3", O_RDWR);
            if (fd < -1){
                perror("failed to open file\n");
            }
            // if open successfully, close it
            close(fd);
            return 0;
        } 
    }
    return -1;
  
}

int flacjacket_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {
    std::cout << "im parent in read call " << std::endl;
    ssize_t read_size = 0;
    int fd = open("/tmp/buffer.mp3", fi->flags);
    fi->fh = fd;
    sleep(5);
    read_size = pread(fi->fh, buf, size, offset);
    cout << "in parent need size: " << size << endl;
    cout << "in parent read size: " << read_size << endl;
    if (read_size < 0) {
        cout << "read size < 0 " << endl;
        return -errno;
    }
    /*
    if (read_size < 0) {
        return -errno;
    } 
    */
    /*
    if (read_size == 0) {
        
        pid_t w = waitpid(pid, NULL, 0);
        cout << "im parent im collecting child's pid " << w  << endl;
        if (w == -1) {
            perror("waitpid");
        }
    }
    */
    close(fi->fh);
    return read_size;
    /*std::cout << "im parent in read call " << std::endl;
    //int fd = open("/tmp/buffer.mp3", fi->flags);
    //fi->fh = fd;
    ssize_t read_size = 0;
    sleep(2);
    read_size = pread(fi->fh, buf, size, offset);
    if (read_size == 0) {
        cout << "read size = 0" << endl;
        close(fi->fh);
        
        pid_t w = waitpid(pid, NULL, 0);
        if (w == -1) {
            perror("waitpid");
            }
    } else if (read_size < 0) {
        perror("read size < 0");
        return -errno;
    } else {
        cout << "read size > 0" << endl;
    }
    return read_size;*/
}
