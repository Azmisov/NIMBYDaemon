#include "logging.h"
#include "comms.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

/*
	String structure manipulation;
	does not do any memory reallocation
*/
String* string_new(){
	String *chunk = malloc(sizeof(String));
	chunk->memory = (char*) malloc(1);
	chunk->size = 0;
	return chunk;
}
void string_free(String* str){
	free(str->memory);
	free(str);
}

/*
	Helper code for making HTTP requests
	Most of this code has been stolen shamelessly from the cURL website
*/
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
	size_t realsize = size * nmemb;
	String *mem = (String *)userp;
	mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		error_log("Not enough memory to read cURL response\n");
		return 0;
	}
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	return realsize;
}
CURL* open_curl(){
	CURL *handle;
	curl_global_init(CURL_GLOBAL_ALL);
	handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	return handle;
}
void close_curl(CURL* curl){
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}
String* request_curl(CURL* curl, const char* url){
	String* chunk = string_new();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) chunk);
	CURLcode res = curl_easy_perform(curl);
	//cURL request failed
	if (res != CURLE_OK){
		error_log("Failed to send cURL request; %s\nError URL: %s\n", curl_easy_strerror(res), url);
		string_free(chunk);
		return NULL;
	}
	//Make sure response code is success
	else{
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		if (http_code == 200 && res != CURLE_ABORTED_BY_CALLBACK)
			return chunk;
		else{
			error_log("cURL request returned %ld!\nError URL: %s\nError Response: %s\n", http_code, url, chunk->memory);
			//Could output the actual response data here, if we wanted too...
			string_free(chunk);
			return NULL;
		}
	}
}

/* Hostname and IP address lookup */
void get_hostname(char* buffer, int size){
	//Get system hostname
	//Try to resolve hostname, if we can; otherwise, just use the value from gethostname
	struct addrinfo hints, *hinfo;
	buffer[size-1] = '\0';
	gethostname(buffer, size-1);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;
	int gai_result;
	if ((gai_result = getaddrinfo(buffer, "http", &hints, &hinfo)) != 0)
		error_log("Could not resolve hostname address; %s\n", gai_strerror(gai_result));
	if (hinfo != NULL)
		sprintf(buffer, "%s", hinfo->ai_canonname);
	freeaddrinfo(hinfo);
}
int get_ipaddress(char* buffer){
	buffer[0] = '\0';
	//Get system IP address
	//We just go with the first IP address that doesn't look like a local IP
	struct ifaddrs *ifAddrStruct = NULL;
	struct ifaddrs *ifa = NULL;
	void * tmpAddrPtr = NULL;
	if (getifaddrs(&ifAddrStruct))
		error_log("getifaddrs failed!\n");
	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr)
			continue;
		//only check valid IP4 addresses
		if (ifa->ifa_addr->sa_family == AF_INET) {
			tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			//We're looking for an ip address that isn't just localhost
			if (strcmp(ifa->ifa_name, "lo") && strcmp(addressBuffer, "127.0.0.1")){
				sprintf(buffer, "%s", addressBuffer);
				break;
			}
		}
	}
	if (ifAddrStruct != NULL)
		freeifaddrs(ifAddrStruct);
	//Return 1 for error (could not find ip address)
	return buffer[0] == '\0';
}
