#include "dispatch.h"

char *default_response = 
            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n\n%s";
char *body = "<html><head><title>Test</title></head><body><h1>Response</h1><p>Response Test</p></body></html>"; 

char * dispatch(client *cli, const char *path) {

    char *buf;
    buf = malloc(sizeof(char)*1024);
    sprintf(buf, default_response,strlen(body), body);
    return buf;
}
