#ifndef COMMS_H
#define COMMS_H

#include <curl/curl.h>
#include "cJSON.h"

/*
	String management:
		string_new: get pointer for new string data
		string_free: free string pointer
*/
typedef struct String {
	char *memory;
	size_t size;
} String;
String* string_new();
void string_free(String* str);

/*
	cURL stuff:
		open_curl: get handle for making cURL requests
		close_curl: cleanup cURL object after done making requests
		request_curl: make request to the particular url
*/
CURL* open_curl();
void close_curl(CURL* curl);
String* request_curl(CURL* curl, const char* url);

/* Hostname and IP address lookup */
void get_hostname(char* buffer, int size);
int get_ipaddress(char* buffer);

#endif
