#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
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


void parse_object(cJSON *root) {
  cJSON* title = NULL;
  cJSON* genre = NULL;
  cJSON* artist = NULL;
  cJSON* version = NULL;
  version = cJSON_GetObjectItem(root, "version");
  printf("cur version is %s\n", version->valuestring);
  cJSON* items = cJSON_GetObjectItem(root, "files");
  int sz = (int)cJSON_GetArraySize(items);
  printf("sz:%d", sz);

  for (int i = 0 ; i < sz; i++) {
    cJSON * subitem = cJSON_GetArrayItem(items, i);
    title = cJSON_GetObjectItem(subitem, "title");
    genre = cJSON_GetObjectItem(subitem, "genre");
    artist = cJSON_GetObjectItem(subitem, "artist");
    printf("music %d: title: %s,  genre: %s, artist: %s \n", i, title->valuestring, genre->valuestring, artist->valuestring);
  }
}

int main(int argc, char *argv[]) {
  CURLcode ret;
  CURL *hnd;
  struct curl_slist *slist1;
  struct MemoryStruct json;
  json.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  json.size = 0;    /* no data at this point */ 
  
  slist1 = NULL;
  slist1 = curl_slist_append(slist1, "x-api-key:UJWr37dxHD4VTmEVZYPpa9U3hNJTr7wgas1Uhvsf");
  //curl set up
  curl_global_init(CURL_GLOBAL_ALL);
  hnd = curl_easy_init();
  //curl_easy_setopt(hnd, CURLOPT_URL, "https://x24cx5vto4.execute-api.us-east-1.amazonaws.com/prod");
  curl_easy_setopt(hnd, CURLOPT_URL, "https://x24cx5vto4.execute-api.us-east-1.amazonaws.com/prod?version=2fd708a9df3486a5c55a52f817e13246");
  curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.35.0");
  curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void *)&json);
  ret = curl_easy_perform(hnd);
  curl_easy_cleanup(hnd);
  

  //cJSON* request_json = NULL;
  //printf("%s\n", json.memory);
  char * temp = "\"not modified\"";
  //printf("%s\n", temp);
  if (strcmp(json.memory, temp) == 0) {
    printf("not modified music system\n");
  }
  //request_json = cJSON_Parse(json.memory);
  //parse_object(request_json);
  
  //clean up
  //cJSON_Delete(request_json);
  free(json.memory);
  hnd = NULL;
  curl_slist_free_all(slist1);
  slist1 = NULL;
  curl_global_cleanup();

  return (int)ret;
}

